#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/physics_world.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "btBulletDynamicsCommon.h"

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "../app/frame_state_dump.hpp"

namespace rinmotiondance {

using d3dx::D3DXMATRIXF;

namespace {

namespace {
void TransformToMatrix(float out[16], const btTransform& t) {
    const btMatrix3x3& b = t.getBasis();
    const btVector3& o = t.getOrigin();
    out[0] = b[0].x();  out[1] = b[1].x();  out[2] = b[2].x();  out[3] = 0.0f;
    out[4] = b[0].y();  out[5] = b[1].y();  out[6] = b[2].y();  out[7] = 0.0f;
    out[8] = b[0].z();  out[9] = b[1].z();  out[10] = b[2].z(); out[11] = 0.0f;
    out[12] = o.x();    out[13] = o.y();    out[14] = o.z();    out[15] = 1.0f;
}

void MatrixToBasis(btTransform* t, const float* m) {
    t->getBasis().setValue(
        m[0], m[4], m[8],
        m[1], m[5], m[9],
        m[2], m[6], m[10]);
    t->setOrigin(btVector3(m[12], m[13], m[14]));
}

inline float F32(const unsigned char* p, std::size_t off) {
    return *reinterpret_cast<const float*>(p + off);
}

inline float& F32R(unsigned char* p, std::size_t off) {
    return *reinterpret_cast<float*>(p + off);
}

D3DXMATRIXF* D3dxRotX(D3DXMATRIXF* o, float a) {
    return d3dx::Get().rotX(o, a);
}
D3DXMATRIXF* D3dxRotY(D3DXMATRIXF* o, float a) {
    return d3dx::Get().rotY(o, a);
}
D3DXMATRIXF* D3dxRotZ(D3DXMATRIXF* o, float a) {
    return d3dx::Get().rotZ(o, a);
}
D3DXMATRIXF* D3dxMul(D3DXMATRIXF* o, const D3DXMATRIXF* a, const D3DXMATRIXF* b) {
    return d3dx::Get().multiply(o, a, b);
}
float* D3dxVec3Normalize(float o[3], const float* src) {
    return d3dx::Get().vec3Normalize(o, src);
}
D3DXMATRIXF* D3dxTranslation(D3DXMATRIXF* o, float x, float y, float z) {
    return d3dx::Get().translation(o, x, y, z);
}
D3DXMATRIXF* D3dxInverse(D3DXMATRIXF* o, const D3DXMATRIXF* m) {
    return d3dx::Get().inverse(o, nullptr, m);
}
float* D3dxQuatFromMatrix(float out[4], const D3DXMATRIXF* m) {
    return d3dx::Get().quatFromMatrix(out, m);
}
float* D3dxQuatMul(float out[4], const float a[4], const float b[4]) {
    return d3dx::Get().quatMultiply(out, a, b);
}
}  

void BuildFollowMatrix(float out[16], unsigned char* m,
                       const mdl::RigidRecord& rigid,
                       int boneOverride) {
    mdl::BoneRecord* bones = mdl::Bones(m);
    D3DXMATRIXF t;
    auto* o = reinterpret_cast<D3DXMATRIXF*>(out);
    D3dxRotZ(o, rigid.rotation[2]);
    D3dxRotX(&t, rigid.rotation[0]);
    D3dxMul(o, o, &t);
    D3dxRotY(&t, rigid.rotation[1]);
    D3dxMul(o, o, &t);
    D3dxTranslation(&t, rigid.position[0], rigid.position[1],
                    rigid.position[2]);
    D3dxMul(o, o, &t);
    const int bone = boneOverride >= 0 ? boneOverride : rigid.boneIndex;
    if (bone >= 0) {
        const mdl::BoneRecord& bn = bones[bone];
        D3dxTranslation(&t, bn.position[0], bn.position[1], bn.position[2]);
        D3dxMul(o, o, &t);
        D3dxMul(o, o, reinterpret_cast<const D3DXMATRIXF*>(bn.matInit));
    }
}

inline btRigidBody* BodyOf(const mdl::RigidRecord& rigid) {
    return static_cast<btRigidBody*>(rigid.body);
}

void ReseatRigidBody(btRigidBody* body, const btTransform& transform) {
    const btVector3 zero(0.0f, 0.0f, 0.0f);
    body->setWorldTransform(transform);
    body->setLinearVelocity(zero);
    body->setAngularVelocity(zero);
    body->setInterpolationLinearVelocity(zero);
    body->setInterpolationAngularVelocity(zero);
}

void StopBodyAt(btRigidBody* body, const btTransform& transform) {
    const btVector3 zero(0.0f, 0.0f, 0.0f);
    body->setWorldTransform(transform);
    body->setLinearVelocity(zero);
    body->setAngularVelocity(zero);
}

#ifdef RINMOTIONDANCE_DIAG

template <typename T>
void WriteStateWords(std::FILE* stream, const char* name, const T& value) {
    std::fprintf(stream, "],\"%s\":[", name);
    const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
    for (std::size_t word = 0; word < sizeof(value) / 4; ++word) {
        if (word != 0)
            std::fputc(',', stream);
        std::uint32_t bits = 0;
        std::memcpy(&bits, bytes + 4 * word, sizeof(bits));
        std::fprintf(stream, "\"%08X\"", bits);
    }
}

void WriteEmptyStateField(std::FILE* stream, const char* name) {
    std::fprintf(stream, "],\"%s\":[", name);
}

void DumpRigidBodyState(MMDApp* app, const char* fileName) {
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_STATE_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;

    CreateDirectoryA(directory, nullptr);
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\%s", directory, fileName);
    std::FILE* stream = nullptr;
    if (fopen_s(&stream, path, "wb") != 0 || stream == nullptr)
        return;

    std::fputs("{\n  \"schema\": 1,\n  \"models\": [", stream);
    bool firstModel = true;
    for (unsigned slot = 0; slot < static_cast<unsigned>(kModelSlotCount);
         ++slot) {
        auto* model = app->ModelSlot(slot);
        if (model == nullptr)
            continue;
        auto* rigids = mdl::Rigids(model);
        const int count = static_cast<int>(mdl::Mdl(model)->rigidCount);
        if (!firstModel)
            std::fputc(',', stream);
        firstModel = false;
        std::fprintf(stream, "\n    {\"slot\": %u, \"rigid_count\": %d, "
                             "\"rigids\": [", slot, count);
        if (rigids != nullptr && count >= 0 && count <= 100000) {
            for (int i = 0; i < count; ++i) {
                if (i != 0)
                    std::fputc(',', stream);
                const mdl::RigidRecord& rec = rigids[i];
                btRigidBody* body = BodyOf(rec);
                std::fprintf(stream,
                    "{\"index\":%d,\"bone\":%d,\"mode\":%d,"
                    "\"world_bits\":[",
                    i, rec.boneIndex,
                    static_cast<int>(static_cast<signed char>(rec.mode)));
                if (body != nullptr) {
                    const auto* transform = reinterpret_cast<const unsigned char*>(
                        &body->getWorldTransform());
                    for (int word = 0; word < 16; ++word) {
                        if (word != 0)
                            std::fputc(',', stream);
                        std::uint32_t value = 0;
                        std::memcpy(&value, transform + 4 * word, 4);
                        std::fprintf(stream, "\"%08X\"", value);
                    }
                }
                if (body != nullptr) {
                    WriteStateWords(stream, "interpolation_world_bits",
                                    body->getInterpolationWorldTransform());
                    WriteStateWords(stream, "interpolation_linear_bits",
                                    body->getInterpolationLinearVelocity());
                    WriteStateWords(stream, "interpolation_angular_bits",
                                    body->getInterpolationAngularVelocity());
                    WriteStateWords(stream, "linear_bits",
                                    body->getLinearVelocity());
                    WriteStateWords(stream, "angular_bits",
                                    body->getAngularVelocity());
                } else {
                    WriteEmptyStateField(stream, "interpolation_world_bits");
                    WriteEmptyStateField(stream, "interpolation_linear_bits");
                    WriteEmptyStateField(stream, "interpolation_angular_bits");
                    WriteEmptyStateField(stream, "linear_bits");
                    WriteEmptyStateField(stream, "angular_bits");
                }
                std::fputs("]}", stream);
            }
        }
        std::fputs("]}", stream);
    }
    if (!firstModel)
        std::fputc('\n', stream);
    std::fputs("  ]\n}\n", stream);
    std::fclose(stream);
}

void DumpWorldSolverInfo(btDiscreteDynamicsWorld* world) {
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_STATE_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH || world == nullptr)
        return;

