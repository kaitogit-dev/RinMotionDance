#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <io.h>
#include <new>
#include <share.h>
#include <sys/stat.h>

#include "rinmotiondance/charset_conv.hpp"
#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"

namespace rinmotiondance {

void InitStandardSkeletonQuats(unsigned char* model, unsigned char flag);

namespace {

template <typename T>
inline T& At(unsigned char* p, std::size_t offset) {
    return *reinterpret_cast<T*>(p + offset);
}

template <typename T>
inline const T& At(const unsigned char* p, std::size_t offset) {
    return *reinterpret_cast<const T*>(p + offset);
}

}  

float* QuaternionNlerp(float* out, float q0x, float q0y, float q0z, float q0w,
                       float q1x, float q1y, float q1z, float q1w, float t) {
    const double dot =
        (double)q1x * (double)q0x + (double)q1w * (double)q0w +
        (double)q1y * (double)q0y + (double)q1z * (double)q0z;  
    const float dotF = (float)dot;
    float tt;                                                   
    if (dotF >= 0.0f) {                                         
        tt = t;
        out[3] = (float)((double)q0w + ((double)q1w - (double)q0w) * (double)t);
        out[0] = (float)((double)q0x + ((double)q1x - (double)q0x) * (double)t);
        out[1] = (float)((double)q0y + ((double)q1y - (double)q0y) * (double)t);
        out[2] = (float)((double)q0z + (double)t * ((double)q1z - (double)q0z));
    } else {
        const float nt = -t;                                    
        tt = nt;
        out[3] = (float)((double)q0w + ((double)q1w + (double)q0w) * (double)nt);
        out[0] = (float)((double)q0x + ((double)q1x + (double)q0x) * (double)nt);
        out[1] = (float)((double)q0y + ((double)q1y + (double)q0y) * (double)nt);
        out[2] = (float)((double)q0z + (double)nt * ((double)q1z + (double)q0z));
    }
    const float norm = (float)((double)out[1] * (double)out[1] +
                               (double)out[0] * (double)out[0] +
                               (double)out[2] * (double)out[2] +
                               (double)out[3] * (double)out[3]);  
    const float len = (float)std::sqrt((double)norm);            
    if (len <= 0.0f) {                                          
        out[3] = 1.0f;
        out[2] = 0.0f;
        out[1] = 0.0f;
        out[0] = 0.0f;
        return out;
    }
    const float inv = (float)(1.0 / (double)len);                
    out[0] = out[0] * inv;
    out[1] = out[1] * inv;
    out[2] = out[2] * inv;
    out[3] = inv * out[3];                                      
    return out;
}

void CopyMatrixToObject(unsigned char* self, const void* matrix16) {
    std::memcpy(self + 16, matrix16, 16 * 4);
}

void ModelBonePhysicsModeApply(unsigned char* model, int mode) {
    const int boneCount = rinmotiondance::mdl::Mdl(model)->boneCount;        
    rinmotiondance::mdl::BoneRecord* bones = rinmotiondance::mdl::Bones(model);
    if (mode == 0) {
        for (int i = 0; boneCount > 0 && i < boneCount; ++i)     
            if (bones[i].hasRigidBody != 0)
                NotifyBonePhysicsMode(model, i, 1);                          
    } else if (mode == 1) {
        for (int i = 0; boneCount > 0 && i < boneCount; ++i)     
            if (bones[i].hasRigidBody != 0)
                NotifyBonePhysicsMode(model, i, 0);                          
    } else if (mode >= 2) {
        for (int i = 0; boneCount > 0 && i < boneCount; ++i) {   
            rinmotiondance::mdl::BoneRecord* bone = &bones[i];
            if (bone->hasRigidBody != 0)
                NotifyBonePhysicsMode(model, i, bone->physicsDisabled != 0);             
        }
    }
}

void VertexHistoryPushAverage(float* current, float* history, int count) {
    mdl::PushSkeletonJointHistory(current, history, count);
}

void ModelVertexHistoryPush(unsigned char* model, int samples) {
    if (model == nullptr || samples <= 0)
        return;
    const int n = samples > 30 ? 30 : samples;
    auto& state = *mdl::Mdl(model);
    for (std::size_t joint = 0; joint < mdl::kTrackedJointCount; ++joint)
        VertexHistoryPushAverage(state.currentJoints.positions[joint],
                                 state.skeletonHistory.positions[joint], n);
}

int SavePmdFile(unsigned char* model, char* path, void* localeTable) {
    mdl::ModelRecord& state = *mdl::Mdl(model);
    int fh = -1;
    const errno_t openErr = _sopen_s(&fh, path, 33537  , 64  ,
                                     128  );          
    if (openErr != 0) {
        char text[256];
        if (rinmotiondance::mdl::Mdl(model)->physicsFlags != 0)                                   
            sprintf_s(text, 0x100u, "Cannot save file:%d", openErr);
        else
            sprintf_s(text, 0x100u,
                      "\203t\203C\203\213\202\252\225\333\221\266"
                      "\202\305\202\253\202\334\202\271\202\361:%d",
                      openErr);  
        return MessageBoxA(*reinterpret_cast<HWND*>(model), text,
                           g_Locale, 0);                         
    }

    _write(fh, "Pmd", 3);                                        
    const float version = 1.0f;
    _write(fh, &version, 4);
    _write(fh, state.name, sizeof(state.name));                  
    _write(fh, state.comment, sizeof(state.comment));            

    if (mdl::Morphs(model) != nullptr && state.morph0Count != 0) {
        for (std::uint32_t i = 0; i < state.morph0Count; ++i) {
            const mdl::PmdVertexMorphEntry& entry = state.morph0Table[i];
            mdl::PmdVertex& vertex = mdl::PmdVertices(model)[entry.vertexIndex];
            vertex.position[0] = entry.offset[0];
            vertex.position[1] = entry.offset[1];
            vertex.position[2] = entry.offset[2];
        }
    }

    _write(fh, &state.vertexCount, sizeof(state.vertexCount));
    for (std::uint32_t i = 0; i < state.vertexCount; ++i) {
        const mdl::PmdVertex& vertex = mdl::PmdVertices(model)[i];
        _write(fh, vertex.position, sizeof(vertex.position));
        _write(fh, vertex.normal, sizeof(vertex.normal));
        _write(fh, vertex.uv, sizeof(vertex.uv));
        _write(fh, vertex.bone, sizeof(vertex.bone));
        _write(fh, &vertex.weightPercent, sizeof(vertex.weightPercent));
        const char edge = vertex.edgeDisabled != 0;               
        _write(fh, &edge, 1);
    }

    _write(fh, &state.indexCount, sizeof(state.indexCount));
    for (std::uint32_t i = 0; i < state.indexCount; ++i) {
        const std::uint16_t index =
            static_cast<std::uint16_t>(mdl::PmxIndices(model)[i]);
        _write(fh, &index, sizeof(index));
    }

    _write(fh, &state.materialCount, sizeof(state.materialCount));
    for (std::uint32_t i = 0; i < state.materialCount; ++i) {
        const mdl::ModelMaterialRecord& mat = mdl::Materials(model)[i];
        _write(fh, &mat.diffuse[0], sizeof(mat.diffuse[0]));
        _write(fh, &mat.diffuse[1], sizeof(mat.diffuse[1]));
        _write(fh, &mat.diffuse[2], sizeof(mat.diffuse[2]));
        _write(fh, &mat.diffuse[3], sizeof(mat.diffuse[3]));  
        _write(fh, &mat.specularPower, sizeof(mat.specularPower));
        _write(fh, mat.specular, sizeof(mat.specular));
        _write(fh, mat.ambient, sizeof(mat.ambient));
        _write(fh, &mat.toonReference, sizeof(mat.toonReference));
        const char edgeFlag = mat.doubleSided != 0;              
        _write(fh, &edgeFlag, 1);
        _write(fh, &mat.faceVertexCount, sizeof(mat.faceVertexCount));

        const wchar_t* prefix = state.modelDirectory;
        const std::size_t skip = wcslen(prefix);
        wchar_t wide[20];
        char sjis[256];
        if (mat.spherePath[0] != 0) {                            
            wcscpy_s(wide, 0x14u, mat.texturePath + skip);
            wcscat_s(wide, 0x14u, L"*");
            wcscat_s(wide, 0x14u, mat.spherePath + skip);
            WideToSjis(static_cast<D3DRenderer*>(localeTable), sjis, wide, 0x100);
        } else {
            const wchar_t* name = mat.texturePath;
            if (name[0] != 0)
                name += skip;
            WideToSjis(static_cast<D3DRenderer*>(localeTable), sjis, name, 0x100);
        }
        _write(fh, sjis, 0x14);
    }

    _write(fh, &state.boneCount, 2);
    for (int i = 0; i < state.boneCount; ++i) {
        const mdl::BoneRecord* bone = &mdl::Bones(model)[i];
        _write(fh, bone->name, 0x14);
        _write(fh, &bone->parent, 2);
        _write(fh, &bone->tailBone, 2);
        _write(fh, &bone->type, 1);
        _write(fh, &bone->tailIdx, 2);
        _write(fh, bone->position, 4);
        _write(fh, &bone->position[1], 4);
        _write(fh, &bone->position[2], 4);
    }

    _write(fh, &state.ikChainCount, 2);
    for (int i = 0; i < state.ikChainCount; ++i) {
        const mdl::IkChain& ik = mdl::IkChains(model)[i];
        _write(fh, &ik.boneIndex, 2);
        _write(fh, &ik.targetBone, 2);
        _write(fh, &ik.linkCount, 1);
        _write(fh, &ik.iterations, 2);
        _write(fh, &ik.maxAngle, 4);
        for (unsigned char j = 0; j < ik.linkCount; ++j)
            _write(fh, &ik.links[j], 2);
    }

    _write(fh, &state.morphCount, 2);
    for (int i = 0; i < static_cast<int>(state.morphCount); ++i) {
        const mdl::MorphRecord& morph = mdl::Morphs(model)[i];
        _write(fh, morph.name, sizeof(morph.name));
        _write(fh, &morph.offsetCount, sizeof(morph.offsetCount));
        _write(fh, &morph.panel, sizeof(morph.panel));
        if (morph.offsetCount == 0)
            continue;
        const mdl::MorphRecord& baseMorph = mdl::Morphs(model)[0];
        const std::uint32_t baseCount =
            static_cast<std::uint32_t>(baseMorph.offsetCount);
        for (std::uint32_t k = 0;
             k < static_cast<std::uint32_t>(morph.offsetCount); ++k) {
            const mdl::PmdVertexMorphEntry& entry = morph.vertexEntries[k];
            const std::uint32_t index = entry.vertexIndex;
            const mdl::PmdVertexMorphEntry* indexEntry = &entry;
            for (std::uint32_t j = 0; j < baseCount; ++j) {
                if (baseMorph.vertexEntries[j].vertexIndex == index) {
                    indexEntry = &baseMorph.vertexEntries[j];
                    break;
                }
            }
            _write(fh, &indexEntry->vertexIndex,
                   sizeof(indexEntry->vertexIndex));             
            _write(fh, entry.offset, sizeof(entry.offset));
        }
    }

    const unsigned char frameCount = state.facialFrameCount;
    _write(fh, &state.facialFrameCount, sizeof(state.facialFrameCount));
    for (unsigned char i = 0; i < frameCount; ++i)
        _write(fh, &state.displayFrames[i].targetIndex,
               sizeof(state.displayFrames[i].targetIndex));
    const char upper = static_cast<char>(
        state.groupCount - 2 - (frameCount != 0 ? 1 : 0));
    _write(fh, &upper, 1);
    if (frameCount != 0)
        for (unsigned char k = 3; k < state.groupCount; ++k)
            _write(fh, mdl::DisplayGroups(model)[k].name,
                   sizeof(mdl::DisplayGroups(model)[k].name));
    else
        for (unsigned char group = 2; group < state.groupCount; ++group)
            _write(fh, mdl::DisplayGroups(model)[group].name,
                   sizeof(mdl::DisplayGroups(model)[group].name));

    _write(fh, &state.rigidBodyCount, sizeof(state.rigidBodyCount));
    for (std::uint32_t i = 0; i < state.rigidBodyCount; ++i) {
        const mdl::FrameGroup& record = mdl::RigidGroups(model)[i];
        const char count = static_cast<char>(record.groupIndex - 1 -
                                             (frameCount != 0 ? 1 : 0));
        _write(fh, &record.targetIndex, sizeof(record.targetIndex));
        _write(fh, &count, 1);
    }

    const char english = 1;
    _write(fh, &english, 1);
    _write(fh, state.nameEn, sizeof(state.nameEn));
    _write(fh, state.commentEn, sizeof(state.commentEn));
    for (int i = 0; i < rinmotiondance::mdl::Mdl(model)->boneCount; ++i)
        _write(fh, &rinmotiondance::mdl::Bones(model)[i].nameEn[0], 0x14);
    if (rinmotiondance::mdl::Mdl(model)->morphCount > 1)
        for (int i = 1; i < rinmotiondance::mdl::Mdl(model)->morphCount; ++i)
            _write(fh, mdl::Morphs(model)[i].nameEn,
                   sizeof(mdl::Morphs(model)[i].nameEn));
    if (frameCount != 0)
        for (unsigned char n = 3; n < state.groupCount; ++n)
            _write(fh, mdl::DisplayGroups(model)[n].nameEn,
                   sizeof(mdl::DisplayGroups(model)[n].nameEn));
    else
        for (unsigned char n = 2; n < state.groupCount; ++n)
            _write(fh, mdl::DisplayGroups(model)[n].nameEn,
                   sizeof(mdl::DisplayGroups(model)[n].nameEn));

    for (int i = 0; i < 10; ++i)
        _write(fh, mdl::PmdToonFileNames(model)[i], 100);

    _write(fh, &state.rigidCount, sizeof(state.rigidCount));
    for (int i = 0; i < state.rigidCount; ++i) {
        const mdl::RigidRecord& rigid = mdl::Rigids(model)[i];
        const std::int16_t boneIndex = static_cast<std::int16_t>(rigid.boneIndex);
        _write(fh, rigid.name, sizeof(rigid.name));
        _write(fh, &boneIndex, sizeof(boneIndex));
        _write(fh, &rigid.group, sizeof(rigid.group));
        _write(fh, &rigid.noCollapse, sizeof(rigid.noCollapse));
        _write(fh, &rigid.shape, sizeof(rigid.shape));
        _write(fh, rigid.size, sizeof(rigid.size));
        _write(fh, rigid.position, sizeof(rigid.position));
        _write(fh, rigid.rotation, sizeof(rigid.rotation));
        _write(fh, &rigid.mass, sizeof(rigid.mass));
        _write(fh, &rigid.linearDamping, sizeof(rigid.linearDamping));
        _write(fh, &rigid.angularDamping, sizeof(rigid.angularDamping));
        _write(fh, &rigid.restitution, sizeof(rigid.restitution));
        _write(fh, &rigid.friction, sizeof(rigid.friction));
        _write(fh, &rigid.mode, sizeof(rigid.mode));
    }

    _write(fh, &state.jointCount, sizeof(state.jointCount));
    for (int i = 0; i < state.jointCount; ++i) {
        const mdl::JointRecord& joint = mdl::Joints(model)[i];
        _write(fh, joint.name, sizeof(joint.name));
        _write(fh, &joint.rigidA, sizeof(joint.rigidA));
        _write(fh, &joint.rigidB, sizeof(joint.rigidB));
        _write(fh, joint.position, sizeof(joint.position));
        _write(fh, joint.rotation, sizeof(joint.rotation));

        constexpr int kPmdLimitOrder[] = {3, 4, 5, 0, 1, 2, 9, 10, 11, 6, 7, 8};
        for (int component : kPmdLimitOrder)
            _write(fh, &joint.limits[component], sizeof(joint.limits[component]));
        _write(fh, joint.springs, sizeof(joint.springs));
    }

    _close(fh);                                                  
    return wcscpy_s(state.path, sizeof(state.path) / sizeof(state.path[0]),
                    reinterpret_cast<const wchar_t*>(path));
}

namespace {

const unsigned char kCenter[9] =                                 
    {0x83, 0x5A, 0x83, 0x93, 0x83, 0x5E, 0x81, 0x5B, 0x00};      
const unsigned char kUpperBody[7] =                              
    {0x8F, 0xE3, 0x94, 0xBC, 0x90, 0x67, 0x00};                  
const unsigned char kNeck[3] =                                   
    {0x8E, 0xF1, 0x00};                                          
const unsigned char kArmL[5] =                                   
    {0x8D, 0xB6, 0x98, 0x72, 0x00};                              
const unsigned char kElbowL[7] =                                 
    {0x8D, 0xB6, 0x82, 0xD0, 0x82, 0xB6, 0x00};                  
const unsigned char kArmR[5] =                                   
    {0x89, 0x45, 0x98, 0x72, 0x00};                              
const unsigned char kElbowR[7] =                                 
    {0x89, 0x45, 0x82, 0xD0, 0x82, 0xB6, 0x00};                  
const unsigned char kLowerBody[7] =                              
    {0x89, 0xBA, 0x94, 0xBC, 0x90, 0x67, 0x00};                  
const unsigned char kLegL[5] =                                   
    {0x8D, 0xB6, 0x91, 0xAB, 0x00};                              
const unsigned char kLegR[5] =                                   
    {0x89, 0x45, 0x91, 0xAB, 0x00};                              
const unsigned char kKneeL[7] =                                  
    {0x8D, 0xB6, 0x82, 0xD0, 0x82, 0xB4, 0x00};                  
const unsigned char kKneeR[7] =                                  
    {0x89, 0x45, 0x82, 0xD0, 0x82, 0xB4, 0x00};                  
const unsigned char kLegIkL[9] =                                 
    {0x8D, 0xB6, 0x91, 0xAB, 0x82, 0x68, 0x82, 0x6A, 0x00};      
const unsigned char kLegIkR[9] =                                 
    {0x89, 0x45, 0x91, 0xAB, 0x82, 0x68, 0x82, 0x6A, 0x00};      
const unsigned char kTwistL[7] =                                 
    {0x8D, 0xB6, 0x8E, 0xEA, 0x00, 0x00, 0x8D};                  
const unsigned char kTwistR[7] =                                 
    {0x89, 0x45, 0x8E, 0xEA, 0x00, 0x00, 0x89};                  
const unsigned char kShoulderL[5] =                              
    {0x8D, 0xB6, 0x8C, 0xA8, 0x00};                              
const unsigned char kShoulderR[5] =                              
    {0x89, 0x45, 0x8C, 0xA8, 0x00};                              
const unsigned char kAnkleL[7] =                                 
    {0x8D, 0xB6, 0x91, 0xAB, 0x8E, 0xF1, 0x00};                  

inline float& TraceSlot(unsigned char* model, std::size_t fieldOffset) {
    return At<float>(
        static_cast<unsigned char*>(rinmotiondance::mdl::PoseTraceBuffer(model)),
        308 * (std::size_t)rinmotiondance::mdl::Mdl(model)->matMisc +
            fieldOffset);
}

void WritePoseGroup(unsigned char* model, rinmotiondance::mdl::BoneRecord* bone,
                    std::size_t slot, int quatBase, float s1, float s2,
                    float s3, float s4) {
    bone->rotQuat[3] = s4;
    bone->rotQuat[0] = s1;
    bone->rotQuat[1] = s2;
    bone->rotQuat[2] = s3;
    if (rinmotiondance::mdl::PoseTraceFlag(model) != 0) {
        TraceSlot(model, slot) = bone->rotQuat[3];
        TraceSlot(model, slot + 4) = bone->rotQuat[0];
        TraceSlot(model, slot + 8) = bone->rotQuat[1];
        TraceSlot(model, slot + 12) = bone->rotQuat[2];
    }
}

}  

char ModelStandardPoseSetup(unsigned char* model, bool recordEnable,
                            unsigned char mirrorLeftRight, unsigned char skeletonFlag) {
    char wasRecording = 0;
    if (rinmotiondance::mdl::PoseTraceFlag(model) == 0 && recordEnable) {  
        rinmotiondance::mdl::PoseTraceFlag(model) = 1;
        if (rinmotiondance::mdl::PoseTraceBuffer(model) != nullptr) {
            ::operator delete(rinmotiondance::mdl::PoseTraceBuffer(model));   
            rinmotiondance::mdl::PoseTraceBuffer(model) = nullptr;
        }
        rinmotiondance::mdl::PoseTraceBuffer(model) =
            ::operator new(0x3B3760);                             
        rinmotiondance::mdl::Mdl(model)->matMisc = 0;
    }
    if (!recordEnable) {                                         
        if (rinmotiondance::mdl::PoseTraceFlag(model) != 0)
            wasRecording = 1;
        rinmotiondance::mdl::PoseTraceFlag(model) = 0;
    }

    bool leftIkAdd = recordEnable;
    bool rightIkAdd = false;
    if (rinmotiondance::mdl::Mdl(model)->ikChainCount > 0) {
        for (std::uint16_t i = 0; i < (std::uint16_t)rinmotiondance::mdl::Mdl(model)->ikChainCount; ++i) {
            const rinmotiondance::mdl::IkChain& ik = rinmotiondance::mdl::IkChains(model)[i];
            const rinmotiondance::mdl::BoneRecord* bone =
                &rinmotiondance::mdl::Bones(model)[ik.boneIndex];
            if (std::memcmp(bone, kLegIkL, 9) == 0)
                leftIkAdd = ik.enabled != 0;                     
            if (std::memcmp(bone, kLegIkR, 9) == 0)
                rightIkAdd = ik.enabled != 0;                    
        }
    }

    InitStandardSkeletonQuats(model, skeletonFlag);                        

    if ((double)rinmotiondance::mdl::Mdl(model)->lightDir[1] == 90.0 &&
        (double)rinmotiondance::mdl::Mdl(model)->lightDir[0] != -1.0) {
        const int boneCount = rinmotiondance::mdl::Mdl(model)->boneCount;
        rinmotiondance::mdl::BoneRecord* bones = rinmotiondance::mdl::Bones(model);
        const int leg = mdl::FindSkeletonBone(bones, boneCount, kLegL, sizeof kLegL);
        if (leg >= 0) {
            const float legY = bones[leg].position[1];
            if ((double)legY != -1.0) {
                const float scale = rinmotiondance::mdl::Mdl(model)->lightDir[0] / legY;  
                const double scaleD = scale;                     
                rinmotiondance::mdl::Mdl(model)->lightDir[1] = scale;
                const float offset =
                    -mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][1] / scale;                
                rinmotiondance::mdl::Mdl(model)->lightDir[2] = offset;
                const int ankle = mdl::FindSkeletonBone(
                    bones, boneCount, kAnkleL, sizeof kAnkleL);
                if (ankle >= 0) {
                    const float ankleY = bones[ankle].position[1];
                    if ((double)ankleY == -1.0) {
                        rinmotiondance::mdl::Mdl(model)->legIkXOffset = (float)(80.0 / scaleD);
                    } else {
                        rinmotiondance::mdl::Mdl(model)->lightDir[2] =
                            (float)((double)ankleY / scaleD + (double)offset);
                        rinmotiondance::mdl::Mdl(model)->legIkXOffset = (float)(80.0 / scaleD);    
                    }
                } else {
                    rinmotiondance::mdl::Mdl(model)->legIkXOffset =
                        (float)(80.0 / scaleD);                  
                }
            }
        }
    }

