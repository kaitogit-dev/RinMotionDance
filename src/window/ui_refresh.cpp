#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

void ClearAccessoryRowSelection(MMDApp* app) {
    for (int slot = 0; slot < 255; ++slot) {
        if (auto* accessory = app->AccessorySlots()[slot])
            accessory->rowSelected = 0;
    }
}

constexpr std::size_t kStreamBufOffset   = 0x14;   
constexpr std::size_t kStreamFilePtr     = 0x2C;   
constexpr std::size_t kStreamAsyncActive = 0x234;  
constexpr std::size_t kStreamFileHandle  = 0x244;  
constexpr std::size_t kStreamCompletion  = 0x248;  

}  

void RefreshRequest(int area) {
    MMDApp* app = g_Block;

    if (area == -1) {                       
        if (app->GlobalTrackSelected(GlobalTimelineTrack::Camera) == 0) {
            app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
            ClearAccessoryRowSelection(app);
            PostLanguageSweep(app);         
        }
        return;
    }

    if (area == -2) {                       
        if (app->GlobalTrackSelected(GlobalTimelineTrack::Light) == 0) {
            app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Light);
            ClearAccessoryRowSelection(app);
            PostLanguageSweep(app);         
        }
        return;
    }

    if (area == -3) {                       
        if (app->GlobalTrackSelected(GlobalTimelineTrack::SelfShadow) == 0) {
            app->SelectGlobalTimelineTrack(GlobalTimelineTrack::SelfShadow);
            ClearAccessoryRowSelection(app);
            PostLanguageSweep(app);         
        }
        return;
    }

    if (area == -4) {                       
        if (app->GlobalTrackSelected(GlobalTimelineTrack::Gravity) == 0) {
            app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Gravity);
            ClearAccessoryRowSelection(app);
            PostLanguageSweep(app);         
        }
        return;
    }

    auto* accessory = app->AccessorySlots()[area];
    if (accessory != nullptr && accessory->rowSelected == 0) {
        app->ClearGlobalTimelineTrackSelection();
        ClearAccessoryRowSelection(app);
        app->AccessorySlots()[area]->rowSelected = 1;
        PostLanguageSweep(app);
    }
}

void CloseDataFile(void* file) {
    auto* audio = static_cast<WaveAudioContext*>(file);

    if (audio->feedThread != 0 && audio->ReadStopFlag() == 0) {
        audio->WriteStopFlag(1);
        while (audio->ReadStopFlag() != 2)
            Sleep(1);
        Sleep(0);
    }

    FILE* stream = audio->parseStream;
    if (stream != nullptr)
        std::fclose(stream);                

    HANDLE hFile = audio->fileHandle;
    if (hFile != nullptr)
        CloseHandle(hFile);                 

    audio->parseStream = nullptr;
    audio->feedThread = 0;
    audio->streamingBuffer = nullptr;
    audio->fileHandle = nullptr;
}

}  
