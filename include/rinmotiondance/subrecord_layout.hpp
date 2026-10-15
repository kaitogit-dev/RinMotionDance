#pragma once

#include <cstddef>
#include <cstdint>

#ifndef RINMOTIONDANCE_X64
#if defined(_M_X64) || defined(__x86_64__)
#define RINMOTIONDANCE_X64 1
#else
#define RINMOTIONDANCE_X64 0
#endif
#endif

namespace rinmotiondance::mdl {

struct PmdVertex {
    float position[3];
    float normal[3];
    float uv[2];
    std::int16_t bone[2];
    std::int8_t weightPercent;
    std::uint8_t edgeDisabled;
};
static_assert(sizeof(PmdVertex) == 40, "PMD vertex ABI");

struct PmdVertexMorphEntry {
    std::uint32_t vertexIndex;
    float offset[3];
};
static_assert(sizeof(PmdVertexMorphEntry) == 16, "PMD vertex morph ABI");

struct PmxGroupMorphEntry {
    std::int32_t morphIndex;
    float weight;
};
static_assert(sizeof(PmxGroupMorphEntry) == 8, "PMX group morph ABI");

struct PmxBoneMorphEntry {
    std::int32_t boneIndex;
    float translation[3];
    float rotation[4];
};
static_assert(sizeof(PmxBoneMorphEntry) == 32, "PMX bone morph ABI");

struct BoneMorphOffsetRecord {
    std::int32_t boneIndex;
    float translation[3];
    float rotation[4];
};
static_assert(sizeof(BoneMorphOffsetRecord) == 32,
              "bone morph offset ABI");

struct PmxUvMorphEntry {
    std::int32_t vertexIndex;
    float offset[4];
};
static_assert(sizeof(PmxUvMorphEntry) == 20, "PMX UV morph ABI");

struct MaterialMorphChannels {
    float diffuse[4];
    float specular[3];
    float reserved0;
    float specularPower;
    float ambient[3];
    float reserved1;
    float edgeColor[4];
    float edgeSize;
    float textureTint[4];
    float sphereTint[4];
    float toonTint[4];
};
static_assert(sizeof(MaterialMorphChannels) == 120,
              "material morph channel ABI");
static_assert(offsetof(MaterialMorphChannels, diffuse) == 0,
              "material morph diffuse ABI");
static_assert(offsetof(MaterialMorphChannels, specular) == 16,
              "material morph specular ABI");
static_assert(offsetof(MaterialMorphChannels, reserved0) == 28,
              "material morph first internal gap ABI");
static_assert(offsetof(MaterialMorphChannels, specularPower) == 32,
              "material morph specular power ABI");
static_assert(offsetof(MaterialMorphChannels, ambient) == 36,
              "material morph ambient ABI");
static_assert(offsetof(MaterialMorphChannels, reserved1) == 48,
              "material morph second internal gap ABI");
static_assert(offsetof(MaterialMorphChannels, edgeColor) == 52,
              "material morph edge color ABI");
static_assert(offsetof(MaterialMorphChannels, edgeSize) == 68,
              "material morph edge size ABI");
static_assert(offsetof(MaterialMorphChannels, textureTint) == 72,
              "material morph texture tint ABI");
static_assert(offsetof(MaterialMorphChannels, sphereTint) == 88,
              "material morph sphere tint ABI");
static_assert(offsetof(MaterialMorphChannels, toonTint) == 104,
              "material morph toon tint ABI");

struct PmxMaterialMorphEntry {
    std::int32_t materialIndex;
    std::uint8_t operation;
    unsigned char operationPadding[3];
    MaterialMorphChannels channels;
};
static_assert(sizeof(PmxMaterialMorphEntry) == 128,
              "PMX material morph ABI");
static_assert(offsetof(PmxMaterialMorphEntry, channels) == 8,
              "PmxMaterialMorphEntry.channels ABI");

struct MaterialMorphPool {
    unsigned char prefix[8];
    MaterialMorphChannels channels;
};
static_assert(sizeof(MaterialMorphPool) == 128, "material morph pool ABI");
static_assert(offsetof(MaterialMorphPool, channels) == 8,
              "material morph pool channel base ABI");

enum class PmxWeightType : std::uint8_t {
    bdef1 = 0,
    bdef2 = 1,
    bdef4 = 2,
    sdef = 3,
    qdef = 4,
};

struct PmxSdefData {
    float center[3];
    float r0Offset[3];
    float r1Offset[3];
};
static_assert(sizeof(PmxSdefData) == 36, "PMX SDEF working data ABI");

struct PmxVertex {
    float position[3];
    float normal[3];
    float uv[2];
    float additionalUvByComponent[4][4];
    PmxWeightType weightType;
    unsigned char weightTypePadding[3];
    std::int32_t bone[4];
    float weight[4];
    PmxSdefData sdef;
    float edgeScale;
    std::uint8_t hasPmxEdgeData;
    unsigned char workPadding[3];
    float uvMorphBaseZW[2];
    std::int32_t materialIndex;
};
static_assert(sizeof(PmxVertex) == 188, "PMX working vertex ABI");
static_assert(offsetof(PmxVertex, weightType) == 96, "PMX weight type");
static_assert(offsetof(PmxVertex, bone) == 100, "PMX bones");
static_assert(offsetof(PmxVertex, weight) == 116, "PMX weights");
static_assert(offsetof(PmxVertex, sdef) == 132, "PMX SDEF working data");
static_assert(offsetof(PmxVertex, sdef.center) == 132, "PMX SDEF center");
static_assert(offsetof(PmxVertex, sdef.r0Offset) == 144, "PMX SDEF R0 offset");
static_assert(offsetof(PmxVertex, sdef.r1Offset) == 156, "PMX SDEF R1 offset");
static_assert(offsetof(PmxVertex, edgeScale) == 168, "PMX edge scale");
static_assert(offsetof(PmxVertex, hasPmxEdgeData) == 172,
              "PMX edge data marker");
static_assert(offsetof(PmxVertex, uvMorphBaseZW) == 176, "PMX UV base ZW");
static_assert(offsetof(PmxVertex, materialIndex) == 184, "PMX material");

struct SkinnedVertexBase {
    float position[3];
    float normal[3];
    float uv[2];
};
static_assert(sizeof(SkinnedVertexBase) == 32, "base GPU vertex ABI");

template <std::size_t AdditionalUvCount>
struct SkinnedVertex {
    SkinnedVertexBase base;
    float additionalUv[AdditionalUvCount][4];
};
static_assert(sizeof(SkinnedVertex<1>) == 48, "one additional UV GPU ABI");
static_assert(sizeof(SkinnedVertex<2>) == 64, "two additional UV GPU ABI");
static_assert(sizeof(SkinnedVertex<3>) == 80, "three additional UV GPU ABI");
static_assert(sizeof(SkinnedVertex<4>) == 96, "four additional UV GPU ABI");

struct EdgeVertex {
    float position[3];
    std::uint32_t diffuse;
};
static_assert(sizeof(EdgeVertex) == 16, "edge GPU vertex ABI");

struct ModelMaterialRecord {
    float diffuse[4];
    float diffuseMirror[4];
    float specular[3];
    unsigned char gap0[4];
    float ambient[3];
    unsigned char gap1[4];
    float specularPower;
    wchar_t texturePath[280];
    wchar_t spherePath[280];
    std::int32_t faceVertexCount;
    std::uint8_t toonReference;
    std::uint8_t doubleSided;
    unsigned char gap2[2];
    float edgeColor[4];
    float edgeSize;
    wchar_t toonPath[512];
    std::uint8_t flags;
    std::uint8_t sphereMode;
    unsigned char tail[50];
};
static_assert(sizeof(ModelMaterialRecord) == 2292, "material record ABI");
static_assert(offsetof(ModelMaterialRecord, texturePath) == 68,
              "ModelMaterialRecord.texturePath ABI");
static_assert(offsetof(ModelMaterialRecord, spherePath) == 628,
              "ModelMaterialRecord.spherePath ABI");
static_assert(offsetof(ModelMaterialRecord, faceVertexCount) == 1188,
              "ModelMaterialRecord.faceVertexCount ABI");
static_assert(offsetof(ModelMaterialRecord, edgeColor) == 1196,
              "ModelMaterialRecord.edgeColor ABI");
static_assert(offsetof(ModelMaterialRecord, toonPath) == 1216,
              "ModelMaterialRecord.toonPath ABI");
static_assert(offsetof(ModelMaterialRecord, flags) == 2240,
              "ModelMaterialRecord.flags ABI");

enum class MorphPanel : std::uint8_t {
    system = 0,
    eyebrow = 1,
    eye = 2,
    mouth = 3,
    other = 4,
};

struct MorphRecord {
    char name[20];              
    char nameEn[20];            
    wchar_t* jpText;            
    wchar_t* enText;            
    float value;                
    std::int32_t offsetCount;   
    std::int32_t uvCounts[5];   
    std::int32_t boneCount;     
    std::int32_t groupCount;    
    std::int32_t materialCount; 
    MorphPanel panel;           
    std::uint8_t type;          
    PmdVertexMorphEntry* vertexEntries;  
    PmxBoneMorphEntry* boneEntries;     
    PmxGroupMorphEntry* groupEntries;   
    PmxUvMorphEntry* uvEntries[5];      
    PmxMaterialMorphEntry* materialEntries; 
    unsigned char* impulseEntries;  
    unsigned char* impulse2Entries; 
};

struct RigidRecord {
    char name[20];              
    wchar_t* jpText;            
    wchar_t* enText;            
    std::int32_t boneIndex;     
    std::uint8_t group;         
    std::uint8_t padGroup;
    std::uint16_t noCollapse;   
    std::uint8_t shape;         
    unsigned char padShape[3];
    float size[3];              
    float position[3];          
    float rotation[3];          
    float mass;                 
    std::uint8_t mode;          
    std::uint8_t kinematicFlag; 
    std::uint8_t staticFlag;    
    unsigned char padMode;
    void* keyData;              
    float linearDamping;        
    float angularDamping;       
    float restitution;          
    float friction;             
    void* body;                 
    float invTransform[16];     
};

struct JointRecord {
    char name[20];              
    wchar_t* jpText;            
    wchar_t* enText;            
    std::int32_t rigidA;        
    std::int32_t rigidB;        
    float position[3];          
    float rotation[3];          
    float limits[12];           
    float springs[6];           
    std::int32_t constraint;    
    float radiusBound;          
};

}  

