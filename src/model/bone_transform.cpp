#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/model.hpp"

namespace rinmotiondance {
namespace {

using d3dx::D3DXMATRIXF;

#ifdef RINMOTIONDANCE_DIAG

LONG gIkProbeArmed = 0;
LONG gIkProbeWritten = 0;

void WriteFloatBits(std::FILE* stream, const float* values, int count) {
    std::fputc('[', stream);
    for (int i = 0; i < count; ++i) {
        if (i != 0) std::fputc(',', stream);
        std::uint32_t bits = 0;
        std::memcpy(&bits, values + i, sizeof(bits));
        std::fprintf(stream, "\"%08X\"", bits);
    }
    std::fputc(']', stream);
}

void WriteIkProbe(unsigned char* model, int chainIndex, int iteration,
                  int linkIndex, int loopCount, int targetIndex, int rootIndex,
                  int boneIndex, const float d1[3], const float d2[3],
                  const float axis[3], float dot, float half,
                  const float acc[4], const float rotq[4]) {
    if (InterlockedCompareExchange(&gIkProbeArmed, 0, 0) == 0 ||
        chainIndex != 0)
        return;
    const LONG recordIndex = InterlockedIncrement(&gIkProbeWritten);
    if (recordIndex > 512)
        return;
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_STATE_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\ik_trace.jsonl", directory);
    std::FILE* stream = nullptr;
    if (fopen_s(&stream, path, recordIndex == 1 ? "wb" : "ab") != 0 ||
        stream == nullptr)
        return;
    std::fprintf(stream,
        "{\"bone_count\":%d,\"chain\":%d,\"iteration\":%d,\"loops\":%d,"
        "\"link_position\": %d, \"target\": %d, \"root\": %d, "
        "\"bone\":%d,\"d1_bits\":",
        static_cast<int>(mdl::Mdl(model)->boneCount), chainIndex, iteration,
        loopCount,
        linkIndex,
        targetIndex, rootIndex, boneIndex);
    WriteFloatBits(stream, d1, 3);
    std::fputs(",\"d2_bits\":", stream);
    WriteFloatBits(stream, d2, 3);
    std::fputs(",\"axis_bits\":", stream);
    WriteFloatBits(stream, axis, 3);
    std::fputs(",\"dot_bits\":", stream);
    WriteFloatBits(stream, &dot, 1);
    std::fputs(",\"half_bits\":", stream);
    WriteFloatBits(stream, &half, 1);
    std::fputs(",\"acc_bits\":", stream);
    WriteFloatBits(stream, acc, 4);
    std::fputs(",\"rotation_bits\":", stream);
    WriteFloatBits(stream, rotq, 4);
    std::fputs("}\n", stream);
    std::fclose(stream);
}

void WriteIkPrecheck(unsigned char* model, int chainIndex, int iteration,
                     int linkIndex, const float d1[3], const float d2[3],
                     float diff2, const mdl::BoneRecord& target,
                     const mdl::BoneRecord& root,
                     const mdl::BoneRecord& link) {
    if (InterlockedCompareExchange(&gIkProbeArmed, 0, 0) == 0 ||
        chainIndex != 0 || iteration != 1 || linkIndex != 0)
        return;
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_STATE_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\ik_precheck.json", directory);
    std::FILE* stream = nullptr;
    if (fopen_s(&stream, path, "wb") != 0 || stream == nullptr)
        return;
    std::fputs("{\n  \"d1_bits\": ", stream);
    WriteFloatBits(stream, d1, 3);
    std::fputs(",\n  \"d2_bits\": ", stream);
    WriteFloatBits(stream, d2, 3);
    std::uint32_t diffBits = 0;
    std::memcpy(&diffBits, &diff2, sizeof(diffBits));
    std::fprintf(stream, ",\n  \"diff2_bits\": \"%08X\"", diffBits);
    const mdl::BoneRecord* records[] = {&target, &root, &link};
    const char* names[] = {"target_matrix_bits", "root_matrix_bits",
                           "link_matrix_bits"};
    for (int record = 0; record < 3; ++record) {
        std::fprintf(stream, ",\n  \"%s\": [", names[record]);
        for (int i = 0; i < 16; ++i) {
            if (i != 0) std::fputc(',', stream);
            std::uint32_t bits = 0;
            std::memcpy(&bits, records[record]->matInit + i, sizeof(bits));
            std::fprintf(stream, "\"%08X\"", bits);
        }
        std::fputc(']', stream);
    }
    std::fputs("\n}\n", stream);
    std::fclose(stream);
}

#endif  

constexpr float kPiF = 3.140000104904175f;
constexpr float kEps = 1.1920929e-7f;              
constexpr float kHalfPi = 1.570796012878418f;      
constexpr float kKneeClamp = 1.535889f;            

const char kNameKneeLJp[7] = "\x8d\xb6\x82\xd0\x82\xb4";
const char kNameKneeRJp[7] = "\x89\x45\x82\xd0\x82\xb4";

bool IsKnee(const mdl::BoneRecord& bone) {
    return std::strncmp(bone.name,
                        kNameKneeLJp, 6) == 0 ||
           std::strncmp(bone.name,
                        kNameKneeRJp, 6) == 0;
}


void MatRotQuat(D3DXMATRIXF* out, const float q[4]) {
    d3dx::Get().matrixRotationQuaternion(out, q);
}

void Vec3Norm(float out[3], const float in[3]) {
    d3dx::Get().vec3Normalize(out, in);
}

void QuatMul(float out[4], const float q1[4], const float q2[4]) {
    d3dx::Get().quatMultiply(out, q1, q2);
}

struct D3 {
    d3dx::Api* api = &d3dx::Get();

