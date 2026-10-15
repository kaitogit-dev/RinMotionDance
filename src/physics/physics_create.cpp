
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <cstring>

#include "btBulletDynamicsCommon.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.h"

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/physics_scene.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

void QuatFromMatrix(float out[4], const void* m16) {
    d3dx::Get().quatFromMatrix(
        out, reinterpret_cast<const d3dx::D3DXMATRIXF*>(m16));
}

void QuatMultiply(float out[4], const float a[4], const float b[4]) {
    d3dx::Get().quatMultiply(out, a, b);
}

#if defined(_M_IX86)
static const double kSetRotationTwo = 2.0;

__declspec(naked) void __fastcall SetRotationOriginal(
    float*  , const float*  ) {
    __asm {
        sub esp, 24h
        mov eax, edx
        fld dword ptr [eax+4]
        fld dword ptr [eax]
        fld dword ptr [eax+8]
        fld dword ptr [eax+0Ch]
        fld st(2)
        fmulp st(3), st
        fld st(3)
        fmulp st(4), st
        fxch st(2)
        faddp st(3), st
        fmul st, st
        faddp st(2), st
        fmul st, st
        faddp st(1), st
        fstp dword ptr [esp+20h]
        fld dword ptr [esp+20h]
        fdivr qword ptr [kSetRotationTwo]
        fstp dword ptr [esp+20h]
        fld dword ptr [eax]
        fld dword ptr [esp+20h]
        fld st
        fmulp st(2), st
        fxch st(1)
        fstp dword ptr [esp+20h]
        fld st
        fmul dword ptr [eax+4]
        fstp dword ptr [esp+4]
        fmul dword ptr [eax+8]
        fstp dword ptr [esp+8]
        fld dword ptr [esp+20h]
        fld st
        fmul dword ptr [eax+0Ch]
        fstp dword ptr [esp]
        fld dword ptr [esp+4]
        fld st
        fmul dword ptr [eax+0Ch]
        fstp dword ptr [esp+20h]
        fld dword ptr [esp+8]
        fld st
        fmul dword ptr [eax+0Ch]
        fstp dword ptr [esp+10h]
        fld dword ptr [eax]
        fmulp st(3), st
        fxch st(2)
        fstp dword ptr [esp+18h]
        fld dword ptr [eax]
        fmul st, st(1)
        fstp dword ptr [esp+0Ch]
        fld dword ptr [eax]
        fmul st, st(2)
        fstp dword ptr [esp+14h]
        fmul dword ptr [eax+4]
        fstp dword ptr [esp+4]
        fld st
        fmul dword ptr [eax+4]
        fstp dword ptr [esp+1Ch]
        fmul dword ptr [eax+8]
        fstp dword ptr [esp+8]
        fld dword ptr [esp+8]
        fld st
        fld dword ptr [esp+4]
        fld st
        faddp st(2), st
        fld1
        fld st
        fsubrp st(3), st
        fxch st(2)
        fstp dword ptr [ecx]
        fld dword ptr [esp+0Ch]
        fld st
        fld dword ptr [esp+10h]
        fld st
        fsubp st(2), st
        fxch st(1)
        fstp dword ptr [ecx+4]
        fld dword ptr [esp+14h]
        fld st
        fadd dword ptr [esp+20h]
        fstp dword ptr [ecx+8]
        fldz
        fst dword ptr [ecx+0Ch]
        fxch st(3)
        faddp st(2), st
        fxch st(1)
        fstp dword ptr [ecx+10h]
        fld dword ptr [esp+18h]
        fld st
        faddp st(6), st
        fld st(4)
        fsubrp st(6), st
        fxch st(5)
        fstp dword ptr [ecx+14h]
        fld dword ptr [esp+1Ch]
        fld st
        fsub dword ptr [esp]
        fstp dword ptr [ecx+18h]
        fxch st(2)
        fst dword ptr [ecx+1Ch]
        fld dword ptr [esp+20h]
        fsubp st(2), st
        fxch st(1)
        fstp dword ptr [ecx+20h]
        fld dword ptr [esp]
        faddp st(2), st
        fxch st(1)
        fstp dword ptr [ecx+24h]
        fxch st(1)
        faddp st(3), st
        fxch st(1)
        fsubrp st(2), st
        fxch st(1)
        fstp dword ptr [ecx+28h]
        fstp dword ptr [ecx+2Ch]
        add esp, 24h
        ret
    }
}
#else
void SetRotationOriginal(float* basis, const float* quaternion) {
    auto* matrix = reinterpret_cast<btMatrix3x3*>(basis);
    matrix->setRotation(btQuaternion(quaternion[0], quaternion[1],
                                     quaternion[2], quaternion[3]));
}
#endif

