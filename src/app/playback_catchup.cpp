#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "btBulletDynamicsCommon.h"

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

float UnsigFlt(std::int32_t v) {
    return static_cast<float>(static_cast<std::uint32_t>(v));
}

inline float& Cursor(MMDApp* app) {
    return app->PlaybackCursorSeconds();
}

btDiscreteDynamicsWorld* PlayWorld(MMDApp* app) {
    PhysicsScene* scene = app->Physics();
    if (scene == nullptr)
        return nullptr;
    return scene->world;   
}

void StepWorld(btDiscreteDynamicsWorld* world) {
#ifdef RINMOTIONDANCE_DIAG
    if (getenv("RINMOTIONDANCE_TRACE_STEPS") != nullptr)
        fprintf(stderr, "CATCHUP step (cursor behind target)\n");
#endif
    world->stepSimulation(1.0f / 60.0f, 10, 1.0f / 60.0f);   
}

void BumpCursor(MMDApp* app) {
    Cursor(app) += 1.0f / 60.0f;
}

void OrderedMorphPhysicsPass(MMDApp* app, int count, bool selSkip) {
    auto& s = *app;
    unsigned char** models = s.ModelSlots();
    for (int order = 0; order < kModelSlotCount; ++order) {
        for (int j = 0; j < kModelSlotCount; ++j) {
            unsigned char* mdl = models[j];
            if (mdl == nullptr || rinmotiondance::mdl::Mdl(mdl)->comboSelIndex2 != order)
                continue;
            if (selSkip && s.state.optflag[0] == 0 &&
                s.SelectedModelSlot() == j)
                continue;                                   
            ModelApplyMorphs(mdl);                            
            SetPhysicsMode(mdl, 0, models, count);
        }
    }
}

void KinematicSyncPass(MMDApp* app, bool reverse, bool selSkip) {
    auto& s = *app;
    unsigned char** models = s.ModelSlots();
    const int begin = reverse ? kModelSlotCount - 1 : 0;
    const int end = reverse ? -1 : kModelSlotCount;
    const int step = reverse ? -1 : 1;
    for (int j = begin; j != end; j += step) {
        unsigned char* mdl = models[j];
        if (mdl == nullptr)
            continue;
        if (selSkip && s.state.optflag[0] == 0 &&
            s.SelectedModelSlot() == j)
            continue;                                       
        ModelKinematicSync(mdl);                             
    }
}

void FrameStepBeyondEnd(MMDApp* app) {
    auto& s = *app;
    DShowRecorder* recorder = s.Recorder();
    IPushSource* push =
        recorder != nullptr
            ? static_cast<IPushSource*>(recorder->framePush)
            : nullptr;
    unsigned char* flagPtr = s.RecordingCompletionFlag();

    auto capture = [&]() -> long {                          
#ifdef RINMOTIONDANCE_DIAG
        static int capCount = 0;
#endif
        if (push == nullptr)
            return 0;
#ifdef RINMOTIONDANCE_DIAG
        if (getenv("RINMOTIONDANCE_TRACE_REC")) {
            if (++capCount <= 5 || capCount % 60 == 0) {
                FILE* tf = fopen(getenv("RINMOTIONDANCE_TRACE_REC"), "a");
                if (tf) {
                    fprintf(tf,
                            "capture#%d push=%p flag=%d\n",
                            capCount, static_cast<void*>(push),
                            (int)*flagPtr);
                    fclose(tf);
                }
            }
        }
#endif
        return push->GetStreamingState(flagPtr);
    };

    capture();
    while (*flagPtr == 0) {                                 
        if (push == nullptr)                                
            break;
        if (capture() == static_cast<long>(0x80004005))     
            break;                                          
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);                         
            DispatchMessageA(&msg);
        }
        Sleep(1);                                           
    }
    Sleep(500);                                             
    FinishAviRecord(app);                                         
}

