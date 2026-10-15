#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <new>
#include <cstdio>
#include <cstring>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void ModelVertexHistoryPush(unsigned char* model, int samples);   
char ModelStandardPoseSetup(unsigned char* model, bool recordEnable,     
                            unsigned char mirrorLeftRight,
                            unsigned char skeletonFlag);
void CurvePanelRepaint(MMDApp* app);                                

namespace {

unsigned char* PoseTraceBuffer(unsigned char* model) {
    return static_cast<unsigned char*>(mdl::PoseTraceBuffer(model));
}

const unsigned char kCenter[9] =       
    {0x83, 0x5A, 0x83, 0x93, 0x83, 0x65, 0x81, 0x5B, 0x00};
const unsigned char kUpperBody[7] =    
    {0x8F, 0xE3, 0x94, 0xBC, 0x90, 0x67, 0x00};
const unsigned char kNeck[3] =         
    {0x8E, 0xF1, 0x00};
const unsigned char kArmL[5] =         
    {0x8D, 0xB6, 0x98, 0x72, 0x00};
const unsigned char kArmR[5] =         
    {0x89, 0x45, 0x98, 0x72, 0x00};
const unsigned char kElbowL[7] =       
    {0x8D, 0xB6, 0x82, 0xD0, 0x82, 0xB6, 0x00};
const unsigned char kElbowR[7] =       
    {0x89, 0x45, 0x82, 0xD0, 0x82, 0xB6, 0x00};
const unsigned char kLowerBody[7] =    
    {0x89, 0xBA, 0x94, 0xBC, 0x90, 0x67, 0x00};
const unsigned char kLegL[5] =         
    {0x8D, 0xB6, 0x91, 0xAB, 0x00};
const unsigned char kLegR[5] =         
    {0x89, 0x45, 0x91, 0xAB, 0x00};
const unsigned char kLegIkL[9] =       
    {0x8D, 0xB6, 0x91, 0xAB, 0x82, 0x68, 0x82, 0x6A, 0x00};
const unsigned char kLegIkR[9] =       
    {0x89, 0x45, 0x91, 0xAB, 0x82, 0x68, 0x82, 0x6A, 0x00};
const unsigned char kKneeL[7] =        
    {0x8D, 0xB6, 0x82, 0xD0, 0x82, 0xB4, 0x00};
const unsigned char kKneeR[7] =        
    {0x89, 0x45, 0x82, 0xD0, 0x82, 0xB4, 0x00};
const unsigned char kWristL[7] =       
    {0x8D, 0xB6, 0x8E, 0xE8, 0x8E, 0xF1, 0x00};
const unsigned char kWristR[7] =       
    {0x89, 0x45, 0x8E, 0xE8, 0x8E, 0xF1, 0x00};
const unsigned char kShoulderL[5] =    
    {0x8D, 0xB6, 0x8C, 0xA8, 0x00};
const unsigned char kShoulderR[5] =    
    {0x89, 0x45, 0x8C, 0xA8, 0x00};

int FindBone(unsigned char* model, const void* name, std::size_t bytes) {
    mdl::BoneRecord* bones = mdl::Bones(model);
    const int count = static_cast<int>(mdl::Mdl(model)->boneCount);
    for (int i = 0; i < count; ++i)
        if (std::memcmp(bones[i].name, name, bytes) == 0)
            return i;
    return -1;
}

void TraceKeyOverflowBox(unsigned char* model) {
    static const char kJpOverflow[] =
        "\x93\x6f\x98\x5e\x83\x7c\x83\x43\x83\x93\x83\x67\x90\x94\x82\xaa%d"
        "\x8c\xc2\x82\xf0\x89\x7a\x82\xa6\x82\xdc\x82\xb5\x82\xbd\n"
        "\x82\xb1\x82\xea\x88\xc8\x8f\xe3\x82\xcc\x93\x6f\x98\x5e\x82\xcd\x8d"
        "\x73\x82\xa6\x82\xdc\x82\xb9\x82\xf1\n"
        "\x81\x75\xcc\xda\xb0\xd1\x95\xd2\x8f\x57\x81\x76\x82\xcc\x81\x75\x95"
        "\x73\x97\x70\xcc\xda\xb0\xd1\x8d\xed\x8f\x9c\x81\x76\x82\xf0\x8e\xfc"
        "\x8d\x73\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2";
    static const char kJpTitle[] = "\xcc\xda\xb0\xd1\x93\x6f\x98\x5e";
    char text[0x100];
    if (mdl::Mdl(model)->physicsFlags != 0) {
        sprintf_s(text, sizeof(text),
                  "You cannot regist over %d point\n"
                  "Please execute 'delete unused frame'",
                  static_cast<int>(mdl::kBoneKeyCapacity));
        MessageBoxA(*reinterpret_cast<HWND*>(model), text, "register frame", 0);
    } else {
        sprintf_s(text, sizeof(text), kJpOverflow,
                  static_cast<int>(mdl::kBoneKeyCapacity));
        MessageBoxA(*reinterpret_cast<HWND*>(model), text, kJpTitle, 0);
    }
}

void FillTraceKey(mdl::BoneKey& record, const unsigned char* traceGroup,
                  int mode) {
    switch (mode) {
    case 0:
        std::memcpy(record.position, traceGroup, sizeof(record.position));
        record.rotation[0] = 0.0f;
        record.rotation[1] = 0.0f;
        record.rotation[2] = 0.0f;
        record.rotation[3] = 1.0f;                                
        break;
    case 1:
        record.position[0] = 0.0f;
        record.position[1] = 0.0f;
        record.position[2] = 0.0f;
        std::memcpy(record.rotation, traceGroup, sizeof(record.rotation));
        break;
    default:  
        std::memcpy(record.position, traceGroup, sizeof(record.position));
        std::memcpy(record.rotation, traceGroup + 16, sizeof(record.rotation));
        break;
    }
    for (int lane = 0; lane < 4; ++lane) {
        record.interpolation[4 * lane + 0] = 20;
        record.interpolation[4 * lane + 1] = 20;
        record.interpolation[4 * lane + 2] = 107;
        record.interpolation[4 * lane + 3] = 107;
    }
    record.allocated = 1;
}

}  

