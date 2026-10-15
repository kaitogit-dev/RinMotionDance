#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>

#include <commdlg.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

struct BoneCopyRecord {
    char name[20];
    float position[3];
    float rotation[4];
};
static_assert(sizeof(BoneCopyRecord) == 0x30,
              "bone copy record ABI");

static std::uint32_t MulOrMax(std::uint32_t a, std::uint32_t b) {
    const std::uint64_t r = static_cast<std::uint64_t>(a) * b;
    return r > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<std::uint32_t>(r);
}

static unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

void ClearFrameSelection(MMDApp* app) {
    app->SceneModified() = 1;
    for (std::size_t i = 0; i < 10000; ++i) {
        app->CameraKeys()[i].selected = 0;
        app->LightKeys()[i].selected = 0;
        app->ShadowKeys()[i].selected = 0;
        app->GravityKeys()[i].selected = 0;
    }
    for (int k = 0; k < 0xFF; ++k) {
        unsigned char* tbl = reinterpret_cast<unsigned char*>(app->AccessoryKeys(k));
        if (tbl != nullptr) {
            for (std::size_t i = 0; i < 0x927C0u; i += 0x3C) {
                *(tbl + i + 0x18) = 0;
            }
        }
    }
}

void ApplySceneLight(MMDApp* app) {
    app->Renderer()->device->SetLight(0, &app->SceneLight());
}

void EnableSceneLight(MMDApp* app) {
    app->Renderer()->device->LightEnable(0, TRUE);
}

static const char kMsgDelAccessoryJp[] =
    "\x83\x41\x83\x4E\x83\x5A\x83\x54\x83\x8A\x81\x46"
    "%s"
    "\x82\xF0\x8D\xED\x8F\x9C\x82\xB5\x82\xDC\x82\xB7\n"
    "\x83\x41\x83\x4E\x83\x5A\x83\x54\x83\x8A\x82\xCC\x83\x74\x83\x8C"
    "\x81\x5B\x83\x80\x83\x66\x81\x5B\x83\x5E\x82\xE0\x91\x53\x82\xC4"
    "\x8D\xED\x8F\x9C\x82\xB3\x82\xEA\x82\xDC\x82\xB7\n"
    "\x28\x82\xB1\x82\xCC\x91\x80\x8D\xEC\x82\xCD\x8C\xB3\x82\xC9"
    "\x96\xDF\x82\xB7\x8E\x96\x82\xCD\x82\xC5\x82\xAB\x82\xDC\x82\xB9"
    "\x82\xF1\x29\n\n"
    "\x8D\xED\x8F\x9C\x82\xB5\x82\xC4\x82\xE0\x82\xE6\x82\xEB\x82\xB5"
    "\x82\xA2\x82\xC5\x82\xB7\x82\xA9\x81\x48";
static const char kCaptionDelAccessoryJp[] =
    "\x83\x41\x83\x4E\x83\x5A\x83\x54\x83\x8A\x8D\xED\x8F\x9C";

static const wchar_t kEmptyPathFormat[] = L"\x0\x0%s%s";

static const wchar_t kFilterAccJp[] =
    L"\x8AAD\x8FBC\x53EF\x80FD\x30D5\x30A1\x30A4\x30EB"
    L"(*.x,*.vac)\x00*.x;*.vac\x00x files(*.x)\x00*.x\x00"
    L"vac files(*.vac)\x00";
static const wchar_t kTitleOpenJp[] = L"\x30D5\x30A1\x30A4\x30EB\x3092\x958B\x304F";

static const char kBoneNameNeedleRight[] = "\x89\x45";
static const char kBoneNameNeedleModelDelete[] =
    "\x83\x82\x83\x66\x83\x8B\x8D\xED\x8F\x9C";

void RefreshRequest(int area);                     
void PanelPaint(MMDApp* app);                      
void SelectionReeval(MMDApp* app);                 
void CopyDirPathW(wchar_t* dest, const wchar_t* src); 

void PushBoneEditUndo(MMDApp* app);                       
void RebuildCameraModePanel(MMDApp* app);                       
void SyncAccessoryEditPanel(MMDApp* app);                       
void RegisterAccessoryKey(MMDApp* app, int frame, int slot);   
void RegisterCameraState(MMDApp* app, int frame);  
void RegisterLightState(MMDApp* app, int frame);   
void CommitEditControl(MMDApp* app, HWND hwnd);            
void DeleteAccessory(mdl::AccessoryRecord* accessory, int flag);  
void IdentityCtor(void* obj);                         
void* ConstructArrayElements(void* block, std::uint32_t elementSize,
                             std::uint32_t count,
                             void* ctor);  