void PlayingBeyondEnd(MMDApp* app) {
    auto& s = *app;
    WaveAudioContext* audio = s.Audio();                    
    if (s.WaveEnabled() != 0) {
        CloseDataFile(audio);                                
        WaveStartPlayback(audio);                            
    }

    if (s.PlaybackLoopEnabled() != 0) {
        char text[8];
        GetWindowTextA(GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kPlayStartFrameEdit),
                       text, 8);
        const std::int32_t frame = std::atol(text);
        s.PlaybackStartSeconds() = UnsigFlt(frame) / 30.0f;
        s.PlaybackPhysicsMode() = s.SavedPlaybackPhysicsMode();
        Cursor(app) = s.PlaybackStartSeconds();
        UpdateBoneFrames(app);                              
        if (s.WaveEnabled() != 0 &&
            s.AutomaticFrameAdvanceEnabled() == 0) {
            SetFrameNormalized(
                s.FrameNormalization());
            WaveSeekAndFeed(audio, static_cast<double>(
                                   s.PlaybackStartSeconds()));
        }
        s.PlaybackClockAnchorLow() =
            s.TimeNowLow();
        s.PlaybackClockAnchorHigh() = s.TimeNowHigh();
        PlaybackPoseAdvance(app, 0);                        
        return;
    }

    s.PlaybackActive() = 0;
    StopPlayback(app);                                         
    SendMessageA(GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kPlayButton),
                 BM_SETCHECK, 0, 0);               
}

}  