    void rotX(D3DXMATRIXF* o, float a) { api->rotX(o, a); }
    void rotY(D3DXMATRIXF* o, float a) { api->rotY(o, a); }
    void rotZ(D3DXMATRIXF* o, float a) { api->rotZ(o, a); }
    void mul(D3DXMATRIXF* o, const D3DXMATRIXF* a, const D3DXMATRIXF* b) {
        api->multiply(o, a, b);
    }
    void translation(D3DXMATRIXF* o, float x, float y, float z) {
        std::memset(o, 0, sizeof(*o));
        o->m[0][0] = o->m[1][1] = o->m[2][2] = 1.0f;
        o->m[3][0] = x;
        o->m[3][1] = y;
        o->m[3][2] = z;
        o->m[3][3] = 1.0f;
    }
    void quatFromMatrix(float out[4], const D3DXMATRIXF* m) {
        api->quatFromMatrix(out, m);
    }

};


void InheritRotQuat(float q[4], const mdl::BoneRecord* bones, int srcIdx,
                    float rate) {
    D3 d3;
    D3DXMATRIXF rot;
    std::memcpy(&rot, bones[srcIdx].matLocal, sizeof(rot));
    float sq[4];
    d3.quatFromMatrix(sq, &rot);
    if (sq[3] <= 1.0f) {
        if (sq[3] < -1.0f) sq[3] = -1.0f;
    } else {
        sq[3] = 1.0f;
    }
    float angle = std::acos(sq[3]);
    if (sq[3] < 0.0f)
        angle -= kPiF;
    const float len = std::sqrt(sq[0] * sq[0] + sq[1] * sq[1] +
                                sq[2] * sq[2]);
    float part[4];
    if (angle == 0.0f || len < kEps) {
        part[0] = 0.0f;
        part[1] = 0.0f;
        part[2] = 0.0f;
        part[3] = 1.0f;
    } else {
        const float scaled = angle * rate;
        const float s = std::sin(scaled) / len;
        part[0] = s * sq[0];
        part[1] = s * sq[1];
        part[2] = s * sq[2];
        part[3] = std::cos(scaled);
    }
    float out[4];
    QuatMul(out, part, q);
    q[0] = out[0];
    q[1] = out[1];
    q[2] = out[2];
    q[3] = out[3];
}

void WorldFromParent(mdl::BoneRecord& bone, const mdl::BoneRecord& parent,
                     D3* d3) {
    D3DXMATRIXF local, pw, t;
    std::memcpy(&local, bone.matLocal, sizeof(local));
    std::memcpy(&pw, parent.matInit, sizeof(pw));
    d3->mul(reinterpret_cast<D3DXMATRIXF*>(bone.matInit), &local, &pw);
    std::memcpy(&t, parent.matInit, sizeof(t));
    t.m[3][0] = t.m[3][1] = t.m[3][2] = 0.0f;
    t.m[3][3] = 1.0f;
    std::memcpy(bone.matWorld, &t, sizeof(t));
}

void WorldPos(float dst[3], const mdl::BoneRecord& bone) {
    const float* mw = bone.matInit;
    const float* rest = bone.position;
#if defined(_M_IX86)
    __asm {
        mov eax, mw
        mov edx, rest
        mov ecx, dst

        fld dword ptr [eax]
        fmul dword ptr [edx]
        fld dword ptr [eax+16]
        fmul dword ptr [edx+4]
        faddp st(1), st
        fld dword ptr [eax+32]
        fmul dword ptr [edx+8]
        faddp st(1), st
        fadd dword ptr [eax+48]
        fstp dword ptr [ecx]

        fld dword ptr [eax+4]
        fmul dword ptr [edx]
        fld dword ptr [eax+20]
        fmul dword ptr [edx+4]
        faddp st(1), st
        fld dword ptr [eax+36]
        fmul dword ptr [edx+8]
        faddp st(1), st
        fadd dword ptr [eax+52]
        fstp dword ptr [ecx+4]

        fld dword ptr [eax+8]
        fmul dword ptr [edx]
        fld dword ptr [eax+24]
        fmul dword ptr [edx+4]
        faddp st(1), st
        fld dword ptr [eax+40]
        fmul dword ptr [edx+8]
        faddp st(1), st
        fadd dword ptr [eax+56]
        fstp dword ptr [ecx+8]
    }
#else
    dst[0] = mw[0] * rest[0] + mw[4] * rest[1] + mw[8] * rest[2] + mw[12];
    dst[1] = mw[1] * rest[0] + mw[5] * rest[1] + mw[9] * rest[2] + mw[13];
    dst[2] = mw[2] * rest[0] + mw[6] * rest[1] + mw[10] * rest[2] + mw[14];
#endif
}

float Dot3X87(const float a[3], const float b[3]) {
#if defined(_M_IX86)
    float result;
    __asm {
        mov eax, a
        mov edx, b
        fld dword ptr [eax]
        fmul dword ptr [edx]
        fld dword ptr [eax+4]
        fmul dword ptr [edx+4]
        faddp st(1), st
        fld dword ptr [eax+8]
        fmul dword ptr [edx+8]
        faddp st(1), st
        fstp dword ptr [result]
    }
    return result;
#else
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
#endif
}

float SquaredDiff3X87(const float a[3], const float b[3]) {
#if defined(_M_IX86)
    float result;
    __asm {
        mov eax, a
        mov edx, b
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fmul st, st
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fmul st, st
        faddp st(1), st
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st, st
        faddp st(1), st
        fstp dword ptr [result]
    }
    return result;
#else
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    return dy * dy + dx * dx + dz * dz;
#endif
}

void Cross3X87(float dst[3], const float a[3], const float b[3]) {
#if defined(_M_IX86)
    __asm {
        mov eax, a
        mov edx, b
        mov ecx, dst

        fld dword ptr [eax+4]
        fmul dword ptr [edx+8]
        fld dword ptr [eax+8]
        fmul dword ptr [edx+4]
        fsubp st(1), st
        fstp dword ptr [ecx]

        fld dword ptr [eax+8]
        fmul dword ptr [edx]
        fld dword ptr [eax]
        fmul dword ptr [edx+8]
        fsubp st(1), st
        fstp dword ptr [ecx+4]

        fld dword ptr [eax]
        fmul dword ptr [edx+4]
        fld dword ptr [eax+4]
        fmul dword ptr [edx]
        fsubp st(1), st
        fstp dword ptr [ecx+8]
    }
#else
    dst[0] = a[1] * b[2] - a[2] * b[1];
    dst[1] = a[2] * b[0] - a[0] * b[2];
    dst[2] = a[0] * b[1] - a[1] * b[0];
#endif
}

void ProjectAxisOnLocal(float axis[3], const mdl::BoneRecord& bone) {
    const float* r = bone.matWorld;
    float out[3];
    out[0] = Dot3X87(r, axis);
    out[1] = Dot3X87(r + 4, axis);
    out[2] = Dot3X87(r + 8, axis);
    Vec3Norm(axis, out);
}

void ClampEuler(float& a, float lo, float hi, bool twist) {
    if (lo > a) {
        const float refl = 2.0f * lo - a;
        a = (hi < refl || twist) ? lo : refl;
    }
    if (hi < a) {
        const float refl = 2.0f * hi - a;
        a = (lo > refl || twist) ? hi : refl;
    }
}


bool ChannelPass(const mdl::BoneRecord& bone, unsigned char afterPhysics) {
    const bool has = (bone.flags & mdl::kBoneFlagAfterPhysics) != 0;
    return afterPhysics ? has : !has;
}

bool CopyGate(const mdl::BoneRecord& bone, int physicsMode) {
    return ((physicsMode == 1 || (physicsMode >= 2 && bone.physicsDisabled == 0)) & bone.hasRigidBody) == 0;
}

void BoneTransform_PhysicsSweptLocals(D3& d3, mdl::BoneRecord* boneRecords,
                                      int boneCount,
                                      unsigned char afterPhysics,
                                      int layer) {
    for (int i = 0; i < boneCount; ++i) {
        mdl::BoneRecord& bone = boneRecords[i];
        if (!ChannelPass(bone, afterPhysics))
            continue;
        if (bone.layer != layer || bone.hasFlag == 0)
            continue;
        float q[4] = {bone.physicsQuat[0], bone.physicsQuat[1], bone.physicsQuat[2], bone.physicsQuat[3]};
        if (bone.flags & mdl::kBoneFlagRotInherit)
            InheritRotQuat(q, boneRecords, bone.tailIdx, bone.inheritRatio);
        MatRotQuat(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), q);
        if (bone.flags & mdl::kBoneFlagTransInherit) {
            mdl::BoneRecord& src = boneRecords[bone.tailIdx];
            const float r = bone.inheritRatio;
            D3DXMATRIXF tmp, cur;
            d3.translation(&tmp, src.physicsOffset[0] * r, src.physicsOffset[1] * r,
                           src.physicsOffset[2] * r);
            std::memcpy(&cur, bone.matLocal, sizeof(cur));
            d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
        }
        D3DXMATRIXF tmp, cur;
        d3.translation(&tmp, -bone.position[0], -bone.position[1],
                       -bone.position[2]);
        std::memcpy(&cur, bone.matLocal, sizeof(cur));
        d3.mul(&cur, &tmp, &cur);
        d3.translation(&tmp, bone.physicsOffset[0], bone.physicsOffset[1], bone.physicsOffset[2]);
        d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
        std::memcpy(&cur, bone.matLocal, sizeof(cur));
        d3.translation(&tmp, bone.position[0], bone.position[1],
                       bone.position[2]);
        d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
    }
}

void BoneTransform_StandardLocals(D3& d3, mdl::BoneRecord* boneRecords,
                                  int boneCount,
                                  unsigned char afterPhysics,
                                  int layer) {
    for (int i = 0; i < boneCount; ++i) {
        mdl::BoneRecord& bone = boneRecords[i];
        if (!ChannelPass(bone, afterPhysics))
            continue;
        if (bone.layer != layer || bone.hasFlag != 0)
            continue;
        if (bone.type == mdl::BoneType::UnderIk) {     
            bone.rotQuat2[0] = bone.rotQuat2[1] = bone.rotQuat2[2] = 0.0f;
            bone.rotQuat2[3] = 1.0f;
        }
        if (bone.type == mdl::BoneType::CoRotate) {    
            const mdl::BoneRecord& tail = boneRecords[bone.tailBone];
            float sq[4] = {tail.physicsQuat[0], tail.physicsQuat[1], tail.physicsQuat[2],
                           tail.physicsQuat[3]};
            if (sq[3] <= 1.0f) {
                if (sq[3] < -1.0f) sq[3] = -1.0f;
            } else {
                sq[3] = 1.0f;
            }
            float angle = std::acos(sq[3]);
            if (sq[3] < 0.0f)
                angle -= kPiF;
            const float len = std::sqrt(sq[0] * sq[0] + sq[1] * sq[1] +
                                        sq[2] * sq[2]);
            if (angle == 0.0f || len < kEps) {
                bone.physicsQuat[0] = bone.physicsQuat[1] = bone.physicsQuat[2] = 0.0f;
                bone.physicsQuat[3] = 1.0f;
            } else {
                const float scaled =
                    (static_cast<float>(bone.tailIdx) * angle) / 100.0f;
                const float s = std::sin(scaled) / len;
                bone.physicsQuat[0] = s * sq[0];
                bone.physicsQuat[1] = s * sq[1];
                bone.physicsQuat[2] = s * sq[2];
                bone.physicsQuat[3] = std::cos(scaled);
            }
        }
        float q[4] = {bone.physicsQuat[0], bone.physicsQuat[1], bone.physicsQuat[2], bone.physicsQuat[3]};
        if (bone.type == mdl::BoneType::RotateGrant) { 
            const mdl::BoneRecord& src = boneRecords[bone.tailIdx];
            float sq[4] = {src.physicsQuat[0], src.physicsQuat[1], src.physicsQuat[2], src.physicsQuat[3]};
            float out[4];
            QuatMul(out, sq, q);
            q[0] = out[0];
            q[1] = out[1];
            q[2] = out[2];
            q[3] = out[3];
        }
        if (bone.flags & mdl::kBoneFlagRotInherit)
            InheritRotQuat(q, boneRecords, bone.tailIdx, bone.inheritRatio);
        MatRotQuat(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), q);
        float inhX = 0.0f, inhY = 0.0f, inhZ = 0.0f;
        if (bone.flags & mdl::kBoneFlagTransInherit) {
            const mdl::BoneRecord& src = boneRecords[bone.tailIdx];
            const float r = bone.inheritRatio;
            inhX = src.physicsOffset[0] * r;
            inhY = src.physicsOffset[1] * r;
            inhZ = src.physicsOffset[2] * r;
            D3DXMATRIXF tmp, cur;
            d3.translation(&tmp, inhX, inhY, inhZ);
            std::memcpy(&cur, bone.matLocal, sizeof(cur));
            d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
        }
        D3DXMATRIXF tmp, cur;
        d3.translation(&tmp, -bone.position[0], -bone.position[1],
                       -bone.position[2]);
        std::memcpy(&cur, bone.matLocal, sizeof(cur));
        d3.mul(&cur, &tmp, &cur);
        d3.translation(&tmp, bone.physicsOffset[0], bone.physicsOffset[1], bone.physicsOffset[2]);
        d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
        std::memcpy(&cur, bone.matLocal, sizeof(cur));
        d3.translation(&tmp, bone.position[0], bone.position[1],
                       bone.position[2]);
        d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
        if (bone.flags & mdl::kBoneFlagTransInherit) { 
            bone.physicsOffset[0] += inhX;
            bone.physicsOffset[1] += inhY;
            bone.physicsOffset[2] += inhZ;
        }
    }
}

