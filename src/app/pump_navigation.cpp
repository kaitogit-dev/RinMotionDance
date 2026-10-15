#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

#include "pump_input.hpp"

namespace rinmotiondance {

void StepFrame(MMDApp* app, bool forward);  
void SelectPreviousDisplayBone(MMDApp* app);  
void SelectNextDisplayBone(MMDApp* app);      
void SelectPrevEditTarget(MMDApp* app);       
void SelectNextEditTarget(MMDApp* app);       
void JumpNextKeyframe(MMDApp* app);   
void JumpPrevKeyframe(MMDApp* app);   

namespace {

constexpr float kNumpadHalfPi = 1.5707999467849731f;  
constexpr float kNumpadPi = 3.1415920257568359f;      
constexpr float kNumpadDistance = -45.0f;         
constexpr float kNumpadDistance1 = -30.0f;        
constexpr float kNumpadTargetY = 10.0f;           

bool BoneEditPathActive(MMDApp* app) {
    auto& state = app->state;
    return state.slotIdx == state.cameraParentModel &&  
           state.cameraParentModel >= 0 &&
           state.playbackActive == 0 &&                 
           state.followCameraEnabled != 0;                           
}

void EnterBoneEditRefresh(MMDApp* app, bool hold) {
    app->state.viewDirty = hold ? 1 : 0;                 
    app->state.windowLayoutReady = 0;                   
    PostLanguageSweep2(app);                            
    app->state.windowLayoutReady = 1;
}

void ApplyNumpadCameraPreset(MMDApp* app, float pitch, float yaw) {
    app->CameraPitch() = pitch;                         
    app->CameraYaw() = yaw;                             
    app->CameraRoll() = 0.0f;                           
    app->CameraDistance() = kNumpadDistance;            
    RefreshRequest(-1);                                 
}

void ApplyNumpadModelPreset(MMDApp* app, float pitch, float yaw,
                            float distance, bool targetFollowsReference) {
    auto& state = app->state;
    if (state.cameraParentModel >= 0 && state.followCameraEnabled != 0)
        app->CameraAttachmentTransformSuppressed() = 1; 
    app->ViewOffsetX() = 0.0f;                          
    app->CameraDistance() = distance;
    app->ViewOffsetY() =                                
        targetFollowsReference &&
        app->CameraReferenceMode() !=
            CameraAttachmentReference::SelectedBone     
            ? kNumpadTargetY
            : 0.0f;
    app->CameraPitch() = pitch;
    app->CameraYaw() = yaw;
    app->CameraRoll() = 0.0f;
    D3DMATRIX& basis = app->CameraAttachmentBasis();
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            basis.m[row][col] = row == col ? 1.0f : 0.0f;
}

void PanCameraPosition(MMDApp* app, int dx, int dy) {
    const float scale = app->ShiftModifierActive() ? 0.5f        
        : app->CtrlModifierActive() ? 0.005f                     
        : 0.05f;                                                 
    auto& api = d3dx::Get();
    d3dx::D3DXMATRIXF pitch{}, yaw{}, roll{}, rotation{};
    api.rotY(&yaw, app->CameraYaw());
    api.rotX(&pitch, app->CameraPitch());
    api.multiply(&rotation, &pitch, &yaw);
    api.rotZ(&roll, -app->CameraRoll());
    api.multiply(&rotation, &roll, &rotation);

    const float rx = -static_cast<float>(dx);
    const float ry = static_cast<float>(dy);
    app->CameraPositionX() +=
        rx * rotation.m[0][0] * scale + ry * rotation.m[1][0] * scale +
        rotation.m[3][0];
    app->CameraPositionY() +=
        rx * rotation.m[0][1] * scale + ry * rotation.m[1][1] * scale +
        rotation.m[3][1];
    app->CameraPositionZ() -=
        rx * rotation.m[0][2] * scale + ry * rotation.m[1][2] * scale +
        rotation.m[3][2];
}

void PanSelectedCameraKey(MMDApp* app, int dx, int dy) {
    const float pixelScale = app->ShiftModifierActive() ? 2e-4f   
        : app->CtrlModifierActive() ? 2e-6f                       
        : 2e-5f;                                                  
    const float depth = std::fabs(app->CameraPositionZ());
    const float depthFactor = depth <= 0.3f          
        ? 10.0f                                      
        : std::max(depth, 1.0f);                     
    const float fov = app->CameraFov();              
    app->CameraPositionX() -=
        static_cast<float>(dx) * pixelScale * depthFactor * fov;
    app->CameraPositionY() +=
        static_cast<float>(dy) * pixelScale * depthFactor * fov;
}

}  

void ConsumeArrowKeyNavigation(MMDApp* app, const KeyboardInputContext& input) {
    auto& state = app->state;
    const HWND main = static_cast<HWND>(app->Hwnd());
    const bool focusOK =
        input.focus == main || app->ViewportInputActive() != 0;  
    const bool focusInEdit = !input.focusNotInPanelEdit;             
    const bool playing = app->PlaybackActive() != 0;            
    if (!(focusOK && !playing && !focusInEdit))                 
        return;

    const bool ctrl = state.ctrlModifierState == 3;             
    const bool cameraPanel = state.optflag[0] != 0;             
    float& repeat = state.keyRepeatTimer;                               
    constexpr float kRepeatThreshold = 0.4f;  
    const auto repeatDue = [&]() {
        repeat += state.deltaTime;                              
        return repeat > kRepeatThreshold;
    };

    if (state.rightKeyState == 1) {                             
        if (ctrl) {
            JumpNextKeyframe(app);                              
        } else {
            StepFrame(app, true);                               
            repeat = 0.0f;
        }
    }
    if (state.rightKeyState == 3 && !ctrl) {                    
        if (repeatDue()) {
            state.windowLayoutReady = 0;                        
            StepFrame(app, true);     
        }
    }
    if (state.rightKeyState == 2)                               
        state.windowLayoutReady = 1;

    if (state.leftKeyState == 1) {                              
        if (ctrl) {
            JumpPrevKeyframe(app);                              
        } else {
            StepFrame(app, false);                              
            repeat = 0.0f;
        }
    }
    if (state.leftKeyState == 3 && !ctrl) {                     
        if (repeatDue())
            StepFrame(app, false);  
    }

    if (state.upKeyState == 1) {                                
        if (cameraPanel)
            SelectPrevEditTarget(app);       
        else
            SelectPreviousDisplayBone(app);  
        repeat = 0.0f;
    }
    if (state.upKeyState == 3) {                                
        if (repeatDue()) {
            if (cameraPanel)
                SelectPrevEditTarget(app);
            else
                SelectPreviousDisplayBone(app);
        }
    }

    if (state.downKeyState == 1) {                              
        if (cameraPanel)
            SelectNextEditTarget(app);       
        else
            SelectNextDisplayBone(app);      
        repeat = 0.0f;
    }
    if (state.downKeyState == 3) {                              
        if (repeatDue()) {
            if (cameraPanel)
                SelectNextEditTarget(app);
            else
                SelectNextDisplayBone(app);
        }
    }
}

void ConsumeRightButtonDrag(MMDApp* app) {
    auto& state = app->state;
    const bool inputActive = app->ViewportInputActive() != 0;   
    const bool modelMode = state.optflag[0] == 0;               
    const int dx = app->MouseX() - app->PreviousMouseX();       
    const int dy = app->MouseY() - app->PreviousMouseY();
    constexpr float kPanScale = 0.1f;     
    constexpr float kRotateScale = 0.01f; 

    if (state.rightMouseButtonState == 3 && inputActive) {      
        if (modelMode && BoneEditPathActive(app)) {             
            EnterBoneEditRefresh(app,  true);           
            return;                 
        }
        if (modelMode && app->ShiftModifierActive()) {          
            app->ViewOffsetY() = app->ViewOffsetY() +           
                static_cast<float>(dy) * kPanScale;
            app->ViewOffsetX() = app->ViewOffsetX() -           
                static_cast<float>(dx) * kPanScale;
        } else if (app->CtrlModifierActive()) {                 
            app->CameraDistance() = app->CameraDistance() -     
                static_cast<float>(dy) * kPanScale;
        } else {                                                
            app->CameraYaw() = app->CameraYaw() -               
                static_cast<float>(dx) * kRotateScale;
            app->CameraPitch() = app->CameraPitch() -           
                static_cast<float>(dy) * kRotateScale;
        }
        if (!modelMode && state.coordinateSystem != 2)                    
            RefreshRequest(-1);                                 
        PostViewRefresh(app);                                   
        return;
    }

    if (state.rightMouseButtonState == 2 && inputActive) {      
        if (modelMode && BoneEditPathActive(app))
            EnterBoneEditRefresh(app,  false);          
    }
}

void ConsumeMiddleButtonPan(MMDApp* app) {
    auto& state = app->state;
    const bool inputActive = app->ViewportInputActive() != 0;   
    const bool modelMode = state.optflag[0] == 0;               
    const int dx = app->MouseX() - app->PreviousMouseX();
    const int dy = app->MouseY() - app->PreviousMouseY();

    if (state.middleMouseButtonState == 3 && inputActive) {     
        if (modelMode) {                                        
            const float scale = app->ShiftModifierActive() ? 0.5f    
                : app->CtrlModifierActive() ? 0.005f                 
                : 0.05f;                                             
            app->ViewOffsetY() = app->ViewOffsetY() +
                static_cast<float>(dy) * scale;                 
            app->ViewOffsetX() = app->ViewOffsetX() -
                static_cast<float>(dx) * scale;                 
            if (BoneEditPathActive(app))                        
                EnterBoneEditRefresh(app,  true);       
        } else if (app->CameraParentModel() >= 0) {             
            PanSelectedCameraKey(app, dx, dy);                  
        } else {
            PanCameraPosition(app, dx, dy);                     
        }
        PostViewRefresh(app);                                   
        return;
    }

    if (state.middleMouseButtonState == 2 && inputActive) {     
        if (modelMode && BoneEditPathActive(app))
            EnterBoneEditRefresh(app,  false);          
    }
}

void ConsumeNumpadViewPresets(MMDApp* app, const KeyboardInputContext& input) {
    auto& state = app->state;
    const bool gate = app->ViewportInputActive() != 0 &&        
                      app->PlaybackActive() == 0 &&             
                      input.focusNotInPanelEdit;
    if (!gate)                                                  
        return;

    const bool cameraMode = state.optflag[0] != 0;              

    if (state.numpadKeyState[2] == 1) {                         
        if (cameraMode)
            ApplyNumpadCameraPreset(app, 0.0f, 0.0f);
        else if (!BoneEditPathActive(app))                      
            ApplyNumpadModelPreset(app, 0.0f, 0.0f, kNumpadDistance,
true);
        PostViewRefresh(app);                                   
    }

    if (state.numpadKeyState[4] == 1) {                         
        if (cameraMode)
            ApplyNumpadCameraPreset(app, 0.0f, kNumpadHalfPi);
        else if (!BoneEditPathActive(app))
            ApplyNumpadModelPreset(app, 0.0f, kNumpadHalfPi, kNumpadDistance,
true);
        PostViewRefresh(app);
    }

    if (state.numpadKeyState[6] == 1) {                         
        if (cameraMode)
            ApplyNumpadCameraPreset(app, 0.0f, -kNumpadHalfPi);
        else if (!BoneEditPathActive(app))
            ApplyNumpadModelPreset(app, 0.0f, -kNumpadHalfPi, kNumpadDistance,
true);
        PostViewRefresh(app);
    }

    if (state.numpadKeyState[8] == 1) {                         
        if (cameraMode)
            ApplyNumpadCameraPreset(app, 0.0f, kNumpadPi);
        else if (!BoneEditPathActive(app))
            ApplyNumpadModelPreset(app, 0.0f, kNumpadPi, kNumpadDistance,
true);
        PostViewRefresh(app);
    }

    if (state.numpadKeyState[5] == 1) {                         
        if (cameraMode)
            ApplyNumpadCameraPreset(app, -kNumpadHalfPi, 0.0f);
        else if (!BoneEditPathActive(app))
            ApplyNumpadModelPreset(app, -kNumpadHalfPi, 0.0f, kNumpadDistance,
false);
        PostViewRefresh(app);
    }

    if (state.numpadKeyState[0] == 1) {                         
        if (cameraMode) {
            ApplyNumpadCameraPreset(app, kNumpadHalfPi, 0.0f);
        } else if (!BoneEditPathActive(app)) {                  
            app->CameraAttachmentTransformSuppressed() = 0;     
            app->ViewOffsetX() = 0.0f;                          
            app->ViewOffsetY() = 0.0f;                          
            ReloadModels(app);                                  
            PostModelReload(app);                               
        }
        PostViewRefresh(app);
    }

    if (state.numpadKeyState[1] == 1) {                         
        if (cameraMode)
            ApplyNumpadCameraPreset(app, kNumpadHalfPi, 0.0f);
        else if (!BoneEditPathActive(app))
            ApplyNumpadModelPreset(app, kNumpadHalfPi, 0.0f, kNumpadDistance1,
false);
        PostViewRefresh(app);
    }
}

}  