bool RegisterTraceBoneKey(unsigned char* model, unsigned boneIdx, int slot,
                          int mode, unsigned frame) {
    mdl::BoneKey* const keys = mdl::BoneKeys(model);
    const std::int32_t boneCount =
        static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);

    unsigned current = boneIdx;
    if (keys[current].frame < frame) {
        for (;;) {
            const unsigned next = keys[current].next;
            if (next == 0)
                break;
            if (keys[next].frame >= frame)
                break;
            current = next;
        }
    }

    unsigned freeSlot = static_cast<unsigned>(boneCount);
    if (keys[freeSlot].frame != 0) {
        for (;;) {
            ++freeSlot;
            if (freeSlot >= mdl::kBoneKeyCapacity) {
                TraceKeyOverflowBox(model);
                return false;
            }
            if (keys[freeSlot].frame == 0)
                break;
        }
    }

    unsigned successor = keys[current].next;
    const unsigned sampleCount =
        static_cast<unsigned>(mdl::Mdl(model)->matMisc);
    if (sampleCount == 0)
        return true;                            

    const unsigned char* const trace = PoseTraceBuffer(model);
    unsigned sampleFrame = frame;
    for (std::size_t group = 4 * static_cast<std::size_t>(slot);;
         group += 308, ++sampleFrame) {
        AppendBoneKeyToUndo(model, static_cast<int>(current));      
        AppendBoneKeyToUndo(model, static_cast<int>(successor));
        AppendBoneKeyToUndo(model, static_cast<int>(freeSlot));

        if (keys[current].frame == sampleFrame) {
            keys[current].next = 0;
            FillTraceKey(keys[current], trace + group, mode);
        } else if (keys[successor].frame == sampleFrame) {
            keys[current].next = successor;
            keys[successor].previous = current;
            FillTraceKey(keys[successor], trace + group, mode);
            current = successor;
            successor = keys[current].next;
        } else {
            keys[current].next = freeSlot;
            keys[freeSlot].previous = current;
            keys[freeSlot].frame = sampleFrame;
            keys[freeSlot].next = 0;
            FillTraceKey(keys[freeSlot], trace + group, mode);
            current = freeSlot;
            ++freeSlot;
            if (keys[freeSlot].frame != 0) {
                for (;;) {
                    ++freeSlot;
                    if (freeSlot >= mdl::kBoneKeyCapacity) {
                        TraceKeyOverflowBox(model);
                        return false;
                    }
                    if (keys[freeSlot].frame == 0)
                        break;
                }
            }
        }

        if (sampleFrame > mdl::Mdl(model)->maxFrame)
            mdl::Mdl(model)->maxFrame = sampleFrame;
        if (successor != 0) {
            keys[current].next = successor;        
            keys[successor].previous = current;    
        }
        if (sampleFrame + 1 - frame >= sampleCount)
            return true;
    }
}