    CreateDirectoryA(directory, nullptr);
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\physics_world.json", directory);
    std::FILE* stream = nullptr;
    if (fopen_s(&stream, path, "wb") != 0 || stream == nullptr)
        return;

    const btContactSolverInfo& info = world->getSolverInfo();
    const auto* worldBytes = reinterpret_cast<const unsigned char*>(world);
    const auto* infoBytes = reinterpret_cast<const unsigned char*>(&info);
    std::fprintf(stream,
                 "{\n  \"solver_info_offset\": %u,\n"
                 "  \"solver_info_size\": %u,\n  \"words\": [",
                 static_cast<unsigned>(infoBytes - worldBytes),
                 static_cast<unsigned>(sizeof(info)));
    for (std::size_t i = 0; i < sizeof(info) / 4; ++i) {
        if (i != 0)
            std::fputc(',', stream);
        std::uint32_t word = 0;
        std::memcpy(&word, infoBytes + i * 4, 4);
        std::fprintf(stream, "\"%08X\"", word);
    }
    std::fputs("]\n}\n", stream);
    std::fclose(stream);
}

#endif  

}  

void ModelKinematicSync(unsigned char* m) {
    const int count = mdl::Mdl(m)->rigidCount;
    if (count <= 0)
        return;
    mdl::RigidRecord* rigids = mdl::Rigids(m);
    for (int i = 0; i < count; ++i) {
        const mdl::RigidRecord& rec = rigids[i];
        if (rec.mode != 0)
            continue;
        btRigidBody* body = BodyOf(rec);
        if (body == nullptr)
            continue;                                       
        float mat[16];
        BuildFollowMatrix(mat, m, rec, -1);
        btTransform tr;
        tr.setIdentity();
        MatrixToBasis(&tr, mat);
        if (btMotionState* ms = body->getMotionState())
            ms->setWorldTransform(tr);
    }
}