void PlaybackCatchup(MMDApp* app, unsigned char selActive) {
    auto& s = *app;
    const bool selSkip = selActive != 0;

    if (s.state.aviStereoOutput == 0) {
        const std::int32_t n = s.state.recordedFrameCount;
        const std::int32_t frameA = s.AviRecordStartFrame();
        const float fps = s.AviRecordFps();
        const int shown = static_cast<int>(
            static_cast<float>(n * 30) / fps + UnsigFlt(frameA));
        wchar_t buf[0x100];
        if (s.state.englishUI != 0) {
            swprintf_s(buf, 0x100, L" %dframe recording(until %dframe)",
                       shown, s.AviRecordEndFrame());
        } else {
            swprintf_s(buf, 0x100, L" %d", shown);
        }
        SetWindowTextW(s.RecordingWindow(), buf);
    }
    s.state.recordedFrameCount += 1;                                     

    if (s.state.messageSeen == 0)
        return;                                    
    g_CatchupDtBudget = s.state.deltaTime;                   
    if (s.PlaybackActive() == 0)
        return;                                    

    const int count =
        s.PlaybackPhysicsMode();

    if (s.FrameStepPlayback() != 0) {
        const std::int32_t n = s.state.recordedFrameCount;
        const std::int32_t frameA = s.AviRecordStartFrame();
        const float fps = s.AviRecordFps();
        g_CatchupDtBudget = 1.0f / fps;                     
        const float term = static_cast<float>(n) * 30.0f / fps +
                           UnsigFlt(frameA);
        const float target = term / 30.0f;                  
        const std::uint32_t gate = static_cast<std::uint32_t>(
            static_cast<std::int64_t>(term));
        const std::uint32_t limit = static_cast<std::uint32_t>(
            s.AviRecordEndFrame() + 1);
        if (gate < limit) {                                 
            if (count >= 1) {
                BumpCursor(app);
                btDiscreteDynamicsWorld* world = PlayWorld(app);
                if (target > Cursor(app)) {
                    do {
                        PlaybackPoseAdvance(app, 1);       
                        OrderedMorphPhysicsPass(app, count, false);
                        KinematicSyncPass(app, false, false);
                        StepWorld(world);
                        g_CatchupDtBudget -= 1.0f / 60.0f;
                        BumpCursor(app);
                    } while (Cursor(app) < target);         
                }
            }
            Cursor(app) = target;                           
            PlaybackPoseAdvance(app, 0);                     
            return;                                         
        }
        FrameStepBeyondEnd(app);                            
        return;                                             
    }

#ifdef RINMOTIONDANCE_DIAG
    static int tracePlayLeft = -1;
    static int tracePassLeft = -1;
    if (const char* tp = getenv("RINMOTIONDANCE_TRACE_PLAY")) {
        if (tracePlayLeft < 0) {
            FILE* tf = fopen(tp, "a");
            if (tf) {
                fprintf(tf, "play-start end=%f cursor=%f count=%d\n",
                        s.PlaybackEndSeconds(), Cursor(app),
                        count);
                fclose(tf);
            }
            tracePlayLeft = 400;
            tracePassLeft = 100;
            unsigned char* mdl = s.ModelSlot(0);
            FILE* tf2 = fopen(tp, "a");
            if (tf2 && mdl != nullptr) {
                const std::int32_t boneCnt = mdl::Mdl(mdl)->boneCount;
                unsigned char* const active =
                    mdl::Mdl(mdl)->boneTrackActive;
                std::uint32_t* const cursors =
                    mdl::Mdl(mdl)->boneKeyCursors;
                rinmotiondance::mdl::BoneRecord* const bones = rinmotiondance::mdl::Bones(mdl);
                const mdl::BoneKey* const keyList = mdl::BoneKeys(mdl);
                fprintf(tf2, "loadstate boneCnt=%d\n", boneCnt);
                for (int b = 0; b < boneCnt && b < 12; ++b)
                    fprintf(tf2,
                            "bone%d active=%d cursor=%d quat=%08X "
                            "keyframe=%d nextframe=%d\n",
                            b, active ? active[b] : -1,
                            cursors ? cursors[b] : -1,
                            *reinterpret_cast<std::uint32_t*>(
                                &bones[b].rotQuat[0]),
                            (keyList && cursors)
                                ? static_cast<int>(keyList[cursors[b]].frame)
                                : -1,
                            (keyList && cursors &&
                             keyList[cursors[b]].next != 0)
                                ? static_cast<int>(
                                      keyList[keyList[cursors[b]].next].frame)
                                : -1);
                fclose(tf2);
            } else if (tf2) {
                fclose(tf2);
            }
        }
    }
#endif
    const std::uint64_t now =
        (static_cast<std::uint64_t>(s.TimeNowHigh()) << 32) | s.TimeNowLow();
    const std::uint64_t t0 =
        (static_cast<std::uint64_t>(s.PlaybackClockAnchorHigh()) << 32) |
        s.PlaybackClockAnchorLow();
    const std::int64_t elapsed =
        static_cast<std::int64_t>(now - t0);
    const float target = static_cast<float>(elapsed) * s.MilliToSec() +
                         s.PlaybackStartSeconds();
    const float end = s.PlaybackEndSeconds();

#ifdef RINMOTIONDANCE_DIAG
    if (tracePassLeft > 0) {
        --tracePassLeft;
        if (FILE* tf = fopen(getenv("RINMOTIONDANCE_TRACE_PLAY"), "a")) {
            fprintf(tf,
                    "pass cursor=%.5f target=%.5f end=%.5f elapsed=%.3f "
                    "b330=%d sel=%d\n",
                    Cursor(app), target, end, static_cast<double>(elapsed),
                    s.PlaybackActive(), selSkip);
            fclose(tf);
        }
    }
#endif

    if (!(end < target)) {                                  
        if (count >= 1) {
            BumpCursor(app);
            btDiscreteDynamicsWorld* world = PlayWorld(app);
            if (target > Cursor(app)) {
                do {
                    PlaybackPoseAdvance(app, 1);           
                    OrderedMorphPhysicsPass(app, count, selSkip);
                    KinematicSyncPass(app, false, selSkip);
                    StepWorld(world);
                    g_CatchupDtBudget -= 1.0f / 60.0f;
                    BumpCursor(app);
#ifdef RINMOTIONDANCE_DIAG
                    if (tracePlayLeft > 0 && getenv("RINMOTIONDANCE_TRACE_PLAY")) {
                        --tracePlayLeft;
                        unsigned char* mdl = s.ModelSlot(0);
                        FILE* tf = fopen(getenv("RINMOTIONDANCE_TRACE_PLAY"), "a");
                        if (tf) {
                            if (mdl != nullptr) {
                                rinmotiondance::mdl::BoneRecord* bones =
                                    rinmotiondance::mdl::Bones(mdl);
                                const std::int32_t boneCount = static_cast<std::int32_t>(
                                    rinmotiondance::mdl::Mdl(mdl)->boneCount);
                                fprintf(tf,
                                        "sub cursor=%.5f target=%.5f bones=",
                                        Cursor(app), target);
                                for (int bi = 0; bi < boneCount && bi < 8;
                                     ++bi)
                                    fprintf(tf, "%08X%08X%08X%08X,",
                                            *reinterpret_cast<
                                                std::uint32_t*>(
                                                &bones[bi].rotQuat[0]),
                                            *reinterpret_cast<
                                                std::uint32_t*>(
                                                &bones[bi].rotQuat[1]),
                                            *reinterpret_cast<
                                                std::uint32_t*>(
                                                &bones[bi].rotQuat[2]),
                                            *reinterpret_cast<
                                                std::uint32_t*>(
                                                &bones[bi].rotQuat[3]));
                                fputc('\n', tf);
                            }
                            fclose(tf);
                        }
                    }
#endif
                } while (Cursor(app) < target);             
            }
        }
        Cursor(app) = target;                               
        PlaybackPoseAdvance(app, 0);                        
        return;
    }

    Cursor(app) = target;
    PlayingBeyondEnd(app);
}

}  