bool RegisterKinectPoseCapture(unsigned char* model, unsigned frame) {
    HWND hwnd = *reinterpret_cast<HWND*>(model);

    for (std::size_t i = 0; i < mdl::kBoneKeyCapacity; ++i)
        mdl::BoneKeys(model)[i].allocated = 0;
    for (std::size_t i = 0; i < mdl::kMorphKeyCapacity; ++i)
        mdl::MorphKeys(model)[i].allocated = 0;
    for (std::size_t i = 0; i < mdl::kDisplayKeyCapacity; ++i)
        mdl::DisplayKeys(model)[i].allocated = 0;

    const std::int32_t samples = mdl::Mdl(model)->matMisc;
    if (samples > 0) {                             
        EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);    
        EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);   

        if (++mdl::Mdl(model)->undoState[0] >= 30u)
            mdl::Mdl(model)->undoState[0] = 0;
        const int cursor = mdl::Mdl(model)->undoState[0];
        mdl::Mdl(model)->undoState[1] = cursor;
        mdl::Mdl(model)->undoDirty = 1;
        mdl::Mdl(model)->redoDirty = 0;
        mdl::UndoRecord& undo = mdl::Mdl(model)->undoRings[0].slots[cursor];
        undo.operation = 2;
        undo.frame = frame;

        ::operator delete(undo.bonePose);
        undo.bonePose = nullptr;
        const std::int32_t boneCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);
        undo.bonePose = static_cast<mdl::BonePoseSnapshot*>(
            ::operator new(sizeof(mdl::BonePoseSnapshot) * boneCount));
        std::memset(undo.bonePose, 0, sizeof(mdl::BonePoseSnapshot) * boneCount);
        mdl::BoneRecord* const bones = mdl::Bones(model);
        unsigned char* const flags = mdl::Mdl(model)->boneSelection;
        for (std::int32_t i = 0; i < boneCount; ++i) {
            undo.bonePose[i].boneIndex = i;
            std::memcpy(undo.bonePose[i].position, bones[i].trans,
                        sizeof(undo.bonePose[i].position));
            std::memcpy(undo.bonePose[i].rotation, bones[i].rotQuat,
                        sizeof(undo.bonePose[i].rotation));
            undo.bonePose[i].physicsDisabled = flags[i];
        }

        undo.dirty = 0;
        ::operator delete(undo.auxiliaryPose);
        undo.auxiliaryPose = nullptr;
        undo.auxiliaryPose =
            ::operator new(3456 * static_cast<std::size_t>(samples));
        std::memset(undo.auxiliaryPose, 0, 3456 * static_cast<std::size_t>(samples));
        std::memset(mdl::Mdl(model)->keyVisitMap, 0, mdl::kBoneKeyCapacity);

        const std::uint8_t version = mdl::Mdl(model)->openniVersion;
        int idx;
        if ((idx = FindBone(model, kCenter, 9)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 0, 0, frame))
            return false;                                         
        if ((idx = FindBone(model, kUpperBody, 7)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 3, 1, frame))
            return false;                                         
        if (version >= 14 &&
            (idx = FindBone(model, kNeck, 3)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 7, 1, frame))
            return false;                                         
        if ((idx = FindBone(model, kArmL, 5)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 11, 1, frame))
            return false;                                         
        if ((idx = FindBone(model, kElbowL, 7)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 15, 1, frame))
            return false;                                         
        if ((idx = FindBone(model, kArmR, 5)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 19, 1, frame))
            return false;                                         
        if ((idx = FindBone(model, kElbowR, 7)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 23, 1, frame))
            return false;                                         
        if ((idx = FindBone(model, kLowerBody, 7)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 27, 1, frame))
            return false;                                         
        if ((idx = FindBone(model, kLegL, 5)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 31, 1, frame))
            return false;                                         
        if ((idx = FindBone(model, kLegR, 5)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 35, 1, frame))
            return false;                                         

        bool legIkLeftEnabled = false;
        bool legIkRightEnabled = false;
        mdl::IkChain* const chains = mdl::IkChains(model);
        const std::int32_t chainCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->ikChainCount);
        for (std::int32_t i = 0; i < chainCount; ++i) {
            if (std::memcmp(bones[chains[i].boneIndex].name, kLegIkL, 9) == 0)
                legIkLeftEnabled = chains[i].enabled != 0;
            if (std::memcmp(bones[chains[i].boneIndex].name, kLegIkR, 9) == 0)
                legIkRightEnabled = chains[i].enabled != 0;
        }
        if (legIkLeftEnabled) {
            if ((idx = FindBone(model, kLegIkL, 9)) >= 0 &&
                !RegisterTraceBoneKey(model, idx, 47, 2, frame))
                return false;
        } else {
            if ((idx = FindBone(model, kKneeL, 7)) >= 0 &&
                !RegisterTraceBoneKey(model, idx, 39, 1, frame))
                return false;
        }
        if (legIkRightEnabled) {
            if ((idx = FindBone(model, kLegIkR, 9)) >= 0 &&
                !RegisterTraceBoneKey(model, idx, 54, 2, frame))
                return false;
        } else {
            if ((idx = FindBone(model, kKneeR, 7)) >= 0 &&
                !RegisterTraceBoneKey(model, idx, 43, 1, frame))
                return false;
        }

        if (version >= 14 &&
            (idx = FindBone(model, kWristL, 7)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 61, 1, frame))
            return false;                                         
        if (version >= 14 &&
            (idx = FindBone(model, kWristR, 7)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 65, 1, frame))
            return false;                                         
        if (version >= 15 &&
            (idx = FindBone(model, kShoulderL, 5)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 69, 1, frame))
            return false;                                         
        if (version >= 15 &&
            (idx = FindBone(model, kShoulderR, 5)) >= 0 &&
            !RegisterTraceBoneKey(model, idx, 73, 1, frame))
            return false;                                         
    }  

    ::operator delete(PoseTraceBuffer(model));
    mdl::PoseTraceBuffer(model) = nullptr;
    mdl::Mdl(model)->matMisc = 0;
    return true;
}