void BoneTransform_WorldPass(D3& d3, mdl::BoneRecord* boneRecords,
                             int boneCount, mdl::BoneOrderEntry* extTable,
                             unsigned char* const* modelSlots,
                             unsigned char afterPhysics, int layer,
                             int physicsMode) {
    for (int i = 0; i < boneCount; ++i) {
        mdl::BoneRecord& bone = boneRecords[i];
        if (!ChannelPass(bone, afterPhysics))
            continue;
        if (bone.layer != layer)
            continue;
        const std::int32_t parent = bone.parent;
        const std::int32_t extIdx = bone.slotIndex;
        bool handled = false;
        bool viaGlobalExt = false;
        if (extIdx >= 0) {
            const std::int32_t slot = extTable[extIdx].linkedModel;
            if (slot >= 0) {                           
                unsigned char* model2 = modelSlots[slot];
                mdl::BoneRecord* bones2 = mdl::Bones(model2);
                const std::int32_t extBone = extTable[extIdx].linkedBone;
                const mdl::BoneRecord& src = bones2[extBone];
                D3DXMATRIXF ext, tmp, local;
                std::memcpy(&ext, src.matExtra, sizeof(ext));
                d3.translation(&tmp, src.position[0], src.position[1],
                               src.position[2]);
                d3.mul(&ext, &tmp, &ext);
                d3.translation(&tmp, -bone.position[0], -bone.position[1],
                               -bone.position[2]);
                d3.mul(&ext, &tmp, &ext);
                std::memcpy(&local, bone.matLocal, sizeof(local));
                d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matInit), &local,
                       &ext);
                ext.m[3][0] = ext.m[3][1] = ext.m[3][2] = 0.0f;
                ext.m[3][3] = 1.0f;
                std::memcpy(bone.matWorld, &ext, sizeof(ext));
                handled = true;
            } else if (slot == -1) {
                if (parent >= 0) {
                    WorldFromParent(bone, boneRecords[parent], &d3);
                    handled = true;
                } else {
                    viaGlobalExt = true;
                }
            }
        } else if (parent >= 0) {
            WorldFromParent(bone, boneRecords[parent], &d3);
            handled = true;
        } else {
            viaGlobalExt = true;
        }
        if (viaGlobalExt) {
            if (extTable[0].linkedModel >= 0) {
                unsigned char* model2 = modelSlots[extTable[0].linkedModel];
                mdl::BoneRecord* bones2 = mdl::Bones(model2);
                const mdl::BoneRecord& src =
                    bones2[extTable[0].linkedBone];
                D3DXMATRIXF ext, tmp, local;
                std::memcpy(&ext, src.matExtra, sizeof(ext));
                d3.translation(&tmp, src.position[0], src.position[1],
                               src.position[2]);
                d3.mul(&ext, &tmp, &ext);
                std::memcpy(&local, bone.matLocal, sizeof(local));
                d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matInit), &local,
                       &ext);
                ext.m[3][0] = ext.m[3][1] = ext.m[3][2] = 0.0f;
                ext.m[3][3] = 1.0f;
                std::memcpy(bone.matWorld, &ext, sizeof(ext));
                handled = true;
            }
        }
        if (!handled) {                                
            std::memcpy(bone.matInit, bone.matLocal, sizeof(bone.matInit));
            D3DXMATRIXF id;
            std::memset(&id, 0, sizeof(id));
            id.m[0][0] = id.m[1][1] = id.m[2][2] = id.m[3][3] = 1.0f;
            std::memcpy(bone.matWorld, &id, sizeof(id));
        }
        if (CopyGate(bone, physicsMode))                       
            std::memcpy(bone.matExtra, bone.matInit, sizeof(bone.matExtra));
    }
}