#if !RINMOTIONDANCE_X64
static_assert(sizeof(rinmotiondance::mdl::MorphRecord) == 136, "morph x86 size");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, jpText) == 40,
          "MorphRecord.jpText x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, value) == 48,
          "MorphRecord.value x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, offsetCount) == 52,
          "MorphRecord.offsetCount x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, uvCounts) == 56,
          "MorphRecord.uvCounts x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, boneCount) == 76,
          "MorphRecord.boneCount x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, groupCount) == 80,
          "MorphRecord.groupCount x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, materialCount) == 84,
          "MorphRecord.materialCount x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, panel) == 88,
          "MorphRecord.panel x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, type) == 89,
          "MorphRecord.type x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, vertexEntries) == 92,
          "MorphRecord.vertexEntries x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, boneEntries) == 96,
          "MorphRecord.boneEntries x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, groupEntries) == 100,
          "MorphRecord.groupEntries x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, uvEntries) == 104,
          "MorphRecord.uvEntries x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, materialEntries) == 124,
          "MorphRecord.materialEntries x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, impulseEntries) == 128,
          "MorphRecord.impulseEntries x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, impulse2Entries) == 132,
          "MorphRecord.impulse2Entries x86 ABI");

static_assert(sizeof(rinmotiondance::mdl::RigidRecord) == 172, "rigid x86 size");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, jpText) == 20,
          "RigidRecord.jpText x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, enText) == 24,
          "RigidRecord.enText x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, boneIndex) == 28,
          "RigidRecord.boneIndex x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, group) == 32,
          "RigidRecord.group x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, noCollapse) == 34,
          "RigidRecord.noCollapse x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, shape) == 36,
          "RigidRecord.shape x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, size) == 40,
          "RigidRecord.size x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, position) == 52,
          "RigidRecord.position x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, rotation) == 64,
          "RigidRecord.rotation x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, mass) == 76,
          "RigidRecord.mass x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, mode) == 80,
          "RigidRecord.mode x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, keyData) == 84,
          "RigidRecord.keyData x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, linearDamping) == 88,
          "RigidRecord.linearDamping x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, body) == 104,
          "RigidRecord.body x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, invTransform) == 108,
          "RigidRecord.invTransform x86 ABI");