void CmdControl450(MMDApp* app, HWND hwnd, std::uint16_t id,
                   std::uint16_t notify) {
    (void)notify;  
    switch (id) {
    case 451: {
        app->CameraPositionX() = 0.0f;
        app->CameraPositionZ() = 0.0f;
        app->CameraPitch() = 0.0f;
        app->CameraYaw() = 0.0f;
        app->CameraRoll() = 0.0f;
        if (app->CameraParentModel() < 0) {
            app->CameraPositionY() = 10.0f;
            app->CameraDistance() = -45.0f;
        } else {
            app->CameraPositionY() = 0.0f;
            app->CameraDistance() = 0.0f;
        }
        RefreshRequest(-1);
        PostViewRefresh(app);
        break;
    }

    case 453:
    case 454:
    case 469:
    case 470:
    case 488:
    case 489: {
        static const struct {
            int id;
            int flagIdx;
            unsigned char value;  
        } kOptFlagRadios[] = {
            {453, 1, 0}, {454, 1, 1},  
            {469, 2, 0}, {470, 2, 1},  
            {488, 3, 0}, {489, 3, 1},  
        };
        for (const auto& radio : kOptFlagRadios) {
            if (radio.id == id) {
                app->state.optflag[radio.flagIdx] = radio.value;
                break;
            }
        }
        PostModelReload2(app);
        HandleWindowSize(app);
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    }

    case 452: {
        ClearFrameSelection(app);
        RefreshRequest(-1);
        RegisterCameraState(app, app->state.currentFrame);
        PanelPaint(app);
        SelectionReeval(app);
        break;
    }

    case 467: {
        float* const direction = app->LightDirection();
        direction[0] = -0.5f;
        direction[1] = -1.0f;
        direction[2] = 0.5f;
        D3DLIGHT9& light = app->SceneLight();
        light = {};
        light.Type = D3DLIGHT_DIRECTIONAL;
        light.Direction = {direction[0], direction[1], direction[2]};
        light.Diffuse = {};
        const float col = 0.602f;  
        app->LightColor()[0] = col;
        app->LightColor()[1] = col;
        app->LightColor()[2] = col;
        light.Ambient = {col, col, col, 0.0f};
        light.Specular = {col, col, col, 0.0f};
        ApplySceneLight(app);
        EnableSceneLight(app);

        char buf[256];
        for (int ch = 0; ch < 3; ++ch) {
            sprintf_s(buf, 0x100, "%3d",
                      static_cast<int>(static_cast<double>(
                                           app->LightColor()[ch]) *
                                       256.0));  
            SetWindowTextA(GetDlgItem(hwnd, 0x1CD + ch), buf);  
        }
        for (int ax = 0; ax < 3; ++ax) {
            sprintf_s(buf, 0x100, "%+3.1f",
                      static_cast<double>(direction[ax]));
            SetWindowTextA(GetDlgItem(hwnd, 0x1D0 + ax), buf);  
        }
        for (int ch = 0; ch < 3; ++ch) {
            SendMessageA(GetDlgItem(hwnd, panel::kLightColorSliderR + ch), TBM_SETPOS,
                         1, static_cast<LPARAM>(static_cast<int>(
                                static_cast<double>(
                                    app->LightColor()[ch]) *
                                256.0)));  
        }
        for (int ax = 0; ax < 3; ++ax) {
            SendMessageA(GetDlgItem(hwnd, panel::kLightDirSliderX + ax), TBM_SETPOS,
                         1, static_cast<LPARAM>(static_cast<int>(
                                static_cast<double>(
                                    direction[ax]) *
                                100.0)));  
        }
        RefreshRequest(-2);
        break;
    }

    case 468: {
        ClearFrameSelection(app);
        RefreshRequest(-2);
        RegisterLightState(app, app->state.currentFrame);
        PanelPaint(app);
        break;
    }

    case 472: {
        SetCurrentDirectoryW(app->ExeDir());  
        wchar_t fileBuf[0x100];
        swprintf_s(fileBuf, 0x100, kEmptyPathFormat, L"", L"");
        OPENFILENAMEW ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = app->state.floatingWindow != 0
                            ? reinterpret_cast<HWND>(
                                  app->state.floatingWindow)
                            : hwnd;
        if (app->state.englishUI != 0) {
            ofn.lpstrFilter = L"accessory files(*.x,*.vac)\0*.x;*.vac\0";
            ofn.lpstrTitle = L"open file";
        } else {
            ofn.lpstrFilter = kFilterAccJp;
            ofn.lpstrTitle = kTitleOpenJp;
        }
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            ofn.lpstrInitialDir = app->DirAccs();  
        } else {
            ofn.lpstrInitialDir = L"UserFile\\Accessory";
        }
        ofn.lpstrDefExt = L"x";
        wchar_t fileTitle[0x100];
        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.nMaxFileTitle = 0x100;
        ofn.Flags = OFN_FILEMUSTEXIST;
        if (!GetOpenFileNameW(&ofn)) {
            break;
        }
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            wchar_t* dir =
                ExtractDirFromPath(app->PathWorkspace().projectDirectory,
                                   fileBuf);
            CopyDirPathW(app->DirAccs(), dir);
        }
        LoadAccessoryFile(fileBuf);
        break;
    }

    case 473: {
        const LRESULT sel =
            SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo), CB_GETCURSEL, 0, 0);
        std::int32_t found = -1;
        for (int i = 0; i < 0xFF; ++i) {
            mdl::AccessoryRecord* acc = app->AccessorySlot(i);
            if (acc != nullptr &&
                static_cast<int>(acc->order) == static_cast<int>(sel)) {
                found = i;
                break;
            }
        }
        if (found < 0) {
            break;
        }
        app->state.enterKeyState = 1;  
        mdl::AccessoryRecord* acc = app->AccessorySlot(found);

        char text[0x100];
        if (app->state.englishUI != 0) {
            sprintf_s(text, 0x100,
                      "Trying to delete Accessory(%s).\n"
                      "All flame data about this accessory will be deleted "
                      "too.\n(This operation cannot undo!!)\n\nAre you OK?",
                      acc->name);
        } else {
            sprintf_s(text, 0x100, kMsgDelAccessoryJp,
                      acc->name);
        }
        const std::uint32_t flags =
            app->state.floatingWindow != 0 ? (MB_OKCANCEL | MB_TOPMOST) : MB_OKCANCEL;
        const char* caption = app->state.englishUI != 0
                                  ? "delete accessory"
                                  : kCaptionDelAccessoryJp;
        if (MessageBoxA(hwnd, text, caption, flags) != IDOK) {
            break;
        }

        const std::int32_t oldSel = acc->order;
        if (acc != nullptr) {
            DeleteAccessory(acc, 1);
        }
        app->AccessorySlot(found) = nullptr;
        if (app->AccessoryKeys(found) != nullptr) {
            ::operator delete(app->AccessoryKeys(found));
            app->AccessoryKeys(found) = nullptr;
        }
        app->SceneModified() = 1;
        unsigned char* blob = static_cast<unsigned char*>(::operator new(0x927C0));
        app->AccessoryKeys(found) = reinterpret_cast<mdl::AccessoryKey*>(blob);
        memset(blob, 0, 0x927C0);
        blob[0x0C] = 1;
        *reinterpret_cast<std::int32_t*>(blob + 0x10) = -1;
        *reinterpret_cast<float*>(blob + 0x34) = 1.0f;
        *reinterpret_cast<float*>(blob + 0x38) = 1.0f;
        SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo), CB_DELETESTRING,
                     static_cast<WPARAM>(sel), 0);
        SendMessageA(GetDlgItem(hwnd, panel::kRegisterScopeCombo), CB_DELETESTRING,
                     static_cast<WPARAM>(sel + 4), 0);
        SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo), CB_SETCURSEL, 0, 0);
        for (int i = 0; i < 0xFF; ++i) {
            mdl::AccessoryRecord* p = app->AccessorySlot(i);
            if (p != nullptr && static_cast<int>(p->order) > oldSel) {
                --p->order;
            }
        }
        std::int32_t firstFree = 0x100;
        for (int i = 0; i < 0xFF; ++i) {
            mdl::AccessoryRecord* p = app->AccessorySlot(i);
            if (p != nullptr && p->order == 0) {
                firstFree = i;
                break;
            }
        }
        if (firstFree == 0x100) {
            app->state.selectedObjectSlot = 0;
            SendMessageA(GetDlgItem(hwnd, panel::kMainComboGround), CB_SETCURSEL, -1, 0);
        } else {
            SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo), CB_SETCURSEL, 0, 0);
            app->state.selectedObjectSlot =
                static_cast<std::uint8_t>(firstFree);
            SyncAccessoryEditPanel(app);
        }
        SendMessageA(GetDlgItem(hwnd, panel::kAttachBoneCombo), CB_RESETCONTENT, 0, 0);
        char empty[0x100];
        strcpy_s(empty, 0x100, "");
        for (int ctl = 0x1DE; ctl <= 0x1E4; ++ctl) {
            HWND edit = GetDlgItem(hwnd, ctl);
            SendMessageA(edit, EM_SETSEL, 0,
                         static_cast<LPARAM>(GetWindowTextLengthA(edit)));
            SendMessageA(edit, EM_REPLACESEL, 0,
                         reinterpret_cast<LPARAM>(empty));
        }
        EnableMenuItem(GetMenu(hwnd), 0xF9, 1);
        RebuildCameraModePanel(app);
        SendMessageA(GetDlgItem(hwnd, panel::kInterpCurveCombo), CB_DELETESTRING, 5, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kInterpCurveCombo), CB_DELETESTRING, 4, 0);
        PostModelReload2(app);
        app->GlobalTrackSelected(GlobalTimelineTrack::Camera) = 0;
        RefreshRequest(-1);
        InvalidateRect(hwnd, nullptr, FALSE);
        PostLanguageSweep2(app);
        break;
    }

    case 476: {
        const std::uint8_t idx = app->state.selectedObjectSlot;
        mdl::AccessoryRecord* acc = app->AccessorySlot(idx);
        if (acc == nullptr) {
            break;
        }
        if (acc->visible != 0) {
            acc->visible = 0;
            SendMessageA(GetDlgItem(hwnd, panel::kAccessoryVisibleCheckbox), BM_SETCHECK, 0, 0);
        } else {
            acc->visible = 1;
            SendMessageA(GetDlgItem(hwnd, panel::kAccessoryVisibleCheckbox), BM_SETCHECK, 1, 0);
        }
        RefreshRequest(idx);
        break;
    }

    case 477: {
        const std::uint8_t idx = app->state.selectedObjectSlot;
        mdl::AccessoryRecord* acc = app->AccessorySlot(idx);
        if (acc == nullptr) {
            break;
        }
        acc->additiveBlend = acc->additiveBlend == 0 ? 1 : 0;
        break;
    }

    case 486: {
        const std::uint8_t idx = app->state.selectedObjectSlot;
        mdl::AccessoryRecord* acc = app->AccessorySlot(idx);
        if (acc == nullptr) {
            break;
        }
        if (acc->shadowEnabled != 0) {
            acc->shadowEnabled = 0;
            SendMessageA(GetDlgItem(hwnd, panel::kAccessoryShadowCheckbox), BM_SETCHECK, 0, 0);
        } else {
            acc->shadowEnabled = 1;
            SendMessageA(GetDlgItem(hwnd, panel::kAccessoryShadowCheckbox), BM_SETCHECK, 1, 0);
        }
        RefreshRequest(idx);
        break;
    }

    case 487: {
        for (int ctl = 0x1DE; ctl <= 0x1E4; ++ctl) {
            CommitEditControl(app, GetDlgItem(hwnd, ctl));
        }
        app->SceneModified() = 1;
        const std::uint8_t idx = app->state.selectedObjectSlot;
        if (app->AccessorySlot(idx) == nullptr) {
            break;
        }
        ClearFrameSelection(app);
        RefreshRequest(idx);
        RegisterAccessoryKey(app, app->state.currentFrame, idx);
        PanelPaint(app);
        break;
    }

    case 490: {
        SendMessageA(GetDlgItem(hwnd, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneMoveRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneRotateRadio), BM_SETCHECK, 0, 0);
        app->EditMode() = IsDlgButtonChecked(hwnd, panel::kBoneSelectRadio) != 0
            ? ViewportEditMode::Bone : ViewportEditMode::None;
        break;
    }

    case 491: {
        SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneMoveRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneRotateRadio), BM_SETCHECK, 0, 0);
        app->EditMode() = IsDlgButtonChecked(hwnd, panel::kBoxSelectRadio) != 0
            ? ViewportEditMode::BoneBox : ViewportEditMode::None;
        break;
    }

    case 492: {
        SendMessageA(GetDlgItem(hwnd, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneRotateRadio), BM_SETCHECK, 0, 0);
        app->EditMode() = IsDlgButtonChecked(hwnd, panel::kBoneMoveRadio) != 0
            ? ViewportEditMode::Light : ViewportEditMode::None;
        break;
    }

    case 493: {
        SendMessageA(GetDlgItem(hwnd, panel::kBoxSelectRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneMoveRadio), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 0, 0);
        app->EditMode() = IsDlgButtonChecked(hwnd, panel::kBoneRotateRadio) != 0
            ? ViewportEditMode::Camera : ViewportEditMode::None;
        break;
    }

    case 494: {
        unsigned char* model = ActiveModel(app);
        if (model == nullptr) {
            SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 1, 0);
            SendMessageA(hwnd, WM_COMMAND, 0x1EA, 0);
            PostLanguageSweep(app);
            PostLanguageSweep2(app);
            break;
        }
        auto* const modelRecord = rinmotiondance::mdl::Mdl(model);
        const std::int32_t boneCount = modelRecord->boneCount;
        rinmotiondance::mdl::BoneRecord* const bones = modelRecord->boneTable;
        unsigned char* const sel = modelRecord->boneSelection;
        for (std::int32_t i = 0; i < boneCount; ++i) {
            const mdl::BoneType type = bones[i].type;
            const std::uint16_t flag = bones[i].flags;
            const bool hit =
                ((flag & mdl::kBoneFlagFixedAxis) == mdl::kBoneFlagFixedAxis &&
                 type == mdl::BoneType::UnderIk) ||
                type == mdl::BoneType::FixedAxis ||
                type <= mdl::BoneType::Effector;
            if (hit) {
                sel[i] = 1;
            }
        }
        SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 1, 0);
        SendMessageA(hwnd, WM_COMMAND, 0x1EA, 0);
        PostLanguageSweep(app);
        PostLanguageSweep2(app);
        break;
    }

    case 495: {
        PushBoneEditUndo(app);  
        unsigned char* model = ActiveModel(app);
        auto* modelRecord = rinmotiondance::mdl::Mdl(model);
        unsigned char* boneSel = modelRecord->boneSelection;
        if (boneSel[0] != 0) {
            auto& root = modelRecord->boneTable[0];
            root.trans[1] = 0.0f;
            root.rotQuat[0] = 0.0f;
            root.rotQuat[1] = 0.0f;
            root.rotQuat[2] = 0.0f;
            root.rotQuat[3] = 1.0f;
        }
        const std::int32_t boneCount = modelRecord->boneCount;
        unsigned char* frameFlag = modelRecord->bonePhysicsState;
        for (std::int32_t i = 1; i < boneCount; ++i) {
            model = ActiveModel(app);
            modelRecord = rinmotiondance::mdl::Mdl(model);
            boneSel = modelRecord->boneSelection;
            frameFlag = modelRecord->bonePhysicsState;
            if (boneSel[i] == 0) {
                continue;
            }
            rinmotiondance::mdl::BoneRecord* const bones = modelRecord->boneTable;
            rinmotiondance::mdl::BoneRecord* bone = &bones[i];
            if (bone->parent == -1) {
                d3dx::D3DXMATRIXF mat{};
                auto* d3dx = &d3dx::Get();
                {
                    d3dx->matrixRotationQuaternion(
                        &mat, reinterpret_cast<const float*>(bone->rotQuat));
                }
                const double posX =
                    *reinterpret_cast<float*>(bone->position);
                const double posY =
                    bone->position[1];
                const double posZ =
                    bone->position[2];
                double nX = posY * mat.m[1][0];
                nX += posX * mat.m[0][0];
                nX += posZ * mat.m[2][0];
                nX += mat.m[3][0];
                nX -= posX;
                nX += bones[0].trans[0];
                double nY = posY * mat.m[1][1];
                nY += posX * mat.m[0][1];
                nY += posZ * mat.m[2][1];
                nY += mat.m[3][1];
                nY -= posY;
                nY += bones[0].trans[1];
                double nZ = posY * mat.m[1][2];
                nZ += posX * mat.m[0][2];
                nZ += posZ * mat.m[2][2];
                nZ += mat.m[3][2];
                nZ -= posZ;
                nZ += bones[0].trans[2];
                const int f0 =
                    (*reinterpret_cast<float*>(bone->trans) != nX) ? 1 : 0;
                const int f1 =
                    (bone->trans[1] != nY) ? 1 : 0;
                const int f2 =
                    (bone->trans[2] != nZ) ? 1 : 0;
                const int f3 = (*reinterpret_cast<float*>(bone->rotQuat) !=
                                bones[0].rotQuat[0])
                                   ? 1
                                   : 0;
                const int f4 = (bone->rotQuat[1] !=
                                bones[0].rotQuat[1])
                                   ? 1
                                   : 0;
                const int f5 = (bone->rotQuat[2] !=
                                bones[0].rotQuat[2])
                                   ? 1
                                   : 0;
                const int f6 = (bone->rotQuat[3] !=
                                bones[0].rotQuat[3])
                                   ? 1
                                   : 0;
                if ((f0 | f1 | f2 | f3 | f4 | f5 | f6) != 0) {
                    *reinterpret_cast<float*>(bone->trans) =
                        static_cast<float>(nX);
                    bone->trans[1] =
                        static_cast<float>(nY);
                    bone->trans[2] =
                        static_cast<float>(nZ);
                    memcpy(bone->rotQuat, bones[0].rotQuat,
                           sizeof bone->rotQuat);
                    frameFlag[i] = 1;
                }
            } else {
                const int g0 =
                    (*reinterpret_cast<float*>(bone->trans) != 0.0) ? 1 : 0;
                const int g1 =
                    (bone->trans[1] != 0.0) ? 1 : 0;
                const int g2 =
                    (bone->trans[2] != 0.0) ? 1 : 0;
                const int g3 =
                    (*reinterpret_cast<float*>(bone->rotQuat) != 0.0) ? 1 : 0;
                const int g4 =
                    (bone->rotQuat[1] != 0.0) ? 1 : 0;
                const int g5 =
                    (bone->rotQuat[2] != 0.0) ? 1 : 0;
                const int g6 =
                    (bone->rotQuat[3] != 1.0) ? 1 : 0;
                if ((g0 | g1 | g2 | g3 | g4 | g5 | g6) != 0) {
                    float* b = reinterpret_cast<float*>(bone->trans);
                    b[0] = 0.0f;  
                    b[1] = 0.0f;  
                    b[2] = 0.0f;  
                    b[3] = 0.0f;  
                    b[4] = 0.0f;  
                    b[6] = 1.0f;  
                    frameFlag[i] = 1;
                }
            }
        }
        PostViewRefresh(app);
        break;
    }

    case 496: {
        unsigned char* model = ActiveModel(app);
        auto* modelRecord = rinmotiondance::mdl::Mdl(model);
        const std::int32_t boneCount = modelRecord->boneCount;
        std::int32_t count = 0;
        if (boneCount > 0) {
            unsigned char* sel = modelRecord->boneSelection;
            for (std::int32_t i = 0; i < boneCount; ++i) {
                if (sel[i] != 0) {
                    ++count;
                }
            }
        }
        if (count == 0) {
            break;  
        }
        EnableWindow(GetDlgItem(hwnd, panel::kBonePasteButton), TRUE);
        EnableWindow(GetDlgItem(hwnd, panel::kBoneReversePasteButton), TRUE);
        app->state.copiedBoneCount = count;
        auto& records =
            reinterpret_cast<BoneCopyRecord*&>(app->BoneCopyRecords());
        if (records != nullptr) {
            free(records);
            records = nullptr;
        }
        auto* block = static_cast<BoneCopyRecord*>(::operator new(
            MulOrMax(static_cast<std::uint32_t>(count),
                     sizeof(BoneCopyRecord))));
        if (block != nullptr) {
            ConstructArrayElements(block, sizeof(BoneCopyRecord),
                                   static_cast<std::uint32_t>(count),
                                   &IdentityCtor);
        }
        records = block;
        memset(block, 0,
               static_cast<std::size_t>(count) * sizeof(BoneCopyRecord));
        app->state.copiedBoneCount = 0;  
        model = ActiveModel(app);
        modelRecord = rinmotiondance::mdl::Mdl(model);
        const std::int32_t nBones = modelRecord->boneCount;
        rinmotiondance::mdl::BoneRecord* const bones = modelRecord->boneTable;
        unsigned char* sel = modelRecord->boneSelection;
        std::int32_t cursor = 0;
        for (std::int32_t i = 0; i < nBones; ++i) {
            if (sel[i] == 0) {
                continue;
            }
            BoneCopyRecord& rec = records[cursor];
            strcpy_s(rec.name, bones[i].name);
            memcpy(rec.position, bones[i].trans, sizeof rec.position);
            memcpy(rec.rotation, bones[i].rotQuat, sizeof rec.rotation);
            ++cursor;
            app->state.copiedBoneCount = cursor;
        }
        break;
    }

    case 497: {
        if (app->state.copiedBoneCount == 0) {
            break;  
        }
        unsigned char* model = ActiveModel(app);
        auto* modelRecord = rinmotiondance::mdl::Mdl(model);
        const std::int32_t boneCount = modelRecord->boneCount;
        unsigned char* sel = modelRecord->boneSelection;
        for (std::int32_t i = 0; i < boneCount; ++i) {
            sel[i] = 0;
        }
        EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);
        EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);
        modelRecord->undoDirty = 1;
        modelRecord->redoDirty = 0;
        ++modelRecord->undoState[0];
        if (modelRecord->undoState[0] >= 0x1E) {
            modelRecord->undoState[0] = 0;
        }
        const std::int32_t pasteIdx = modelRecord->undoState[0];
        modelRecord->undoState[1] = pasteIdx;
        auto& undo = modelRecord->undoRings[0].slots[pasteIdx];
        undo.operation = 1;
        undo.dirty = app->state.copiedBoneCount;
        unsigned char* pasteBlob =
            reinterpret_cast<unsigned char*>(undo.bonePose);
        if (pasteBlob != nullptr) {
            ::operator delete(pasteBlob);
            undo.bonePose = nullptr;
        }
        const std::int32_t count =
            app->state.copiedBoneCount;
        pasteBlob = static_cast<unsigned char*>(::operator new(
            MulOrMax(static_cast<std::uint32_t>(count), 0x24u)));
        if (pasteBlob != nullptr) {
            ConstructArrayElements(pasteBlob, 0x24,
                                   static_cast<std::uint32_t>(count),
                                   &IdentityCtor);
        }
        undo.bonePose = reinterpret_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
            pasteBlob);
        memset(pasteBlob, 0, static_cast<std::size_t>(count) * 0x24u);

        auto& records =
            reinterpret_cast<BoneCopyRecord*&>(app->BoneCopyRecords());
        for (std::int32_t j = 0; j < count; ++j) {
            model = ActiveModel(app);
            modelRecord = rinmotiondance::mdl::Mdl(model);
            const std::int32_t nBones = modelRecord->boneCount;
            sel = modelRecord->boneSelection;
            rinmotiondance::mdl::BoneRecord* const bones = modelRecord->boneTable;
            unsigned char* frameFlag = modelRecord->bonePhysicsState;
            const BoneCopyRecord& src = records[j];
            std::int32_t found = -1;
            for (std::int32_t k = 0; k < nBones; ++k) {
                if (strcmp(bones[k].name, src.name) == 0) {
                    found = k;
                    break;
                }
            }
            if (found < 0) {
                continue;
            }
            pasteBlob = reinterpret_cast<unsigned char*>(undo.bonePose);
            auto& rec = reinterpret_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
                pasteBlob)[j];
            rinmotiondance::mdl::BoneRecord* bone = &bones[found];
            rec.boneIndex = found;
            sel[found] = 1;
            memcpy(rec.position, bone->trans, sizeof rec.position);
            memcpy(bone->trans, src.position, sizeof bone->trans);
            memcpy(rec.rotation, bone->rotQuat, sizeof rec.rotation);
            memcpy(bone->rotQuat, src.rotation, sizeof bone->rotQuat);
            rec.physicsDisabled = frameFlag[found];
            frameFlag[found] = 1;
            modelRecord->selectedBone = found;
            PostLanguageSweep(app);
            PostLanguageSweep2(app);
        }
        SetFocus(hwnd);
        app->PhysicsResetPending() = 1;
        break;
    }

    case 498: {
        if (app->state.copiedBoneCount == 0) {
            break;  
        }
        unsigned char* model = ActiveModel(app);
        auto* modelRecord = rinmotiondance::mdl::Mdl(model);
        const std::int32_t boneCount = modelRecord->boneCount;
        unsigned char* sel = modelRecord->boneSelection;
        for (std::int32_t i = 0; i < boneCount; ++i) {
            sel[i] = 0;
        }
        EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);
        EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);
        modelRecord->undoDirty = 1;
        modelRecord->redoDirty = 0;
        ++modelRecord->undoState[0];
        if (modelRecord->undoState[0] >= 0x1E) {
            modelRecord->undoState[0] = 0;
        }
        const std::int32_t pasteIdx = modelRecord->undoState[0];
        modelRecord->undoState[1] = pasteIdx;
        auto& undo = modelRecord->undoRings[0].slots[pasteIdx];
        undo.operation = 1;
        undo.dirty = app->state.copiedBoneCount;
        unsigned char* pasteBlob =
            reinterpret_cast<unsigned char*>(undo.bonePose);
        if (pasteBlob != nullptr) {
            ::operator delete(pasteBlob);
            undo.bonePose = nullptr;
        }
        const std::int32_t count =
            app->state.copiedBoneCount;
        pasteBlob = static_cast<unsigned char*>(::operator new(
            MulOrMax(static_cast<std::uint32_t>(count), 0x24u)));
        if (pasteBlob != nullptr) {
            ConstructArrayElements(pasteBlob, 0x24,
                                   static_cast<std::uint32_t>(count),
                                   &IdentityCtor);
        }
        undo.bonePose = reinterpret_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
            pasteBlob);
        memset(pasteBlob, 0, static_cast<std::size_t>(count) * 0x24u);

        auto& records =
            reinterpret_cast<BoneCopyRecord*&>(app->BoneCopyRecords());
        for (std::int32_t j = 0; j < count; ++j) {
            model = ActiveModel(app);
            modelRecord = rinmotiondance::mdl::Mdl(model);
            const std::int32_t nBones = modelRecord->boneCount;
            sel = modelRecord->boneSelection;
            rinmotiondance::mdl::BoneRecord* const bones = modelRecord->boneTable;
            unsigned char* frameFlag = modelRecord->bonePhysicsState;
            const BoneCopyRecord& src = records[j];
            std::int32_t k1 = -1;
            for (std::int32_t k = 0; k < nBones; ++k) {
                if (strcmp(bones[k].name, src.name) == 0) {
                    k1 = k;
                    break;
                }
            }
            if (k1 < 0) {
                continue;  
            }
            std::int32_t target = -1;
            const char* tag1 = strstr(bones[k1].name, kBoneNameNeedleModelDelete);
            if (tag1 != nullptr) {
                const char* suffix = tag1 + 2;
                for (std::int32_t k = 0; k < nBones; ++k) {
                    const char* q = strstr(bones[k].name, kBoneNameNeedleRight);
                    if (q != nullptr && strcmp(q + 2, suffix) == 0) {
                        target = k;
                        break;
                    }
                }
            }
            if (target < 0) {
                const char* suffix = strstr(
                    reinterpret_cast<const char*>(bones) + j, kBoneNameNeedleRight);
                if (suffix != nullptr) {
                    suffix += 2;
                    for (std::int32_t k = 0; k < nBones; ++k) {
                        const char* q = strstr(bones[k].name, kBoneNameNeedleModelDelete);
                        if (q != nullptr && strcmp(q + 2, suffix) == 0) {
                            target = k;
                            break;
                        }
                    }
                }
            }
            if (target < 0) {
                target = k1;  
            }
            pasteBlob = reinterpret_cast<unsigned char*>(undo.bonePose);
            auto& rec = reinterpret_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
                pasteBlob)[j];
            rinmotiondance::mdl::BoneRecord* bone = &bones[target];
            rec.boneIndex = target;
            sel[target] = 1;
            memcpy(rec.position, bone->trans, sizeof rec.position);
            bone->trans[0] = -src.position[0];
            bone->trans[1] = src.position[1];
            bone->trans[2] = src.position[2];
            memcpy(rec.rotation, bone->rotQuat, sizeof rec.rotation);
            bone->rotQuat[0] = src.rotation[0];
            bone->rotQuat[1] = -src.rotation[1];
            bone->rotQuat[2] = -src.rotation[2];
            bone->rotQuat[3] = src.rotation[3];
            rec.physicsDisabled = frameFlag[target];
            frameFlag[target] = 1;
            modelRecord->selectedBone = target;
            PostLanguageSweep(app);
            PostLanguageSweep2(app);
        }
        SetFocus(hwnd);
        app->PhysicsResetPending() = 1;
        break;
    }

    default:
        break;
    }
}

}  
