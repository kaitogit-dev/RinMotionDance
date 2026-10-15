#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/wave_audio_context.hpp"
#include "rinmotiondance/panel_controls.hpp"

#include "../model/keyframe_common.hpp"

namespace rinmotiondance {
namespace {

using kfa::Rd32;
using kfa::RdI32;
using kfa::RdF32;
using kfa::Wr32;
using kfa::WrF32;

using FnQuatInverseLocal = float*(WINAPI*)(float out[4], const float q[4]);
using FnQuatRotationAxisLocal =
    float*(WINAPI*)(float out[4], const float axis[3], float angle);

FnQuatInverseLocal LocalQuatInverse() { return d3dx::Api::quatInverse; }
FnQuatRotationAxisLocal LocalQuatRotationAxis() { return d3dx::Api::quatRotationAxis; }

constexpr float kPiFloat = 3.141592f;      
static_assert(__builtin_bit_cast(std::uint32_t, kPiFloat) == 0x40490FD8u,
              "flt_52B738 bit-exact");
constexpr float k35deg5311A4 = 0.61086512f; 
constexpr float k30deg53119C = 0.52359867f; 
constexpr float kNeg0d7 = -0.69999999f;     
constexpr float kNeg0d3 = -0.30000001f;     
constexpr float k75f = 75.0f;               
constexpr float kSentinel = -999.0;         

const char kJpOverflow[] =
    "\x93\x6f\x98\x5e\x83\x7c\x83\x43\x83\x93\x83\x67\x90\x94\x82\xaa%d"
    "\x8c\xc2\x82\xf0\x89\x7a\x82\xa6\x82\xdc\x82\xb5\x82\xbd\n"
    "\x82\xb1\x82\xea\x88\xc8\x8f\xe3\x82\xcc\x93\x6f\x98\x5e\x82\xcd\x8d"
    "\x73\x82\xa6\x82\xdc\x82\xb9\x82\xf1\n"
    "\x81\x75\xcc\xda\xb0\xd1\x95\xd2\x8f\x57\x81\x76\x82\xcc\x81\x75\x95"
    "\x73\x97\x70\xcc\xda\xb0\xd1\x8d\xed\x8f\x9c\x81\x76\x82\xf0\x8e\xc0"
    "\x8d\x73\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2";
const char kJpFrameRegTitle[] = "\xcc\xda\xb0\xd1\x93\x6f\x98\x5e";

void OverflowBoxBoneKey(unsigned char* m) {
    char text[256];
    if (rinmotiondance::mdl::Mdl(m)->physicsFlags != 0)
        sprintf_s(text, 0x100,
                  "You cannot regist over %d point\n"
                  "Please execute 'delete unused frame'",
                  static_cast<int>(mdl::kBoneKeyCapacity));
    else
        sprintf_s(text, 0x100, kJpOverflow,
                  static_cast<int>(mdl::kBoneKeyCapacity));
    MessageBoxA(*reinterpret_cast<HWND*>(m), text,
                rinmotiondance::mdl::Mdl(m)->physicsFlags != 0
                    ? "register frame"
                    : kJpFrameRegTitle,
                0);
}

void WriteBoneKeyRecord(mdl::BoneKey& rec, const float* p, int mode) {
    if (mode == 0) {
        rec.position[0] = p[0]; rec.position[1] = p[1];
        rec.position[2] = p[2];
        rec.rotation[0] = rec.rotation[1] = rec.rotation[2] = 0.0f;
        rec.rotation[3] = 1.0f;
    } else if (mode == 1) {
        rec.position[0] = rec.position[1] = rec.position[2] = 0.0f;
        rec.rotation[3] = p[0]; rec.rotation[0] = p[1];
        rec.rotation[1] = p[2]; rec.rotation[2] = p[3];
    } else if (mode == 2) {
        rec.position[0] = p[0]; rec.position[1] = p[1];
        rec.position[2] = p[2];
        rec.rotation[3] = p[3]; rec.rotation[0] = p[4];
        rec.rotation[1] = p[5]; rec.rotation[2] = p[6];
    } else {
        return;
    }
    for (int g = 0; g < 4; ++g) {
        rec.interpolation[4 * g + 0] = 20;
        rec.interpolation[4 * g + 1] = 20;
        rec.interpolation[4 * g + 2] = 107;
        rec.interpolation[4 * g + 3] = 107;
    }
    rec.allocated = 1;
}

const unsigned char kNameCenter[] =     
    {0x83, 0x5A, 0x83, 0x93, 0x83, 0x5E, 0x81, 0x5B, 0x00};
const unsigned char kNameUpper[] =      
    {0x8F, 0xE3, 0x94, 0xBC, 0x90, 0x67, 0x00};
const unsigned char kNameNeck[] =      
    {0x8E, 0xF1, 0x00};
const unsigned char kNameLArm[] =  
    {0x8D, 0xB6, 0x98, 0x72, 0x00};
const unsigned char kNameLElbow[] =     
    {0x8D, 0xB6, 0x82, 0xD0, 0x82, 0xB6, 0x00};
const unsigned char kNameRArm[] =  
    {0x89, 0x45, 0x98, 0x72, 0x00};
const unsigned char kNameRElbow[] =     
    {0x89, 0x45, 0x82, 0xD0, 0x82, 0xB6, 0x00};
const unsigned char kNameLower[] =      
    {0x89, 0xBA, 0x94, 0xBC, 0x90, 0x67, 0x00};
const unsigned char kNameLLeg[] =      
    {0x8D, 0xB6, 0x91, 0xAB, 0x00};
const unsigned char kNameRLeg[] =      
    {0x89, 0x45, 0x91, 0xAB, 0x00};
const unsigned char kNameLLegIk[] =     
    {0x8D, 0xB6, 0x91, 0xAB, 0x82, 0x68, 0x82, 0x6A, 0x00};
const unsigned char kNameRLegIk[] =     
    {0x89, 0x45, 0x91, 0xAB, 0x82, 0x68, 0x82, 0x6A, 0x00};
const unsigned char kNameLKnee[] =      
    {0x8D, 0xB6, 0x82, 0xD0, 0x82, 0xB4, 0x00};
const unsigned char kNameRKnee[] =      
    {0x89, 0x45, 0x82, 0xD0, 0x82, 0xB4, 0x00};
const unsigned char kNameLWrist[] =  
    {0x8D, 0xB6, 0x8E, 0xE8, 0x8E, 0xF1, 0x00};
const unsigned char kNameRWrist[] =  
    {0x89, 0x45, 0x8E, 0xE8, 0x8E, 0xF1, 0x00};
const unsigned char kNameLShoulder[] =      
    {0x8D, 0xB6, 0x8C, 0xA8, 0x00};
const unsigned char kNameRShoulder[] =      
    {0x89, 0x45, 0x8C, 0xA8, 0x00};

int FindBoneByName(unsigned char* m, const void* name, std::size_t cb) {
    const int cnt = rinmotiondance::mdl::Mdl(m)->boneCount;
    if (cnt <= 0) return -1;
    rinmotiondance::mdl::BoneRecord* const bones = rinmotiondance::mdl::Bones(m);
    for (int i = 0; i < cnt; ++i)
        if (std::memcmp(name, bones[i].name, cb) == 0) return i;
    return -1;  
}

}  

void* ClearWaveContextFields(void* obj) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    audio->path[0] = L'\0';                              
    audio->englishUI = 0;                                
    audio->waveformMax = nullptr;                        
    audio->waveformMin = nullptr;                        
    audio->volume = 0;                                   
    return audio;                                        
}