void ModelDynamicReseat(unsigned char* m) {
    const int count = mdl::Mdl(m)->rigidCount;
    if (count <= 0)
        return;
    mdl::RigidRecord* rigids = mdl::Rigids(m);
    const int centerBone = mdl::Mdl(m)->centerBone;   
#ifdef RINMOTIONDANCE_DIAG
    static int reseatCalls = 0;
    const bool traceReseat = getenv("RINMOTIONDANCE_TRACE_RESEAT") != nullptr;
    if (traceReseat && reseatCalls < 12) {
        int active = 0;
        for (int i = 0; i < count; ++i)
            if (static_cast<signed char>(rigids[i].mode) > 0)
                ++active;
        std::fprintf(stderr,
                     "RESEAT call=%d count=%d active(rec80>0)=%d\n",
                     reseatCalls, count, active);
    }
    ++reseatCalls;
#endif
    for (int i = 0; i < count; ++i) {
        const mdl::RigidRecord& rec = rigids[i];
        if (static_cast<signed char>(rec.mode) <= 0)
            continue;
        btRigidBody* body = BodyOf(rec);
        if (body == nullptr)
            continue;                                       
#ifdef RINMOTIONDANCE_DIAG
        if (traceReseat && reseatCalls <= 3) {
            const int id = body->m_debugBodyId;
            if (id == 3) {
                unsigned bits;
                const float ox = body->getWorldTransform().getOrigin().getX();
                std::memcpy(&bits, &ox, 4);
                std::fprintf(stderr,
                             "RESEAT body3 iter=%d rec80=%d pre_origin=%08X\n",
                             reseatCalls, (int)rec.mode, bits);
            }
        }
#endif
        float mat[16];
        const int link = rec.boneIndex;
        BuildFollowMatrix(mat, m, rec, link < 0 ? centerBone : -1);
        btTransform tr;
        tr.setIdentity();
        MatrixToBasis(&tr, mat);
        ReseatRigidBody(body, tr);
    }
}

