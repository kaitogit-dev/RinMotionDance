#pragma once

#include <cstddef>
#include <cstdint>

#include "rinmotiondance/bone_layout.hpp"
#include "rinmotiondance/model_layout.hpp"
#include "rinmotiondance/subrecord_layout.hpp"

namespace rinmotiondance::mdl {

constexpr std::size_t kSize = sizeof(ModelRecord);
#if defined(_M_X64)
constexpr std::size_t kBoneKeyCapacity = 600000;
#else
constexpr std::size_t kBoneKeyCapacity = 300000;
#endif
constexpr std::size_t kMorphKeyCapacity = 20000;
constexpr std::size_t kDisplayKeyCapacity = 1000;

void SelectPhysicsOnBoneKeys(ModelRecord& model);

struct BoneReference {
    std::int32_t modelIndex;
    std::int32_t boneIndex;
};

struct IkChain {
    std::int32_t boneIndex;
    std::int32_t targetBone;
    std::uint8_t linkCount;
    std::uint8_t reserved0[3];
    std::uint16_t* links;
    std::uint16_t iterations;
    std::uint8_t enabled;
    std::uint8_t reserved1;
    float maxAngle;
};

struct FrameGroup {
    char name[20];
    char nameEn[20];
    std::uint16_t groupIndex;
    std::uint16_t targetIndex;
    std::uint8_t selected;
    std::uint8_t reserved;
};
static_assert(sizeof(FrameGroup) == 46, "frame group ABI");
static_assert(offsetof(FrameGroup, name) == 0, "frame group name ABI");
static_assert(offsetof(FrameGroup, nameEn) == 20,
              "frame group English name ABI");
static_assert(offsetof(FrameGroup, groupIndex) == 40,
              "frame group index ABI");
static_assert(offsetof(FrameGroup, targetIndex) == 42,
              "frame group target ABI");
static_assert(offsetof(FrameGroup, selected) == 44,
              "frame group selection ABI");

struct DisplayGroup {
    char name[50];
    char nameEn[50];
    std::uint8_t flags;
};
static_assert(sizeof(DisplayGroup) == 101, "display group ABI");
static_assert(offsetof(DisplayGroup, name) == 0,
              "display group name ABI");
static_assert(offsetof(DisplayGroup, nameEn) == 50,
              "display group English name ABI");
static_assert(offsetof(DisplayGroup, flags) == 100,
              "display group flags ABI");

struct PmxDisplayFrameEntry {
    std::uint8_t type;  
    unsigned char padding[3];
    std::int32_t index;
};
static_assert(sizeof(PmxDisplayFrameEntry) == 8,
              "PMX display-frame entry ABI");

#if RINMOTIONDANCE_X64
static_assert(sizeof(IkChain) == 32, "IK chain x64 ABI");
static_assert(offsetof(IkChain, links) == 16, "IK chain links x64 ABI");
static_assert(offsetof(IkChain, iterations) == 24,
              "IK chain iterations x64 ABI");
static_assert(offsetof(IkChain, enabled) == 26, "IK chain enabled x64 ABI");
static_assert(offsetof(IkChain, maxAngle) == 28,
              "IK chain angle x64 ABI");
#else
static_assert(sizeof(IkChain) == 24, "IK chain x86 ABI");
static_assert(offsetof(IkChain, links) == 12, "IK chain links x86 ABI");
static_assert(offsetof(IkChain, iterations) == 16,
              "IK chain iterations x86 ABI");
static_assert(offsetof(IkChain, enabled) == 18, "IK chain enabled x86 ABI");
static_assert(offsetof(IkChain, maxAngle) == 20,
              "IK chain angle x86 ABI");
#endif

static_assert(sizeof(BoneReference) == 8, "bone reference ABI");

struct BoneKey {
    std::uint32_t frame;
    std::uint32_t previous;
    std::uint32_t next;
    std::uint8_t interpolation[16];
    float position[3];
    float rotation[4];
    std::uint8_t allocated;
    std::uint8_t physicsDisabled;
    std::uint8_t reserved[2];
};
static_assert(sizeof(BoneKey) == 60, "bone key ABI");
static_assert(offsetof(BoneKey, frame) == 0, "bone key frame ABI");
static_assert(offsetof(BoneKey, previous) == 4, "bone key previous ABI");
static_assert(offsetof(BoneKey, next) == 8, "bone key next ABI");
static_assert(offsetof(BoneKey, interpolation) == 12,
              "bone key interpolation ABI");
static_assert(offsetof(BoneKey, position) == 28, "bone key position ABI");
static_assert(offsetof(BoneKey, rotation) == 40, "bone key rotation ABI");
static_assert(offsetof(BoneKey, allocated) == 56, "bone key allocated ABI");
static_assert(offsetof(BoneKey, physicsDisabled) == 57,
              "bone key physics ABI");

struct MorphKey {
    std::uint32_t frame;
    std::uint32_t previous;
    std::uint32_t next;
    float value;
    std::uint8_t allocated;
    std::uint8_t reserved[3];
};
static_assert(sizeof(MorphKey) == 20, "morph key ABI");
static_assert(offsetof(MorphKey, frame) == 0, "morph key frame ABI");
static_assert(offsetof(MorphKey, previous) == 4, "morph key previous ABI");
static_assert(offsetof(MorphKey, next) == 8, "morph key next ABI");
static_assert(offsetof(MorphKey, value) == 12, "morph key value ABI");
static_assert(offsetof(MorphKey, allocated) == 16, "morph key allocated ABI");

struct DisplayKey {
    std::uint32_t frame;
    std::uint32_t previous;
    std::uint32_t next;
    std::uint8_t visible;
    std::uint8_t reserved0[3];
    std::uint8_t* ikStates;
    std::uint8_t allocated;
    std::uint8_t reserved1[3];
    BoneReference* selectorStates;
};
#if RINMOTIONDANCE_X64
static_assert(sizeof(DisplayKey) == 40, "display key x64 ABI");
static_assert(offsetof(DisplayKey, ikStates) == 16,
              "display key IK state x64 ABI");
static_assert(offsetof(DisplayKey, allocated) == 24,
              "display key allocated x64 ABI");
static_assert(offsetof(DisplayKey, selectorStates) == 32,
              "display key selector state x64 ABI");
#else
static_assert(sizeof(DisplayKey) == 28, "display key x86 ABI");
static_assert(offsetof(DisplayKey, ikStates) == 16,
              "display key IK state x86 ABI");
static_assert(offsetof(DisplayKey, allocated) == 20,
              "display key allocated x86 ABI");
static_assert(offsetof(DisplayKey, selectorStates) == 24,
              "display key selector state x86 ABI");
#endif
static_assert(offsetof(DisplayKey, frame) == 0, "display key frame ABI");
static_assert(offsetof(DisplayKey, previous) == 4,
              "display key previous ABI");
static_assert(offsetof(DisplayKey, next) == 8, "display key next ABI");
static_assert(offsetof(DisplayKey, visible) == 12,
              "display key visible ABI");

inline BoneReference*& SelectorStates(DisplayKey& key) {
    return key.selectorStates;
}

inline std::uint8_t*& IkStates(DisplayKey& key) {
    return key.ikStates;
}

inline const std::uint8_t* IkStates(const DisplayKey& key) {
    return static_cast<const std::uint8_t*>(key.ikStates);
}

inline const BoneReference* SelectorStates(const DisplayKey& key) {
    return static_cast<const BoneReference*>(key.selectorStates);
}

constexpr std::size_t kPoseTraceFlag = offsetof(ModelRecord, poseTraceRecording);
constexpr std::size_t kPoseTraceBuffer = offsetof(ModelRecord, poseTraceBuffer);
constexpr std::size_t kPmdModelNameBytes = 20;

enum class PmxTextBufferSlot : std::size_t {
    japaneseName,
    englishName,
    japaneseComment,
    englishComment,
};

template <typename T>
inline T& At(unsigned char* m, std::size_t off) {
    return *reinterpret_cast<T*>(m + off);
}

template <typename T>
inline const T& At(const unsigned char* m, std::size_t off) {
    return *reinterpret_cast<const T*>(m + off);
}

inline std::uint8_t& PoseTraceFlag(unsigned char* m) {
    return reinterpret_cast<ModelRecord*>(m)->poseTraceRecording;
}

inline void*& PoseTraceBuffer(unsigned char* m) {
    return reinterpret_cast<ModelRecord*>(m)->poseTraceBuffer;
}

inline ModelRecord* Mdl(unsigned char* m) {
    return reinterpret_cast<ModelRecord*>(m);
}

inline const ModelRecord* Mdl(const unsigned char* m) {
    return reinterpret_cast<const ModelRecord*>(m);
}

inline IkChain*& IkChains(unsigned char* m) {
    return Mdl(m)->ikChains;
}

inline PmdVertex*& PmdVertices(unsigned char* m) {
    return Mdl(m)->rawVertices;
}

inline std::uint32_t& BaseVertexMorphCount(unsigned char* m) {
    return Mdl(m)->morph0Count;
}

inline std::int32_t& BoneMorphOffsetCount(unsigned char* m) {
    return Mdl(m)->boneMorphCount;
}

inline PmdVertexMorphEntry*& BaseVertexMorphTable(unsigned char* m) {
    return Mdl(m)->morph0Table;
}

inline BoneMorphOffsetRecord*& BoneMorphOffsets(unsigned char* m) {
    return Mdl(m)->boneMorphTable;
}

template <typename T>
inline T*& ResourceAs(void*& storage) {
    return reinterpret_cast<T*&>(storage);
}

inline wchar_t*& PmxTextBuffer(unsigned char* m, PmxTextBufferSlot slot) {
    return Mdl(m)->pmxTextBuffers[static_cast<std::size_t>(slot)];
}

inline ModelMaterialRecord*& Materials(unsigned char* m) {
    return Mdl(m)->materials;
}

inline const ModelMaterialRecord& Material(const unsigned char* bytes) {
    return *reinterpret_cast<const ModelMaterialRecord*>(bytes);
}

inline FrameGroup*& RigidGroups(unsigned char* m) {
    return ResourceAs<FrameGroup>(Mdl(m)->rbGroups);
}

inline FrameGroup*& DisplayFrames(unsigned char* m) {
    return Mdl(m)->displayFrames;
}

inline DisplayGroup*& DisplayGroups(unsigned char* m) {
    return ResourceAs<DisplayGroup>(Mdl(m)->groupNames);
}

inline char (*PmdToonFileNames(unsigned char* m))[100] {
    return Mdl(m)->pmdToonFileNames;
}

inline std::uint16_t*& Indices(unsigned char* m) {
    return ResourceAs<std::uint16_t>(Mdl(m)->indices);
}

inline std::int32_t*& PmxIndices(unsigned char* m) {
    return ResourceAs<std::int32_t>(Mdl(m)->indices);
}

inline PmxUvMorphCounts& UvMorphCounts(unsigned char* m) {
    return Mdl(m)->uvMorphCounts;
}

inline PmxUvMorphTables& UvMorphTables(unsigned char* m) {
    return Mdl(m)->uvMorphTables;
}

inline PmxUvMorphEntry*& PmxUvMorphBaseTable(unsigned char* m) {
    return UvMorphTables(m).byFamily[0];
}

inline PmxUvMorphEntry*& AdditionalUvMorphTable(unsigned char* m,
                                                 std::size_t family) {
    return UvMorphTables(m).byFamily[family + 1];
}

inline PmxMaterialMorphPools& MaterialMorphPools(unsigned char* m) {
    return Mdl(m)->materialMorphPools;
}

inline MaterialMorphPool*& MaterialMorphBase(unsigned char* m) {
    return MaterialMorphPools(m).base;
}

inline MaterialMorphPool*& MaterialMorphAdd(unsigned char* m) {
    return MaterialMorphPools(m).additive;
}

inline MaterialMorphPool*& MaterialMorphMul(unsigned char* m) {
    return MaterialMorphPools(m).multiplicative;
}

inline BoneRecord*& Bones(unsigned char* m) {
    return Mdl(m)->boneTable;
}

inline unsigned char* BoneBytes(BoneRecord* bones, std::size_t index) {
    return reinterpret_cast<unsigned char*>(&bones[index]);
}

inline MorphRecord*& Morphs(unsigned char* m) {
    return Mdl(m)->morphs;
}

inline BoneKey*& BoneKeys(unsigned char* m) {
    return Mdl(m)->boneKeys;
}

inline MorphKey*& MorphKeys(unsigned char* m) {
    return Mdl(m)->morphKeys;
}

inline std::uint32_t*& BoneKeyIndices(unsigned char* m) {
    return Mdl(m)->boneKeyIndices;
}

inline std::uint32_t*& MorphKeyIndices(unsigned char* m) {
    return Mdl(m)->morphKeyIndices;
}

inline unsigned char* MorphBytes(MorphRecord* morphs, std::size_t index) {
    return reinterpret_cast<unsigned char*>(&morphs[index]);
}

inline RigidRecord*& Rigids(unsigned char* m) {
    return Mdl(m)->rigidTable;
}

inline JointRecord*& Joints(unsigned char* m) {
    return Mdl(m)->jointTable;
}

inline unsigned char* RigidBytes(RigidRecord* records, std::size_t index) {
    return reinterpret_cast<unsigned char*>(&records[index]);
}

inline unsigned char* JointBytes(JointRecord* records, std::size_t index) {
    return reinterpret_cast<unsigned char*>(&records[index]);
}

inline DisplayKey*& DisplayKeys(unsigned char* m) {
    return Mdl(m)->displayKeys;
}

inline BoneOrderEntry*& BoneOrder(unsigned char* m) {
    return Mdl(m)->boneOrderTable;
}

inline std::uint32_t& BoneOrderCount(unsigned char* m) {
    return Mdl(m)->boneOrderCount;
}

}  
