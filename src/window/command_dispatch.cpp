#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>

#include <cstdio>
#include <cstdint>
#include <cstring>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/version.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void CmdFileMenu(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);
void CmdViewMenu(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);
void CmdControl400(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);
void CmdControl450(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);
void CmdControl500(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);

namespace {

constexpr std::size_t kModelComboOrder2D7C = 0x2D7C;


void RepopulateBoneCombo(MMDApp* app, HWND combo, std::int32_t modelSlot) {
    unsigned char* model = app->ModelSlot(modelSlot);
    const mdl::ModelRecord* record = mdl::Mdl(model);
    const mdl::BoneRecord* bones = record->boneTable;
    for (std::int32_t i = 0;
         i < static_cast<std::int32_t>(record->boneCount); ++i) {
        const mdl::BoneType type = bones[i].type;
        if (type == mdl::BoneType::FixedAxis ||
            type < mdl::BoneType::InertTip) {
            const char* name = app->state.englishUI != 0
                                   ? bones[i].nameEn
                                   : bones[i].name;
            SendMessageA(combo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(name));
        }
    }
    SendMessageA(combo, CB_SETCURSEL, 0, 0);
}

std::int32_t FindBoneByName(unsigned char* model, const char* text) {
    const mdl::ModelRecord* record = mdl::Mdl(model);
    const mdl::BoneRecord* bones = record->boneTable;
    for (std::int32_t i = 0;
         i < static_cast<std::int32_t>(record->boneCount); ++i) {
        if (strcmp(bones[i].nameEn, text) == 0 ||
            strcmp(bones[i].name, text) == 0) {
            return i;
        }
    }
    return -1;
}

void MorphRowSelect(MMDApp* app, HWND hwnd, HWND combo, std::size_t lane,
                    int spinId, int textId) {
    char text[0x100];
    const LRESULT sel =
        SendMessageA(combo, CB_GETCURSEL, 0, 0);
    SendMessageA(combo, CB_GETLBTEXT, sel,
                 reinterpret_cast<LPARAM>(text));
    const mdl::ModelRecord* record = mdl::Mdl(app->SelectedModel());
    if (static_cast<std::int32_t>(record->morphCount) <= 0) {
        return;
    }
    const mdl::MorphRecord* morphs = record->morphs;
    std::int32_t index = -1;
    for (std::uint32_t i = 0; i < record->morphCount; ++i) {
        if (strcmp(morphs[i].nameEn, text) == 0 ||
            strcmp(morphs[i].name, text) == 0) {
            index = static_cast<std::int32_t>(i);
            break;
        }
    }
    if (index < 0) {
        return;
    }
    mdl::Mdl(app->SelectedModel())->selectedMorphs[lane] = index;
    const float v = record->morphs[index].value;
    SendMessageA(GetDlgItem(hwnd, spinId), TBM_SETPOS, 1,
                 static_cast<LPARAM>(
                     static_cast<std::int32_t>(v * 100.0)));  
    sprintf_s(text, 0x100u, "%5.4f", static_cast<double>(v));
    SetWindowTextA(GetDlgItem(hwnd, textId), text);
    mdl::ModelRecord* live = mdl::Mdl(app->SelectedModel());
    mdl::FrameGroup* groups = live->displayFrames;
    for (std::uint8_t i = 0; i < live->facialFrameCount; ++i) {
        groups[i].selected = (groups[i].targetIndex == index) ? 1 : 0;
    }
    PostLanguageSweep(app);   
    PostLanguageSweep2(app);  
}

bool DefaultSelChangeChain(MMDApp* app, HWND hwnd, HWND ctrl) {
    if (ctrl == GetDlgItem(hwnd, panel::kIkChainCombo)) {
        const LRESULT sel = SendMessageA(GetDlgItem(hwnd, panel::kIkChainCombo),
                                         CB_GETCURSEL, 0, 0);
        mdl::IkChain* chains = mdl::Mdl(app->SelectedModel())->ikChains;
        if (chains != nullptr) {
            CheckRadioButton(hwnd, 0x1BC, 0x1BD,
                             chains[sel].enabled != 0 ? 0x1BC : 0x1BD);
        }
        SetFocus(hwnd);
        return true;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kAccessoryCombo)) {
        const LRESULT sel = SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo),
                                         CB_GETCURSEL, 0, 0);
        std::int32_t slot = 0;
        bool found = false;
        for (; slot < 0xFF; ++slot) {
            mdl::AccessoryRecord* acc = app->AccessorySlot(slot);
            if (acc != nullptr &&
                static_cast<int>(acc->order) == static_cast<int>(sel)) {
                found = true;
                break;
            }
        }
        if (found) {
            app->GlobalTrackSelected(GlobalTimelineTrack::Camera) = 0;
            app->GlobalTrackSelected(GlobalTimelineTrack::Light) = 0;
            app->GlobalTrackSelected(GlobalTimelineTrack::SelfShadow) = 0;
            app->GlobalTrackSelected(GlobalTimelineTrack::Gravity) = 0;
            for (std::int32_t i = 0; i < 0xFF; ++i) {
                mdl::AccessoryRecord* acc = app->AccessorySlot(i);
                if (acc != nullptr) {
                    acc->rowSelected = 0;
                }
            }
            app->SelectedAccessorySlot() =
                static_cast<std::uint8_t>(slot);
            app->AccessorySlot(slot)->rowSelected = 1;
            SyncAccessoryEditPanel(app);  
        }
        PostLanguageSweep(app);
        PostLanguageSweep2(app);
        SetFocus(hwnd);
        return true;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kMainComboNormal)) {
        const LRESULT sel = SendMessageA(GetDlgItem(hwnd, panel::kMainComboNormal),
                                         CB_GETCURSEL, 0, 0);
        HWND boneCombo = GetDlgItem(hwnd, panel::kBoneRegisterCombo);
        SendMessageA(boneCombo, CB_RESETCONTENT, 0, 0);
        if (sel > 0) {
            std::int32_t slot = 0;
            bool found = false;
            for (; slot < kModelSlotCount; ++slot) {
                unsigned char* model = app->ModelSlot(slot);
                if (model != nullptr &&
                    mdl::Mdl(model)->comboSelIndex ==
                        static_cast<int>(sel)) {
                    found = true;
                    break;
                }
            }
            if (found) {
                if (app->CameraParentModel() < 0) {
                    app->CameraPosition()[0] = 0.0f;
                    app->CameraPosition()[1] = 0.0f;
                    app->CameraPosition()[2] = 0.0f;
                    app->CameraRotation()[0] = 0.0f;
                    app->CameraRotation()[1] = 0.0f;
                    app->CameraRotation()[2] = 0.0f;
                }
                app->CameraParentModel() = slot;
                RepopulateBoneCombo(app, boneCombo, slot);
                app->CameraParentBone() = 0;
            }
        } else {
            const std::int32_t parent = app->CameraParentModel();
            if (parent >= 0) {
                const mdl::BoneRecord& bone =
                    mdl::Mdl(app->ModelSlot(parent))
                        ->boneTable[app->CameraParentBone()];
                const double px = bone.position[0];
                const double py = bone.position[1];
                const double pz = bone.position[2];
                app->CameraPosition()[0] = static_cast<float>(
                    py * bone.matInit[4] + px * bone.matInit[0] +
                    pz * bone.matInit[8] + bone.matInit[12]);
                app->CameraPosition()[1] = static_cast<float>(
                    py * bone.matInit[5] + px * bone.matInit[1] +
                    pz * bone.matInit[9] + bone.matInit[13]);
                app->CameraPosition()[2] = static_cast<float>(
                    py * bone.matInit[6] + px * bone.matInit[2] +
                    pz * bone.matInit[10] + bone.matInit[14]);
                app->CameraDistance() = -20.0f;
                app->CameraRotation()[0] = 0.0f;
                app->CameraRotation()[1] = 0.0f;
                app->CameraRotation()[2] = 0.0f;
            }
            SendMessageA(GetDlgItem(hwnd, panel::kBoneRegisterCombo),
                         CB_RESETCONTENT, 0, 0);
            app->CameraParentModel() = -1;
        }
        RefreshRequest(-1);    
        PostViewRefresh(app);  
        SetFocus(hwnd);
        return true;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kBoneRegisterCombo)) {
        char text[0x100];
        const LRESULT sel = SendMessageA(GetDlgItem(hwnd, panel::kBoneRegisterCombo),
                                         CB_GETCURSEL, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneRegisterCombo), CB_GETLBTEXT, sel,
                     reinterpret_cast<LPARAM>(text));
        const std::int32_t parent = app->CameraParentModel();
        if (parent >= 0) {
            const std::int32_t index =
                FindBoneByName(app->ModelSlot(parent), text);
            if (index >= 0) {
                app->CameraParentBone() = index;
                RefreshRequest(-1);
            }
        }
        SetFocus(hwnd);
        return true;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kMainComboGround)) {
        mdl::AccessoryRecord* acc =
            app->AccessorySlot(app->SelectedAccessorySlot());
        if (acc == nullptr) {
            return true;
        }
        const LRESULT sel = SendMessageA(GetDlgItem(hwnd, panel::kMainComboGround),
                                         CB_GETCURSEL, 0, 0);
        HWND boneCombo = GetDlgItem(hwnd, panel::kAttachBoneCombo);
        SendMessageA(boneCombo, CB_RESETCONTENT, 0, 0);
        if (sel > 0) {
            std::int32_t slot = 0;
            bool found = false;
            for (; slot < kModelSlotCount; ++slot) {
                unsigned char* model = app->ModelSlot(slot);
                if (model != nullptr &&
                    mdl::Mdl(model)->comboSelIndex ==
                        static_cast<int>(sel)) {
                    found = true;
                    break;
                }
            }
            if (found) {
                acc->parentModel = slot;
                RepopulateBoneCombo(app, boneCombo, slot);
                acc->parentBone = 0;
                RefreshRequest(
                    static_cast<int>(app->SelectedAccessorySlot()));
            }
        } else {
            acc->parentModel = -1;
        }
        SetFocus(hwnd);
        return true;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kAttachBoneCombo)) {
        char text[0x100];
        const LRESULT sel = SendMessageA(GetDlgItem(hwnd, panel::kAttachBoneCombo),
                                         CB_GETCURSEL, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kAttachBoneCombo), CB_GETLBTEXT, sel,
                     reinterpret_cast<LPARAM>(text));
        mdl::AccessoryRecord* acc =
            app->AccessorySlot(app->SelectedAccessorySlot());
        if (acc != nullptr && acc->parentModel >= 0) {
            const std::int32_t index =
                FindBoneByName(app->ModelSlot(acc->parentModel), text);
            if (index >= 0) {
                acc->parentBone = index;
                RefreshRequest(
                    static_cast<int>(app->SelectedAccessorySlot()));
            }
        }
        SetFocus(hwnd);
        return true;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kMorphCombo0)) {
        MorphRowSelect(app, hwnd, ctrl, 0, 0x1F9, 0x1FA);
        SetFocus(hwnd);
        return true;
    }
    if (ctrl == GetDlgItem(hwnd, panel::kMorphCombo1)) {
        MorphRowSelect(app, hwnd, ctrl, 1, 0x1FE, 0x1FF);
        SetFocus(hwnd);
        return true;
    }
    if (ctrl == GetDlgItem(hwnd, panel::kMorphCombo2)) {
        MorphRowSelect(app, hwnd, ctrl, 2, 0x203, 0x204);
        SetFocus(hwnd);
        return true;
    }
    if (ctrl == GetDlgItem(hwnd, panel::kMorphCombo3)) {
        MorphRowSelect(app, hwnd, ctrl, 3, 0x208, 0x209);
        SetFocus(hwnd);
        return true;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kInterpCurveCombo)) {
        SelectionReeval(app);
        SetFocus(hwnd);
        return true;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kRegisterScopeCombo)) {
        unsigned char* model = app->SelectedModel();
        if (model != nullptr) {
            const LRESULT sel = SendMessageA(GetDlgItem(hwnd, panel::kRegisterScopeCombo),
                                             CB_GETCURSEL, 0, 0);
            mdl::Mdl(model)->frameRegistrationSelection =
                static_cast<std::int32_t>(sel);
        }
        SetFocus(hwnd);
        return true;
    }

    return false;
}

}  

