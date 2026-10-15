#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void PanelPaint(MMDApp* app);                                   
void TimelineDrawTicks(int frameOffset, int width);             
void SetFrameNormalized(int frame);                             

void RefreshLightPanel(MMDApp* app);                                    
void RefreshSelfShadowPanel(MMDApp* app);                                    
void ApplyGravityTrack(MMDApp* app);                                    
void SyncAccessoryEditPanel(MMDApp* app);                                    
void ApplyAccessoryTrack(MMDApp* app, int idx);                           
void SnapshotPoseBeforeFrameChange(unsigned char* model, int frame);  
void SyncModelEditControls(unsigned char* model);                     
int SeekModelFrame(unsigned char* model, int frame, int physicsMode);  
void WaveRestartAt(void* sub, double v);                       
void AviBgOverlayRefresh(MMDApp* app);                                    

void ApplyModelComboSelection(MMDApp* app);

void HandleLButtonDblClk(MMDApp* app) {
    if (app->FullscreenMode() != 0)                             
        return;

    const HWND hwnd = static_cast<HWND>(app->state.hwnd);  
    app->CameraAttachmentTransformSuppressed() = 0;
    RECT rect;
    GetClientRect(hwnd, &rect);                                 

    if (app->MouseY() > 144 &&
        app->MouseY() < 160 &&
        app->MouseX() <
            app->SidebarWidth() - 18 &&
        app->MouseX() > 100 &&
        app->PlaybackActive() == 0) {

        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model == nullptr)
                continue;
            const std::int32_t flagCount = mdl::Mdl(model)->boneCount;
            if (flagCount <= 0)
                continue;
            unsigned char* ikFlags = mdl::Mdl(model)->bonePhysicsState;
            int idx = 0;
            while (ikFlags[idx] == 0) {
                if (++idx >= flagCount)
                    break;
            }
            if (idx < flagCount) {
                SnapshotPoseBeforeFrameChange(model, app->state.currentFrame);
                for (int j = 0;
                     j < static_cast<int>(mdl::Mdl(model)->boneCount); ++j)
                    mdl::Mdl(model)->bonePhysicsState[j] = 0;
            }
        }

        app->state.currentFrame =
            app->state.timelineStartFrame + (app->MouseX() - 100) / 13;
        const LRESULT textLen = GetWindowTextLengthA(GetDlgItem(hwnd, panel::kCurrentFrameEdit));
        SendMessageA(GetDlgItem(hwnd, panel::kCurrentFrameEdit), EM_SETSEL, 0, textLen);
        char frameText[256];                                    
        sprintf_s(frameText, 0x100u, "%d",
                  app->state.currentFrame);
        SendMessageA(GetDlgItem(hwnd, panel::kCurrentFrameEdit), EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(frameText));

        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model == nullptr)
                continue;
            SeekModelFrame(model,
                      app->state.currentFrame,
                      app->PlaybackPhysicsMode());
            if (i == static_cast<int>(app->SelectedModelSlot())) {
                SyncModelEditControls(model);
            }
        }

        if (app->state.optflag[0] != 0) {  
            ReloadModels(app);                                    
            RefreshLightPanel(app);                                       
            RefreshSelfShadowPanel(app);                                       
            ApplyGravityTrack(app);                                       
            for (int i = 0; i < 255; ++i) {                       
                if (app->AccessorySlot(i) != nullptr)
                    ApplyAccessoryTrack(app, i);                            
            }
            SyncAccessoryEditPanel(app);                                       
        } else if (app->state.followCameraEnabled != 0) {  
            app->ViewOffsetX() = 0.0f;
            app->ViewOffsetY() = 0.0f;
            ReloadModels(app);                                    
            RefreshLightPanel(app);
            RefreshSelfShadowPanel(app);
            ApplyGravityTrack(app);
            for (int i = 0; i < 255; ++i) {
                if (app->AccessorySlot(i) != nullptr)
                    ApplyAccessoryTrack(app, i);
            }
            SyncAccessoryEditPanel(app);
            const std::int32_t sel =
                app->CameraParentModel();
            if (sel >= 0) {
                unsigned char* selModel = app->ModelSlot(sel);
                ModelApplyMorphs(selModel);                       
                SetPhysicsMode(selModel, 0,
                               app->ModelSlots(),
                               app->PlaybackPhysicsMode());                    
            }
            PostModelReload(app);                                 
        } else {
            EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);            
            EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);           
        }

        const std::int32_t pages =
            (app->SidebarWidth() - 84) / 26;
        const std::uint32_t frameU =
            static_cast<std::uint32_t>(app->state.currentFrame);
        if (frameU <= static_cast<std::uint32_t>(pages))          
            app->state.timelineStartFrame = 0;       
        else
            app->state.timelineStartFrame =
                static_cast<std::int32_t>(frameU - static_cast<std::uint32_t>(pages));
        PanelPaint(app);                                          

        if (app->state.waveEnabled != 0) {   
            TimelineDrawTicks(app->state.timelineStartFrame,
                              app->SidebarWidth());
            RECT rc;
            rc.left = 6;
            rc.top = 95;
            rc.right = app->SidebarWidth() - 3;
            rc.bottom = 146;
            InvalidateRect(hwnd, &rc, 0);
            if (app->state.wavPlaysOnFrameMove != 0) {  
                const bool gate =
                    app->state.automaticFrameAdvanceEnabled == 0;  
                app->AudioSeekReady() = 1;  
                if (gate) {
                    SetFrameNormalized(app->FrameNormalization());  
                    const std::int32_t frameMinus1 =
                        app->state.currentFrame - 1;
                    double t = static_cast<double>(static_cast<std::uint32_t>(frameMinus1)) / 30.0;
                    if (t < 0.0)                                  
                        t = 0.0;
                    WaveRestartAt(app->Audio(), t);
                }
            }
        }

        if (app->state.aviBackgroundEnabled == 1)      
            AviBgOverlayRefresh(app);                                       
        app->PhysicsResetPending() = 1;
    }
}