void BoneTransform_CcdIk(D3& d3, unsigned char* m,
                         mdl::BoneRecord* boneRecords, mdl::IkChain* iks,
                         int ikCount, unsigned char afterPhysics,
                         int layer, int physicsMode, bool pmx2) {
    for (int ci = 0; ci < ikCount; ++ci) {
        mdl::IkChain& ik = iks[ci];
        mdl::BoneRecord& target = boneRecords[ik.boneIndex];
        if (!ChannelPass(target, afterPhysics))
            continue;
        if (target.layer != layer || ik.enabled == 0)
            continue;
        mdl::BoneRecord& ikRoot = boneRecords[ik.targetBone];
        const mdl::BoneRecord& rootParent = boneRecords[ikRoot.parent];
        if (((physicsMode == 1 || (physicsMode >= 2 && rootParent.physicsDisabled == 0)) &
             rootParent.hasRigidBody) != 0)
            continue;
        float eff[3];
        WorldPos(eff, target);                         
        {
            mdl::BoneRecord& root = ikRoot;
            float q[4] = {root.physicsQuat[0], root.physicsQuat[1], root.physicsQuat[2],
                          root.physicsQuat[3]};
            MatRotQuat(reinterpret_cast<D3DXMATRIXF*>(root.matLocal), q);
            D3DXMATRIXF tmp, cur;
            d3.translation(&tmp, -root.position[0], -root.position[1],
                           -root.position[2]);
            std::memcpy(&cur, root.matLocal, sizeof(cur));
            d3.mul(&cur, &tmp, &cur);
            d3.translation(&tmp, root.physicsOffset[0], root.physicsOffset[1], root.physicsOffset[2]);
            d3.mul(reinterpret_cast<D3DXMATRIXF*>(root.matLocal), &cur, &tmp);
            std::memcpy(&cur, root.matLocal, sizeof(cur));
            d3.translation(&tmp, root.position[0], root.position[1],
                           root.position[2]);
            d3.mul(reinterpret_cast<D3DXMATRIXF*>(root.matLocal), &cur, &tmp);
        }
        const std::uint16_t* child = ik.links;
        const int loops = ik.iterations;
        const float chainAngle = ik.maxAngle;
        if (ik.linkCount == 0)
            continue;
        const int twist = loops >> 1;                  
        int iter = 0;
        while (iter < loops) {
            if (ik.linkCount == 0)
                break;                                 
            bool exitAll = false;
            for (int li = 0; li < ik.linkCount; ++li) {
                mdl::BoneRecord& root = ikRoot;
                float rootPos[3];
                WorldPos(rootPos, root);               
                mdl::BoneRecord& link = boneRecords[child[li]];
                float linkPos[3];
                WorldPos(linkPos, link);               
                float d1[3] = {linkPos[0] - rootPos[0],
                               linkPos[1] - rootPos[1],
                               linkPos[2] - rootPos[2]};
                float d2[3] = {linkPos[0] - eff[0], linkPos[1] - eff[1],
                               linkPos[2] - eff[2]};
                Vec3Norm(d1, d1);
                Vec3Norm(d2, d2);
                const float diff2 = SquaredDiff3X87(d1, d2);
#ifdef RINMOTIONDANCE_DIAG
                WriteIkPrecheck(m, ci, iter, li, d1, d2, diff2, target,
                                root, link);
#endif
                if (diff2 < 1e-7f) {                   
                    iter = loops;                      
                    exitAll = true;
                    break;
                }
                float axis[3];
                Cross3X87(axis, d1, d2);
                float align = 1.0f;                    
                const float* r180 = link.matWorld;
                if (!pmx2) {                           
                    if (IsKnee(link)) {
                        const float dot1 = r180[0] * axis[0] +
                                           r180[1] * axis[1] +
                                           r180[2] * axis[2];
                        if (dot1 >= 0.0f) {
                            axis[0] = 1.0f;
                            axis[1] = 0.0f;
                            axis[2] = 0.0f;
                        } else {
                            axis[0] = -1.0f;
                            axis[1] = 0.0f;
                            axis[2] = 0.0f;
                        }
                    } else {
                        ProjectAxisOnLocal(axis, link);
                    }
                } else if (link.twistEnable != 0 && iter < twist) {
                    const float limXlo = link.ikLimitMin[0];
                    const float limXhi = link.ikLimitMax[0];
                    const float limYlo = link.ikLimitMin[1];
                    const float limYhi = link.ikLimitMax[1];
                    const float limZlo = link.ikLimitMin[2];
                    const float limZhi = link.ikLimitMax[2];
                    if (limYlo != 0.0f || limYhi != 0.0f || limZlo != 0.0f ||
                        limZhi != 0.0f) {
                        if (limXlo == 0.0f && limXhi == 0.0f &&
                            limZlo == 0.0f && limZhi == 0.0f) {
                            const float dot2 = r180[4] * axis[0] +
                                               r180[5] * axis[1] +
                                               r180[6] * axis[2];
                            axis[0] = 0.0f;
                            axis[1] = dot2 < 0.0f ? -1.0f : 1.0f;
                            axis[2] = 0.0f;
                        } else if (limXlo == 0.0f && limXhi == 0.0f &&
                                   limYlo == 0.0f && limYhi == 0.0f) {
                            const float dot3 = r180[8] * axis[0] +
                                               r180[9] * axis[1] +
                                               r180[10] * axis[2];
                            axis[0] = 0.0f;
                            axis[1] = 0.0f;
                            axis[2] = dot3 < 0.0f ? -1.0f : 1.0f;
                        } else if (link.flags & mdl::kBoneFlagFixedAxis) {
                            ProjectAxisOnLocal(axis, link);
                            const float fx = link.axis[0];
                            const float fy = link.axis[1];
                            const float fz = link.axis[2];
                            align = fy * axis[1] + fx * axis[0] + fz * axis[2];
                            axis[0] = fx;
                            axis[1] = fy;
                            axis[2] = fz;
                        } else {
                            ProjectAxisOnLocal(axis, link);
                        }
                    } else {
                        const float dot1 = r180[0] * axis[0] +
                                           r180[1] * axis[1] +
                                           r180[2] * axis[2];
                        if (dot1 < 0.0f) {
                            axis[0] = -1.0f;
                            axis[1] = 0.0f;
                            axis[2] = 0.0f;
                        } else {
                            axis[0] = 1.0f;
                            axis[1] = 0.0f;
                            axis[2] = 0.0f;
                        }
                    }
                } else {
                    if ((link.flags & mdl::kBoneFlagFixedAxis) == 0) {
                        ProjectAxisOnLocal(axis, link);   
                    } else {                             
                        ProjectAxisOnLocal(axis, link);
                        const float fx = link.axis[0];
                        const float fy = link.axis[1];
                        const float fz = link.axis[2];
                        align = fy * axis[1] + fx * axis[0] + fz * axis[2];
                        axis[0] = fx;
                        axis[1] = fy;
                        axis[2] = fz;
                    }
                }
                float dot = Dot3X87(d2, d1);
                if (dot > 1.0f)
                    dot = 1.0f;
                else if (dot < -1.0f)
                    dot = -1.0f;
                float half = (std::acos(dot) * 0.5f) * align;
                const float wlim =
                    (static_cast<float>(li + 1) * chainAngle) * 2.0f;
                if (half >= 0.0f) {
                    if (wlim < half)
                        half = wlim;
                } else {
                    if (-wlim > half)
                        half = -wlim;
                }
                const float sinHalf = std::sin(half);
                float rotq[4] = {
                    sinHalf * axis[0],
                    sinHalf * axis[1],
                    sinHalf * axis[2],
                    std::cos(half)};
                float acc[4] = {link.rotQuat2[0], link.rotQuat2[1],
                                link.rotQuat2[2], link.rotQuat2[3]};
#ifdef RINMOTIONDANCE_DIAG
                WriteIkProbe(m, ci, iter, li, loops,
                             ik.boneIndex, ik.targetBone, child[li], d1, d2,
                             axis, dot, static_cast<float>(half), acc, rotq);
#endif
                float outq[4];
                QuatMul(outq, acc, rotq);              
                link.rotQuat2[0] = outq[0];
                link.rotQuat2[1] = outq[1];
                link.rotQuat2[2] = outq[2];
                link.rotQuat2[3] = outq[3];
                if (iter == 0) {                       
                    float q0[4] = {link.physicsQuat[0], link.physicsQuat[1], link.physicsQuat[2],
                                   link.physicsQuat[3]};
                    float linkQuat[4] = {link.rotQuat2[0], link.rotQuat2[1],
                                   link.rotQuat2[2], link.rotQuat2[3]};
                    QuatMul(outq, q0, linkQuat);             
                    link.rotQuat2[0] = outq[0];
                    link.rotQuat2[1] = outq[1];
                    link.rotQuat2[2] = outq[2];
                    link.rotQuat2[3] = outq[3];
                }
                float lq[4] = {link.rotQuat2[0], link.rotQuat2[1],
                               link.rotQuat2[2], link.rotQuat2[3]};
                D3DXMATRIXF rotM;
                MatRotQuat(&rotM, lq);
                if (pmx2 && link.twistEnable != 0) {
                    float eX = 0.0f, eY = 0.0f, eZ = 0.0f;
                    D3DXMATRIXF final;
                    if (link.ikLimitMin[0] > -kHalfPi &&
                        link.ikLimitMax[0] < kHalfPi) {
                        float a = std::asin(-rotM.m[2][1]);
                        float c = std::cos(a);
                        if (std::fabs(a) > kKneeClamp) {
                            a = a > 0.0f ? kKneeClamp : -kKneeClamp;
                            c = std::cos(a);
                        }
                        eX = a;
                        eY = std::atan2(rotM.m[2][0] / c, rotM.m[2][2] / c);
                        eZ = std::atan2(rotM.m[0][1] / c, rotM.m[1][1] / c);
                        ClampEuler(eX, link.ikLimitMin[0], link.ikLimitMax[0],
                                   iter >= twist);
                        ClampEuler(eY, link.ikLimitMin[1], link.ikLimitMax[1],
                                   iter >= twist);
                        ClampEuler(eZ, link.ikLimitMin[2], link.ikLimitMax[2],
                                   iter >= twist);
                        D3DXMATRIXF mx, my, mz, t1;
                        d3.rotX(&mx, eX);
                        d3.rotY(&my, eY);
                        d3.rotZ(&mz, eZ);
                        d3.mul(&t1, &mz, &mx);         
                        d3.mul(&final, &t1, &my);      
                    } else if (link.ikLimitMin[1] > -kHalfPi &&
                               link.ikLimitMax[1] < kHalfPi) {
                        float a = std::asin(-rotM.m[0][2]);
                        float c = std::cos(a);
                        if (std::fabs(a) > kKneeClamp) {
                            a = a > 0.0f ? kKneeClamp : -kKneeClamp;
                            c = std::cos(a);
                        }
                        eY = a;
                        eX = std::atan2(rotM.m[1][2] / c, rotM.m[2][2] / c);
                        eZ = std::atan2(rotM.m[0][1] / c, rotM.m[0][0] / c);
                        ClampEuler(eX, link.ikLimitMin[0], link.ikLimitMax[0],
                                   iter >= twist);
                        ClampEuler(eY, link.ikLimitMin[1], link.ikLimitMax[1],
                                   iter >= twist);
                        ClampEuler(eZ, link.ikLimitMin[2], link.ikLimitMax[2],
                                   iter >= twist);
                        D3DXMATRIXF mx, my, mz, t1;
                        d3.rotX(&mx, eX);
                        d3.rotY(&my, eY);
                        d3.rotZ(&mz, eZ);
                        d3.mul(&t1, &mx, &my);         
                        d3.mul(&final, &t1, &mz);      
                    } else {
                        float a = std::asin(-rotM.m[1][0]);
                        float c = std::cos(a);
                        if (std::fabs(a) > kKneeClamp) {
                            a = a > 0.0f ? kKneeClamp : -kKneeClamp;
                            c = std::cos(a);
                        }
                        eZ = a;
                        eX = std::atan2(rotM.m[1][2] / c, rotM.m[1][1] / c);
                        eY = std::atan2(rotM.m[2][0] / c, rotM.m[0][0] / c);
                        ClampEuler(eX, link.ikLimitMin[0], link.ikLimitMax[0],
                                   iter >= twist);
                        ClampEuler(eY, link.ikLimitMin[1], link.ikLimitMax[1],
                                   iter >= twist);
                        ClampEuler(eZ, link.ikLimitMin[2], link.ikLimitMax[2],
                                   iter >= twist);
                        D3DXMATRIXF mx, my, mz, t1;
                        d3.rotX(&mx, eX);
                        d3.rotY(&my, eY);
                        d3.rotZ(&mz, eZ);
                        d3.mul(&t1, &my, &mz);         
                        d3.mul(&final, &t1, &mx);      
                    }
                    rotM = final;
                    float nq[4];
                    d3.quatFromMatrix(nq, &rotM);
                    link.rotQuat2[0] = nq[0];
                    link.rotQuat2[1] = nq[1];
                    link.rotQuat2[2] = nq[2];
                    link.rotQuat2[3] = nq[3];
                } else if (!pmx2 && IsKnee(link) &&
                           std::atan2(rotM.m[2][1], rotM.m[2][2]) < 0.0f) {
                    rotM.m[1][2] = -rotM.m[1][2];
                    rotM.m[2][1] = -rotM.m[2][1];
                    link.rotQuat2[0] = -link.rotQuat2[0];
                }
                std::memcpy(link.matLocal, &rotM, sizeof(rotM));
                {
                    D3DXMATRIXF tmp, cur;
                    d3.translation(&tmp, -link.position[0], -link.position[1],
                                   -link.position[2]);
                    std::memcpy(&cur, link.matLocal, sizeof(cur));
                    d3.mul(&cur, &tmp, &cur);
                    d3.translation(&tmp, link.physicsOffset[0], link.physicsOffset[1],
                                   link.physicsOffset[2]);
                    d3.mul(reinterpret_cast<D3DXMATRIXF*>(link.matLocal),
                           &cur, &tmp);
                    std::memcpy(&cur, link.matLocal, sizeof(cur));
                    d3.translation(&tmp, link.position[0], link.position[1],
                                   link.position[2]);
                    d3.mul(reinterpret_cast<D3DXMATRIXF*>(link.matLocal),
                           &cur, &tmp);
                }
                for (int lj = li; lj >= 0; --lj) {
                    mdl::BoneRecord& linkedBone = boneRecords[child[lj]];
                    const bool j3 = (physicsMode == 3);
                    if ((linkedBone.hasRigidBody & (linkedBone.physicsDisabled == 0 ? 1 : 0) &
                         (j3 ? 1 : 0)) == 0) {
                        WorldFromParent(linkedBone,
                                        boneRecords[linkedBone.parent], &d3);
                        if (((physicsMode == 1 || (physicsMode >= 2 && linkedBone.physicsDisabled == 0)) &
                             linkedBone.hasRigidBody) == 0)
                            std::memcpy(linkedBone.matExtra,
                                        linkedBone.matInit,
                                        sizeof(linkedBone.matExtra));
                    }
                }
                {
                    mdl::BoneRecord& root = ikRoot;
                    WorldFromParent(root, boneRecords[root.parent], &d3);
                    if (((physicsMode == 1 || (physicsMode >= 2 && root.physicsDisabled == 0)) &
                         root.hasRigidBody) == 0)
                        std::memcpy(root.matExtra, root.matInit,
                                    sizeof(root.matExtra));
                }
            }
            if (exitAll)
                break;
            ++iter;
        }
    }
}

