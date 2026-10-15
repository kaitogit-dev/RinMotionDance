#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"
#include "pump_input.hpp"

namespace rinmotiondance {

namespace {

enum LetterSlot {
    kSlotX = 0, kSlotZ = 1, kSlotC = 2, kSlotV = 3,
    kSlotD = 4,  kSlotA = 5,  kSlotB = 6,
    kSlotG = 7,  
    kSlotS = 8,  
    kSlotI = 9,  kSlotH = 10, kSlotK = 11, kSlotP = 12, kSlotU = 13,
    kSlotJ = 14, kSlotF = 15, kSlotR = 16, kSlotL = 17,
};

HWND LadderOwner(MMDApp* app) {
    HWND owner = app->FloatingWindow();
    return owner != nullptr ? owner : static_cast<HWND>(app->Hwnd());
}

void SendMenuCommand(MMDApp* app, int id) {
    SendMessageA(static_cast<HWND>(app->Hwnd()), WM_COMMAND, id, 0);
}

}  

namespace {

void PumpDisplayShortcuts(MMDApp* app, const KeyboardInputContext& input) {
    auto& state = app->state;
    const auto pressed = [&](int slot) {
        return app->state.dialogFlags[slot] == 1;
    };
    const HWND main = static_cast<HWND>(app->Hwnd());
    const bool focusInEdit = !input.focusNotInPanelEdit;
    const auto focusOK = [&] { return input.focus == main || app->ViewportInputActive() != 0; };
    const auto shift = [&] { return state.shiftModifierState == 3; };
    const auto ctrl = [&] { return state.ctrlModifierState == 3; };
    const auto playing = [&] { return app->PlaybackActive() != 0; };
    const auto modelMode = [&] { return state.optflag[0] == 0; };

    if (focusOK() && !focusInEdit && pressed(kSlotB)) {
        HMENU menu = GetMenu(main);
        if (state.blackBackgroundEnabled != 0) {
            state.blackBackgroundEnabled = 0;
            CheckMenuItem(menu, 0x11A, MF_UNCHECKED);
        } else {
            state.blackBackgroundEnabled = 1;
            CheckMenuItem(menu, 0x11A, MF_CHECKED);
        }
    }

    if (focusOK() && !focusInEdit && pressed(kSlotV)) {
        if (ctrl() && !playing()) {
            SendMenuCommand(app, 0x1A5);                   
        } else if (shift()) {
            HMENU menu = GetMenu(main);
            if (state.modelNonDisplayMode != 0) {
                state.modelNonDisplayMode = 0;
                CheckMenuItem(menu, 0x11B, MF_UNCHECKED);
            } else {
                state.modelNonDisplayMode = 1;
                CheckMenuItem(menu, 0x11B, MF_CHECKED);
            }
        } else {
            HMENU menu = GetMenu(main);
            state.characterTransparentMode = state.characterTransparentMode != 0 ? 0 : 1;
            CheckMenuItem(menu, 0xD6,
                          state.characterTransparentMode != 0 ? MF_CHECKED : MF_UNCHECKED);
            for (int slot = 0; slot < kModelSlotCount; ++slot) {
                unsigned char* model = app->ModelSlot(slot);
                if (model != nullptr)
                    mdl::Mdl(model)->displayState = state.characterTransparentMode;
            }
        }
    }

    if (focusOK() && !playing() && !focusInEdit && pressed(kSlotC)) {
        if (ctrl()) {
            SendMenuCommand(app, 0x1A4);                   
        } else if (modelMode()) {
            SendMessageA(GetDlgItem(main, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, panel::kBoneMoveRadio), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, panel::kBoneRotateRadio), BM_SETCHECK, 0, 0);
            if (IsDlgButtonChecked(main, panel::kBoneSelectRadio) != 1) {
                app->EditMode() = ViewportEditMode::Bone;
                SendMessageA(GetDlgItem(main, panel::kBoneSelectRadio), BM_SETCHECK, 1, 0);
            } else {
                app->EditMode() = ViewportEditMode::None;
                SendMessageA(GetDlgItem(main, panel::kBoneSelectRadio), BM_SETCHECK, 0, 0);
            }
        }
    }
}

void PumpFrameLineShortcuts(MMDApp* app, const KeyboardInputContext& input) {
    auto& state = app->state;
    const auto pressed = [&](int slot) {
        return app->state.dialogFlags[slot] == 1;
    };
    const HWND main = static_cast<HWND>(app->Hwnd());
    const bool focusInEdit = !input.focusNotInPanelEdit;
    const auto playing = [&] { return app->PlaybackActive() != 0; };
    const auto modelMode = [&] { return state.optflag[0] == 0; };

    if (input.focus == main && modelMode() && !playing() && !focusInEdit &&
        pressed(kSlotH)) {
        SendMenuCommand(app, 0xE5);                        
    }

    if (input.focus == main && !playing() && !focusInEdit && pressed(kSlotI))
        InsertBoneCameraFrameLine(app);                                    

    if (input.focus == main && !playing() && !focusInEdit && pressed(kSlotK))
        DeleteBoneCameraFrameLine(app);                                    

    if (input.focus == main && !playing() && !focusInEdit && pressed(kSlotU))
        InsertFacialLightFrameLine(app);                                    

    if (input.focus == main && !playing() && !focusInEdit && pressed(kSlotJ))
        DeleteFacialLightFrameLine(app);                                    
}

void PumpViewAndPlaybackShortcuts(MMDApp* app, const KeyboardInputContext& input) {
    auto& state = app->state;
    const auto pressed = [&](int slot) {
        return app->state.dialogFlags[slot] == 1;
    };
    const HWND main = static_cast<HWND>(app->Hwnd());
    const bool focusInEdit = !input.focusNotInPanelEdit;
    const auto focusOK = [&] { return input.focus == main || app->ViewportInputActive() != 0; };
    const auto shift = [&] { return state.shiftModifierState == 3; };
    const auto ctrl = [&] { return state.ctrlModifierState == 3; };
    const auto playing = [&] { return app->PlaybackActive() != 0; };
    const auto modelMode = [&] { return state.optflag[0] == 0; };

    if (focusOK() && !focusInEdit && pressed(kSlotG)) {
        if (shift()) {
            app->SelfShadowCompositionEnabled() =
                app->SelfShadowCompositionEnabled() != 0 ? 0 : 1;
        } else if (ctrl()) {
            FineShadowModeNotice(app);                     
        } else {
            char buf[0x100];
            GetWindowTextA(GetDlgItem(LadderOwner(app), panel::kGotoFrameEdit), buf, 0xA);
            long value = std::atol(buf);
            if (value < 0)
                value = 0;
            app->CurrentFrame() = static_cast<std::int32_t>(value);
            RefreshAfterFrameApply(app);                   
            PostViewRefresh(app);                          
            sprintf_s(buf, 0x100u, "%d", app->CurrentFrame());
            SetWindowTextA(GetDlgItem(main, panel::kCurrentFrameEdit), buf);
        }
    }

    if (focusOK() && !focusInEdit && pressed(kSlotL)) {
        ++state.coordinateSystem;
        const int limit = modelMode() ? 2 : 3;
        if (state.coordinateSystem >= limit)
            state.coordinateSystem = 0;
    }

    ConsumeArrowKeyNavigation(app, input);

    if (focusOK() && pressed(kSlotS) && ctrl()) {
        SendMenuCommand(app, 0xCF);                        
        MessageBeep(0x40);
    }

    if (focusOK() && app->FrameStepPlayback() == 0 &&
        !focusInEdit && pressed(kSlotP)) {
        SendMessageA(GetDlgItem(main, panel::kPlayButton), BM_SETCHECK,
                     playing() ? 0 : 1, 0);
        SendMenuCommand(app, 0x198);                       
        SetFocus(main);
    }

    if (focusOK() && app->FrameStepPlayback() == 0 && !focusInEdit &&
        pressed(kSlotF)) {
        SendMenuCommand(app, 0xFA);                        
    }
}

void PumpModelShortcuts(MMDApp* app, const KeyboardInputContext& input) {
    auto& state = app->state;
    const auto pressed = [&](int slot) {
        return app->state.dialogFlags[slot] == 1;
    };
    const HWND main = static_cast<HWND>(app->Hwnd());
    const bool focusInEdit = !input.focusNotInPanelEdit;
    const auto focusOK = [&] { return input.focus == main || app->ViewportInputActive() != 0; };
    const auto ctrl = [&] { return state.ctrlModifierState == 3; };
    const auto playing = [&] { return app->PlaybackActive() != 0; };
    const auto modelMode = [&] { return state.optflag[0] == 0; };

    if (focusOK() && modelMode() && !playing() && !focusInEdit) {
        if (pressed(kSlotX)) {
            unsigned char* model = app->SelectedModel();
            if (ctrl() && model != nullptr &&
                mdl::Mdl(model)->redoDirty != 0) {
                RedoModelEdit(model, app->CurrentFrame());
                auto* record = mdl::Mdl(model);
                if (record->undoState[0] == record->undoState[1]) {
                    EnableWindow(GetDlgItem(main, panel::kRedoButton), FALSE);
                    record->redoDirty = 0;
                }
                EnableWindow(GetDlgItem(main, panel::kUndoButton), TRUE);
                record->undoDirty = 1;
                PanelPaint(app);                               
                SelectionReeval(app);                          
                if (record->undoRings[0].slots[record->undoState[0]].operation
                        != 1) {
                    SeekModelFrame(model, static_cast<int>(app->CurrentFrame()),
                              app->PlaybackPhysicsMode());
                }
            } else if (!ctrl()) {
                SendMessageA(GetDlgItem(main, panel::kBoneSelectRadio), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(main, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(main, panel::kBoneMoveRadio), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(main, panel::kBoneRotateRadio), BM_SETCHECK, 1, 0);
                app->EditMode() = ViewportEditMode::Camera;    
            }
        }

        if (pressed(kSlotZ)) {
            unsigned char* model = app->SelectedModel();
            if (ctrl() && model != nullptr &&
                mdl::Mdl(model)->undoDirty != 0) {
                UndoModelEdit(model, app->CurrentFrame());
                auto* record = mdl::Mdl(model);
                if (record->undoRings[0].slots[record->undoState[0]].operation
                        == 0) {
                    EnableWindow(GetDlgItem(main, panel::kUndoButton), FALSE);
                    record->undoDirty = 0;
                }
                if (record->undoState[0] == record->undoState[1]) {
                    EnableWindow(GetDlgItem(main, panel::kUndoButton), FALSE);
                    record->undoDirty = 0;
                }
                EnableWindow(GetDlgItem(main, panel::kRedoButton), TRUE);
                record->redoDirty = 1;
                PanelPaint(app);                               
                SelectionReeval(app);                          
            } else if (!ctrl()) {
                SendMessageA(GetDlgItem(main, panel::kBoneSelectRadio), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(main, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(main, panel::kBoneRotateRadio), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(main, panel::kBoneMoveRadio), BM_SETCHECK, 1, 0);
                app->EditMode() = ViewportEditMode::Light;     
            }
        }

        PumpModelEnterRegister(app);

        if (pressed(kSlotA))
            SendMenuCommand(app, 0x1EE);                       

        if (pressed(kSlotS) && !ctrl())
            SendMenuCommand(app, 0x1F5);                       

        if (pressed(kSlotD))
            SendMenuCommand(app, 0xDB);                        
    }
}

void PumpFrameDialogShortcut(MMDApp* app, const KeyboardInputContext& input) {
    auto& state = app->state;
    const auto pressed = [&](int slot) {
        return app->state.dialogFlags[slot] == 1;
    };
    const HWND main = static_cast<HWND>(app->Hwnd());
    const bool focusInEdit = !input.focusNotInPanelEdit;
    const auto focusOK = [&] { return input.focus == main || app->ViewportInputActive() != 0; };
    const auto modelMode = [&] { return state.optflag[0] == 0; };

    if (focusOK() && modelMode() && app->FrameStepPlayback() == 0 &&
        !focusInEdit && pressed(kSlotR)) {
        SendMenuCommand(app, 0xFB);                        
    }
}

}  

void ConsumeKeyboardInput(MMDApp* app, const KeyboardInputContext& input) {
    PumpDisplayShortcuts(app, input);
    PumpInterpolationToggle(app, input.focus, input.focusNotInPanelEdit);
    PumpDeleteRebuild(app, input.focus, input.focusNotInPanelEdit);
    PumpFrameLineShortcuts(app, input);
    PumpTabCycle(app, input.focus, input.focusNotInPanelEdit);
    PumpFullscreenKeys(app, input.focus, input.focusNotInPanelEdit);
    PumpViewAndPlaybackShortcuts(app, input);
    PumpGlobalEnterRegister(app, input.focus, input.focusNotInPanelEdit);
    PumpModelShortcuts(app, input);
    PumpEscStop(app, input.focusNotInPanelEdit);
    ConsumeNumpadViewPresets(app, input);
    PumpFrameDialogShortcut(app, input);
}

void ConsumeKeyboardInput(MMDApp* app) {
    KeyboardInputContext input{};
    input.focus = GetFocus();
    input.focusNotInPanelEdit = !PumpPanelFocusChain(app, input.focus);
    ConsumeKeyboardInput(app, input);
}

}  