#ifdef RINMOTIONDANCE_DIAG
void DumpRigidCreateState(int shape, int mode, const float* inputMatrix,
                          const float* quaternion, const btRigidBody* body) {
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_STATE_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH || body == nullptr)
        return;
    CreateDirectoryA(directory, nullptr);
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\rigid_create.jsonl", directory);
    std::FILE* stream = nullptr;
    if (fopen_s(&stream, path, "ab") != 0 || stream == nullptr)
        return;
    const auto writeWords = [stream](const void* source, int count) {
        const auto* bytes = static_cast<const unsigned char*>(source);
        for (int i = 0; i < count; ++i) {
            if (i != 0)
                std::fputc(',', stream);
            std::uint32_t value = 0;
            std::memcpy(&value, bytes + 4 * i, 4);
            std::fprintf(stream, "\"%08X\"", value);
        }
    };
    std::fprintf(stream, "{\"shape\":%d,\"mode\":%d,"
                         "\"input_matrix_bits\":[", shape, mode);
    writeWords(inputMatrix, 16);
    std::fputs("],\"quaternion_bits\":[", stream);
    writeWords(quaternion, 4);
    std::fputs("],\"world_bits\":[", stream);
    writeWords(&body->getWorldTransform(), 16);
    std::fputs("],\"solver_bits\":[", stream);
    writeWords(reinterpret_cast<const unsigned char*>(body) + 0x110,
               (0x208 - 0x110) / 4);
    std::fputs("]}\n", stream);
    std::fclose(stream);
}

void DumpConstraintCreateState(
        int index, btGeneric6DofSpringConstraint* constraint) {
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_STATE_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH || constraint == nullptr)
        return;
    CreateDirectoryA(directory, nullptr);
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\constraint_create.jsonl", directory);
    std::FILE* stream = nullptr;
    if (fopen_s(&stream, path, "ab") != 0 || stream == nullptr)
        return;
    const auto writeWords = [stream](const void* source, int count) {
        const auto* bytes = static_cast<const unsigned char*>(source);
        for (int i = 0; i < count; ++i) {
            if (i != 0)
                std::fputc(',', stream);
            std::uint32_t value = 0;
            std::memcpy(&value, bytes + 4 * i, 4);
            std::fprintf(stream, "\"%08X\"", value);
        }
    };
    const auto* base = reinterpret_cast<const unsigned char*>(constraint);
    const auto* linear = constraint->getTranslationalLimitMotor();
    std::fprintf(stream,
                 "{\"index\":%d,\"object_size\":%u,"
                 "\"user_constraint_id\":%d,"
                 "\"frame_a_offset\":%u,\"frame_b_offset\":%u,"
                 "\"linear_motor_offset\":%u,"
                 "\"angular_motor_offset\":%u,\"frame_a_bits\":[",
                 index, static_cast<unsigned>(sizeof(*constraint)),
                 constraint->getUserConstraintId(),
                 static_cast<unsigned>(
                     reinterpret_cast<const unsigned char*>(
                         &constraint->getFrameOffsetA()) - base),
                 static_cast<unsigned>(
                     reinterpret_cast<const unsigned char*>(
                         &constraint->getFrameOffsetB()) - base),
                 static_cast<unsigned>(
                     reinterpret_cast<const unsigned char*>(linear) - base),
                 static_cast<unsigned>(
                     reinterpret_cast<const unsigned char*>(
                         constraint->getRotationalLimitMotor(0)) - base));
    writeWords(&constraint->getFrameOffsetA(), 16);
    std::fputs("],\"frame_b_bits\":[", stream);
    writeWords(&constraint->getFrameOffsetB(), 16);
    std::fputs("],\"limit_bits\":[", stream);
    writeWords(&linear->m_lowerLimit, 3);
    std::fputc(',', stream);
    writeWords(&linear->m_upperLimit, 3);
    for (int axis = 0; axis < 3; ++axis) {
        const auto* motor = constraint->getRotationalLimitMotor(axis);
        std::fputc(',', stream);
        writeWords(&motor->m_loLimit, 1);
        std::fputc(',', stream);
        writeWords(&motor->m_hiLimit, 1);
    }
    std::fputs("],\"spring_tail_bits\":[", stream);
    writeWords(base + sizeof(*constraint) - 80, 20);
    std::fputs("]}\n", stream);
    std::fclose(stream);
}
#else
inline void DumpRigidCreateState(int, int, const float*, const float*,
                                 const btRigidBody*) {}