void ModelPhysicsReadback(unsigned char* m) {
    mdl::BoneRecord* bones = mdl::Bones(m);
    mdl::RigidRecord* rigids = mdl::Rigids(m);
    const int rcount = mdl::Mdl(m)->rigidCount;
    mdl::JointRecord* joints = mdl::Joints(m);
    const int jcount = mdl::Mdl(m)->jointCount;

    for (int j = 0; j < jcount; ++j) {
        const mdl::JointRecord& joint = joints[j];
        const int ra = joint.rigidA;
        const int rb = joint.rigidB;
        if (ra < 0 || ra >= rcount || rb < 0 || rb >= rcount)
            continue;                                       
        btRigidBody* bodyA = BodyOf(rigids[ra]);
        btRigidBody* bodyB = BodyOf(rigids[rb]);
        if (bodyA == nullptr || bodyB == nullptr)
            continue;                                       
        float ma[16], mb[16];
        TransformToMatrix(ma, bodyA->getWorldTransform());
        TransformToMatrix(mb, bodyB->getWorldTransform());
        const float dy = ma[13] - mb[13], dx = ma[12] - mb[12],
                    dz = ma[14] - mb[14];
        const float d2 = dy * dy + dx * dx + dz * dz;
        const float dist = std::sqrt(d2);
        const float limit = joint.radiusBound;
        if (dist - limit < 2.0f)
            continue;
        const float k = limit / dist;
        float t[16];
        D3dxTranslation(reinterpret_cast<D3DXMATRIXF*>(t),
                        (mb[12] - ma[12]) * k,
                        (mb[13] - ma[13]) * k,
                        (mb[14] - ma[14]) * k);
        D3dxMul(reinterpret_cast<D3DXMATRIXF*>(ma),
                reinterpret_cast<const D3DXMATRIXF*>(ma),
                reinterpret_cast<const D3DXMATRIXF*>(t));
        btTransform tr = bodyB->getWorldTransform();
        tr.setOrigin(btVector3(ma[12], ma[13], ma[14]));
        StopBodyAt(bodyB, tr);   
    }

    for (int i = 0; i < rcount; ++i) {
        const mdl::RigidRecord& rec = rigids[i];
        if (static_cast<signed char>(rec.mode) <= 0)
            continue;
        const int bone = rec.boneIndex;
        if (bone < 0)
            continue;
        btRigidBody* body = BodyOf(rec);
        if (body == nullptr)
            continue;                                       
        float w[16];
        TransformToMatrix(w, body->getWorldTransform());
        D3dxMul(reinterpret_cast<D3DXMATRIXF*>(mdl::Bones(m)[bone].matInit),
                reinterpret_cast<const D3DXMATRIXF*>(rec.invTransform),
                reinterpret_cast<const D3DXMATRIXF*>(w));
    }

    for (int i = 0; i < rcount; ++i) {
        const mdl::RigidRecord& rigid = rigids[i];
        if (static_cast<signed char>(rigid.mode) <= 0)
            continue;
        const int bi = rigid.boneIndex;
        if (bi < 0)
            continue;
        mdl::BoneRecord& bone = mdl::Bones(m)[bi];
        float* world = bone.matInit;
        float* local = bone.matLocal;
        const int parent = bone.parent;
        float inv[16];
        bool haveLocal;
        if (parent < 0) {
            std::memcpy(local, world, 16 * sizeof(float));
            haveLocal = true;
        } else if (D3dxInverse(reinterpret_cast<D3DXMATRIXF*>(inv),
                               reinterpret_cast<const D3DXMATRIXF*>(
                                   mdl::Bones(m)[parent].matInit))) {
            D3dxMul(reinterpret_cast<D3DXMATRIXF*>(local),
                    reinterpret_cast<const D3DXMATRIXF*>(world),
                    reinterpret_cast<const D3DXMATRIXF*>(inv));
            haveLocal = true;
        } else {
            haveLocal = false;
        }
        if (!haveLocal) {
            std::memcpy(bone.matExtra, bone.matInit,
                        sizeof(bone.matExtra));               
            continue;
        }

        float* q = bone.ikBackup + 3;                         
        D3dxQuatFromMatrix(q, reinterpret_cast<const D3DXMATRIXF*>(local));

        float work[16], pos[16], t[16];
        D3dxTranslation(reinterpret_cast<D3DXMATRIXF*>(t), bone.position[0],
                        bone.position[1], bone.position[2]);
        D3dxMul(reinterpret_cast<D3DXMATRIXF*>(work),
                reinterpret_cast<const D3DXMATRIXF*>(t),
                reinterpret_cast<const D3DXMATRIXF*>(local));
        D3dxTranslation(reinterpret_cast<D3DXMATRIXF*>(t), -bone.position[0],
                        -bone.position[1], -bone.position[2]);
        D3dxMul(reinterpret_cast<D3DXMATRIXF*>(pos),
                reinterpret_cast<const D3DXMATRIXF*>(work),
                reinterpret_cast<const D3DXMATRIXF*>(t));
        std::memcpy(bone.ikBackup, pos + 12, 3 * sizeof(float));

        const int target = bone.tailIdx;
        mdl::BoneRecord& tb = target >= 0 ? mdl::Bones(m)[target] : bone;

        if (bone.type == mdl::BoneType::RotateGrant) {        
            float conj[4] = {-tb.physicsQuat[0], -tb.physicsQuat[1],
                             -tb.physicsQuat[2], tb.physicsQuat[3]};
            float out[4];
            D3dxQuatMul(out, conj, q);
            std::memcpy(q, out, sizeof out);
        }
        if ((bone.flags & mdl::kBoneFlagRotInherit) != 0) {
            float limit[4];
            D3dxQuatFromMatrix(limit, reinterpret_cast<const D3DXMATRIXF*>(
                                           tb.matLocal));
            if (limit[3] > 1.0f) limit[3] = 1.0f;
            if (limit[3] < -1.0f) limit[3] = -1.0f;
            float angle = std::acos(limit[3]);
            if (limit[3] < 0.0f)
                angle -= 3.140000104904175f;                 
            const float axisLen = std::sqrt(limit[1] * limit[1] +
                                            limit[0] * limit[0] +
                                            limit[2] * limit[2]);
            if (angle != 0.0f && axisLen >= 0.00000011920929f) {
                const float a = angle * bone.inheritRatio;
                const float s = std::sin(a) / axisLen;
                const float rot[4] = {-limit[0] * s, -limit[1] * s,
                                      -limit[2] * s,
                                      std::cos(a)};
                float out[4];
                if (parent >= 0)
                    D3dxQuatMul(out, rot, q);
                else
                    D3dxQuatMul(q, q, rot);
                if (parent >= 0)
                    std::memcpy(q, out, sizeof out);
            }
        }
        if ((bone.flags & mdl::kBoneFlagTransInherit) != 0) {
            for (int c = 0; c < 3; ++c)
                bone.physicsOffset[c] -= tb.physicsOffset[c] * bone.inheritRatio;
        }
        if (rigid.mode == 2) {
            std::memset(bone.trans, 0, sizeof(bone.trans));
            std::memset(bone.ikBackup, 0, 3 * sizeof(float));
            work[12] = work[13] = work[14] = 0.0f;
            D3dxTranslation(reinterpret_cast<D3DXMATRIXF*>(t),
                            -bone.position[0], -bone.position[1],
                            -bone.position[2]);
            D3dxMul(reinterpret_cast<D3DXMATRIXF*>(local),
                    reinterpret_cast<const D3DXMATRIXF*>(t),
                    reinterpret_cast<const D3DXMATRIXF*>(work));
            D3dxTranslation(reinterpret_cast<D3DXMATRIXF*>(t),
                            bone.position[0], bone.position[1],
                            bone.position[2]);
            D3dxMul(reinterpret_cast<D3DXMATRIXF*>(local),
                    reinterpret_cast<const D3DXMATRIXF*>(local),
                    reinterpret_cast<const D3DXMATRIXF*>(t));
            if (parent >= 0)
                D3dxMul(reinterpret_cast<D3DXMATRIXF*>(world),
                        reinterpret_cast<const D3DXMATRIXF*>(local),
                        reinterpret_cast<const D3DXMATRIXF*>(
                            mdl::Bones(m)[parent].matInit));
        }
        std::memcpy(bone.matExtra, bone.matInit, sizeof(bone.matExtra));
    }
}

