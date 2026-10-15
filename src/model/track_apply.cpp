#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

constexpr float kWrap2p32 = 4294967296.0f;   

void ApplyGravityRecord(MMDApp* app, const mdl::GravityKey& key) {
    std::uint32_t noiseModeWord;
    std::memcpy(&noiseModeWord, &key.noiseEnabled, sizeof noiseModeWord);
    app->GravityNoiseModeWord() = noiseModeWord;
    app->state.gravityNoise =
        static_cast<std::uint32_t>(key.noise);
    app->state.gravityMagnitude = key.acceleration;
    app->state.gravityX = key.direction[0];
    app->state.gravityY = key.direction[1];
    app->state.gravityZ = key.direction[2];
}

void RefreshPhysicsDialog(MMDApp* app) {
    HWND dlg = app->GravitySettingDialog();
    if (dlg == nullptr) return;
    char text[0x32];
    sprintf_s(text, 0x32, "%3.2f",
              app->state.gravityMagnitude);       
    SetWindowTextA(GetDlgItem(dlg, panel::kGravityAccelEdit), text);
    sprintf_s(text, 0x32, "%3.2f",
              app->state.gravityX);
    SetWindowTextA(GetDlgItem(dlg, panel::kGravityDirXEdit), text);
    sprintf_s(text, 0x32, "%3.2f",
              app->state.gravityY);
    SetWindowTextA(GetDlgItem(dlg, panel::kGravityDirYEdit), text);
    sprintf_s(text, 0x32, "%3.2f",
              app->state.gravityZ);
    SetWindowTextA(GetDlgItem(dlg, panel::kGravityDirZEdit), text);
    SendMessageA(GetDlgItem(dlg, panel::kGravityDirXSlider), TBM_SETPOS, 1,       
        static_cast<LPARAM>(
            static_cast<int>(app->state.gravityX * 100.0)));
    SendMessageA(GetDlgItem(dlg, panel::kGravityDirYSlider), TBM_SETPOS, 1,
        static_cast<LPARAM>(
            static_cast<int>(app->state.gravityY * 100.0)));
    SendMessageA(GetDlgItem(dlg, panel::kGravityDirZSlider), TBM_SETPOS, 1,
        static_cast<LPARAM>(
            static_cast<int>(app->state.gravityZ * 100.0)));
    sprintf_s(text, 0x32, "%d",
              static_cast<int>(
                  app->state.gravityNoise));
    SetWindowTextA(GetDlgItem(dlg, panel::kGravityNoiseEdit), text);             
    const BOOL mode = app->state.gravityNoiseEnabled != 0;
    SendMessageA(GetDlgItem(dlg, panel::kGravityNoiseCheckbox), BM_SETCHECK, mode, 0);
    EnableWindow(GetDlgItem(dlg, panel::kGravityNoiseEdit), mode);               
}

}  

void ApplyGravityTrack(MMDApp* app) {  
    const std::uint32_t frame = app->state.currentFrame;
    mdl::GravityKey* const track = app->GravityKeys();
    if (track == nullptr) return;   

    const auto recFrame = [&track](unsigned int idx) -> std::uint32_t& {
        return track[idx].frame;
    };

    unsigned int idx = 0;
    if (recFrame(0) < frame) {                                  
        for (;;) {
            const unsigned int next =
                track[idx].next;
            if (next == 0) {
                ApplyGravityRecord(app, track[idx]);             
                RefreshPhysicsDialog(app);
                return;
            }
            idx = next;
            if (recFrame(idx) >= frame) break;
        }
    }

    const mdl::GravityKey& rec = track[idx];
    if (recFrame(idx) == frame) {                               
        ApplyGravityRecord(app, rec);
        RefreshPhysicsDialog(app);
        return;
    }

    const mdl::GravityKey& prev = track[rec.previous];
    float num = static_cast<float>(
        static_cast<std::int32_t>(frame - recFrame(rec.previous)));
    if (num < 0) num += kWrap2p32;
    float den = static_cast<float>(
        static_cast<std::int32_t>(recFrame(idx) - recFrame(rec.previous)));
    if (den < 0) den += kWrap2p32;
    const float t = num / den;

    const int dIter =
        rec.noise - prev.noise;
    app->state.gravityNoise =
        static_cast<std::uint32_t>(
            static_cast<int>(t * static_cast<float>(dIter)) + prev.noise);
    app->state.gravityMagnitude =
        (rec.acceleration - prev.acceleration) * t + prev.acceleration;
    app->state.gravityX =
        (rec.direction[0] - prev.direction[0]) * t + prev.direction[0];
    app->state.gravityY =
        (rec.direction[1] - prev.direction[1]) * t + prev.direction[1];
    app->state.gravityZ =
        (rec.direction[2] - prev.direction[2]) * t + prev.direction[2];
    std::uint32_t noiseModeWord;
    std::memcpy(&noiseModeWord, &rec.noiseEnabled, sizeof noiseModeWord);
    app->GravityNoiseModeWord() = noiseModeWord;
    RefreshPhysicsDialog(app);
}


}  