void CommandDispatch(HWND ctrl, WPARAM wParam) {
    MMDApp* app = g_Block;
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());
    const std::uint16_t id = LOWORD(wParam);
    const std::uint16_t notification = HIWORD(wParam);

    switch (id) {
    case 200:        
        s.state.dialogFlags[0] = 1;
        SendMessageA(hwnd, WM_CLOSE, 0, 0);
        break;

    case 0xC9: {     
        s.state.dialogFlags[9] = 1;
        s.state.enterKeyState = 1;
        char text[256];
#if defined(_M_X64)
#define RMD_ABOUT_ARCH_TAG "(64bitOS Version)"
#else
#define RMD_ABOUT_ARCH_TAG "(DirectX9 Version)"
#endif
        sprintf_s(text, 256,
                  s.EnglishUI() != 0
                      ? "RinMotionDance Ver.%s\n  " RMD_ABOUT_ARCH_TAG "\n\n"
                        "programmed by Yu Higuchi"
                      : "RinMotionDance Ver.%s\n  " RMD_ABOUT_ARCH_TAG "\n\n"
                        "programmed by \x94\xF3\x8C\xFB\x97\x44",
                  kVersion);
#undef RMD_ABOUT_ARCH_TAG
        const HWND owner = s.state.floatingWindow != 0
                               ? reinterpret_cast<HWND>(s.state.floatingWindow)
                               : hwnd;
        MessageBoxA(owner, text, "About", MB_OK);
        break;
    }

    case 0xCA:      
        CmdLoadPose(app);
        break;
    case 0xCB:      
        CmdSavePose(app);
        break;
    case 0xCC:      
        CmdResetState(app);
        break;
    case 0xCD:      
        CmdOpenScene(app);
        break;
    case 0xCE:      
        CmdOpenWave(app);
        break;
    case 0xCF:      
        s.state.dialogFlags[8] = 1;             
        if (app->EnvFileName()[0] != L'\0') {
            SaveSceneFile(app);
        } else {
            CmdSaveScene(app);
        }
        break;
    case 0xD0:      
        CmdSaveScene(app);
        break;
    case 0xD1:      
        CmdLoadMotion(app);
        break;
    case 0xD2:      
        CmdSaveMotion(app);
        break;
    case 0xD3: {    
        s.state.dialogFlags[4] = 1;             
        auto& flag = s.state.fpsOverlayEnabled;
        if (flag != 0) {
            flag = 0;
            CheckMenuItem(GetMenu(hwnd), 0xD3, MF_UNCHECKED);
        } else {
            flag = 1;
            s.state.fpsOverlayElapsedSeconds = 0.0f;   
            s.state.fpsOverlayFrameCount = 0;
            CheckMenuItem(GetMenu(hwnd), 0xD3, MF_CHECKED);
        }
        HWND owner3 = static_cast<HWND>(s.FloatingWindow());
        if (owner3 == nullptr) owner3 = hwnd;
        SendMessageA(GetDlgItem(owner3, panel::kInfoCheckbox), BM_SETCHECK,
                     s.state.fpsOverlayEnabled, 0);
        break;
    }
    case 0xD5:      
        CmdLoadAvi(app);
        break;
    case 0xD6: {    
        auto& flag = s.state.characterTransparentMode;
        flag = flag ? 0 : 1;
        CheckMenuItem(GetMenu(hwnd), 0xD6, flag ? MF_CHECKED : MF_UNCHECKED);
        for (int slot = 0; slot < kModelSlotCount; ++slot) {
            unsigned char* model = s.ModelSlot(slot);
            if (model != nullptr)
                mdl::Mdl(model)->displayState = flag;
        }
        break;
    }
    case 0xD7: {    
        auto& flag = s.state.groundGridEnabled;
        flag = flag ? 0 : 1;
        CheckMenuItem(GetMenu(hwnd), 0xD7, flag ? MF_CHECKED : MF_UNCHECKED);
        HWND owner7 = static_cast<HWND>(s.FloatingWindow());
        if (owner7 == nullptr) owner7 = hwnd;
        SendMessageA(GetDlgItem(owner7, panel::kCoordAxisCheckbox), BM_SETCHECK,
                     flag, 0);
        break;
    }
    case 0xD8: {    
        auto& flag = s.state.aviBackgroundEnabled;
        if (flag == 1) {
            flag = 0;
            CheckMenuItem(GetMenu(hwnd), 0xD8, MF_UNCHECKED);
        } else if (s.state.aviStream != 0) {
            flag = 1;
            CheckMenuItem(GetMenu(hwnd), 0xD8, MF_CHECKED);
        }
        break;
    }
    case 0xD9:      
        SelectFrameGroup(app, 0);
        break;
    case 0xDA:      
        SelectFrameGroup(app, 1);
        break;
    case 0xDC:      
        SelectFrameGroup(app, 2);
        break;

    default:
        if (notification == 1   &&
            DefaultSelChangeChain(app, hwnd, ctrl)) {
            break;
        }
        if (id >= 200 && id <= 250) {
            CmdFileMenu(app, hwnd, id, notification);
        } else if (id >= 251 && id <= 302) {
            CmdViewMenu(app, hwnd, id, notification);
        } else if (id >= 400 && id <= 449) {
            CmdControl400(app, hwnd, id, notification);
        } else if (id >= 450 && id <= 499) {
            CmdControl450(app, hwnd, id, notification);
        } else if (id >= 500 && id <= 567) {
            CmdControl500(app, hwnd, id, notification);
        }
        break;
    }
}

}  