void PhysicsFrame(MMDApp* app, unsigned char selActive) {
    auto& s = *app;
    if (s.state.messageSeen == 0)
        return;                       
    PhysicsScene* scene = s.Physics();
    if (scene == nullptr)
        return;
    btDiscreteDynamicsWorld* world = scene->world;   
    if (world == nullptr)
        return;
    const int count = s.PlaybackPhysicsMode();
    bool windRan = false;
    if (count > 0 && s.state.gravityNoiseEnabled != 0) {
        s.state.gravityNoiseTimer = s.state.gravityNoiseTimer + s.DeltaTime();
        if (s.state.gravityNoiseTimer > 0.5f) {                          
#if defined(_M_IX86)
            const int strengthInt =
                s.GravityNoise();          
            const float gravmag = s.GravityMagnitude();
            static const double kTen = 10.0;            
            static const double kRandMax = 32767.0;     
            static const double kNoise = 0.20000000298023224;   
            static const double kOffset = 0.10000000149011612;  
            int rStrength = std::rand(), rX = std::rand(),
                rY = std::rand(), rZ = std::rand();
            float strengthF, e0, e1, e2;
            const float dirX = s.GravityX(),
                        dirY = s.GravityY(),
                        dirZ = s.GravityZ();
            __asm {
                fild        rStrength
                fmul        qword ptr [kTen]
                fdiv        qword ptr [kRandMax]
                fimul       strengthInt
                fadd        gravmag
                fstp        strengthF
                fild        rX
                fmul        qword ptr [kNoise]
                fdiv        qword ptr [kRandMax]
                fsub        qword ptr [kOffset]
                fadd        dirX
                fmul        strengthF
                fstp        e0
                fild        rY
                fmul        qword ptr [kNoise]
                fdiv        qword ptr [kRandMax]
                fsub        qword ptr [kOffset]
                fadd        dirY
                fmul        strengthF
                fstp        e1
                fild        rZ
                fmul        qword ptr [kNoise]
                fdiv        qword ptr [kRandMax]
                fsub        qword ptr [kOffset]
                fadd        dirZ
                fmul        strengthF
                fstp        e2
            }
            world->setGravity(btVector3(e0, e1, e2));
            s.state.gravityNoiseTimer = 0.0f;                     
#else
            const int rStrength = std::rand();
            const float noise =
                static_cast<float>(s.GravityNoise());
            const float gravmag = s.GravityMagnitude();
            const float strength =
                ((static_cast<float>(rStrength) * 10.0f) / 32767.0f) *
                    noise + gravmag;
            const float e0 =
                ((static_cast<float>(std::rand()) * 0.2f) / 32767.0f -
                 0.1f + s.GravityX()) * strength;
            const float e1 =
                ((static_cast<float>(std::rand()) * 0.2f) / 32767.0f -
                 0.1f + s.GravityY()) * strength;
            const float e2 =
                ((static_cast<float>(std::rand()) * 0.2f) / 32767.0f -
                 0.1f + s.GravityZ()) * strength;
            world->setGravity(btVector3(e0, e1, e2));
            s.state.gravityNoiseTimer = 0.0f;                     
#endif
        }
        windRan = true;  
    }
    if (count > 0 && !windRan) {
        float dir[3] = {s.state.gravityX,
                        s.state.gravityY,
                        s.state.gravityZ};
        if (dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f)
            s.state.gravityY = dir[1] = 0.1f;                  
        D3dxVec3Normalize(dir, dir);                          
        const float mag = s.state.gravityMagnitude;
        world->setGravity(btVector3(
            (dir[0] * mag) * 10.0f,
            (dir[1] * mag) * 10.0f,
            (dir[2] * mag) * 10.0f));                         
    }
    unsigned char** models = s.ModelSlots();
#ifdef RINMOTIONDANCE_DIAG
    char stageCaptureValue[2]{};
    static LONG stagesCaptured = 0;
    const bool captureStages =
        GetEnvironmentVariableA("RINMOTIONDANCE_AB_PHYSICS_STAGES",
                                stageCaptureValue,
                                sizeof(stageCaptureValue)) == 1 &&
        stageCaptureValue[0] == '1' &&
        s.ModelSlots()[0] != nullptr &&
        InterlockedCompareExchange(&stagesCaptured, 1, 0) == 0;
    if (captureStages)
        ArmBoneTransformIkProbe();
    if (captureStages)
        DumpWorldSolverInfo(world);
#endif

    if (selActive != 0 &&
        s.state.optflag[0] == 0 &&        
        s.state.frameStepPlayback == 0) {     
        if (s.state.selectionActiveLatch == 0)
            s.PhysicsResetPending() = 1;
    }

    if (s.state.accessoryEditDialogOpen != 0)
        return;                       

#ifdef RINMOTIONDANCE_DIAG
    int stepCount = 0;
    const char* stepLabels[16]{};
    const bool traceSteps = getenv("RINMOTIONDANCE_TRACE_STEPS") != nullptr;
    if (traceSteps) {
        const float localTime =
            static_cast<const PhysicsWorld*>(world)->SubstepRemainder();
        unsigned localBits = 0;
        std::memcpy(&localBits, &localTime, 4);
        fprintf(stderr,
                "PHYSFRAME enter settle=%d count=%d localTime=%08X\n",
                (int)(s.PhysicsResetPending() != 0), count,
                localBits);
    }
    auto stepWorld = [world, &stepCount, &stepLabels](const char* label) {
        if (stepCount < 16) stepLabels[stepCount] = label;
        ++stepCount;
        world->stepSimulation(1.0f / 60.0f, 10, 1.0f / 60.0f);  
    };
#else
    auto stepWorld = [world](const char*) {
        world->stepSimulation(1.0f / 60.0f, 10, 1.0f / 60.0f);  
    };
#endif

    float mainTimeStep = g_CatchupDtBudget;
#ifdef RINMOTIONDANCE_DIAG
    if (const char* forced = getenv("RINMOTIONDANCE_FORCE_MAINDT"))
        mainTimeStep = static_cast<float>(atof(forced));
    static bool zeroLocalTimeAfterLoad =
        getenv("RINMOTIONDANCE_ZERO_LT_AFTER_LOAD") != nullptr;
#endif

    const int posePassMode = s.PhysicsResetPending() != 0
                       ? 0
                       : count;
    for (int order = 0; order < kModelSlotCount; ++order) {
        for (int j = 0; j < kModelSlotCount; ++j) {
            unsigned char* mdl = models[j];
            if (mdl != nullptr && mdl::Mdl(mdl)->comboSelIndex2 == order) {
                ModelApplyMorphs(mdl);                            
                SetPhysicsMode(mdl, 0, models, posePassMode);
            }
        }
    }
#ifdef RINMOTIONDANCE_DIAG
    if (captureStages)
        DumpFrameEntryState(app, "physics_pose0.json", false);
#endif

    const bool moved = s.state.physicsBodiesMoved != 0;
    const bool settle = s.PhysicsResetPending() != 0;
    const bool frameAdv = s.PlaybackActive() != 0;           
    const bool runWorldPass = count == 1 || count == 2 ||
        (count == 3 && (moved || settle || frameAdv));
#ifdef RINMOTIONDANCE_DIAG
    const bool idleNoStep = getenv("RINMOTIONDANCE_IDLE_NO_STEP") != nullptr &&
        !(moved || settle || frameAdv);
#else
    constexpr bool idleNoStep = false;
#endif
    if (runWorldPass && !idleNoStep &&
        s.state.frameCopyDialog == 0) {
        for (int j = 0; j < kModelSlotCount; ++j)
            if (models[j] != nullptr)
                ModelKinematicSync(models[j]);              
#ifdef RINMOTIONDANCE_DIAG
        if (captureStages)
            DumpRigidBodyState(app, "physics_sync.rigids.json");
        if (captureStages)
            DumpFrameEntryState(app, "physics_sync.json", false);
#endif

        const float mainDt =
            (moved && count == 3) ? (1.0f / 60.0f) : mainTimeStep;
#ifdef RINMOTIONDANCE_DIAG
        if (stepCount < 16) stepLabels[stepCount] = "main";
        ++stepCount;
#endif
        world->stepSimulation(mainDt, 10, 1.0f / 60.0f);
        if (moved && count == 3)
            stepWorld("extra");                              
#ifdef RINMOTIONDANCE_DIAG
        if (captureStages)
            DumpRigidBodyState(app, "physics_step.rigids.json");
        if (captureStages)
            DumpFrameEntryState(app, "physics_step.json", false);
#endif

        if (settle) {
            for (int iter = 3; iter >= 1; --iter) {
                for (int j = 0; j < kModelSlotCount; ++j)
                    if (models[j] != nullptr)
                        ModelDynamicReseat(models[j]);      
#ifdef RINMOTIONDANCE_DIAG
                if (captureStages) {
                    char name[64]{};
                    std::snprintf(name, sizeof(name),
                                  "physics_settle_pre%d.rigids.json", 4 - iter);
                    DumpRigidBodyState(app, name);
                }
#endif
                stepWorld("settle");
#ifdef RINMOTIONDANCE_DIAG
                if (captureStages) {
                    char name[64]{};
                    std::snprintf(name, sizeof(name),
                                  "physics_settle_post%d.rigids.json", 4 - iter);
                    DumpRigidBodyState(app, name);
                }
#endif
            }
            s.PhysicsResetPending() = 0;
        }
#ifdef RINMOTIONDANCE_DIAG
        if (captureStages)
            DumpRigidBodyState(app, "physics_settle.rigids.json");
        if (captureStages)
            DumpFrameEntryState(app, "physics_settle.json", false);
#endif

        for (int j = 0; j < kModelSlotCount; ++j)
            if (models[j] != nullptr)
                ModelPhysicsReadback(models[j]);            
#ifdef RINMOTIONDANCE_DIAG
        if (captureStages)
            DumpRigidBodyState(app, "physics_readback.rigids.json");
        if (captureStages)
            DumpFrameEntryState(app, "physics_readback.json", false);
#endif
        s.state.physicsBodiesMoved = 0;                       
    }

#ifdef RINMOTIONDANCE_DIAG
    if (traceSteps && stepCount > 0) {
        fprintf(stderr, "PHYSFRAME steps=%d settle=%d moved=%d count=%d labels=",
                stepCount, (int)settle, (int)moved, count);
        for (int i = 0; i < stepCount && i < 16; ++i)
            fprintf(stderr, "%s%s", i ? "," : "",
                    stepLabels[i] ? stepLabels[i] : "?");
        fprintf(stderr, "\n");
    }
#endif

    for (int order = 0; order < kModelSlotCount; ++order) {
        for (int j = 0; j < kModelSlotCount; ++j) {
            unsigned char* mdl = models[j];
            if (mdl != nullptr && mdl::Mdl(mdl)->comboSelIndex2 == order) {
                ModelApplyMorphs(mdl);                            
                SetPhysicsMode(mdl, 1, models, count);
            }
        }
    }
#ifdef RINMOTIONDANCE_DIAG
    if (zeroLocalTimeAfterLoad) {
        static_cast<PhysicsWorld*>(world)->ClearSubstepRemainder();
        zeroLocalTimeAfterLoad = false;
    }
    if (captureStages)
        DumpFrameEntryState(app, "physics_pose1.json", false);
#endif
}

}  