inline void DumpConstraintCreateState(int, btGeneric6DofSpringConstraint*) {}
#endif

}  

#if defined(_M_IX86)
__declspec(naked) void __fastcall TransformJointPointOriginalFast(
    float*  , const float*  , const float*  ) {
    __asm {
        mov eax, [esp+4]

        fld dword ptr [edx]
        fmul dword ptr [eax]
        fld dword ptr [edx+4]
        fmul dword ptr [eax+10h]
        faddp st(1), st
        fld dword ptr [edx+8]
        fmul dword ptr [eax+20h]
        faddp st(1), st
        fadd dword ptr [eax+30h]
        fstp dword ptr [ecx]

        fld dword ptr [edx]
        fmul dword ptr [eax+4]
        fld dword ptr [edx+4]
        fmul dword ptr [eax+14h]
        faddp st(1), st
        fld dword ptr [edx+8]
        fmul dword ptr [eax+24h]
        faddp st(1), st
        fadd dword ptr [eax+34h]
        fstp dword ptr [ecx+4]

        fld dword ptr [edx]
        fmul dword ptr [eax+8]
        fld dword ptr [edx+4]
        fmul dword ptr [eax+18h]
        faddp st(1), st
        fld dword ptr [edx+8]
        fmul dword ptr [eax+28h]
        faddp st(1), st
        fadd dword ptr [eax+38h]
        fstp dword ptr [ecx+8]
        ret 4
    }
}
#endif

void TransformJointPointOriginal(float out[3], const float point[3],
                                 const float matrix[16]) {
#if defined(_M_IX86)
    TransformJointPointOriginalFast(out, point, matrix);
#else
    for (int column = 0; column < 3; ++column) {
        out[column] = point[0] * matrix[column]
                    + point[1] * matrix[4 + column]
                    + point[2] * matrix[8 + column]
                    + matrix[12 + column];
    }
#endif
}

float JointRadiusBound(const float limits[6], float distA, float distB) {
    float maxAbs[3];
    for (int axis = 0; axis < 3; ++axis) {
        float upper = std::fabs(limits[axis]);
        const float lower = std::fabs(limits[3 + axis]);
        if (lower > upper)
            upper = lower;
        maxAbs[axis] = upper;
    }
    return distA + distB + std::sqrt(maxAbs[0] * maxAbs[0] +
                                     maxAbs[1] * maxAbs[1] +
                                     maxAbs[2] * maxAbs[2]);
}

