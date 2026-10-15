#pragma once

#include <cstddef>
#include <cstdint>

#include "rinmotiondance/raw_pad.hpp"

namespace rinmotiondance::mdl {

using rinmotiondance::RawPad;

struct PmxIkLinkRecord {
    std::int32_t boneIndex;
    std::uint8_t hasLimits;
    unsigned char padding[3];
    float minimum[3];
    float maximum[3];
};
static_assert(sizeof(PmxIkLinkRecord) == 32, "PMX IK link ABI");

enum class BoneType : unsigned char {
    RotateMove = 0,   
    Move = 1,         
    Ik = 2,           
    Unused3 = 3,      
    UnderIk = 4,      
    RotateGrant = 5,  
    Effector = 6,     
    InertTip = 7,     
    FixedAxis = 8,    
    CoRotate = 9,     
};

constexpr std::uint16_t kBoneFlagTailIsBone = 0x0001;     
constexpr std::uint16_t kBoneFlagMovable = 0x0004;        
constexpr std::uint16_t kBoneFlagVisible = 0x0008;        
constexpr std::uint16_t kBoneFlagIk = 0x0020;             
constexpr std::uint16_t kBoneFlagRotInherit = 0x0100;     
constexpr std::uint16_t kBoneFlagTransInherit = 0x0200;   
constexpr std::uint16_t kBoneFlagInheritMask = 0x0300;    
constexpr std::uint16_t kBoneFlagFixedAxis = 0x0400;      
constexpr std::uint16_t kBoneFlagLocalAxes = 0x0800;      
constexpr std::uint16_t kBoneFlagAfterPhysics = 0x1000;   
constexpr std::uint16_t kBoneFlagExternalParent = 0x2000; 

struct BoneRecord {
    char name[20];  
    char nameEn[20];  
    wchar_t* jpText;  
    wchar_t* enText;  
    std::int32_t parent;  
    float matInit[16];  
    float matLocal[16];  
    float matWorld[16];  
    float matExtra[16];  
    float position[3];  
    float trans[3];  
    float rotQuat[4];  
    float rotQuat2[4];  
    float physicsOffset[3];  
    float physicsQuat[4];  
    float ikBackup[7];  
    std::int32_t rigidIdx;  
    float ikWorkingPos[3];  
    float ikWorkingQuat[4];  
    std::int32_t selState;  
    std::int32_t selState2;  
    std::int32_t tailBone;  
    float tailOffset[3];  
    std::int32_t tailScreenX;  
    std::int32_t tailScreenY;  
    BoneType type;  
    RawPad<3> gap0;  
    std::int32_t tailIdx;  
    unsigned char hasRigidBody;  
    unsigned char physicsDisabled;  
    RawPad<2> gap1;  
    std::int32_t layer;  
    std::uint16_t flags;  
    RawPad<2> gap2;  
    float inheritRatio;  
    float axis[3];  
    float localAxes[6];  
    std::int32_t extParent;  
    std::int32_t ikTarget;  
    std::int32_t ikLoop;  
    float ikAngle;  
    std::int32_t ikLinkCount;  
#if defined(_M_X64)
    RawPad<4> gap3;  
#endif
    PmxIkLinkRecord* ikLinks;  
    unsigned char twistEnable;  
    RawPad<3> gap4;  
    float ikLimitMin[3];  
    float ikLimitMax[3];  
    unsigned char hasFlag;  
    RawPad<3> gap5;  
    std::int32_t slotIndex;  
#if defined(_M_X64)
    RawPad<4> gapTail;  
#else
#endif
};

#if !defined(_M_X64)
static_assert(offsetof(BoneRecord, name) == 0,
              "name x86");
static_assert(offsetof(BoneRecord, nameEn) == 20,
              "nameEn x86");
static_assert(offsetof(BoneRecord, jpText) == 40,
              "jpText x86");
static_assert(offsetof(BoneRecord, enText) == 44,
              "enText x86");
static_assert(offsetof(BoneRecord, parent) == 48,
              "parent x86");
static_assert(offsetof(BoneRecord, matInit) == 52,
              "matInit x86");
static_assert(offsetof(BoneRecord, matLocal) == 116,
              "matLocal x86");
static_assert(offsetof(BoneRecord, matWorld) == 180,
              "matWorld x86");
static_assert(offsetof(BoneRecord, matExtra) == 244,
              "matExtra x86");
static_assert(offsetof(BoneRecord, position) == 308,
              "position x86");
static_assert(offsetof(BoneRecord, trans) == 320,
              "trans x86");
static_assert(offsetof(BoneRecord, rotQuat) == 332,
              "rotQuat x86");
static_assert(offsetof(BoneRecord, rotQuat2) == 348,
              "rotQuat2 x86");
static_assert(offsetof(BoneRecord, physicsOffset) == 364,
              "physicsOffset x86");
static_assert(offsetof(BoneRecord, physicsQuat) == 376,
              "physicsQuat x86");
static_assert(offsetof(BoneRecord, ikBackup) == 392,
              "ikBackup x86");
static_assert(offsetof(BoneRecord, rigidIdx) == 420,
              "rigidIdx x86");
static_assert(offsetof(BoneRecord, ikWorkingPos) == 424,
              "ikWorkingPos x86");
static_assert(offsetof(BoneRecord, ikWorkingQuat) == 436,
              "ikWorkingQuat x86");
static_assert(offsetof(BoneRecord, selState) == 452,
              "selState x86");
static_assert(offsetof(BoneRecord, selState2) == 456,
              "selState2 x86");
static_assert(offsetof(BoneRecord, tailBone) == 460,
              "tailBone x86");
static_assert(offsetof(BoneRecord, tailOffset) == 464,
              "tailOffset x86");
static_assert(offsetof(BoneRecord, tailScreenX) == 476,
              "tailScreenX x86");
static_assert(offsetof(BoneRecord, tailScreenY) == 480,
              "tailScreenY x86");
static_assert(offsetof(BoneRecord, type) == 484,
              "type x86");
static_assert(offsetof(BoneRecord, tailIdx) == 488,
              "tailIdx x86");
static_assert(offsetof(BoneRecord, hasRigidBody) == 492,
              "hasRigidBody x86");
static_assert(offsetof(BoneRecord, physicsDisabled) == 493,
              "physicsDisabled x86");
static_assert(offsetof(BoneRecord, layer) == 496,
              "layer x86");
static_assert(offsetof(BoneRecord, flags) == 500,
              "flags x86");
static_assert(offsetof(BoneRecord, inheritRatio) == 504,
              "inheritRatio x86");
static_assert(offsetof(BoneRecord, axis) == 508,
              "axis x86");
static_assert(offsetof(BoneRecord, localAxes) == 520,
              "localAxes x86");
static_assert(offsetof(BoneRecord, extParent) == 544,
              "extParent x86");
static_assert(offsetof(BoneRecord, ikTarget) == 548,
              "ikTarget x86");
static_assert(offsetof(BoneRecord, ikLoop) == 552,
              "ikLoop x86");
static_assert(offsetof(BoneRecord, ikAngle) == 556,
              "ikAngle x86");
static_assert(offsetof(BoneRecord, ikLinkCount) == 560,
              "ikLinkCount x86");
static_assert(offsetof(BoneRecord, ikLinks) == 564,
              "ikLinks x86");
static_assert(offsetof(BoneRecord, twistEnable) == 568,
              "twistEnable x86");
static_assert(offsetof(BoneRecord, ikLimitMin) == 572,
              "ikLimitMin x86");
static_assert(offsetof(BoneRecord, ikLimitMax) == 584,
              "ikLimitMax x86");
static_assert(offsetof(BoneRecord, hasFlag) == 596,
              "hasFlag x86");
static_assert(offsetof(BoneRecord, slotIndex) == 600,
              "slotIndex x86");
static_assert(sizeof(BoneRecord) == 604,
              "bone record x86 size");
#else
static_assert(offsetof(BoneRecord, nameEn) == 20,
              "nameEn x64");
static_assert(offsetof(BoneRecord, jpText) == 40,
              "jpText x64");
static_assert(offsetof(BoneRecord, matInit) == 60,
              "matInit x64 (PMX skinning worker)");
static_assert(offsetof(BoneRecord, matWorld) == 188,
              "matWorld x64");
static_assert(offsetof(BoneRecord, position) == 316,
              "position x64");
static_assert(offsetof(BoneRecord, trans) == 328,
              "trans x64");
static_assert(offsetof(BoneRecord, rotQuat) == 340,
              "rotQuat x64");
static_assert(offsetof(BoneRecord, rigidIdx) == 428,
              "rigidIdx x64");
static_assert(offsetof(BoneRecord, type) == 492,
              "type x64");
static_assert(offsetof(BoneRecord, flags) == 508,
              "flags x64");
static_assert(offsetof(BoneRecord, ikLinks) == 576,
              "ikLinks x64");
static_assert(offsetof(BoneRecord, hasFlag) == 612,
              "hasFlag x64");
static_assert(offsetof(BoneRecord, slotIndex) == 616,
              "slotIndex x64");
static_assert(sizeof(BoneRecord) == 624,
              "bone record x64 size");
#endif

}  