void BoneTransform_PostIkPass(D3& d3, mdl::BoneRecord* boneRecords,
                              int boneCount, unsigned char afterPhysics,
                              int layer, int physicsMode) {
    for (int i = 0; i < boneCount; ++i) {
        mdl::BoneRecord& bone = boneRecords[i];
        if (!ChannelPass(bone, afterPhysics))
            continue;
        if (bone.layer != layer || bone.type == mdl::BoneType::UnderIk ||
            bone.hasFlag == 0)
            continue;
        if (bone.type == mdl::BoneType::CoRotate) {    
            const mdl::BoneRecord& tail = boneRecords[bone.tailBone];
            const float* sourceQuat =
                tail.type == mdl::BoneType::UnderIk ? tail.rotQuat2
                                                    : tail.physicsQuat;
            float sq[4] = {sourceQuat[0], sourceQuat[1], sourceQuat[2],
                           sourceQuat[3]};
            if (sq[3] <= 1.0f) {
                if (sq[3] < -1.0f) sq[3] = -1.0f;
            } else {
                sq[3] = 1.0f;
            }
            float angle = std::acos(sq[3]);
            if (sq[3] < 0.0f)
                angle -= kPiF;
            const float len = std::sqrt(sq[0] * sq[0] + sq[1] * sq[1] +
                                        sq[2] * sq[2]);
            if (angle == 0.0f || len < kEps) {
                bone.physicsQuat[0] = bone.physicsQuat[1] = bone.physicsQuat[2] = 0.0f;
                bone.physicsQuat[3] = 1.0f;
            } else {
                const float scaled =
                    (static_cast<float>(bone.tailIdx) * angle) / 100.0f;
                const float s = std::sin(scaled) / len;
                bone.physicsQuat[0] = s * sq[0];
                bone.physicsQuat[1] = s * sq[1];
                bone.physicsQuat[2] = s * sq[2];
                bone.physicsQuat[3] = std::cos(scaled);
            }
        }
        float q[4] = {bone.physicsQuat[0], bone.physicsQuat[1], bone.physicsQuat[2], bone.physicsQuat[3]};
        if (bone.type == mdl::BoneType::RotateGrant) { 
            const mdl::BoneRecord& src = boneRecords[bone.tailIdx];
            const float* sourceQuat =
                src.type == mdl::BoneType::UnderIk ? src.rotQuat2 : src.physicsQuat;
            float sq[4] = {sourceQuat[0], sourceQuat[1], sourceQuat[2],
                           sourceQuat[3]};
            float out[4];
            QuatMul(out, sq, q);
            q[0] = out[0];
            q[1] = out[1];
            q[2] = out[2];
            q[3] = out[3];
        }
        if (bone.flags & mdl::kBoneFlagRotInherit)
            InheritRotQuat(q, boneRecords, bone.tailIdx, bone.inheritRatio);
        MatRotQuat(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), q);
        float inhX = 0.0f, inhY = 0.0f, inhZ = 0.0f;
        if (bone.flags & mdl::kBoneFlagTransInherit) {
            const mdl::BoneRecord& src = boneRecords[bone.tailIdx];
            const float r = bone.inheritRatio;
            inhX = src.physicsOffset[0] * r;
            inhY = src.physicsOffset[1] * r;
            inhZ = src.physicsOffset[2] * r;
            D3DXMATRIXF tmp, cur;
            d3.translation(&tmp, inhX, inhY, inhZ);
            std::memcpy(&cur, bone.matLocal, sizeof(cur));
            d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
        }
        D3DXMATRIXF tmp, cur;
        d3.translation(&tmp, -bone.position[0], -bone.position[1],
                       -bone.position[2]);
        std::memcpy(&cur, bone.matLocal, sizeof(cur));
        d3.mul(&cur, &tmp, &cur);
        d3.translation(&tmp, bone.physicsOffset[0], bone.physicsOffset[1], bone.physicsOffset[2]);
        d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
        std::memcpy(&cur, bone.matLocal, sizeof(cur));
        d3.translation(&tmp, bone.position[0], bone.position[1],
                       bone.position[2]);
        d3.mul(reinterpret_cast<D3DXMATRIXF*>(bone.matLocal), &cur, &tmp);
        if (bone.flags & mdl::kBoneFlagTransInherit) {
            bone.physicsOffset[0] += inhX;
            bone.physicsOffset[1] += inhY;
            bone.physicsOffset[2] += inhZ;
        }
        WorldFromParent(bone, boneRecords[bone.parent], &d3);
        if (CopyGate(bone, physicsMode))                       
            std::memcpy(bone.matExtra, bone.matInit, sizeof(bone.matExtra));
    }
}

}  