void CreateRigidBody(PhysicsScene* scene, void** out, int shape, float sx, float sy,
                     float sz, const float* mat16, int mode, float mass,
                     float dampLin, float dampAng, float restitution,
                     float friction, char grp, std::uint16_t mask) {
    btDiscreteDynamicsWorld* world = scene->world;
    btCollisionShape* colShape = nullptr;
    if (shape == 0) {
        colShape = new btSphereShape(sx);                      
    } else if (shape == 1) {
        colShape = new btBoxShape(btVector3(sx, sy, sz));      
    } else if (shape == 2) {
        colShape = new btCapsuleShape(sx, sy);                 
    } else {
        out[0] = reinterpret_cast<void*>(                       
            static_cast<std::intptr_t>(-1));
        out[1] = nullptr;
        return;
    }

    btTransform startTrans;
    startTrans.setIdentity();
    startTrans.setOrigin(btVector3(mat16[12], mat16[13], mat16[14]));
    float q[4];
    QuatFromMatrix(q, mat16);
    SetRotationOriginal(reinterpret_cast<float*>(&startTrans.getBasis()), q);

    const float useMass = mode ? mass : 0.0f;
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    colShape->calculateLocalInertia(useMass, inertia);

    btDefaultMotionState* motionState =
        new btDefaultMotionState(startTrans);                  
    btRigidBody::btRigidBodyConstructionInfo ci(               
        useMass, motionState, colShape, inertia);
    btRigidBody* body = new btRigidBody(ci);                   

    world->addRigidBody(body, 1 << grp, mask);                 
    body->setDamping(dampLin, dampAng);                        
    body->setRestitution(restitution);                         
    body->setFriction(friction);                               
    if (mode == 0)
        body->setCollisionFlags(                               
            body->getCollisionFlags() |
            btCollisionObject::CF_KINEMATIC_OBJECT);
    body->setActivationState(DISABLE_DEACTIVATION);           

    DumpRigidCreateState(shape, mode, mat16, q, body);

    out[0] = reinterpret_cast<void*>(                           
        static_cast<std::intptr_t>(body->m_debugBodyId));
    out[1] = body;
}