bool RegisterPhysicsPoseChain(unsigned char* m, int boneIdx, int srcIdx,
                              int mode, std::uint32_t startFrame) {
    mdl::BoneKey* const keys = mdl::BoneKeys(m);

    int cur = boneIdx;                                            
    if (keys[cur].frame < startFrame) {
        for (;;) {
            const int nxt = static_cast<int>(keys[cur].next);
            if (nxt == 0) break;                                  
            if (keys[nxt].frame >= startFrame) break;
            cur = nxt;                                            
        }
    }

    int next = static_cast<int>(keys[cur].next);
    int scan = rinmotiondance::mdl::Mdl(m)->boneCount;                  
    int cand = scan;                                              
    if (keys[scan].frame != 0) {
        do {
            ++scan;
            if (scan >= static_cast<int>(mdl::kBoneKeyCapacity)) {  
                OverflowBoxBoneKey(m);                            
                return false;
            }
        } while (keys[scan].frame != 0);
        cand = scan;                                              
    }

    if (rinmotiondance::mdl::Mdl(m)->matMisc == 0) return true;          

    std::uint32_t frame = startFrame;                             
    const float* p = static_cast<const float*>(
                          rinmotiondance::mdl::PoseTraceBuffer(m)) +
                     srcIdx;                                      
    unsigned int iter = 0;                                        
    for (;;) {
        AppendBoneKeyToUndo(m, cur);                                        
        AppendBoneKeyToUndo(m, next);                                       
        AppendBoneKeyToUndo(m, scan);                                       

        if (keys[cur].frame == frame) {
            keys[cur].next = 0;
            WriteBoneKeyRecord(keys[cur], p, mode);
        } else {
            if (keys[next].frame == frame) {
                keys[cur].next = static_cast<std::uint32_t>(next);
                keys[next].previous = static_cast<std::uint32_t>(cur);
                WriteBoneKeyRecord(keys[next], p, mode);
                cur = next;                                       
                scan = cand;                                      
                next = static_cast<int>(keys[cur].next);
            } else {
                keys[cur].next = static_cast<std::uint32_t>(cand);
                keys[cand].previous = static_cast<std::uint32_t>(cur);
                keys[cand].frame = frame;
                keys[cand].next = 0;
                WriteBoneKeyRecord(keys[cand], p, mode);
                cur = cand;                                       
                scan = cand + 1;                                  
                cand = scan;
                if (keys[scan].frame != 0) {
                    for (;;) {
                        ++scan;
                        cand = scan;                              
                        if (scan >=
                            static_cast<int>(mdl::kBoneKeyCapacity))
                            break;                   
                        if (keys[scan].frame == 0) break;
                    }
                    if (scan >= static_cast<int>(mdl::kBoneKeyCapacity)) {
                        OverflowBoxBoneKey(m);                   
                        return false;                             
                    }
                }
            }
        }
        if (frame > rinmotiondance::mdl::Mdl(m)->maxFrame)
            rinmotiondance::mdl::Mdl(m)->maxFrame = frame;
        if (next != 0) {                                           
            keys[cur].next = static_cast<std::uint32_t>(next);
            keys[next].previous = static_cast<std::uint32_t>(cur);
        }
        ++frame;                                                   
        ++iter;
        p += 77;                                                   
        if (iter >= static_cast<unsigned int>(
                        rinmotiondance::mdl::Mdl(m)->matMisc))
            return true;                                          
    }
}