void HandleMouseActivate(MMDApp* app) {
    const HWND hwnd = static_cast<HWND>(app->state.hwnd);  
    RECT rect;
    GetClientRect(hwnd, &rect);
    if (app->PlaybackActive() != 0)
        return;

    const HWND fg = GetForegroundWindow();
    const HWND cached = app->FloatingWindow();  

    if (app->state.optflag[0] != 0) {    
        if (fg == cached || app->MouseY() <= rect.bottom - 158) {
            SendMessageA(GetDlgItem(hwnd, panel::kMainComboModel), CB_SETCURSEL,
                         app->MainModelComboSelection(), 0);     
            ApplyModelComboSelection(app);                                       
            return;
        }
    } else if (app->MouseY() > rect.bottom - 158 &&
               fg != cached) {
        SendMessageA(GetDlgItem(hwnd, panel::kMainComboModel), CB_SETCURSEL, 0, 0);
        ApplyModelComboSelection(app);                                           
        return;
    } else if (app->EditMode() != ViewportEditMode::Bone) {
        SendMessageA(GetDlgItem(hwnd, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneMoveRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneRotateRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 1, 0);
        app->EditMode() = ViewportEditMode::Bone;

        const float scale =
            app->Renderer()->viewScale;  
        const std::int32_t hideLeft =
            app->state.hideLeft;      
        const std::int32_t hideBottom =
            app->state.hideBottom;    
        const std::int32_t winW = app->MouseX();
        const std::int32_t winH = app->MouseY();

        const bool inside =
            winH > static_cast<std::int32_t>(static_cast<double>(hideBottom) -
                                             static_cast<double>(scale) * 80.0) &&
            winW > static_cast<std::int32_t>(static_cast<double>(hideLeft) -
                                             static_cast<double>(scale) * 127.0);
        if ((inside && winW < hideLeft) && winH < hideBottom) {
            POINT pt;
            pt.x = 40;                                            
            pt.y = 150;                                           
            ClientToScreen(hwnd, &pt);
            SetCursorPos(pt.x, pt.y);
            app->state.separateWindowMouseSeen = 1;      
            SetForegroundWindow(hwnd);
        }
    } else {
        SendMessageA(GetDlgItem(hwnd, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneMoveRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneRotateRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 0, 0);
        app->EditMode() = ViewportEditMode::None;

        const float scale =
            app->Renderer()->viewScale;  
        POINT pt;
        pt.x = app->state.hideLeft -
               static_cast<std::int32_t>(static_cast<double>(scale) * 110.0);
        pt.y = app->state.hideBottom -
               static_cast<std::int32_t>(static_cast<double>(scale) * 65.0);
        if (cached != nullptr) {
            ClientToScreen(cached, &pt);
            SetForegroundWindow(cached);
        } else {
            ClientToScreen(hwnd, &pt);
        }
        SetCursorPos(pt.x, pt.y);
        app->state.separateWindowMouseSeen = 1;          
    }
}

}  