static_assert(sizeof(rinmotiondance::mdl::JointRecord) == 140, "joint x86 size");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, jpText) == 20,
          "JointRecord.jpText x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, enText) == 24,
          "JointRecord.enText x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, rigidA) == 28,
          "JointRecord.rigidA x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, rigidB) == 32,
          "JointRecord.rigidB x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, position) == 36,
          "JointRecord.position x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, rotation) == 48,
          "JointRecord.rotation x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, limits) == 60,
          "JointRecord.limits x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, springs) == 108,
          "JointRecord.springs x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, constraint) == 132,
          "JointRecord.constraint x86 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, radiusBound) == 136,
          "JointRecord.radiusBound x86 ABI");
#else
static_assert(sizeof(rinmotiondance::mdl::MorphRecord) == 192, "morph x64 size");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, panel) == 0x60,
          "MorphRecord.panel x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, type) == 0x61,
          "MorphRecord.type x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::MorphRecord, vertexEntries) == 0x68,
          "MorphRecord.vertexEntries x64 ABI");

static_assert(sizeof(rinmotiondance::mdl::RigidRecord) == 192, "rigid x64 size 0xC0");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, jpText) == 24,
          "RigidRecord.jpText x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, enText) == 32,
          "RigidRecord.enText x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, size) == 52,
          "RigidRecord.size x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, rotation) == 0x4C,
          "RigidRecord.rotation x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, keyData) == 96,
          "RigidRecord.keyData x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, body) == 120,
          "RigidRecord.body x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::RigidRecord, invTransform) == 128,
          "RigidRecord.invTransform x64 ABI");

static_assert(sizeof(rinmotiondance::mdl::JointRecord) == 152, "joint x64 size 0x98");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, jpText) == 24,
          "JointRecord.jpText x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, enText) == 32,
          "JointRecord.enText x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, rigidA) == 0x28,
          "JointRecord.rigidA x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, rigidB) == 0x2C,
          "JointRecord.rigidB x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, limits) == 0x48,
          "JointRecord.limits x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, springs) == 0x78,
          "JointRecord.springs x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, constraint) == 0x90,
          "JointRecord.constraint x64 ABI");
static_assert(offsetof(rinmotiondance::mdl::JointRecord, radiusBound) == 0x94,
          "JointRecord.radiusBound x64 ABI");
#endif