void PumpKinectSkeleton(MMDApp* app, unsigned char selActive) {
    auto& s = *app;

    if (selActive == 0 ||
        s.state.optflag[0] != 0 ||            
        s.state.frameStepPlayback != 0)       
        return;

    unsigned char* model = s.SelectedModel();  

    auto getJoint = reinterpret_cast<void (__stdcall*)(int, float*)>(
        s.OniExportSlot(4));  
    if (model == nullptr || getJoint == nullptr)
        return;
    mdl::CaptureSkeletonJoints(mdl::Mdl(model)->currentJoints,
                               s.state.openniVersion, getJoint);

    ModelVertexHistoryPush(
        model, s.state.framesPerSecond / 10);    

    if (static_cast<std::uint32_t>(mdl::Mdl(model)->matMisc) >= 12600) {
        CheckMenuItem(GetMenu(static_cast<HWND>(s.Hwnd())), 0x124,
                      MF_UNCHECKED);
        s.state.autoRepeat = 0;                  
    }

    const char wasRecording = ModelStandardPoseSetup(
        model,
        s.state.autoRepeat == 4 ? 1 : 0,         
        s.state.kinectMirrorEnabled,             
        s.state.kinectInitLostBone);             
    if (wasRecording == 0)
        return;

    DisableKinect(app);                                          
    if (model != nullptr)
        RegisterKinectPoseCapture(
            model, static_cast<unsigned>(s.state.currentFrame));  
    CurvePanelRepaint(app);                            
    if (model != nullptr)
        SeekModelFrame(model, s.state.currentFrame,
                      s.PlaybackPhysicsMode());       

    if (s.state.lastRegisteredFrame < mdl::Mdl(model)->maxFrame)
        s.state.lastRegisteredFrame = mdl::Mdl(model)->maxFrame;
    s.PhysicsResetPending() = 1;                  
    s.state.kinectCaptureActive = 0;              
    s.state.fpsLimit = s.state.fpsLimitSaved;     
    PanelPaint(app);                              
}

void ManageKinectRecordGate(MMDApp* app, unsigned char selActive) {
    auto& s = *app;

    auto drawDepthMap = reinterpret_cast<void (__stdcall*)(unsigned char)>(
        s.OniExportSlot(2));                      
    if (drawDepthMap != nullptr)
        drawDepthMap(
            (s.state.kinectCaptureActive == 0 &&   
             s.state.autoRepeat == 0)              
                ? 0
                : 1);

    HMENU menu = GetMenu(static_cast<HWND>(s.Hwnd()));
    if (selActive != 0 && s.state.optflag[0] == 0) {
        EnableMenuItem(menu, 0x124, 0);           
        return;
    }
    EnableMenuItem(menu, 0x124, 1);               
    CheckMenuItem(menu, 0x124, MF_UNCHECKED);
    s.state.autoRepeat = 0;                       
}

}  
