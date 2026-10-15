#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdio>
#include <cstdlib>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void RefreshAfterFrameApply(MMDApp* app);      

void PushBoneEditUndo(MMDApp* app);                   

static const char kJpRecStopText[] =
    "\x98\x5e\x89\xe6\x82\xf0\x92\x86\x92\x66\x82\xb5\x82\xc4\x82\xe0"
    "\x82\xe6\x82\xa2\x82\xc5\x82\xb7\x82\xa9";
static const char kJpRecStopCaption[] =
    "\x98\x5e\x89\xe6\x92\x86\x92\x66\x8a\x6d\x94\x46";

LRESULT CALLBACK RecWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    MMDApp* app = g_Block;
    if (msg == WM_PAINT) {                                       
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg != WM_CLOSE && msg != WM_KEYDOWN)                    
        return DefWindowProcA(hwnd, msg, wp, lp);
    if (msg == WM_KEYDOWN && wp != 27  )
        return 0;
    const bool english = app != nullptr && app->EnglishUI();     
    const int choice =
        english
            ? MessageBoxA(hwnd, "Do you want to stop recording?",
                          "recording", 0x24u)
            : MessageBoxA(hwnd, kJpRecStopText, kJpRecStopCaption,
                    (MB_YESNO | MB_ICONQUESTION));
    if (choice == 6  ) {
        FinishAviRecord(app);                                          
        return 0;
    }
    return 0;
}

