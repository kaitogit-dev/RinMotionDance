#pragma once

#include <cstddef>
#include <cstdint>

#include "rinmotiondance/raw_pad.hpp"
#include "rinmotiondance/bone_binding.hpp"
#include "rinmotiondance/undo_layout.hpp"
#include "rinmotiondance/skeleton_tracking.hpp"

namespace rinmotiondance { class PhysicsScene; }

namespace rinmotiondance::mdl {

struct BoneRecord;
struct MorphRecord;
struct ModelMaterialRecord;
struct BoneKey;
struct MorphKey;
struct DisplayKey;
struct FrameGroup;
struct RigidRecord;
struct JointRecord;
struct IkChain;
struct PmdVertex;
struct PmdVertexMorphEntry;
struct PmxVertex;
struct PmxUvMorphEntry;
struct BoneMorphOffsetRecord;
struct MaterialMorphPool;

struct PmxUvMorphCounts { std::int32_t byFamily[5]; };
struct PmxUvMorphTables { PmxUvMorphEntry* byFamily[5]; };
struct PmxMaterialMorphPools {
    MaterialMorphPool* base;
    MaterialMorphPool* additive;
    MaterialMorphPool* multiplicative;
};

using rinmotiondance::RawPad;

struct ModelRecord {
    void* hwnd;  
    std::uint32_t vertexCount;  
    void* vertexBuffer;  
    void* vertexBuffer2;  
    void* indexBuffer;  
    std::uint32_t indexCount;  
    void* indices;  
    std::uint32_t materialCount;  
    ModelMaterialRecord* materials;  
    std::uint32_t* boneKeyCursors;  
    unsigned char* boneTrackActive;  
    std::uint32_t* morphKeyCursors;  
    unsigned char* morphTrackActive;  
    std::uint32_t displayKeyCursor;  
    unsigned char displayTrackActive;  
    RawPad<3> gap0;  
    ::rinmotiondance::PhysicsScene* scenePtr;  
    StandardSkeletonPose standardPose;
    SkeletonHistory skeletonHistory;
    std::uint8_t poseTraceRecording;
    void* poseTraceBuffer;
    std::uint8_t pmxTextEncoding;        
    std::uint8_t pmxAdditionalUvCount;   
    std::uint8_t pmxVertexIndexSize;     
    std::uint8_t pmxTextureIndexSize;    
    std::uint8_t pmxBoneIndexSize;       
    std::uint8_t pmxMorphIndexSize;      
    std::uint8_t pmxMaterialIndexSize;   
    std::uint8_t pmxRigidIndexSize;      
#if defined(_M_X64)
    RawPad<60> pmxReserved;
#else
    RawPad<52> pmxReserved;
#endif
    std::uint32_t morph0Count;  
    PmxUvMorphCounts uvMorphCounts;
    std::int32_t boneMorphCount;  
    RawPad<12> gap6;  
    PmdVertexMorphEntry* morph0Table;  
    BoneMorphOffsetRecord* boneMorphTable;
    void* reservedMorphTable;
    PmxUvMorphTables uvMorphTables;
    PmxMaterialMorphPools materialMorphPools;
    std::int32_t searchCursor;  
    std::int32_t maxBoneLayer;  
    char name[50];  
    char nameEn[50];
    char comment[256];  
    char commentEn[256];  
#if defined(_M_X64)
    RawPad<4> pmxTextAlignment;
#endif
    wchar_t* pmxTextBuffers[4];  
    wchar_t path[256];  
    BoneRecord* boneTable;  
    IkChain* ikChains;  
    MorphRecord* morphs;  
    PmdVertex* rawVertices;  
    PmxVertex* pmxVertices;  
    void* groupNames;  
    unsigned char groupCount;  
    RawPad<3> gap11;  
    void* rbGroups;  
    FrameGroup* displayFrames;  
    BoneKey* boneKeys;  
    MorphKey* morphKeys;  
    DisplayKey* displayKeys;  
    UndoRing undoRings[2];  
    unsigned char comboSelIndex;  
    unsigned char comboSelIndex2;  
    RawPad<2> gap12;  
    std::uint32_t morphCount;  
    std::uint32_t boneCount;  
    std::uint32_t ikChainCount;  
    unsigned char displayState;  
    unsigned char loadComplete;  
    RawPad<2> gap13;  
    std::int32_t selectedBone;  
    unsigned char* boneSelection;  
    unsigned char* bonePhysicsState;  
    std::int32_t selectedMorphs[4];  
    unsigned char facialFrameCount;  
    RawPad<3> gap14;  
    std::uint32_t rigidBodyCount;  
    std::uint32_t* boneKeyIndices;  
    std::uint32_t* morphKeyIndices;  
#if defined(_M_X64)
#endif
    std::int32_t boneListSelLine;         
    unsigned char boneListRowType[200];   
    std::int32_t boneListRowRecord[200];  
    std::int32_t boneListRows;  
    std::int32_t boneListPos;  
    std::uint32_t maxFrame;  
    std::uint32_t undoState[2];  
    unsigned char undoDirty;  
    unsigned char redoDirty;  
    unsigned char postLoadFlag2;  
    RawPad<1> gap16;  
    float edgeScale;  
    unsigned char physicsFlags;  
    RawPad<3> gap17;  
    RigidRecord* rigidTable;  
    JointRecord* jointTable;  
    std::uint32_t rigidCount;  
    std::uint32_t jointCount;  
    wchar_t modelDirectory[256];       
    char pmdToonFileNames[10][100];    
    unsigned char toonFlag;  
    RawPad<3> gap19;  
    std::uint32_t toonShared;  
    SkeletonJoints currentJoints;
    RawPad<12> skeletonReserved;
    float lightDir[3];  
    float legIkXOffset;  
    std::int32_t matMisc;  
    unsigned char displayKeyframesPresent;  
    unsigned char openniVersion;  
    unsigned char physicsMode;  
    RawPad<1> gap25;  
    std::uint32_t displayRootBone;  
#if defined(_M_X64)
    unsigned char keyVisitMap[600000];  
#else
    unsigned char keyVisitMap[300000];  
#endif
    BoneOrderEntry* boneOrderTable;  
    std::uint32_t boneOrderCount;  
    std::int32_t centerBone;  
    std::int32_t frameRegistrationSelection;  
};

#if !defined(_M_X64)
static_assert(offsetof(ModelRecord, hwnd) == 0,
              "hwnd x86");
static_assert(offsetof(ModelRecord, vertexCount) == 4,
              "vertexCount x86");
static_assert(offsetof(ModelRecord, vertexBuffer) == 8,
              "vertexBuffer x86");
static_assert(offsetof(ModelRecord, vertexBuffer2) == 12,
              "vertexBuffer2 x86");
static_assert(offsetof(ModelRecord, indexBuffer) == 16,
              "indexBuffer x86");
static_assert(offsetof(ModelRecord, indexCount) == 20,
              "indexCount x86");
static_assert(offsetof(ModelRecord, indices) == 24,
              "indices x86");
static_assert(offsetof(ModelRecord, materialCount) == 28,
              "materialCount x86");
static_assert(offsetof(ModelRecord, materials) == 32,
              "materials x86");
static_assert(offsetof(ModelRecord, boneKeyCursors) == 36,
              "boneKeyCursors x86");
static_assert(offsetof(ModelRecord, boneTrackActive) == 40,
              "boneTrackActive x86");
static_assert(offsetof(ModelRecord, morphKeyCursors) == 44,
              "morphKeyCursors x86");
static_assert(offsetof(ModelRecord, morphTrackActive) == 48,
              "morphTrackActive x86");
static_assert(offsetof(ModelRecord, displayKeyCursor) == 52,
              "displayKeyCursor x86");
static_assert(offsetof(ModelRecord, displayTrackActive) == 56,
              "displayTrackActive x86");
static_assert(offsetof(ModelRecord, scenePtr) == 60,
              "scenePtr x86");
static_assert(offsetof(ModelRecord, pmxTextEncoding) == 8624,
              "pmxTextEncoding x86");
static_assert(offsetof(ModelRecord, morph0Count) == 8684,
              "morph0Count x86");
static_assert(offsetof(ModelRecord, boneMorphCount) == 8708,
              "boneMorphCount x86");
static_assert(offsetof(ModelRecord, morph0Table) == 8724,
              "morph0Table x86");
static_assert(offsetof(ModelRecord, boneMorphTable) == 8728,
              "boneMorphTable x86");
static_assert(offsetof(ModelRecord, maxBoneLayer) == 8772,
              "maxBoneLayer x86");
static_assert(offsetof(ModelRecord, name) == 8776,
              "name x86");
static_assert(offsetof(ModelRecord, nameEn) == 8826,
              "nameEn x86");
static_assert(offsetof(ModelRecord, searchCursor) == 8768,
              "searchCursor x86");
static_assert(offsetof(ModelRecord, comment) == 8876,
              "comment x86");
static_assert(offsetof(ModelRecord, commentEn) == 9132,
              "commentEn x86");
static_assert(offsetof(ModelRecord, pmxTextBuffers) == 9388,
              "pmxTextBuffers x86");
static_assert(offsetof(ModelRecord, path) == 9404,
              "path x86");
static_assert(offsetof(ModelRecord, boneTable) == 9916,
              "boneTable x86");
static_assert(offsetof(ModelRecord, ikChains) == 9920,
              "ikChains x86");
static_assert(offsetof(ModelRecord, morphs) == 9924,
              "morphs x86");
static_assert(offsetof(ModelRecord, rawVertices) == 9928,
              "rawVertices x86");
static_assert(offsetof(ModelRecord, pmxVertices) == 9932,
              "pmxVertices x86");
static_assert(offsetof(ModelRecord, groupNames) == 9936,
              "groupNames x86");
static_assert(offsetof(ModelRecord, groupCount) == 9940,
              "groupCount x86");
static_assert(offsetof(ModelRecord, rbGroups) == 9944,
              "rbGroups x86");
static_assert(offsetof(ModelRecord, displayFrames) == 9948,
              "displayFrames x86");
static_assert(offsetof(ModelRecord, boneKeys) == 9952,
              "boneKeys x86");
static_assert(offsetof(ModelRecord, morphKeys) == 9956,
              "morphKeys x86");
static_assert(offsetof(ModelRecord, displayKeys) == 9960,
              "displayKeys x86");
static_assert(offsetof(ModelRecord, undoRings) == 9964,
              "undoRings x86");
static_assert(offsetof(ModelRecord, comboSelIndex) == 11644,
              "comboSelIndex x86");
static_assert(offsetof(ModelRecord, comboSelIndex2) == 11645,
              "comboSelIndex2 x86");
static_assert(offsetof(ModelRecord, morphCount) == 11648,
              "morphCount x86");
static_assert(offsetof(ModelRecord, boneCount) == 11652,
              "boneCount x86");
static_assert(offsetof(ModelRecord, ikChainCount) == 11656,
              "ikChainCount x86");
static_assert(offsetof(ModelRecord, displayState) == 11660,
              "displayState x86");
static_assert(offsetof(ModelRecord, loadComplete) == 11661,
              "loadComplete x86");
static_assert(offsetof(ModelRecord, selectedBone) == 11664,
              "selectedBone x86");
static_assert(offsetof(ModelRecord, boneSelection) == 11668,
              "boneSelection x86");
static_assert(offsetof(ModelRecord, bonePhysicsState) == 11672,
              "bonePhysicsState x86");
static_assert(offsetof(ModelRecord, selectedMorphs) == 11676,
              "selectedMorphs x86");
static_assert(offsetof(ModelRecord, facialFrameCount) == 11692,
              "facialFrameCount x86");
static_assert(offsetof(ModelRecord, rigidBodyCount) == 11696,
              "rigidBodyCount x86");
static_assert(offsetof(ModelRecord, boneKeyIndices) == 11700,
              "boneKeyIndices x86");
static_assert(offsetof(ModelRecord, morphKeyIndices) == 11704,
              "morphKeyIndices x86");
static_assert(offsetof(ModelRecord, boneListSelLine) == 11708,
              "boneListSelLine x86");
static_assert(offsetof(ModelRecord, boneListRowType) == 11712,
              "boneListRowType x86");
static_assert(offsetof(ModelRecord, boneListRowRecord) == 11912,
              "boneListRowRecord x86");
static_assert(offsetof(ModelRecord, boneListRows) == 12712,
              "boneListRows x86");
static_assert(offsetof(ModelRecord, boneListPos) == 12716,
              "boneListPos x86");
static_assert(offsetof(ModelRecord, maxFrame) == 12720,
              "maxFrame x86");
static_assert(offsetof(ModelRecord, undoState) == 12724,
              "undoState x86");
static_assert(offsetof(ModelRecord, undoDirty) == 12732,
              "undoDirty x86");
static_assert(offsetof(ModelRecord, redoDirty) == 12733,
              "redoDirty x86");
static_assert(offsetof(ModelRecord, postLoadFlag2) == 12734,
              "postLoadFlag2 x86");
static_assert(offsetof(ModelRecord, edgeScale) == 12736,
              "edgeScale x86");
static_assert(offsetof(ModelRecord, physicsFlags) == 12740,
              "physicsFlags x86");
static_assert(offsetof(ModelRecord, rigidTable) == 12744,
              "rigidTable x86");
static_assert(offsetof(ModelRecord, jointTable) == 12748,
              "jointTable x86");
static_assert(offsetof(ModelRecord, rigidCount) == 12752,
              "rigidCount x86");
static_assert(offsetof(ModelRecord, jointCount) == 12756,
              "jointCount x86");
static_assert(offsetof(ModelRecord, modelDirectory) == 12760,
              "modelDirectory x86");
static_assert(offsetof(ModelRecord, pmdToonFileNames) == 13272,
              "pmdToonFileNames x86");
static_assert(offsetof(ModelRecord, toonFlag) == 14272,
              "toonFlag x86");
static_assert(offsetof(ModelRecord, toonShared) == 14276,
              "toonShared x86");
static_assert(offsetof(ModelRecord, lightDir) == 14568,
              "lightDir x86");
static_assert(offsetof(ModelRecord, legIkXOffset) == 14580,
              "legIkXOffset x86");
static_assert(offsetof(ModelRecord, matMisc) == 14584,
              "matMisc x86");
static_assert(offsetof(ModelRecord, openniVersion) == 14589,
              "openniVersion x86");
static_assert(offsetof(ModelRecord, physicsMode) == 14590,
              "physicsMode x86");
static_assert(offsetof(ModelRecord, displayRootBone) == 14592,
              "displayRootBone x86");
static_assert(offsetof(ModelRecord, keyVisitMap) == 14596,
              "keyVisitMap x86");
static_assert(offsetof(ModelRecord, boneOrderTable) == 314596,
              "boneOrderTable x86");
static_assert(offsetof(ModelRecord, boneOrderCount) == 314600,
              "boneOrderCount x86");
static_assert(offsetof(ModelRecord, centerBone) == 314604,
              "centerBone x86");
static_assert(offsetof(ModelRecord, frameRegistrationSelection) == 314608,
              "frameRegistrationSelection x86");
static_assert(sizeof(ModelRecord) == 0x4CCF4,
              "model record x86 size");
#else
static_assert(offsetof(ModelRecord, name) == 8896,
              "name x64 (select-dialog JP name 0x22C0)");
static_assert(offsetof(ModelRecord, nameEn) == 8946,
              "nameEn x64");
static_assert(offsetof(ModelRecord, comboSelIndex) == 12552,
              "comboSelIndex x64 (0x3108)");
static_assert(offsetof(ModelRecord, comboSelIndex2) == 12553,
              "comboSelIndex2 x64 (0x3109; display-order byte scans in "
              "physics_frame/playback_catchup and the select dialog)");
static_assert(offsetof(ModelRecord, comment) == 8996,
              "comment x64");
static_assert(offsetof(ModelRecord, commentEn) == 9252,
              "commentEn x64");
static_assert(offsetof(ModelRecord, pmxTextBuffers) == 0x2528,
              "pmxTextBuffers x64");
static_assert(offsetof(ModelRecord, path) == 0x2548,
              "path x64");
static_assert(offsetof(ModelRecord, searchCursor) == 8888,
              "searchCursor x64 (0x22B8; bone pool 0x7FF7CB4E9958/"
              "0x7FF7CB4EA46F, morph pool 0x7FF7CB48E416, display pool "
              "0x7FF7CB48E5EA)");
static_assert(offsetof(ModelRecord, materialCount) == 56,
              "materialCount x64");
static_assert(offsetof(ModelRecord, materials) == 64,
              "materials x64");
static_assert(offsetof(ModelRecord, boneTable) == 10056,
              "boneTable x64");
static_assert(offsetof(ModelRecord, morphs) == 10072,
              "morphs x64");
static_assert(offsetof(ModelRecord, rawVertices) == 10080,
              "rawVertices x64");
static_assert(offsetof(ModelRecord, pmxVertices) == 10088,
              "pmxVertices x64");
static_assert(offsetof(ModelRecord, groupNames) == 10096,
              "groupNames x64");
static_assert(offsetof(ModelRecord, displayFrames) == 10120,
              "displayFrames x64");
static_assert(offsetof(ModelRecord, boneKeys) == 10128,
              "boneKeys x64");
static_assert(offsetof(ModelRecord, morphKeys) == 10136,
              "morphKeys x64");
static_assert(offsetof(ModelRecord, displayKeys) == 10144,
              "displayKeys x64");
static_assert(offsetof(ModelRecord, undoRings) == 10152,
              "undoRings x64");
static_assert(offsetof(ModelRecord, morphCount) == 12556,
              "morphCount x64");
static_assert(offsetof(ModelRecord, boneCount) == 12560,
              "boneCount x64");
static_assert(offsetof(ModelRecord, facialFrameCount) == 12608,
              "facialFrameCount x64");
static_assert(offsetof(ModelRecord, displayRootBone) == 0x3CA8,
              "displayRootBone x64");
static_assert(offsetof(ModelRecord, boneKeyIndices) == 0x3148,
              "boneKeyIndices x64");
static_assert(offsetof(ModelRecord, morphKeyIndices) == 0x3150,
              "morphKeyIndices x64");
static_assert(offsetof(ModelRecord, boneListSelLine) == 0x3158,
              "boneListSelLine x64");
static_assert(offsetof(ModelRecord, boneListRowType) == 0x315C,
              "boneListRowType x64");
static_assert(offsetof(ModelRecord, boneListRowRecord) == 0x3224,
              "boneListRowRecord x64");
static_assert(offsetof(ModelRecord, boneListRows) == 0x3544,
              "boneListRows x64");
static_assert(offsetof(ModelRecord, physicsFlags) == 13664,
              "physicsFlags x64 (0x3560; per-model language-flag sweep "
              "store 0x7FF7CB43A4F5)");
static_assert(offsetof(ModelRecord, rigidTable) == 13672,
              "rigidTable x64");
static_assert(offsetof(ModelRecord, jointTable) == 13680,
              "jointTable x64");
static_assert(offsetof(ModelRecord, rigidCount) == 13688,
              "rigidCount x64");
static_assert(offsetof(ModelRecord, jointCount) == 13692,
              "jointCount x64");
static_assert(offsetof(ModelRecord, modelDirectory) == 13696,
              "modelDirectory x64");
static_assert(offsetof(ModelRecord, pmdToonFileNames) == 14208,
              "pmdToonFileNames x64");
static_assert(offsetof(ModelRecord, undoState) == 13648,
              "undoState x64 (0x3550, ring depth 30, 0x7FF7CB4470A0)");
static_assert(offsetof(ModelRecord, undoDirty) == 13656,
              "undoDirty x64 (0x3558, 0x7FF7CB447074)");
static_assert(offsetof(ModelRecord, redoDirty) == 13657,
              "redoDirty x64 (0x3559, 0x7FF7CB44708A)");
static_assert(offsetof(ModelRecord, selectedBone) == 0x311C,
              "selectedBone x64");
static_assert(offsetof(ModelRecord, selectedMorphs) == 0x3130,
              "selectedMorphs x64");
static_assert(offsetof(ModelRecord, keyVisitMap) == 0x3CAC,
              "keyVisitMap x64");
static_assert(offsetof(ModelRecord, boneOrderTable) == 0x96470,
              "boneOrderTable x64");
static_assert(offsetof(ModelRecord, boneOrderCount) == 0x96478,
              "boneOrderCount x64");
#endif


#if defined(_M_X64)
static_assert(offsetof(ModelRecord, standardPose) == 120);
static_assert(offsetof(ModelRecord, skeletonHistory) == 392);
static_assert(offsetof(ModelRecord, poseTraceRecording) == 8672);
static_assert(offsetof(ModelRecord, poseTraceBuffer) == 8680);
static_assert(offsetof(ModelRecord, pmxTextEncoding) == 8688);
static_assert(offsetof(ModelRecord, morph0Count) == 8756);
static_assert(offsetof(ModelRecord, uvMorphCounts) == 8760);
static_assert(offsetof(ModelRecord, boneMorphCount) == 8780);
static_assert(offsetof(ModelRecord, morph0Table) == 8800);
static_assert(offsetof(ModelRecord, boneMorphTable) == 8808);
static_assert(offsetof(ModelRecord, reservedMorphTable) == 8816);
static_assert(offsetof(ModelRecord, uvMorphTables) == 8824);
static_assert(offsetof(ModelRecord, materialMorphPools) == 8864);
static_assert(offsetof(ModelRecord, currentJoints) == 15216);
#else
static_assert(offsetof(ModelRecord, standardPose) == 64);
static_assert(offsetof(ModelRecord, skeletonHistory) == 336);
static_assert(offsetof(ModelRecord, poseTraceRecording) == 8616);
static_assert(offsetof(ModelRecord, poseTraceBuffer) == 8620);
static_assert(offsetof(ModelRecord, uvMorphCounts) == 8688);
static_assert(offsetof(ModelRecord, reservedMorphTable) == 8732);
static_assert(offsetof(ModelRecord, uvMorphTables) == 8736);
static_assert(offsetof(ModelRecord, materialMorphPools) == 8756);
static_assert(offsetof(ModelRecord, currentJoints) == 14280);
#endif

}  
