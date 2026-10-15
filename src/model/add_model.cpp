#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

#include "model_labels.inc"  


}  

void LoadModelFile(MMDApp* app, const wchar_t* path) {      
    HWND hwnd = static_cast<HWND>(app->Hwnd());
    int slot = 0;
    while (app->ModelSlot(slot) != nullptr) {
        ++slot;
        if (slot >= kModelSlotCount) {  
            app->state.enterKeyState = 1;
            char text[256];
            if (app->EnglishUI() == 0) {
                sprintf_s(text, 0x100, kMsgModelLimitJp, kModelSlotCount);
                MessageBoxA(hwnd, text, kTitleAddModelJp, 0);
            } else {
                sprintf_s(text, 0x100, "you cannot add models over %d!",
                          kModelSlotCount);  
                MessageBoxA(hwnd, text, "add model", 0);
            }
            return;
        }
    }

    void* block = operator new(mdl::kSize);   
    if (block == nullptr)                     
        return;
    unsigned char* model = static_cast<unsigned char*>(block);
    app->ModelSlot(slot) = model;
    std::memset(model, 0, mdl::kSize);
    ModelInitDefaults(model);                                 

    if (ModelLoadPMD(model, hwnd, path,
                     app->Renderer(), 1,
                     1,                                    
                     app->EnglishUI(),                     
                     app->Physics(),                      
                     app->PathWorkspace())) {       
        app->SceneModified() = 1;
        const char* name =
            app->EnglishUI() == 0 ? mdl::Mdl(model)->name
                                  : mdl::Mdl(model)->nameEn;
        mdl::Mdl(model)->comboSelIndex = static_cast<std::uint8_t>(SendMessageA(
            GetDlgItem(hwnd, panel::kMainComboModel), CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(name)));
        SendMessageA(GetDlgItem(hwnd, panel::kMainComboGround), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(name));
        SendMessageA(GetDlgItem(hwnd, panel::kMainComboNormal), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(name));
        mdl::Mdl(model)->comboSelIndex2 = mdl::Mdl(model)->comboSelIndex;
        SendMessageA(GetDlgItem(hwnd, panel::kMainComboModel), CB_SETCURSEL,
                     mdl::Mdl(model)->comboSelIndex, 0);
        app->state.mainModelComboSelection =
            mdl::Mdl(model)->comboSelIndex;
        app->SetSelectedModelSlot(static_cast<std::uint8_t>(slot));
        rinmotiondance::mdl::Mdl(model)->displayState = app->state.characterTransparentMode;

        if (app->state.optflag[0] != 0) {
            RebuildModelModePanel(app);                                   
            app->state.optflag[0] = 0;
            PostLanguageSweep(app);                           
            PostModelReload2(app);                            
            HandleWindowSize(app);                            
            InvalidateRect(hwnd, nullptr, FALSE);
        } else {
            PostLanguageSweep(app);                           
        }

        SendMessageA(GetDlgItem(hwnd, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneMoveRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneRotateRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 1, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kShadowCheckbox), BM_SETCHECK, 1, 0);
        app->EditMode() = ViewportEditMode::Bone;

        HMENU menu = GetMenu(hwnd);
        static const UINT kEnable[] = {0xFD, 0xD9, 0xDC, 0xDA, 0xCA, 0xCB,
                                       0xDB, 0xDE, 0xFB, 0xFC, 0x120, 0x121};
        for (UINT item : kEnable)
            EnableMenuItem(menu, item, 0);
        for (UINT j = 224; j <= 231; ++j)
            EnableMenuItem(GetMenu(hwnd), j, 0);
        for (UINT k = 273; k <= 275; ++k)
            EnableMenuItem(GetMenu(hwnd), k, 0);
        static const UINT kDisable[] = {0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2};
        for (UINT item : kDisable)
            EnableMenuItem(GetMenu(hwnd), item, 1);
        EnableWindow(GetDlgItem(hwnd, panel::kExpandShrinkButton), 1);

        MENUITEMINFOA mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;
        unsigned char* curModel = app->SelectedModel();
        mii.fState = mdl::Mdl(curModel)->physicsMode == 2
                         ? (MFS_GRAYED | MFS_DISABLED)
                         : 0;
        SetMenuItemInfoA(GetSubMenu(GetMenu(hwnd), 7), 2, TRUE, &mii);
        DrawMenuBar(hwnd);

        EnableWindow(GetDlgItem(hwnd, panel::kPasteButton), 0);
        if (app->ClipboardCounts().morphs != 0 ||
            app->ClipboardCounts().displays != 0) {
            EnableWindow(GetDlgItem(hwnd, panel::kPasteButton), 1);
            if (app->ClipboardCounts().bones != 0) {
                EnableWindow(GetDlgItem(hwnd, panel::kReversePasteButton), 1);
                PostLanguageSweep2(app);                      
                return;
            }
        } else if (app->ClipboardCounts().bones != 0) {
            EnableWindow(GetDlgItem(hwnd, panel::kPasteButton), 1);
            EnableWindow(GetDlgItem(hwnd, panel::kReversePasteButton), 1);
            PostLanguageSweep2(app);                          
            return;
        }
        EnableWindow(GetDlgItem(hwnd, panel::kReversePasteButton), 0);
        PostLanguageSweep2(app);                          
    } else {
        void* oldModel = app->ModelSlot(slot);
        if (oldModel != nullptr) {
            ModelDispose(static_cast<unsigned char*>(oldModel));    
            ::operator delete(oldModel);
            app->ModelSlot(slot) = nullptr;
        }
    }
}

}  