    float centerPos[3] = {0.0f, 0.0f, 0.0f};
    const int boneCount = rinmotiondance::mdl::Mdl(model)->boneCount;
    rinmotiondance::mdl::BoneRecord* bones = rinmotiondance::mdl::Bones(model);
    for (int i = 0; i < boneCount; ++i) {
        rinmotiondance::mdl::BoneRecord* bone = &bones[i];
        if (std::memcmp(bone, kCenter, 9) == 0) {
            if ((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::Center][1] != -999.0) {
                const double inv = 1.0 / (double)rinmotiondance::mdl::Mdl(model)->lightDir[1];
                bone->trans[0] =
                    (float)((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::Center][0] * inv);
                bone->trans[1] =
                    (float)((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::Center][1] * inv);
                bone->trans[2] =
                    (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::Center][2]);
                if (mirrorLeftRight == 0)
                    bone->trans[0] = -bone->trans[0];
            }
            bone->rotQuat[3] = 1.0f;
            bone->rotQuat[0] = 0.0f;
            bone->rotQuat[1] = 0.0f;
            bone->rotQuat[2] = 0.0f;
            if (mdl::PoseTraceFlag(model) != 0) {
                TraceSlot(model, 0) = bone->trans[0];
                TraceSlot(model, 4) = bone->trans[1];
                TraceSlot(model, 8) = bone->trans[2];
            }
            centerPos[0] = bone->trans[0];
            centerPos[1] = bone->trans[1];
            centerPos[2] = bone->trans[2];
        } else if (std::memcmp(bone, kUpperBody, 7) == 0) {
            WritePoseGroup(model, bone, 12, 0,
                mdl::Mdl(model)->standardPose.upperBody[0], mirrorLeftRight ? mdl::Mdl(model)->standardPose.upperBody[1] : -mdl::Mdl(model)->standardPose.upperBody[1],
                mirrorLeftRight ? mdl::Mdl(model)->standardPose.upperBody[2] : -mdl::Mdl(model)->standardPose.upperBody[2],
                mdl::Mdl(model)->standardPose.upperBody[3]);
        } else if (std::memcmp(bone, kNeck, 3) == 0) {
            WritePoseGroup(model, bone, 28, 0,
                mdl::Mdl(model)->standardPose.neck[0], mirrorLeftRight ? mdl::Mdl(model)->standardPose.neck[1] : -mdl::Mdl(model)->standardPose.neck[1],
                mirrorLeftRight ? mdl::Mdl(model)->standardPose.neck[2] : -mdl::Mdl(model)->standardPose.neck[2],
                mdl::Mdl(model)->standardPose.neck[3]);
        } else if (std::memcmp(bone, kArmL, 5) == 0) {
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 44, 0, mdl::Mdl(model)->standardPose.leftArm[0],
                    mdl::Mdl(model)->standardPose.leftArm[1], mdl::Mdl(model)->standardPose.leftArm[2],
                    mdl::Mdl(model)->standardPose.leftArm[3]);
            else
                WritePoseGroup(model, bone, 44, 0, mdl::Mdl(model)->standardPose.rightArm[0],
                    -mdl::Mdl(model)->standardPose.rightArm[1], -mdl::Mdl(model)->standardPose.rightArm[2],
                    mdl::Mdl(model)->standardPose.rightArm[3]);
        } else if (std::memcmp(bone, kElbowL, 7) == 0) {
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 60, 0, mdl::Mdl(model)->standardPose.leftElbow[0],
                    mdl::Mdl(model)->standardPose.leftElbow[1], mdl::Mdl(model)->standardPose.leftElbow[2],
                    mdl::Mdl(model)->standardPose.leftElbow[3]);
            else
                WritePoseGroup(model, bone, 60, 0, mdl::Mdl(model)->standardPose.rightElbow[0],
                    -mdl::Mdl(model)->standardPose.rightElbow[1], -mdl::Mdl(model)->standardPose.rightElbow[2],
                    mdl::Mdl(model)->standardPose.rightElbow[3]);
        } else if (std::memcmp(bone, kArmR, 5) == 0) {
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 76, 0, mdl::Mdl(model)->standardPose.rightArm[0],
                    mdl::Mdl(model)->standardPose.rightArm[1], mdl::Mdl(model)->standardPose.rightArm[2],
                    mdl::Mdl(model)->standardPose.rightArm[3]);
            else
                WritePoseGroup(model, bone, 76, 0, mdl::Mdl(model)->standardPose.leftArm[0],
                    -mdl::Mdl(model)->standardPose.leftArm[1], -mdl::Mdl(model)->standardPose.leftArm[2],
                    mdl::Mdl(model)->standardPose.leftArm[3]);
        } else if (std::memcmp(bone, kElbowR, 7) == 0) {
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 92, 0, mdl::Mdl(model)->standardPose.rightElbow[0],
                    mdl::Mdl(model)->standardPose.rightElbow[1], mdl::Mdl(model)->standardPose.rightElbow[2],
                    mdl::Mdl(model)->standardPose.rightElbow[3]);
            else
                WritePoseGroup(model, bone, 92, 0, mdl::Mdl(model)->standardPose.leftElbow[0],
                    -mdl::Mdl(model)->standardPose.leftElbow[1], -mdl::Mdl(model)->standardPose.leftElbow[2],
                    mdl::Mdl(model)->standardPose.leftElbow[3]);
        } else if (std::memcmp(bone, kLowerBody, 7) == 0) {
            WritePoseGroup(model, bone, 108, 0, mdl::Mdl(model)->standardPose.lowerBody[0],
                mirrorLeftRight ? mdl::Mdl(model)->standardPose.lowerBody[1] : -mdl::Mdl(model)->standardPose.lowerBody[1],
                mirrorLeftRight ? mdl::Mdl(model)->standardPose.lowerBody[2] : -mdl::Mdl(model)->standardPose.lowerBody[2],
                mdl::Mdl(model)->standardPose.lowerBody[3]);
        } else if (std::memcmp(bone, kLegL, 5) == 0) {
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 124, 0, mdl::Mdl(model)->standardPose.leftLeg[0],
                    mdl::Mdl(model)->standardPose.leftLeg[1], mdl::Mdl(model)->standardPose.leftLeg[2],
                    mdl::Mdl(model)->standardPose.leftLeg[3]);
            else
                WritePoseGroup(model, bone, 124, 0, mdl::Mdl(model)->standardPose.rightLeg[0],
                    -mdl::Mdl(model)->standardPose.rightLeg[1], -mdl::Mdl(model)->standardPose.rightLeg[2],
                    mdl::Mdl(model)->standardPose.rightLeg[3]);
        } else if (std::memcmp(bone, kLegR, 5) == 0) {
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 140, 0, mdl::Mdl(model)->standardPose.rightLeg[0],
                    mdl::Mdl(model)->standardPose.rightLeg[1], mdl::Mdl(model)->standardPose.rightLeg[2],
                    mdl::Mdl(model)->standardPose.rightLeg[3]);
            else
                WritePoseGroup(model, bone, 140, 0, mdl::Mdl(model)->standardPose.leftLeg[0],
                    -mdl::Mdl(model)->standardPose.leftLeg[1], -mdl::Mdl(model)->standardPose.leftLeg[2],
                    mdl::Mdl(model)->standardPose.leftLeg[3]);
        } else if (std::memcmp(bone, kKneeL, 7) == 0) {           
            if (leftIkAdd)
                WritePoseGroup(model, bone, 156, 0, 0.0f, 0.0f, 0.0f, 1.0f);
            else if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 156, 0, mdl::Mdl(model)->standardPose.leftKnee[0],
                    mdl::Mdl(model)->standardPose.leftKnee[1], mdl::Mdl(model)->standardPose.leftKnee[2],
                    mdl::Mdl(model)->standardPose.leftKnee[3]);
            else
                WritePoseGroup(model, bone, 156, 0, mdl::Mdl(model)->standardPose.rightKnee[0],
                    -mdl::Mdl(model)->standardPose.rightKnee[1], -mdl::Mdl(model)->standardPose.rightKnee[2],
                    mdl::Mdl(model)->standardPose.rightKnee[3]);
        } else if (std::memcmp(bone, kKneeR, 7) == 0) {           
            if (rightIkAdd)
                WritePoseGroup(model, bone, 172, 0, 0.0f, 0.0f, 0.0f, 1.0f);
            else if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 172, 0, mdl::Mdl(model)->standardPose.rightKnee[0],
                    mdl::Mdl(model)->standardPose.rightKnee[1], mdl::Mdl(model)->standardPose.rightKnee[2],
                    mdl::Mdl(model)->standardPose.rightKnee[3]);
            else
                WritePoseGroup(model, bone, 172, 0, mdl::Mdl(model)->standardPose.leftKnee[0],
                    -mdl::Mdl(model)->standardPose.leftKnee[1], -mdl::Mdl(model)->standardPose.leftKnee[2],
                    mdl::Mdl(model)->standardPose.leftKnee[3]);
        } else if (std::memcmp(bone, kLegIkL, 9) == 0) {
            if (mirrorLeftRight != 0) {
                if ((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::LeftAnkle][1] != -999.0) {
                    const double inv = 1.0 / (double)rinmotiondance::mdl::Mdl(model)->lightDir[1];
                    bone->trans[0] =
                        (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::LeftAnkle][0]);
                    bone->trans[1] =
                        (float)((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::LeftAnkle][1] * inv);
                    bone->trans[2] =
                        (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::LeftAnkle][2]);
                    bone->trans[1] = rinmotiondance::mdl::Mdl(model)->lightDir[2] +
                        bone->trans[1];
                    bone->trans[0] = bone->trans[0] -
                        rinmotiondance::mdl::Mdl(model)->legIkXOffset;
                    bone->rotQuat[3] = mdl::Mdl(model)->standardPose.leftFoot[3];
                    bone->rotQuat[0] = mdl::Mdl(model)->standardPose.leftFoot[0];
                    bone->rotQuat[1] = mdl::Mdl(model)->standardPose.leftFoot[1];
                    bone->rotQuat[2] = mdl::Mdl(model)->standardPose.leftFoot[2];
                } else if (skeletonFlag != 0) {
                    bone->trans[0] = centerPos[0];
                    bone->trans[1] = centerPos[1];
                    bone->trans[2] = centerPos[2];
                }
            } else if ((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][1] != -999.0) {
                const double inv = 1.0 / (double)rinmotiondance::mdl::Mdl(model)->lightDir[1];
                bone->trans[0] =
                    (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][0]);
                bone->trans[1] =
                    (float)((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][1] * inv);
                bone->trans[2] =
                    (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][2]);
                bone->trans[1] = rinmotiondance::mdl::Mdl(model)->lightDir[2] +
                    bone->trans[1];
                bone->trans[0] = rinmotiondance::mdl::Mdl(model)->legIkXOffset +
                    bone->trans[0];
                bone->trans[0] = -bone->trans[0];
                bone->rotQuat[3] = mdl::Mdl(model)->standardPose.rightFoot[3];
                bone->rotQuat[0] = mdl::Mdl(model)->standardPose.rightFoot[0];
                bone->rotQuat[1] = -mdl::Mdl(model)->standardPose.rightFoot[1];
                bone->rotQuat[2] = -mdl::Mdl(model)->standardPose.rightFoot[2];
            } else if (skeletonFlag != 0) {
                bone->trans[0] = centerPos[0];
                bone->trans[1] = centerPos[1];
                bone->trans[2] = centerPos[2];
            }
            if (mdl::PoseTraceFlag(model) != 0) {
                TraceSlot(model, 188) = bone->trans[0];
                TraceSlot(model, 192) = bone->trans[1];
                TraceSlot(model, 196) = bone->trans[2];
                TraceSlot(model, 200) = bone->rotQuat[3];
                TraceSlot(model, 204) = bone->rotQuat[0];
                TraceSlot(model, 208) = bone->rotQuat[1];
                TraceSlot(model, 212) = bone->rotQuat[2];
            }
        } else if (std::memcmp(bone, kLegIkR, 9) == 0) {
            if (mirrorLeftRight != 0) {
                if ((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][1] != -999.0) {
                    const double inv = 1.0 / (double)rinmotiondance::mdl::Mdl(model)->lightDir[1];
                    bone->trans[0] =
                        (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][0]);
                    bone->trans[1] =
                        (float)((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][1] * inv);
                    bone->trans[2] =
                        (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::RightAnkle][2]);
                    bone->trans[1] = rinmotiondance::mdl::Mdl(model)->lightDir[2] +
                        bone->trans[1];
                bone->trans[0] = rinmotiondance::mdl::Mdl(model)->legIkXOffset +
                        bone->trans[0];
                    bone->rotQuat[3] = mdl::Mdl(model)->standardPose.rightFoot[3];
                    bone->rotQuat[0] = mdl::Mdl(model)->standardPose.rightFoot[0];
                    bone->rotQuat[1] = mdl::Mdl(model)->standardPose.rightFoot[1];
                    bone->rotQuat[2] = mdl::Mdl(model)->standardPose.rightFoot[2];
                } else if (skeletonFlag != 0) {
                    bone->trans[0] = centerPos[0];
                    bone->trans[1] = centerPos[1];
                    bone->trans[2] = centerPos[2];
                }
            } else if ((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::LeftAnkle][1] != -999.0) {
                const double inv = 1.0 / (double)rinmotiondance::mdl::Mdl(model)->lightDir[1];
                bone->trans[0] =
                    (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::LeftAnkle][0]);
                bone->trans[1] =
                    (float)((double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::LeftAnkle][1] * inv);
                bone->trans[2] =
                    (float)(inv * (double)mdl::Mdl(model)->currentJoints[mdl::TrackedJoint::LeftAnkle][2]);
                bone->trans[1] = rinmotiondance::mdl::Mdl(model)->lightDir[2] +
                    bone->trans[1];
                bone->trans[0] = bone->trans[0] -
                    rinmotiondance::mdl::Mdl(model)->legIkXOffset;
                bone->trans[0] = -bone->trans[0];
                bone->rotQuat[3] = mdl::Mdl(model)->standardPose.leftFoot[3];
                bone->rotQuat[0] = mdl::Mdl(model)->standardPose.leftFoot[0];
                bone->rotQuat[1] = -mdl::Mdl(model)->standardPose.leftFoot[1];
                bone->rotQuat[2] = -mdl::Mdl(model)->standardPose.leftFoot[2];
            } else if (skeletonFlag != 0) {
                bone->trans[0] = centerPos[0];
                bone->trans[1] = centerPos[1];
                bone->trans[2] = centerPos[2];
            }
            if (mdl::PoseTraceFlag(model) != 0) {
                TraceSlot(model, 216) = bone->trans[0];
                TraceSlot(model, 220) = bone->trans[1];
                TraceSlot(model, 224) = bone->trans[2];
                TraceSlot(model, 228) = bone->rotQuat[3];
                TraceSlot(model, 232) = bone->rotQuat[0];
                TraceSlot(model, 236) = bone->rotQuat[1];
                TraceSlot(model, 240) = bone->rotQuat[2];
            }
        } else if (std::memcmp(bone, kTwistL, 7) == 0) {         
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 244, 0, mdl::Mdl(model)->standardPose.leftWrist[0],
                    mdl::Mdl(model)->standardPose.leftWrist[1], mdl::Mdl(model)->standardPose.leftWrist[2],
                    mdl::Mdl(model)->standardPose.leftWrist[3]);
            else
                WritePoseGroup(model, bone, 244, 0, mdl::Mdl(model)->standardPose.rightWrist[0],
                    -mdl::Mdl(model)->standardPose.rightWrist[1], -mdl::Mdl(model)->standardPose.rightWrist[2],
                    mdl::Mdl(model)->standardPose.rightWrist[3]);
        } else if (std::memcmp(bone, kTwistR, 7) == 0) {         
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 260, 0, mdl::Mdl(model)->standardPose.rightWrist[0],
                    mdl::Mdl(model)->standardPose.rightWrist[1], mdl::Mdl(model)->standardPose.rightWrist[2],
                    mdl::Mdl(model)->standardPose.rightWrist[3]);
            else
                WritePoseGroup(model, bone, 260, 0, mdl::Mdl(model)->standardPose.leftWrist[0],
                    -mdl::Mdl(model)->standardPose.leftWrist[1], -mdl::Mdl(model)->standardPose.leftWrist[2],
                    mdl::Mdl(model)->standardPose.leftWrist[3]);
        } else if (std::memcmp(bone, kShoulderL, 5) == 0) {
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 276, 0, mdl::Mdl(model)->standardPose.leftShoulder[0],
                    mdl::Mdl(model)->standardPose.leftShoulder[1], mdl::Mdl(model)->standardPose.leftShoulder[2],
                    mdl::Mdl(model)->standardPose.leftShoulder[3]);
            else
                WritePoseGroup(model, bone, 276, 0, mdl::Mdl(model)->standardPose.rightShoulder[0],
                    -mdl::Mdl(model)->standardPose.rightShoulder[1], -mdl::Mdl(model)->standardPose.rightShoulder[2],
                    mdl::Mdl(model)->standardPose.rightShoulder[3]);
        } else if (std::memcmp(bone, kShoulderR, 5) == 0) {
            if (mirrorLeftRight != 0)
                WritePoseGroup(model, bone, 292, 0, mdl::Mdl(model)->standardPose.rightShoulder[0],
                    mdl::Mdl(model)->standardPose.rightShoulder[1], mdl::Mdl(model)->standardPose.rightShoulder[2],
                    mdl::Mdl(model)->standardPose.rightShoulder[3]);
            else
                WritePoseGroup(model, bone, 292, 0, mdl::Mdl(model)->standardPose.leftShoulder[0],
                    -mdl::Mdl(model)->standardPose.leftShoulder[1], -mdl::Mdl(model)->standardPose.leftShoulder[2],
                    mdl::Mdl(model)->standardPose.leftShoulder[3]);
        }
    }
    if (mdl::PoseTraceFlag(model) != 0)
        ++rinmotiondance::mdl::Mdl(model)->matMisc;                        
    return wasRecording;
}

}  