void RegisterPhysicsPose(unsigned char* m, std::uint32_t frame) {
    mdl::ModelRecord& model = *mdl::Mdl(m);
    mdl::BoneKey* const boneKeys = mdl::BoneKeys(m);
    for (int i = 0; i < static_cast<int>(mdl::kBoneKeyCapacity); ++i)
        boneKeys[i].allocated = 0;
    mdl::MorphKey* const morphKeys = mdl::MorphKeys(m);
    for (int i = 0; i < 20000; ++i) morphKeys[i].allocated = 0;
    mdl::DisplayKey* const masterKeys = mdl::DisplayKeys(m);
    for (int i = 0; i < 1000; ++i) masterKeys[i].allocated = 0;

    const auto cleanup = [&]() {                                   
        if (rinmotiondance::mdl::PoseTraceBuffer(m) != nullptr) {
            ::operator delete(rinmotiondance::mdl::PoseTraceBuffer(m));
            rinmotiondance::mdl::PoseTraceBuffer(m) = nullptr;
        }
        mdl::Mdl(m)->matMisc = 0;
    };

    if (model.matMisc == 0) {                                    
        cleanup();
        return;
    }

    HWND hwnd = static_cast<HWND>(model.hwnd);
    EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), 1);                        
    EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), 0);                        

    if (++model.undoState[0] >= 30)
        model.undoState[0] = 0;                                   
    const int slot = model.undoState[0];
    model.undoState[1] = slot;                                    
    model.undoDirty = 1;                                          
    model.redoDirty = 0;                                          

    auto& undo = rinmotiondance::mdl::Mdl(m)->undoRings[0].slots[slot];
    undo.operation = 2;                                            
    undo.frame = frame;                                             

    const int boneCnt = model.boneCount;
    auto*& pose = undo.bonePose;
    if (pose != nullptr) {                                         
        ::operator delete(pose);
        pose = nullptr;
    }
    pose = static_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
        ::operator new(sizeof(*pose) * boneCnt));                    
    std::memset(pose, 0, sizeof(*pose) * static_cast<std::size_t>(boneCnt));
    if (boneCnt > 0) {                                             
        rinmotiondance::mdl::BoneRecord* const bones = model.boneTable;
        const unsigned char* flags = model.bonePhysicsState;
        for (int b = 0; b < boneCnt; ++b) {
            auto& rec = pose[b];
            const rinmotiondance::mdl::BoneRecord* bone = &bones[b];
            rec.boneIndex = b;                                    
            std::memcpy(rec.position, bone->trans, sizeof(rec.position));
            std::memcpy(rec.rotation, bone->rotQuat, sizeof(rec.rotation));
            rec.physicsDisabled = flags[b];                        
        }
    }
    undo.dirty = 0;                                                 

    auto*& poseSrc = undo.auxiliaryPose;
    if (poseSrc != nullptr) {                                      
        ::operator delete(poseSrc);
        poseSrc = nullptr;
    }
    poseSrc = static_cast<unsigned char*>(
        ::operator new(3456 * static_cast<std::size_t>(model.matMisc))); 
    std::memset(poseSrc, 0,
                3456 * static_cast<std::size_t>(model.matMisc));   
    std::memset(mdl::Mdl(m)->keyVisitMap, 0,
                sizeof(mdl::Mdl(m)->keyVisitMap));               

    int i;
    i = FindBoneByName(m, kNameCenter, 9);                         
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 0, 0, frame)) return;          
    i = FindBoneByName(m, kNameUpper, 7);                          
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 3, 1, frame)) return;          
    if (model.openniVersion >= 14) {                               
        i = FindBoneByName(m, kNameNeck, 3);                      
        if (i != -1 && !RegisterPhysicsPoseChain(m, i, 7, 1, frame)) return;      
    }
    i = FindBoneByName(m, kNameLArm, 5);                      
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 11, 1, frame)) return;         
    i = FindBoneByName(m, kNameLElbow, 7);                         
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 15, 1, frame)) return;         
    i = FindBoneByName(m, kNameRArm, 5);                      
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 19, 1, frame)) return;         
    i = FindBoneByName(m, kNameRElbow, 7);                         
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 23, 1, frame)) return;         
    i = FindBoneByName(m, kNameLower, 7);                          
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 27, 1, frame)) return;         
    i = FindBoneByName(m, kNameLLeg, 5);                          
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 31, 1, frame)) return;         
    i = FindBoneByName(m, kNameRLeg, 5);                          
    if (i != -1 && !RegisterPhysicsPoseChain(m, i, 35, 1, frame)) return;         

    {
        const int ikCnt = model.ikChainCount;
        const mdl::IkChain* const chains = model.ikChains;
        rinmotiondance::mdl::BoneRecord* const bones = model.boneTable;
        int chain = -1;
        for (int c = 0; c < ikCnt; ++c) {                          
            const rinmotiondance::mdl::BoneRecord* root = &bones[chains[c].boneIndex];
            if (std::memcmp(kNameLLegIk, root->name, 9) == 0) { chain = c; break; }
        }
        if (chain >= 0) {
            if (chains[chain].enabled != 0) {                      
                i = FindBoneByName(m, kNameLLegIk, 9);             
                if (i != -1 && !RegisterPhysicsPoseChain(m, i, 47, 2, frame)) return; 
            } else {
                i = FindBoneByName(m, kNameLKnee, 7);              
                if (i != -1 && !RegisterPhysicsPoseChain(m, i, 39, 1, frame)) return; 
            }
        }
    }
    {
        const int ikCnt = model.ikChainCount;
        const mdl::IkChain* const chains = model.ikChains;
        rinmotiondance::mdl::BoneRecord* const bones = model.boneTable;
        int chain = -1;
        for (int c = 0; c < ikCnt; ++c) {                          
            const rinmotiondance::mdl::BoneRecord* root = &bones[chains[c].boneIndex];
            if (std::memcmp(kNameRLegIk, root->name, 9) == 0) { chain = c; break; }
        }
        if (chain >= 0) {
            if (chains[chain].enabled != 0) {                      
                i = FindBoneByName(m, kNameRLegIk, 9);             
                if (i != -1 && !RegisterPhysicsPoseChain(m, i, 54, 2, frame)) return; 
            } else {
                i = FindBoneByName(m, kNameRKnee, 7);              
                if (i != -1 && !RegisterPhysicsPoseChain(m, i, 43, 1, frame)) return; 
            }
        }
    }

    if (model.openniVersion >= 14) {                               
        i = FindBoneByName(m, kNameLWrist, 7);                  
        if (i != -1 && !RegisterPhysicsPoseChain(m, i, 61, 1, frame)) return;     
        if (model.openniVersion >= 14) {                           
            i = FindBoneByName(m, kNameRWrist, 7);              
            if (i != -1 && !RegisterPhysicsPoseChain(m, i, 65, 1, frame)) return; 
        }
    }
    if (model.openniVersion >= 15) {                               
        i = FindBoneByName(m, kNameLShoulder, 5);                      
        if (i != -1 && !RegisterPhysicsPoseChain(m, i, 69, 1, frame))             
            return;
        if (model.openniVersion >= 15) {                           
            i = FindBoneByName(m, kNameRShoulder, 5);                  
            if (i != -1 && !RegisterPhysicsPoseChain(m, i, 73, 1, frame))         
                return;
        }
    }
    cleanup();                                                     
}

float* BuildLookAtQuaternion(float out[4], const float quat[4], float ax,
                             float ay, float az, float bx, float by, float bz,
                             unsigned char mode) {
    auto* d3 = &d3dx::Get();
    FnQuatInverseLocal quatInverse = LocalQuatInverse();

    float qInv[4];
    quatInverse(qInv, quat);                                       
    d3dx::D3DXMATRIXF m1;
    d3->matrixRotationQuaternion(&m1, qInv);                       
    d3dx::D3DXMATRIXF m2;
    d3->rotY(&m2, kPiFloat);                                      
    d3dx::D3DXMATRIXF m3;
    d3->multiply(&m3, &m1, &m2);                                   

    float dir[3] = {bx - ax, by - ay, bz - az};                    
    float t[4];
    d3->vec3Transform(t, dir, &m3);                                

    float n[3] = {t[0], mode == 5 ? 0.0f : t[1], t[2]};            
    d3->vec3Normalize(n, n);                                       

    const float len = sqrtf(n[2] * n[2] + n[0] * n[0]);            
    float c;   
    float s;   
    float ang = 0.0f;  

    if (mode == 5) {
        c = -n[2] / len;                                           
        s = n[0] / len;                                            
    } else {
        c = n[0] / len;                                            
        s = n[2] / len;                                            
    }
    if (mode == 1) {                                           
        const float a = acosf(c);                              
        ang = s < 0.0f ? -a : a;   
    } else if (mode >= 2) {        
        const float ang0 = acosf(c);                               
        float r;
        if (s < 0.0f) {
            r = !(c < 0.0f) ? -acosf(-s) : acosf(-s);              
            ang = -ang0 - r * len;                                 
        } else {
            r = !(c < 0.0f) ? -acosf(s) : acosf(s);                
            ang = ang0 - r * len;                                  
        }
    }
    {
        d3dx::D3DXMATRIXF& a = m1;  
        std::memset(&a, 0, sizeof(a));
        a.m[0][0] = c;                                             
        a.m[0][2] = -s;                                            
        a.m[2][0] = s;                                             
        a.m[2][2] = c;                                             
        a.m[1][1] = 1.0f;                                          
        a.m[3][3] = 1.0f;                                          
    }
    float e1, e2;   
    if (mode != 0) {                                               
        e1 = -n[1];                                                
        e2 = len;                                                  
    } else {
        e1 = len;                                                  
        e2 = n[1];                                                 
    }
    {
        d3dx::D3DXMATRIXF& b = m2;  
        std::memset(&b, 0, sizeof(b));
        b.m[0][0] = e1;                                            
        b.m[0][1] = -e2;                                           
        b.m[1][0] = e2;                                            
        b.m[1][1] = e1;                                            
        b.m[2][2] = 1.0f;                                          
        b.m[3][3] = 1.0f;                                          
    }
    d3dx::D3DXMATRIXF r;   
    if (mode < 4) {                                                
        d3->multiply(&m3, &m1, &m2);                               
        std::memcpy(&r, &m3, sizeof(r));                           
    } else {
        std::memcpy(&r, &m1, sizeof(r));                           
    }
    if (mode == 1 || mode == 2 || mode == 3) {                     
        d3->rotY(&m1, -ang);                                       
        d3->multiply(&m3, &r, &m1);                                
        std::memcpy(&r, &m3, sizeof(r));                           
    }
    d3->quatFromMatrix(out, &r);                                   
    out[1] = -out[1];                                              
    return out;                                                    
}