#ifdef RINMOTIONDANCE_DIAG
static unsigned JointBits(float value) {
    unsigned bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

static std::string WordsHex(const float* values, int count) {
    std::string out;
    char cell[16];
    for (int i = 0; i < count; ++i) {
        std::snprintf(cell, sizeof cell, "%08X ", JointBits(values[i]));
        out += cell;
    }
    return out;
}
#endif

int CreatePhysJoint(PhysicsScene* scene, void* rbA, void* rbB,
                    float ax, float ay, float az, float aq0, float aq1,
                    float aq2, float aq3, float bx, float by, float bz,
                    float bq0, float bq1, float bq2, float bq3,
                    float linUpper0, float linUpper1, float linUpper2,
                    float linLower0, float linLower1, float linLower2,
                    float angUpper0, float angUpper1, float angUpper2,
                    float angLower0, float angLower1, float angLower2,
                    float spring0, float spring1, float spring2,
                    float spring3, float spring4, float spring5) {
    btDiscreteDynamicsWorld* world = scene->world;
#ifdef RINMOTIONDANCE_DIAG
    static FILE* jointArgLog = []() -> FILE* {
        const char* path = std::getenv("RINMOTIONDANCE_DUMP_JOINTARGS");
        return path ? std::fopen(path, "w") : nullptr;
    }();
    if (jointArgLog) {
        std::fprintf(jointArgLog,
            "jnt a=%08X %08X %08X qa=%08X %08X %08X %08X "
            "b=%08X %08X %08X qb=%08X %08X %08X %08X "
            "spr=%08X %08X %08X %08X %08X %08X\n",
            JointBits(ax), JointBits(ay), JointBits(az),
            JointBits(aq0), JointBits(aq1), JointBits(aq2), JointBits(aq3),
            JointBits(bx), JointBits(by), JointBits(bz),
            JointBits(bq0), JointBits(bq1), JointBits(bq2), JointBits(bq3),
            JointBits(spring0), JointBits(spring1), JointBits(spring2),
            JointBits(spring3), JointBits(spring4), JointBits(spring5));
        std::fflush(jointArgLog);
    }
#endif
    btTransform frameA, frameB;
    frameA.setIdentity();
    frameA.setOrigin(btVector3(ax, ay, az));
    const float qa[4] = {aq0, aq1, aq2, aq3};
    SetRotationOriginal(reinterpret_cast<float*>(&frameA.getBasis()), qa);
    frameB.setIdentity();
    frameB.setOrigin(btVector3(bx, by, bz));
    const float qb[4] = {bq0, bq1, bq2, bq3};
    SetRotationOriginal(reinterpret_cast<float*>(&frameB.getBasis()), qb);
#ifdef RINMOTIONDANCE_DIAG
    if (jointArgLog) {
        std::fprintf(jointArgLog, "frm A=%s B=%s\n",
            WordsHex(reinterpret_cast<const float*>(&frameA), 16).c_str(),
            WordsHex(reinterpret_cast<const float*>(&frameB), 16).c_str());
        std::fflush(jointArgLog);
    }
#endif

    auto* con = new btGeneric6DofSpringConstraint(             
        *static_cast<btRigidBody*>(rbA),
        *static_cast<btRigidBody*>(rbB), frameA, frameB, true);

    con->setLinearLowerLimit(                                  
        btVector3(linLower0, linLower1, linLower2));
    con->setLinearUpperLimit(
        btVector3(linUpper0, linUpper1, linUpper2));
    const auto canonicalZero = [](float value) {
        return value == 0.0f ? 0.0f : value;
    };
    con->setAngularLowerLimit(                                 
        btVector3(canonicalZero(angLower0), canonicalZero(angLower1),
                  canonicalZero(angLower2)));
    con->setAngularUpperLimit(                                 
        btVector3(canonicalZero(angUpper0), canonicalZero(angUpper1),
                  canonicalZero(angUpper2)));

    world->addConstraint(con, false);                          

    if (spring0 > 0.0f || spring1 > 0.0f || spring2 > 0.0f ||
        spring3 > 0.0f || spring4 > 0.0f || spring5 > 0.0f) {
        const float springs[6] = {spring0, spring1, spring2,
                                  spring3, spring4, spring5};
        for (int axis = 0; axis < 6; ++axis) {
            if (springs[axis] > 0.0f) {
                con->enableSpring(axis, true);                 
                con->setStiffness(axis, springs[axis]);        
            }
        }
        con->setEquilibriumPoint();                            
    }

    int& counter = scene->constraintId;                         
    con->setUid(counter);
    DumpConstraintCreateState(counter, con);
    return counter++;
}

void SetPhysicsMode(unsigned char* m, int afterPhysics,
                    unsigned char* const* modelSlots, int physicsMode) {
    mdl::ModelRecord& model = *mdl::Mdl(m);
    const int boneCount = model.boneCount;
    rinmotiondance::mdl::BoneRecord* bones = rinmotiondance::mdl::Bones(m);
    for (int i = 0; i < boneCount; ++i) {
        rinmotiondance::mdl::BoneRecord* bone = &bones[i];
        const unsigned char flag = bone->hasRigidBody;
        if (((physicsMode == 1 || (physicsMode >= 2 && bone->physicsDisabled == 0)) && flag) != 0) {
            std::memcpy(bone->physicsOffset, bone->ikBackup, 12);           
            std::memcpy(bone->physicsQuat, bone->ikBackup + 3, 16);           
        } else {
            std::memcpy(bone->physicsOffset, bone->trans, 12);
            std::memcpy(bone->physicsQuat, bone->rotQuat, 16);
        }
    }

    if (model.physicsMode == 2) {
        const int recCount = mdl::BoneMorphOffsetCount(m);
        mdl::BoneMorphOffsetRecord* records = mdl::BoneMorphOffsets(m);
        for (int i = 0; i < recCount; ++i) {
            const mdl::BoneMorphOffsetRecord& record = records[i];
            rinmotiondance::mdl::BoneRecord* bone = &bones[record.boneIndex];
            bone->physicsOffset[0] += record.translation[0];
            bone->physicsOffset[1] += record.translation[1];
            bone->physicsOffset[2] += record.translation[2];
            float out[4];
            QuatMultiply(out,
                         reinterpret_cast<const float*>(bone->physicsQuat),
                         record.rotation);
            std::memcpy(bone->physicsQuat, out, 16);
        }
    }

    const int maxLayer = model.maxBoneLayer;
    for (int i = 0; i <= maxLayer; ++i)
        BoneFrameTransform(m, afterPhysics, i, modelSlots, physicsMode);  
}

}  
