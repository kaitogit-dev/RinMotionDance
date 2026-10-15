#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"
#include "pump_input.hpp"

namespace rinmotiondance {

namespace {

HWND PanelEditOwner(MMDApp* app) {
    HWND owner = app->FloatingWindow();
    return owner != nullptr ? owner : static_cast<HWND>(app->Hwnd());
}

bool FocusOk(MMDApp* app, HWND focus) {
    return focus == static_cast<HWND>(app->Hwnd()) ||
           app->ViewportInputActive() != 0;
}

}  

bool PumpPanelFocusChain(MMDApp* app, HWND focus) {
    auto& state = app->state;
    const HWND main = static_cast<HWND>(app->Hwnd());
    const HWND owner = PanelEditOwner(app);
    const bool tab = state.tabKeyState == 1;             
    const bool shift = state.shiftModifierState == 3;    
    const bool cameraMode = state.optflag[0] != 0;       
    bool notInEdit = true;                               

    const auto focusIs = [&](HWND on, int id) {
        return GetDlgItem(on, id) == focus;
    };
    const auto moveTo = [&](HWND on, int id, bool selectAll, bool beep) {
        SetFocus(GetDlgItem(on, id));
        if (selectAll) {
            const LRESULT len =
                GetWindowTextLengthA(GetDlgItem(on, id));
            SendMessageA(GetDlgItem(on, id), EM_SETSEL, 0, len);
        }
        if (beep)
            MessageBeep(0);
    };
    const auto arrowsToPanelRoot = [&]() {
        if (state.leftKeyState == 1)
            SetFocus(GetDlgItem(main, 0));
        if (state.rightKeyState == 1)
            SetFocus(GetDlgItem(main, 0));
        if (state.upKeyState == 1)
            SetFocus(GetDlgItem(main, 0));
        if (state.downKeyState == 1)
            SetFocus(GetDlgItem(main, 0));
    };

    if (focusIs(main, 0x199)) {
        if (tab)
            moveTo(main, 0x19A, true, false);
        notInEdit = false;
    }
    if (focusIs(main, 0x19A)) {
        if (tab)
            moveTo(main, 0x199, true, false);
        notInEdit = false;
    }
    if (focusIs(main, 0x198)) {
        if (tab)
            moveTo(main, 0x199, true, true);
        notInEdit = false;
    }
    if (focusIs(main, 0x1A1))
        notInEdit = false;
    if (focusIs(main, 0x1A9)) {
        if (tab)
            moveTo(main, 0x1AA, true, false);
        notInEdit = false;
    }
    if (focusIs(main, 0x1AA)) {
        if (tab)
            moveTo(main, 0x1A9, true, false);
        notInEdit = false;
    }
    if (focusIs(main, 0x19F)) {
        if (tab) {
            if (cameraMode)
                moveTo(main, 0x1A9, true, true);
            else
                moveTo(main, 0x1A8, false, true);
        }
        notInEdit = false;
    }
    if (focusIs(main, 0x1A8)) {
        if (tab)
            moveTo(main, 0x1A9, true, false);
        notInEdit = false;
    }

    for (int id = 0x1CD; id <= 0x1D2; ++id) {
        if (!focusIs(main, id))
            continue;
        if (tab) {
            const int target = shift
                ? (id == 0x1CD ? 0x1D2 : id - 1)
                : (id == 0x1D2 ? 0x1CD : id + 1);
            moveTo(main, target, true, false);
        }
        notInEdit = false;
    }
    for (int id = 0x1DE; id < 0x1E5; ++id) {
        if (!focusIs(main, id))
            continue;
        if (tab) {
            const int target = shift
                ? (id == 0x1DE ? 0x1E5 : id - 1)
                : id + 1;
            moveTo(main, target, true, false);
        }
        notInEdit = false;
    }

    if (focusIs(main, 0x1FA) || focusIs(main, 0x1FF) ||
        focusIs(main, 0x204) || focusIs(main, 0x209) ||
        focusIs(main, 0x231))
        notInEdit = false;

    if (focusIs(main, 0x1E5)) {
        if (tab) {
            if (shift) {
                moveTo(main, 0x1E4, true, false);
            } else {
                moveTo(main, 0x1DE, true, false);
                CommitEditControl(app, GetDlgItem(main, panel::kAccScaleYEdit));     
            }
        }
        notInEdit = false;
    }

    if (focusIs(main, 0x1DA)) {
        if (tab)
            moveTo(main, 0x1DB, false, true);
        arrowsToPanelRoot();
        notInEdit = false;
    }
    if (focusIs(main, 0x1C1)) {
        if (tab)
            moveTo(main, 0x1C2, false, true);
        arrowsToPanelRoot();
        notInEdit = false;
    }
    if (focusIs(main, 0x1DB)) {
        if (tab)
            moveTo(main, 0x1DE, true, true);
        arrowsToPanelRoot();
        notInEdit = false;
    }
    if (focusIs(main, 0x1C2)) {
        if (tab)
            moveTo(main, 0x1C1, false, true);
        arrowsToPanelRoot();
        notInEdit = false;
    }

    const int chainEnd = cameraMode ? 0x226 : 0x225;
    for (int id = 0x220; id <= 0x226; ++id) {
        if (!focusIs(owner, id))
            continue;
        if (tab) {
            const int target = shift
                ? (id == 0x220 ? chainEnd : id - 1)
                : (id == chainEnd ? 0x220 : id + 1);
            moveTo(owner, target, true, false);
        }
        notInEdit = false;
    }
    if (focusIs(owner, 0x22A))
        notInEdit = false;

    return !notInEdit;
}

void PumpInterpolationToggle(MMDApp* app, HWND focus,
                             bool focusNotInPanelEdit) {
    if (!(FocusOk(app, focus) && app->state.keyState221 == 1 &&
          app->PlaybackActive() == 0 && focusNotInPanelEdit))
        return;
    const HWND main = static_cast<HWND>(app->Hwnd());
    const HWND box = GetDlgItem(main, panel::kPhysicsFrameCheckbox);
    const LRESULT checked = SendMessageA(box, BM_GETCHECK, 0, 0);
    SendMessageA(box, BM_SETCHECK, checked == 1 ? 0 : 1, 0);
}

void PumpDeleteRebuild(MMDApp* app, HWND focus, bool focusNotInPanelEdit) {
    const HWND main = static_cast<HWND>(app->Hwnd());
    if (!(focus == main && app->state.deleteKeyState == 1 &&
          app->PlaybackActive() == 0 && focusNotInPanelEdit))
        return;
    const HWND current = GetFocus();                      
    static const int kFrameEdits[] = {0x1AA, 0x1A9, 0x1A1, 0x19A, 0x199};
    for (int id : kFrameEdits) {
        if (GetDlgItem(main, id) == current)
            return;
    }
    DeleteMarkedKeyframes(app);                                       
}

void PumpTabCycle(MMDApp* app, HWND focus, bool focusNotInPanelEdit) {
    const auto cycle = [&](bool cellPressed) {
        if (!cellPressed || !FocusOk(app, focus) ||
            app->PlaybackActive() != 0 || !focusNotInPanelEdit ||
            app->FrameRangeDialog() != nullptr)
            return;
        const HWND main = static_cast<HWND>(app->Hwnd());
        const HWND combo = GetDlgItem(main, panel::kMainComboModel);
        int index;
        if (app->state.shiftModifierState == 3) {
            index = static_cast<int>(
                         SendMessageA(combo, CB_GETCURSEL, 0, 0)) - 1;
            if (index < 0)
                index = static_cast<int>(
                            SendMessageA(combo, CB_GETCOUNT, 0, 0)) - 1;
        } else {
            index = static_cast<int>(
                         SendMessageA(combo, CB_GETCURSEL, 0, 0)) + 1;
            if (index >= static_cast<int>(
                             SendMessageA(combo, CB_GETCOUNT, 0, 0)))
                index = 0;
        }
        SendMessageA(combo, CB_SETCURSEL, index, 0);
        ApplyModelComboSelection(app);                                   
    };
    cycle(app->state.tabKeyState == 1);                   
    cycle(app->state.keyState226 == 1);                   
}

void PumpFullscreenKeys(MMDApp* app, HWND  ,
                        bool focusNotInPanelEdit) {
    auto& state = app->state;
    if (app->FrameStepPlayback() == 0 &&
        app->ViewportInputActive() != 0 &&
        focusNotInPanelEdit &&
        state.menuKeyState == 3 &&                         
        state.enterKeyState == 1) {                                   
        app->FullscreenMode() =
            app->FullscreenMode() == 0 ? 1 : 0;            
        ApplyFullscreenWindowState(app);                                    
        PostDeviceReset(app);                              
    }
    if (app->FullscreenMode() != 0 &&
        app->ViewportInputActive() != 0 &&
        state.escKeyState == 1 &&
        focusNotInPanelEdit) {
        app->FullscreenMode() = 0;                         
        ApplyFullscreenWindowState(app);                                    
        PostDeviceReset(app);                              
    }
}

void PumpGlobalEnterRegister(MMDApp* app, HWND focus,
                            bool focusNotInPanelEdit) {
    auto& state = app->state;
    const bool enterPressed = state.enterKeyState == 1;               
    const bool altHeld = state.menuKeyState == 3;          

    if (FocusOk(app, focus) && app->PlaybackActive() == 0 &&
        state.optflag[0] != 0 && focusNotInPanelEdit &&
        enterPressed && !altHeld) {
        app->SceneModified() = 1;                          
        for (std::size_t i = 0; i < mdl::kTimelineKeyCapacity; ++i) {
            app->CameraKeys()[i].selected = 0;             
            app->LightKeys()[i].selected = 0;              
            app->ShadowKeys()[i].selected = 0;             
            app->GravityKeys()[i].selected = 0;            
        }
        for (int slot = 0; slot < 0xFF; ++slot)
            for (std::size_t i = 0; i < mdl::kTimelineKeyCapacity; ++i)
                app->AccessoryKeys(slot)[i].selected = 0;  

        const std::int32_t frame = app->CurrentFrame();    
        if (app->GlobalTrackSelected(GlobalTimelineTrack::Camera) != 0)
            RegisterCameraState(app, frame);               
        if (app->GlobalTrackSelected(GlobalTimelineTrack::Light) != 0)
            RegisterLightState(app, frame);                
        if (app->GlobalTrackSelected(GlobalTimelineTrack::SelfShadow) != 0)
            RegisterSelfShadowState(app, frame);           
        if (app->GlobalTrackSelected(GlobalTimelineTrack::Gravity) != 0)
            RegisterGravityKeyCurrent(app, frame);         
        for (int slot = 0; slot < 0xFF; ++slot) {
            mdl::AccessoryRecord* accessory = app->AccessorySlot(slot);
            if (accessory != nullptr && accessory->rowSelected != 0)
                RegisterAccessoryKey(app, frame, slot);               
        }
        PanelPaint(app);                                   
        SelectionReeval(app);                              
    }

}

void PumpModelEnterRegister(MMDApp* app) {
    if (app->state.enterKeyState == 1 && app->state.menuKeyState != 3)
        SendMessageA(static_cast<HWND>(app->Hwnd()), WM_COMMAND, 0x1F4, 0);
}

void PumpEscStop(MMDApp* app, bool focusNotInPanelEdit) {
    if (app->ViewportInputActive() != 0 &&
        app->FrameStepPlayback() != 0 &&
        app->state.escKeyState == 1 &&
        focusNotInPanelEdit)
        FinishAviRecord(app);                                    
}

}  
