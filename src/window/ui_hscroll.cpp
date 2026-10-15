#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>

#include <cstdint>
#include <cstdio>
#include <cmath>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

void HandleBoneSlider(MMDApp* app, HWND hwnd, int sliderId,
                      std::size_t lane, int echoId) {
    unsigned char* model = app->SelectedModel();
    const std::int32_t boneIdx = mdl::Mdl(model)->selectedMorphs[lane];
    if (boneIdx < 0 ||
        (boneIdx <= 0 && mdl::Mdl(model)->physicsMode != 2)) {
        return;
    }

    const LRESULT pos = SendMessageA(GetDlgItem(hwnd, sliderId),
                                     TBM_GETPOS, 0, 0);
    const double scaled = static_cast<double>(pos) / 100.0;

    model = app->SelectedModel();
    mdl::MorphRecord* records = mdl::Morphs(model);
    if (records != nullptr) {
        const std::int32_t idx = mdl::Mdl(model)->selectedMorphs[lane];
        records[static_cast<std::size_t>(idx)].value =
            static_cast<float>(scaled);
    }

    char buf[256];
    sprintf_s(buf, 0x100, "%5.4f", scaled);
    SetWindowTextA(GetDlgItem(hwnd, echoId), buf);
}

void EchoEditText(HWND hwnd, int id, const char* text) {
    HWND edit = GetDlgItem(hwnd, id);
    SendMessageA(edit, EM_SETSEL, 0,
                 static_cast<LPARAM>(GetWindowTextLengthA(edit)));
    SendMessageA(edit, EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(text));
}

void FormatLegacyOneDecimal(char* text, std::size_t size, float value) {
    const double magnitude = std::floor(std::fabs(
        static_cast<double>(value)) * 10.0 + 0.5) / 10.0;
    const double rounded = std::signbit(value) ? -magnitude : magnitude;
    sprintf_s(text, size, "%+3.1f", rounded);
}

void ApplySceneLight(MMDApp* app) {
    app->ApplyTimelineLightState();
    app->Renderer()->device->SetLight(0, &app->SceneLight());
}

}  

void RefreshRequest(int area);        
void PanelPaint(MMDApp* app);         
void TimelineDrawTicks(int frameOffset, int width);  