void DispatchSeparateWindowCommand(MMDApp* app, unsigned short id) {  
    auto& s = *app;
    HWND main = static_cast<HWND>(s.Hwnd());
    HWND separate = s.FloatingWindow();
    char text[0x100];
    switch (id) {
    case 0x218: {                                                
        if (s.CameraMode() != 0) {
            const std::int32_t sel = s.MainModelComboSelection();
            if (sel != 0) {
                SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_SETCURSEL,
                             static_cast<WPARAM>(sel), 0);
                ApplyModelComboSelection(app);                                  
            } else {
                SendMessageA(main, WM_COMMAND, 0x1B3, 0);        
            }
        } else {
            SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_SETCURSEL, 0, 0);
            ApplyModelComboSelection(app);                                      
        }
        break;
    }
    case 0x219:                                                  
    case 0x21A:
    case 0x21B: {
        const std::size_t axis = id - 0x219;  
        if (s.CameraMode() != 0) {
            s.CameraPosition()[axis] = 0.0f;
            RefreshRequest(-1);                                  
            PostViewRefresh(app);                                
        } else {
            unsigned char* model = s.SelectedModel();
            const std::int32_t bone = rinmotiondance::mdl::Mdl(model)->selectedBone;
            if (bone >= 0) {
                PushBoneEditUndo(app);                                  
                rinmotiondance::mdl::BoneRecord* bones =
                    rinmotiondance::mdl::Bones(model);
                bones[bone].trans[axis] = 0.0f;
                unsigned char* flags =
                    rinmotiondance::mdl::Mdl(model)->bonePhysicsState;
                flags[bone] = 1;
                PostViewRefresh(app);
            } else {
                PostViewRefresh(app);                            
            }
        }
        break;
    }
    case 0x21C:                                                  
    case 0x21D:
    case 0x21E: {
        const int axis = id - 0x21C;  
        if (s.CameraMode() == 0) {
            unsigned char* model = s.SelectedModel();
            const std::int32_t bone = rinmotiondance::mdl::Mdl(model)->selectedBone;
            if (bone >= 0) {
                PushBoneEditUndo(app);
                constexpr float kPiLow = 3.141592025756836f;   
                constexpr float kPiHigh = 3.141594886779785f;  
                float& degX = s.BoneRotationEditDegreesX();
                float& degY = s.BoneRotationEditDegreesY();
                float& degZ = s.BoneRotationEditDegreesZ();
                if (axis == 0) {                                 
                    degX = 0.0f;
                    degY = -degY * kPiLow / 180.0f;
                    degZ = -degZ * kPiLow / 180.0f;
                } else if (axis == 1) {                          
                    degX = degX * kPiHigh / 180.0f;
                    degY = 0.0f;
                    degZ = -degZ * kPiLow / 180.0f;
                } else {                                         
                    degX = degX * kPiHigh / 180.0f;
                    degY = -degY * kPiLow / 180.0f;
                    degZ = 0.0f;
                }
                auto& api = d3dx::Get();
                d3dx::D3DXMATRIXF rz{}, rx{}, ry{}, rot{};
                api.rotZ(&rz, axis == 0 ? degZ
                        : axis == 1 ? degZ : 0.0f);
                api.rotX(&rx, degX);
                api.multiply(&rot, &rz, &rx);
                api.rotY(&ry, degY);
                api.multiply(&rot, &rot, &ry);
                rinmotiondance::mdl::BoneRecord* bones =
                    rinmotiondance::mdl::Bones(model);
                api.quatFromMatrix(
                    reinterpret_cast<float*>(&bones[bone].rotQuat[0]),
                    &rot);                                   
                unsigned char* flags =
                    rinmotiondance::mdl::Mdl(model)->bonePhysicsState;
                flags[bone] = 1;
            }
            PostViewRefresh(app);                                
        } else {
            s.CameraRotation()[axis] = 0.0f;
            RefreshRequest(-1);
            PostViewRefresh(app);
        }
        break;
    }
    case 0x21F:                                                  
        if (s.CameraMode() != 0)
            s.CameraDistance() = 0.0f;
        PostViewRefresh(app);
        break;
    case 0x227: {                                                
        if (s.FpsOverlayEnabled() != 0) {
            s.FpsOverlayEnabled() = 0;
            CheckMenuItem(GetMenu(main), 0xD3, MF_UNCHECKED);
        } else {
            s.FpsOverlayElapsedSeconds() = 0.0f;
            s.FpsOverlayEnabled() = 1;
            s.FpsOverlayFrameCount() = 0;
            CheckMenuItem(GetMenu(main), 0xD3, MF_CHECKED);
        }
        break;
    }
    case 0x228: {                                                
        const bool checked =
            (GetMenuState(GetMenu(main), 0x12B, 0) & MF_CHECKED) != 0;
        CheckMenuItem(GetMenu(main), 0x12B,
                      checked ? MF_UNCHECKED : MF_CHECKED);
        s.FrameVolumeControlEnabled() = checked ? 0 : 1;
        break;
    }
    case 0x229: {                                                
        GetWindowTextA(GetDlgItem(separate, panel::kGotoFrameEdit), text, 10);
        std::int32_t frame = atol(text);
        if (frame < 0)
            frame = 0;
        s.CurrentFrame() = frame;                                
        RefreshAfterFrameApply(app);                             
        PostViewRefresh(app);
        sprintf_s(text, 0x100, "%d",
                  s.CurrentFrame());
        SetWindowTextA(GetDlgItem(main, panel::kCurrentFrameEdit), text);
        break;
    }
    case 0x22B: {                                                
        sprintf_s(text, 0x100, "%d", s.CurrentFrame());
        SetWindowTextA(GetDlgItem(separate, panel::kGotoFrameEdit), text);
        break;
    }
    case 0x22C: {                                                
        const bool on = SendMessageA(GetDlgItem(separate, panel::kSelfShadowCheckbox),
                                     BM_GETCHECK, 0, 0) == 1;
        const std::int32_t value = on ? 4096 : 2048;
        D3DRenderer* sub = s.Renderer();
        sub->renderTargetWidth = value;   
        sub->renderTargetHeight = value;  
        IUnknown* released[] = {sub->hdrTexture,          
                                sub->shadowDepthSurface,  
                                sub->shadowSurface};      
        for (IUnknown* com : released) {
            if (com != nullptr) {
                com->Release();
            }
        }
        sub->hdrTexture = nullptr;
        sub->shadowDepthSurface = nullptr;
        sub->shadowSurface = nullptr;
        break;
    }
    case 0x22D: {                                                
        if (s.GroundGridEnabled() != 0) {
            s.GroundGridEnabled() = 0;
            CheckMenuItem(GetMenu(main), 0xD7, MF_UNCHECKED);
        } else {
            s.GroundGridEnabled() = 1;
            CheckMenuItem(GetMenu(main), 0xD7, MF_CHECKED);
        }
        break;
    }
    default:
        break;
    }
}

}  