void InitStandardSkeletonQuats(unsigned char* m, unsigned char flag) {
    auto* d3 = &d3dx::Get();
    FnQuatRotationAxisLocal quatRotationAxis = LocalQuatRotationAxis();

    auto& state = *mdl::Mdl(m);
    auto& joints = state.currentJoints;
    auto& pose = state.standardPose;
    const auto normalizeComponents = [d3](float& x, float& y, float& z) {
        float value[3] = {x, y, z};
        d3->vec3Normalize(value, value);
        x = value[0]; y = value[1]; z = value[2];
    };
    float qByVal[4];  
    const float kAxisZ[3] = {0.0f, 0.0f, 1.0f};

    float qA[4], qB[4], t1[4], t2[4];

    float q150[4] = {}, q155[4] = {}, q159[4] = {};

    if (state.lightDir[0] == -1.0f) {
        if (joints[mdl::TrackedJoint::RightKnee][1] != kSentinel && joints[mdl::TrackedJoint::RightAnkle][1] != kSentinel &&
            joints[mdl::TrackedJoint::RightHip][1] != kSentinel) {
            const float d1x = joints[mdl::TrackedJoint::RightAnkle][0] - joints[mdl::TrackedJoint::RightKnee][0];
            const float d1y = joints[mdl::TrackedJoint::RightAnkle][1] - joints[mdl::TrackedJoint::RightKnee][1];
            const float d1z = joints[mdl::TrackedJoint::RightAnkle][2] - joints[mdl::TrackedJoint::RightKnee][2];
            const float d2x = joints[mdl::TrackedJoint::RightKnee][0] - joints[mdl::TrackedJoint::RightHip][0];
            const float d2y = joints[mdl::TrackedJoint::RightKnee][1] - joints[mdl::TrackedJoint::RightHip][1];
            const float d2z = joints[mdl::TrackedJoint::RightKnee][2] - joints[mdl::TrackedJoint::RightHip][2];
            const float len1 = sqrtf(d1y * d1y + d1x * d1x + d1z * d1z); 
            const float len2 = sqrtf(d2x * d2x + d2y * d2y + d2z * d2z); 
            state.lightDir[0] = len2 + len1;                         
        } else if (joints[mdl::TrackedJoint::RightKnee][1] != kSentinel && joints[mdl::TrackedJoint::LeftAnkle][1] != kSentinel &&
                   joints[mdl::TrackedJoint::LeftHip][1] != kSentinel) {
            const float d1x = joints[mdl::TrackedJoint::LeftAnkle][0] - joints[mdl::TrackedJoint::LeftKnee][0];
            const float d1y = joints[mdl::TrackedJoint::LeftAnkle][1] - joints[mdl::TrackedJoint::LeftKnee][1];
            const float d1z = joints[mdl::TrackedJoint::LeftAnkle][2] - joints[mdl::TrackedJoint::LeftKnee][2];
            const float d2x = joints[mdl::TrackedJoint::LeftKnee][0] - joints[mdl::TrackedJoint::LeftHip][0];
            const float d2y = joints[mdl::TrackedJoint::LeftKnee][1] - joints[mdl::TrackedJoint::LeftHip][1];
            const float d2z = joints[mdl::TrackedJoint::LeftKnee][2] - joints[mdl::TrackedJoint::LeftHip][2];
            const float lenA = sqrtf(d1y * d1y + d1x * d1x + d1z * d1z); 
            const float lenB = sqrtf(d2x * d2x + d2y * d2y + d2z * d2z); 
            state.lightDir[0] = lenB + lenA;                         
        }
    }

    if (joints[mdl::TrackedJoint::Torso][1] != kSentinel && joints[mdl::TrackedJoint::RightShoulder][1] != kSentinel &&
        joints[mdl::TrackedJoint::LeftShoulder][1] != kSentinel && joints[mdl::TrackedJoint::Neck][1] != kSentinel) {
        float dx = joints[mdl::TrackedJoint::Neck][0] - joints[mdl::TrackedJoint::Torso][0];                            
        float dy = joints[mdl::TrackedJoint::Neck][1] - joints[mdl::TrackedJoint::Torso][1];                            
        float dz = joints[mdl::TrackedJoint::Neck][2] - joints[mdl::TrackedJoint::Torso][2];                            
        normalizeComponents(dx, dy, dz);                               
        float cx = dz * 1.0f - dy * 0.0f;                          
        float cy = dx * 0.0f - dz * 0.0f;                          
        float cz = dy * 0.0f - dx * 1.0f;                          
        const float dot =
            1.0f * dy + dx * 0.0f + 0.0f * dz;                     
        float axis[3] = {cx, cy, cz};
        d3->vec3Normalize(axis, axis);                             
        const float ang = acosf(dot);                              
        quatRotationAxis(qA, axis, ang);                           
        std::memcpy(qByVal, pose.upperBody, sizeof qByVal);
        BuildLookAtQuaternion(q150, qByVal, joints[mdl::TrackedJoint::LeftShoulder][0], joints[mdl::TrackedJoint::LeftShoulder][1], joints[mdl::TrackedJoint::LeftShoulder][2],
                  joints[mdl::TrackedJoint::RightShoulder][0], joints[mdl::TrackedJoint::RightShoulder][1], joints[mdl::TrackedJoint::RightShoulder][2], 4);                
        std::memcpy(pose.upperBody, q150, 16);                           
        d3->quatMultiply(t1, pose.upperBody, qA);                        
        std::memcpy(pose.upperBody, t1, 16);                             
    } else if (flag != 0) {
        pose.upperBody[3] = 1.0f;                                       
        pose.upperBody[0] = 0.0f;                                       
        pose.upperBody[1] = 0.0f;                                       
        pose.upperBody[2] = 0.0f;                                       
    }

    if (joints[mdl::TrackedJoint::Center][1] != kSentinel && joints[mdl::TrackedJoint::Head][1] != kSentinel &&
        joints[mdl::TrackedJoint::Neck][1] != kSentinel) {
        std::memcpy(qByVal, pose.upperBody, sizeof qByVal);
        BuildLookAtQuaternion(t1, qByVal, joints[mdl::TrackedJoint::Head][0], joints[mdl::TrackedJoint::Head][1], joints[mdl::TrackedJoint::Head][2],
                  joints[mdl::TrackedJoint::Neck][0], joints[mdl::TrackedJoint::Neck][1], joints[mdl::TrackedJoint::Neck][2], 1);                
        std::memcpy(pose.neck, t1, 16);                             
        if (joints[mdl::TrackedJoint::HeadDirection][1] != kSentinel) {                               
            d3->quatMultiply(qB, pose.neck, pose.upperBody);              
            BuildLookAtQuaternion(qA, qB, joints[mdl::TrackedJoint::HeadDirection][0], joints[mdl::TrackedJoint::HeadDirection][1], joints[mdl::TrackedJoint::HeadDirection][2],
                      joints[mdl::TrackedJoint::Head][0], joints[mdl::TrackedJoint::Head][1], joints[mdl::TrackedJoint::Head][2], 5);            
            d3->quatMultiply(t1, qA, pose.neck);                    
            std::memcpy(pose.neck, t1, 16);                         
        }
    } else {
        pose.neck[3] = 1.0f;                                       
        pose.neck[0] = 0.0f;                                       
        pose.neck[1] = 0.0f;                                       
        pose.neck[2] = 0.0f;                                       
    }

    if (joints[mdl::TrackedJoint::LeftShoulder][1] != kSentinel && joints[mdl::TrackedJoint::RightClavicle][1] != kSentinel &&
        joints[mdl::TrackedJoint::RightShoulder][1] != kSentinel) {
        const float mx = (joints[mdl::TrackedJoint::RightShoulder][0] + joints[mdl::TrackedJoint::LeftShoulder][0]) * 0.5f;             
        const float my = (joints[mdl::TrackedJoint::LeftShoulder][1] + joints[mdl::TrackedJoint::RightShoulder][1]) * 0.5f;             
        const float mz = (joints[mdl::TrackedJoint::LeftShoulder][2] + joints[mdl::TrackedJoint::RightShoulder][2]) * 0.5f;             
        float ax = joints[mdl::TrackedJoint::RightShoulder][0] - mx, ay = joints[mdl::TrackedJoint::RightShoulder][1] - my,
              az = joints[mdl::TrackedJoint::RightShoulder][2] - mz;                                  
        float bx = joints[mdl::TrackedJoint::RightClavicle][0] - mx, by = joints[mdl::TrackedJoint::RightClavicle][1] - my,
              bz = joints[mdl::TrackedJoint::RightClavicle][2] - mz;                                  
        normalizeComponents(ax, ay, az);                               
        normalizeComponents(bx, by, bz);                               
        float cx = bz * ay - by * az;                              
        float cy = bx * az - bz * ax;                              
        float cz = by * ax - bx * ay;                              
        const float dot = ay * by + bx * ax + az * bz;             
        float axis[3] = {cx, cy, cz};
        d3->vec3Normalize(axis, axis);                             
        quatRotationAxis(pose.rightShoulder, axis, acosf(dot));             
    } else {
        pose.rightShoulder[3] = 1.0f;                                      
        pose.rightShoulder[0] = 0.0f;                                      
        pose.rightShoulder[1] = 0.0f;                                      
        pose.rightShoulder[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::RightShoulder][1] != kSentinel && joints[mdl::TrackedJoint::RightElbow][1] != kSentinel &&
        joints[mdl::TrackedJoint::Neck][1] != kSentinel) {
        quatRotationAxis(qA, kAxisZ, -k35deg5311A4);               
        if (joints[mdl::TrackedJoint::RightClavicle][1] == kSentinel) {                               
            std::memcpy(qByVal, pose.upperBody, sizeof qByVal);
            BuildLookAtQuaternion(q150, qByVal, joints[mdl::TrackedJoint::RightShoulder][0], joints[mdl::TrackedJoint::RightShoulder][1], joints[mdl::TrackedJoint::RightShoulder][2],
                      joints[mdl::TrackedJoint::RightElbow][0], joints[mdl::TrackedJoint::RightElbow][1], joints[mdl::TrackedJoint::RightElbow][2], 0);            
        } else {
            d3->quatMultiply(qB, pose.rightShoulder, pose.upperBody);             
            BuildLookAtQuaternion(q150, qB, joints[mdl::TrackedJoint::RightClavicle][0], joints[mdl::TrackedJoint::RightClavicle][1], joints[mdl::TrackedJoint::RightClavicle][2],
                      joints[mdl::TrackedJoint::RightElbow][0], joints[mdl::TrackedJoint::RightElbow][1], joints[mdl::TrackedJoint::RightElbow][2], 0);            
        }
        d3->quatMultiply(pose.rightArm, qA, q150);                     
    } else if (flag != 0) {                                        
        quatRotationAxis(qA, kAxisZ, k35deg5311A4);                
        std::memcpy(pose.rightArm, qA, 16);                            
    }

    if (joints[mdl::TrackedJoint::RightElbow][1] != kSentinel && joints[mdl::TrackedJoint::RightWrist][1] != kSentinel &&
        joints[mdl::TrackedJoint::RightShoulder][1] != kSentinel) {
        quatRotationAxis(qA, kAxisZ, -k30deg53119C);               
        d3->quatMultiply(t1, q150, pose.rightShoulder);                     
        d3->quatMultiply(t2, t1, pose.upperBody);                        
        BuildLookAtQuaternion(q155, t2, joints[mdl::TrackedJoint::RightElbow][0], joints[mdl::TrackedJoint::RightElbow][1], joints[mdl::TrackedJoint::RightElbow][2],
                  joints[mdl::TrackedJoint::RightWrist][0], joints[mdl::TrackedJoint::RightWrist][1], joints[mdl::TrackedJoint::RightWrist][2], 0);                
        d3->quatMultiply(pose.rightElbow, qA, q155);                     
        quatRotationAxis(qA, kAxisZ, k30deg53119C);                
        d3->quatMultiply(pose.rightElbow, pose.rightElbow, qA);                
    } else if (flag != 0) {                                        
        pose.rightElbow[3] = 1.0f;                                      
        pose.rightElbow[0] = 0.0f;                                      
        pose.rightElbow[1] = 0.0f;                                      
        pose.rightElbow[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::RightWrist][1] != kSentinel && joints[mdl::TrackedJoint::RightHand][1] != kSentinel &&
        joints[mdl::TrackedJoint::RightElbow][1] != kSentinel) {
        if (joints[mdl::TrackedJoint::RightHand][0] == joints[mdl::TrackedJoint::RightWrist][0] && joints[mdl::TrackedJoint::RightHand][1] == joints[mdl::TrackedJoint::RightWrist][1] &&
            joints[mdl::TrackedJoint::RightHand][2] == joints[mdl::TrackedJoint::RightWrist][2]) {                                
            pose.rightWrist[3] = 1.0f;                                  
            pose.rightWrist[0] = 0.0f;                                  
            pose.rightWrist[1] = 0.0f;                                  
            pose.rightWrist[2] = 0.0f;                                  
        } else {
            quatRotationAxis(qA, kAxisZ, -k30deg53119C);           
            d3->quatMultiply(t1, q155, q150);                      
            d3->quatMultiply(t2, t1, pose.rightShoulder);                   
            d3->quatMultiply(qB, t2, pose.upperBody);                    
            BuildLookAtQuaternion(q159, qB, joints[mdl::TrackedJoint::RightWrist][0], joints[mdl::TrackedJoint::RightWrist][1], joints[mdl::TrackedJoint::RightWrist][2],
                      joints[mdl::TrackedJoint::RightHand][0], joints[mdl::TrackedJoint::RightHand][1], joints[mdl::TrackedJoint::RightHand][2], 0);            
            d3->quatMultiply(pose.rightWrist, qA, q159);                 
            quatRotationAxis(qA, kAxisZ, k30deg53119C);            
            d3->quatMultiply(pose.rightWrist, pose.rightWrist, qA);            
        }
    } else if (flag != 0) {                                        
        pose.rightWrist[3] = 1.0f;                                      
        pose.rightWrist[0] = 0.0f;                                      
        pose.rightWrist[1] = 0.0f;                                      
        pose.rightWrist[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::LeftShoulder][1] != kSentinel && joints[mdl::TrackedJoint::LeftClavicle][1] != kSentinel &&
        joints[mdl::TrackedJoint::RightShoulder][1] != kSentinel) {
        const float mx = (joints[mdl::TrackedJoint::RightShoulder][0] + joints[mdl::TrackedJoint::LeftShoulder][0]) * 0.5f;             
        const float my = (joints[mdl::TrackedJoint::LeftShoulder][1] + joints[mdl::TrackedJoint::RightShoulder][1]) * 0.5f;             
        const float mz = (joints[mdl::TrackedJoint::LeftShoulder][2] + joints[mdl::TrackedJoint::RightShoulder][2]) * 0.5f;             
        float ax = joints[mdl::TrackedJoint::LeftShoulder][0] - mx, ay = joints[mdl::TrackedJoint::LeftShoulder][1] - my,
              az = joints[mdl::TrackedJoint::LeftShoulder][2] - mz;                                  
        float bx = joints[mdl::TrackedJoint::LeftClavicle][0] - mx, by = joints[mdl::TrackedJoint::LeftClavicle][1] - my,
              bz = joints[mdl::TrackedJoint::LeftClavicle][2] - mz;                                  
        normalizeComponents(ax, ay, az);                               
        normalizeComponents(bx, by, bz);                               
        float cx = bz * ay - by * az;                              
        float cy = bx * az - bz * ax;                              
        float cz = by * ax - bx * ay;                              
        const float dot = ay * by + bx * ax + az * bz;             
        float axis[3] = {cx, cy, cz};
        d3->vec3Normalize(axis, axis);                             
        quatRotationAxis(pose.leftShoulder, axis, acosf(dot));             
    } else {
        pose.leftShoulder[3] = 1.0f;                                      
        pose.leftShoulder[0] = 0.0f;                                      
        pose.leftShoulder[1] = 0.0f;                                      
        pose.leftShoulder[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::LeftShoulder][1] != kSentinel && joints[mdl::TrackedJoint::LeftElbow][1] != kSentinel &&
        joints[mdl::TrackedJoint::Neck][1] != kSentinel) {
        quatRotationAxis(qA, kAxisZ, k35deg5311A4);                
        if (joints[mdl::TrackedJoint::LeftClavicle][1] == kSentinel) {                               
            std::memcpy(qByVal, pose.upperBody, sizeof qByVal);
            BuildLookAtQuaternion(q150, qByVal, joints[mdl::TrackedJoint::LeftElbow][0], joints[mdl::TrackedJoint::LeftElbow][1], joints[mdl::TrackedJoint::LeftElbow][2],
                      joints[mdl::TrackedJoint::LeftShoulder][0], joints[mdl::TrackedJoint::LeftShoulder][1], joints[mdl::TrackedJoint::LeftShoulder][2], 0);            
        } else {
            d3->quatMultiply(qB, pose.leftShoulder, pose.upperBody);             
            BuildLookAtQuaternion(q150, qB, joints[mdl::TrackedJoint::LeftElbow][0], joints[mdl::TrackedJoint::LeftElbow][1], joints[mdl::TrackedJoint::LeftElbow][2],
                      joints[mdl::TrackedJoint::LeftClavicle][0], joints[mdl::TrackedJoint::LeftClavicle][1], joints[mdl::TrackedJoint::LeftClavicle][2], 0);            
        }
        d3->quatMultiply(pose.leftArm, qA, q150);                      
    } else if (flag != 0) {                                        
        quatRotationAxis(qA, kAxisZ, -k35deg5311A4);               
        std::memcpy(pose.leftArm, qA, 16);                             
    }

    if (joints[mdl::TrackedJoint::LeftElbow][1] != kSentinel && joints[mdl::TrackedJoint::LeftWrist][1] != kSentinel &&
        joints[mdl::TrackedJoint::LeftShoulder][1] != kSentinel) {
        quatRotationAxis(qA, kAxisZ, k30deg53119C);                
        d3->quatMultiply(t1, q150, pose.leftShoulder);                     
        d3->quatMultiply(t2, t1, pose.upperBody);                        
        BuildLookAtQuaternion(q155, t2, joints[mdl::TrackedJoint::LeftWrist][0], joints[mdl::TrackedJoint::LeftWrist][1], joints[mdl::TrackedJoint::LeftWrist][2],
                  joints[mdl::TrackedJoint::LeftElbow][0], joints[mdl::TrackedJoint::LeftElbow][1], joints[mdl::TrackedJoint::LeftElbow][2], 0);                
        d3->quatMultiply(pose.leftElbow, qA, q155);                     
        quatRotationAxis(qA, kAxisZ, -k30deg53119C);               
        d3->quatMultiply(pose.leftElbow, pose.leftElbow, qA);                
    } else if (flag != 0) {                                        
        pose.leftElbow[3] = 1.0f;                                      
        pose.leftElbow[0] = 0.0f;                                      
        pose.leftElbow[1] = 0.0f;                                      
        pose.leftElbow[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::LeftWrist][1] != kSentinel && joints[mdl::TrackedJoint::LeftHand][1] != kSentinel &&
        joints[mdl::TrackedJoint::LeftElbow][1] != kSentinel) {
        if (joints[mdl::TrackedJoint::LeftHand][0] == joints[mdl::TrackedJoint::LeftWrist][0] && joints[mdl::TrackedJoint::LeftHand][1] == joints[mdl::TrackedJoint::LeftWrist][1] &&
            joints[mdl::TrackedJoint::LeftHand][2] == joints[mdl::TrackedJoint::LeftWrist][2]) {                                
            pose.leftWrist[3] = 1.0f;                                  
            pose.leftWrist[0] = 0.0f;                                  
            pose.leftWrist[1] = 0.0f;                                  
            pose.leftWrist[2] = 0.0f;                                  
        } else {
            quatRotationAxis(qA, kAxisZ, k30deg53119C);            
            d3->quatMultiply(t1, q155, q150);                      
            d3->quatMultiply(t2, t1, pose.leftShoulder);                   
            d3->quatMultiply(qB, t2, pose.upperBody);                    
            BuildLookAtQuaternion(q159, qB, joints[mdl::TrackedJoint::LeftHand][0], joints[mdl::TrackedJoint::LeftHand][1], joints[mdl::TrackedJoint::LeftHand][2],
                      joints[mdl::TrackedJoint::LeftWrist][0], joints[mdl::TrackedJoint::LeftWrist][1], joints[mdl::TrackedJoint::LeftWrist][2], 0);            
            d3->quatMultiply(pose.leftWrist, qA, q159);                 
            quatRotationAxis(qA, kAxisZ, -k30deg53119C);           
            d3->quatMultiply(pose.leftWrist, pose.leftWrist, qA);            
        }
    } else if (flag != 0) {                                        
        pose.leftWrist[3] = 1.0f;                                      
        pose.leftWrist[0] = 0.0f;                                      
        pose.leftWrist[1] = 0.0f;                                      
        pose.leftWrist[2] = 0.0f;                                      
    }

    const float lmx = (joints[mdl::TrackedJoint::LeftHip][0] + joints[mdl::TrackedJoint::RightHip][0]) * 0.5f;                
    const float lmy = (joints[mdl::TrackedJoint::LeftHip][1] + joints[mdl::TrackedJoint::RightHip][1]) * 0.5f;                
    const float lmz = (joints[mdl::TrackedJoint::LeftHip][2] + joints[mdl::TrackedJoint::RightHip][2]) * 0.5f;                
    if (joints[mdl::TrackedJoint::Torso][1] != kSentinel && joints[mdl::TrackedJoint::RightHip][1] != kSentinel &&
        joints[mdl::TrackedJoint::LeftHip][1] != kSentinel && lmy != kSentinel) {               
        float ax = joints[mdl::TrackedJoint::Torso][0] - lmx, ay = joints[mdl::TrackedJoint::Torso][1] - lmy,
              az = joints[mdl::TrackedJoint::Torso][2] - lmz;                                 
        normalizeComponents(ax, ay, az);                               
        float cx = az * 1.0f - ay * 0.0f;                          
        float cy = ax * 0.0f - az * 0.0f;                          
        float cz = ay * 0.0f - ax * 1.0f;                          
        const float dot = 1.0f * ay + ax * 0.0f + 0.0f * az;       
        float axis[3] = {cx, cy, cz};
        d3->vec3Normalize(axis, axis);                              
        const float ang = acosf(dot);                              
        quatRotationAxis(qA, axis, ang);                           
        BuildLookAtQuaternion(t1, qA, joints[mdl::TrackedJoint::LeftHip][0], joints[mdl::TrackedJoint::LeftHip][1], joints[mdl::TrackedJoint::LeftHip][2],
                  joints[mdl::TrackedJoint::RightHip][0], joints[mdl::TrackedJoint::RightHip][1], joints[mdl::TrackedJoint::RightHip][2], 4);                
        std::memcpy(pose.lowerBody, t1, 16);                            
        d3->quatMultiply(t2, pose.lowerBody, qA);                       
        std::memcpy(pose.lowerBody, t2, 16);                            
    } else if (flag != 0) {
        pose.lowerBody[3] = 1.0f;                                      
        pose.lowerBody[0] = 0.0f;                                      
        pose.lowerBody[1] = 0.0f;                                      
        pose.lowerBody[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::RightHip][1] != kSentinel && joints[mdl::TrackedJoint::RightKnee][1] != kSentinel &&
        joints[mdl::TrackedJoint::Center][1] != kSentinel) {
        std::memcpy(qByVal, pose.lowerBody, sizeof qByVal);
        BuildLookAtQuaternion(t1, qByVal, joints[mdl::TrackedJoint::RightHip][0], joints[mdl::TrackedJoint::RightHip][1], joints[mdl::TrackedJoint::RightHip][2],
                  joints[mdl::TrackedJoint::RightKnee][0], joints[mdl::TrackedJoint::RightKnee][1], joints[mdl::TrackedJoint::RightKnee][2], 2);                
        std::memcpy(pose.rightLeg, t1, 16);                            
    } else if (flag != 0) {                                        
        pose.rightLeg[3] = 1.0f;                                      
        pose.rightLeg[0] = 0.0f;                                      
        pose.rightLeg[1] = 0.0f;                                      
        pose.rightLeg[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::LeftHip][1] != kSentinel && joints[mdl::TrackedJoint::LeftKnee][1] != kSentinel &&
        joints[mdl::TrackedJoint::Center][1] != kSentinel) {
        std::memcpy(qByVal, pose.lowerBody, sizeof qByVal);
        BuildLookAtQuaternion(t1, qByVal, joints[mdl::TrackedJoint::LeftHip][0], joints[mdl::TrackedJoint::LeftHip][1], joints[mdl::TrackedJoint::LeftHip][2],
                  joints[mdl::TrackedJoint::LeftKnee][0], joints[mdl::TrackedJoint::LeftKnee][1], joints[mdl::TrackedJoint::LeftKnee][2], 3);                
        std::memcpy(pose.leftLeg, t1, 16);                            
    } else if (flag != 0) {                                        
        pose.leftLeg[3] = 1.0f;                                      
        pose.leftLeg[0] = 0.0f;                                      
        pose.leftLeg[1] = 0.0f;                                      
        pose.leftLeg[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::RightKnee][1] != kSentinel && joints[mdl::TrackedJoint::RightAnkle][1] != kSentinel &&
        joints[mdl::TrackedJoint::Center][1] != kSentinel) {
        d3->quatMultiply(qB, pose.rightLeg, pose.lowerBody);                
        BuildLookAtQuaternion(t1, qB, joints[mdl::TrackedJoint::RightKnee][0], joints[mdl::TrackedJoint::RightKnee][1], joints[mdl::TrackedJoint::RightKnee][2],
                  joints[mdl::TrackedJoint::RightAnkle][0], joints[mdl::TrackedJoint::RightAnkle][1], joints[mdl::TrackedJoint::RightAnkle][2], 1);                
        std::memcpy(pose.rightKnee, t1, 16);                            
    } else if (flag != 0) {                                        
        pose.rightKnee[3] = 1.0f;                                      
        pose.rightKnee[0] = 0.0f;                                      
        pose.rightKnee[1] = 0.0f;                                      
        pose.rightKnee[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::LeftKnee][1] != kSentinel && joints[mdl::TrackedJoint::LeftAnkle][1] != kSentinel &&
        joints[mdl::TrackedJoint::Center][1] != kSentinel) {
        d3->quatMultiply(qB, pose.leftLeg, pose.lowerBody);                
        BuildLookAtQuaternion(t1, qB, joints[mdl::TrackedJoint::LeftKnee][0], joints[mdl::TrackedJoint::LeftKnee][1], joints[mdl::TrackedJoint::LeftKnee][2],
                  joints[mdl::TrackedJoint::LeftAnkle][0], joints[mdl::TrackedJoint::LeftAnkle][1], joints[mdl::TrackedJoint::LeftAnkle][2], 1);                
        std::memcpy(pose.leftKnee, t1, 16);                            
    } else if (flag != 0) {                                        
        pose.leftKnee[3] = 1.0f;                                      
        pose.leftKnee[0] = 0.0f;                                      
        pose.leftKnee[1] = 0.0f;                                      
        pose.leftKnee[2] = 0.0f;                                      
    }

    if (joints[mdl::TrackedJoint::RightFoot][1] != kSentinel && joints[mdl::TrackedJoint::RightAnkle][1] != kSentinel) {
        float ax = joints[mdl::TrackedJoint::RightAnkle][0] - joints[mdl::TrackedJoint::RightKnee][0], ay = joints[mdl::TrackedJoint::RightAnkle][1] - joints[mdl::TrackedJoint::RightKnee][1],
              az = joints[mdl::TrackedJoint::RightAnkle][2] - joints[mdl::TrackedJoint::RightKnee][2];                            
        normalizeComponents(ax, ay, az);                               
        float bx = joints[mdl::TrackedJoint::RightFoot][0] - joints[mdl::TrackedJoint::RightAnkle][0], by = joints[mdl::TrackedJoint::RightFoot][1] - joints[mdl::TrackedJoint::RightAnkle][1],
              bz = joints[mdl::TrackedJoint::RightFoot][2] - joints[mdl::TrackedJoint::RightAnkle][2];                            
        normalizeComponents(bx, by, bz);                               
        const float dot = by * ay + bx * ax + bz * az;             
        bool straight = false;                                     
        if (joints[mdl::TrackedJoint::RightFoot][2] - joints[mdl::TrackedJoint::RightAnkle][2] > 0.0f) {                          
            const float w = fabsf(joints[mdl::TrackedJoint::RightFoot][0] - joints[mdl::TrackedJoint::RightAnkle][0]);            
            if (w < k75f && dot > 0.5f) straight = true;           
        }
        if (dot < kNeg0d3 || straight) {                           
            d3->quatMultiply(t1, pose.rightKnee, pose.rightLeg);            
            d3->quatMultiply(t2, t1, pose.lowerBody);                   
            std::memcpy(pose.rightFoot, t2, 16);                        
        } else {
            float ex = 0.0f, ey = kNeg0d7, ez = -1.0f;             
            normalizeComponents(ex, ey, ez);                           
            float cx = bz * ey - by * ez;                          
            float cy = bx * ez - bz * ex;                          
            float cz = by * ex - bx * ey;                          
            const float dot2 = ey * by + bx * ex + ez * bz;        
            float axis[3] = {cx, cy, cz};
            d3->vec3Normalize(axis, axis);                         
            quatRotationAxis(pose.rightFoot, axis, acosf(dot2));        
        }
    } else {
        d3->quatMultiply(t1, pose.rightKnee, pose.rightLeg);                
        d3->quatMultiply(t2, t1, pose.lowerBody);                       
        std::memcpy(pose.rightFoot, t2, 16);                            
    }

    if (joints[mdl::TrackedJoint::LeftFoot][1] != kSentinel && joints[mdl::TrackedJoint::LeftAnkle][1] != kSentinel) {
        float ax = joints[mdl::TrackedJoint::LeftAnkle][0] - joints[mdl::TrackedJoint::LeftKnee][0], ay = joints[mdl::TrackedJoint::LeftAnkle][1] - joints[mdl::TrackedJoint::LeftKnee][1],
              az = joints[mdl::TrackedJoint::LeftAnkle][2] - joints[mdl::TrackedJoint::LeftKnee][2];                            
        normalizeComponents(ax, ay, az);                               
        float bx = joints[mdl::TrackedJoint::LeftFoot][0] - joints[mdl::TrackedJoint::LeftAnkle][0], by = joints[mdl::TrackedJoint::LeftFoot][1] - joints[mdl::TrackedJoint::LeftAnkle][1],
              bz = joints[mdl::TrackedJoint::LeftFoot][2] - joints[mdl::TrackedJoint::LeftAnkle][2];                            
        normalizeComponents(bx, by, bz);                               
        const float dot = by * ay + bx * ax + bz * az;             
        bool straight = false;                                     
        if (joints[mdl::TrackedJoint::LeftFoot][2] - joints[mdl::TrackedJoint::LeftAnkle][2] > 0.0f) {                          
            const float w = fabsf(joints[mdl::TrackedJoint::LeftFoot][0] - joints[mdl::TrackedJoint::LeftAnkle][0]);            
            if (w < k75f && dot > 0.5f) straight = true;           
        }
        if (dot < kNeg0d3 || straight) {                           
            d3->quatMultiply(t1, pose.leftKnee, pose.leftLeg);            
            d3->quatMultiply(t2, t1, pose.lowerBody);                   
            std::memcpy(pose.leftFoot, t2, 16);                        
        } else {
            float ex = 0.0f, ey = kNeg0d7, ez = -1.0f;             
            normalizeComponents(ex, ey, ez);                           
            float cx = bz * ey - by * ez;                          
            float cy = bx * ez - bz * ex;                          
            float cz = by * ex - bx * ey;                          
            const float dot2 = ey * by + bx * ex + ez * bz;        
            float axis[3] = {cx, cy, cz};
            d3->vec3Normalize(axis, axis);                         
            quatRotationAxis(pose.leftFoot, axis, acosf(dot2));        
        }
    } else {
        d3->quatMultiply(t1, pose.leftKnee, pose.leftLeg);                
        d3->quatMultiply(t2, t1, pose.lowerBody);                       
        std::memcpy(pose.leftFoot, t2, 16);                            
    }
}

}  