void ArmBoneTransformIkProbe() {
#ifdef RINMOTIONDANCE_DIAG
    InterlockedExchange(&gIkProbeArmed, 1);
    InterlockedExchange(&gIkProbeWritten, 0);
#endif
}

void BoneFrameTransform(unsigned char* m, unsigned char afterPhysics,
                        int layer, unsigned char* const* modelSlots,
                        int physicsMode) {                             
    D3 d3;
    mdl::ModelRecord& model = *mdl::Mdl(m);
    const int boneCount = static_cast<int>(model.boneCount);
    mdl::BoneRecord* boneRecords = model.boneTable;
    mdl::IkChain* iks = mdl::IkChains(m);
    const int ikCount = static_cast<int>(model.ikChainCount);
    const bool pmx2 = model.physicsMode == 2;
    mdl::BoneOrderEntry* extTable = mdl::BoneOrder(m);

    BoneTransform_PhysicsSweptLocals(d3, boneRecords, boneCount,
                                     afterPhysics,
                                     layer);                         
    BoneTransform_StandardLocals(d3, boneRecords, boneCount,
                                 afterPhysics,
                                 layer);                             
    BoneTransform_WorldPass(d3, boneRecords, boneCount, extTable,
                            modelSlots, afterPhysics, layer,
                            physicsMode);                            
    BoneTransform_CcdIk(d3, m, boneRecords, iks, ikCount, afterPhysics,
                        layer, physicsMode,
                        pmx2);                                        
    BoneTransform_PostIkPass(d3, boneRecords, boneCount, afterPhysics,
                              layer,
                              physicsMode);                          
}

}  