void HandleHScroll(LPARAM lParam, WPARAM wParam) {
    MMDApp* app = g_Block;
    HWND hwnd = static_cast<HWND>(app->Hwnd());               
    const HWND ctrl = reinterpret_cast<HWND>(lParam);         
    const std::uint32_t code = LOWORD(wParam);                

    if (ctrl == GetDlgItem(hwnd, panel::kMorphSlider0)) {
        HandleBoneSlider(app, hwnd, 505, 0, 506);
    } else if (ctrl == GetDlgItem(hwnd, panel::kMorphSlider1)) {
        HandleBoneSlider(app, hwnd, 510, 1, 511);
    } else if (ctrl == GetDlgItem(hwnd, panel::kMorphSlider2)) {
        HandleBoneSlider(app, hwnd, 515, 2, 516);
    } else if (ctrl == GetDlgItem(hwnd, panel::kMorphSlider3)) {
        HandleBoneSlider(app, hwnd, 520, 3, 521);
    }
    else if (ctrl == GetDlgItem(hwnd, panel::kLightColorSliderR)) {
        const LRESULT pos = SendMessageA(GetDlgItem(hwnd, panel::kLightColorSliderR),
                                         TBM_GETPOS, 0, 0);
        const float v =
            static_cast<float>(static_cast<double>(pos) * 0.00390625);
        app->LightColor()[0] = v;
        app->SceneLight().Ambient.r = v;
        char buf[256];
        sprintf_s(buf, 0x100, "%3d",
                  static_cast<int>(static_cast<double>(
                                       app->LightColor()[0]) *
                                   256.0));
        EchoEditText(hwnd, 461, buf);
        RefreshRequest(-2);
    } else if (ctrl == GetDlgItem(hwnd, panel::kLightColorSliderG)) {
        const LRESULT pos = SendMessageA(GetDlgItem(hwnd, panel::kLightColorSliderG),
                                         TBM_GETPOS, 0, 0);
        const float v =
            static_cast<float>(static_cast<double>(pos) * 0.00390625);
        app->LightColor()[1] = v;
        app->SceneLight().Ambient.g = v;
        char buf[256];
        sprintf_s(buf, 0x100, "%3d",
                  static_cast<int>(static_cast<double>(
                                       app->LightColor()[1]) *
                                   256.0));
        EchoEditText(hwnd, 462, buf);
        RefreshRequest(-2);
    } else if (ctrl == GetDlgItem(hwnd, panel::kLightColorSliderB)) {
        const LRESULT pos = SendMessageA(GetDlgItem(hwnd, panel::kLightColorSliderB),
                                         TBM_GETPOS, 0, 0);
        const float v =
            static_cast<float>(static_cast<double>(pos) * 0.00390625);
        app->LightColor()[2] = v;
        app->SceneLight().Ambient.b = v;
        char buf[256];
        sprintf_s(buf, 0x100, "%3d",
                  static_cast<int>(static_cast<double>(
                                       app->LightColor()[2]) *
                                   256.0));
        EchoEditText(hwnd, 463, buf);
        RefreshRequest(-2);
    }
    else if (ctrl == GetDlgItem(hwnd, panel::kLightDirSliderX)) {
        const LRESULT pos = SendMessageA(GetDlgItem(hwnd, panel::kLightDirSliderX),
                                         TBM_GETPOS, 0, 0);
        app->LightDirection()[0] =
            static_cast<float>(static_cast<double>(pos) / 100.0);
        char buf[256];
        FormatLegacyOneDecimal(buf, 0x100, app->LightDirection()[0]);
        EchoEditText(hwnd, 464, buf);
        ApplySceneLight(app);
        RefreshRequest(-2);
    } else if (ctrl == GetDlgItem(hwnd, panel::kLightDirSliderY)) {
        const LRESULT pos = SendMessageA(GetDlgItem(hwnd, panel::kLightDirSliderY),
                                         TBM_GETPOS, 0, 0);
        app->LightDirection()[1] =
            static_cast<float>(static_cast<double>(pos) / 100.0);
        char buf[256];
        FormatLegacyOneDecimal(buf, 0x100, app->LightDirection()[1]);
        EchoEditText(hwnd, 465, buf);
        ApplySceneLight(app);
        RefreshRequest(-2);
    } else if (ctrl == GetDlgItem(hwnd, panel::kLightDirSliderZ)) {
        const LRESULT pos = SendMessageA(GetDlgItem(hwnd, panel::kLightDirSliderZ),
                                         TBM_GETPOS, 0, 0);
        app->LightDirection()[2] =
            static_cast<float>(static_cast<double>(pos) / 100.0);
        char buf[256];
        FormatLegacyOneDecimal(buf, 0x100, app->LightDirection()[2]);
        EchoEditText(hwnd, 466, buf);
        ApplySceneLight(app);
        RefreshRequest(-2);
    }
    else if (ctrl == GetDlgItem(hwnd, panel::kFovSlider)) {
        const LRESULT pos = SendMessageA(GetDlgItem(hwnd, panel::kFovSlider),
                                         TBM_GETPOS, 0, 0);
        app->CameraFov() = static_cast<float>(pos);
        char buf[256];
        sprintf_s(buf, 0x100, "%3d", static_cast<int>(app->CameraFov()));
        EchoEditText(hwnd, 448, buf);

        const float fovRad = static_cast<float>(
            static_cast<double>(app->CameraFov()) * 0.01745329238474369);
        D3DRenderer* locale = app->Renderer();
        const float aspect = locale->aspectRatio;  

        auto* d3dx = &d3dx::Get();
        {
            d3dx::D3DXMATRIXF proj;
            d3dx->perspectiveFovLH(&proj, fovRad, aspect, 1.0f, 100000.0f);
            locale->device->SetTransform(
                D3DTS_PROJECTION,
                reinterpret_cast<const D3DMATRIX*>(&proj));
        }
        RefreshRequest(-1);
    }
    else if (ctrl == GetDlgItem(hwnd, panel::kSelfShadowRangeSlider)) {
        const LRESULT pos = SendMessageA(GetDlgItem(hwnd, panel::kSelfShadowRangeSlider),
                                         TBM_GETPOS, 0, 0);
        app->state.physicsInterval = static_cast<float>(
            static_cast<double>(10000 - static_cast<int>(pos)) / 100000.0);
        char buf[256];
        sprintf_s(buf, 0x100, "%d", static_cast<int>(pos));
        SetWindowTextA(GetDlgItem(hwnd, panel::kSelfShadowRangeEdit), buf);
        app->SceneModified() = 1;
        RefreshRequest(-3);
    }
    else if (ctrl == GetDlgItem(hwnd, panel::kTimelineHScroll)) {
        switch (code) {
        case 0:  
            if (app->FloatingWindow() != nullptr ||
                app->MouseX() <=
                    app->state.hideRight) {
                --app->state.timelineStartFrame;
            } else {
                app->CameraDistance() = static_cast<float>(
                    static_cast<double>(app->CameraDistance()) + 1.0);
            }
            break;
        case 1:  
            if (app->FloatingWindow() != nullptr ||
                app->MouseX() <=
                    app->state.hideRight) {
                ++app->state.timelineStartFrame;
            } else {
                app->CameraDistance() = static_cast<float>(
                    static_cast<double>(app->CameraDistance()) - 1.0);
            }
            break;
        case 2:  
            app->state.timelineStartFrame -=
                app->state.timelineScrollNPage;  
            break;
        case 3:  
            app->state.timelineStartFrame +=
                app->state.timelineScrollNPage;
            break;
        case 5:  
            app->state.timelineStartFrame +=
                static_cast<std::int32_t>(HIWORD(wParam)) -
                app->state.timelineScrollNPos;  
            break;
        default:
            break;
        }
        if (static_cast<std::uint32_t>(app->state.timelineStartFrame) >
            0xFFFEF920u) {
            app->state.timelineStartFrame = 0;
        }
        PanelPaint(app);                                        
        if (app->state.waveEnabled != 0) {
            TimelineDrawTicks(app->state.timelineStartFrame,
                              app->SidebarWidth());
            RECT rc;
            rc.left = 6;
            rc.top = 95;
            rc.right = app->SidebarWidth() - 3;
            rc.bottom = 146;
            InvalidateRect(hwnd, &rc, FALSE);
        }
    }
}

}  
