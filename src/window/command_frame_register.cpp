#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <commctrl.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void PanelPaint(MMDApp* app);                                   
void RefreshRequest(int area);                                  
void SelectionReeval(MMDApp* app);                              
void RefreshLightPanel(MMDApp* app);                                    
void RefreshSelfShadowPanel(MMDApp* app);                                    
void ApplyGravityTrack(MMDApp* app);                             
void ApplyAccessoryTrack(MMDApp* app, int slot);                  
void SyncAccessoryEditPanel(MMDApp* app);                                    
void ReloadModels(MMDApp* app);                    

void JumpPrevKeyframe(MMDApp* app);
void ApplyModelComboSelection(MMDApp* app);
void JumpNextKeyframe(MMDApp* app);
void ApplyCameraReferenceModeChange(MMDApp* app, int oldMode);
void PushBoneEditUndo(MMDApp* app);
void RegisterSelectedBoneKeys(unsigned char* model, int frame, int mode);
void RegisterMorphKeyCurrent(unsigned char* model, int idx, int frame);
void RefreshAfterFrameApply(MMDApp* app);  
void RegisterSelfShadowState(MMDApp* app, int frame);

namespace {

static unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

void SyncMorphRow(MMDApp* app, HWND hwnd, std::size_t lane, int comboId,
                  int spinId, int textId, bool next) {
    const LRESULT count =
        SendMessageA(GetDlgItem(hwnd, comboId), CB_GETCOUNT, 0, 0);
    const LRESULT cur =
        SendMessageA(GetDlgItem(hwnd, comboId), CB_GETCURSEL, 0, 0);
    if (!next) {
        SendMessageA(GetDlgItem(hwnd, comboId), CB_SETCURSEL,
                     (cur == 0) ? count - 1 : cur - 1, 0);
    } else {
        SendMessageA(GetDlgItem(hwnd, comboId), CB_SETCURSEL,
                     (cur == count - 1) ? 0 : cur + 1, 0);
    }
    const mdl::ModelRecord* model = mdl::Mdl(ActiveModel(app));
    const std::int32_t idx = model->selectedMorphs[lane];
    const float v = model->morphs[idx].value;
    SendMessageA(GetDlgItem(hwnd, spinId), TBM_SETPOS, 1,
                 static_cast<LPARAM>(static_cast<std::int32_t>(v * 100.0)));
    char buf[0x100];
    sprintf_s(buf, 0x100u, "%5.4f", static_cast<double>(v));
    SetWindowTextA(GetDlgItem(hwnd, textId), buf);
}

void MorphStep(MMDApp* app, HWND hwnd, std::size_t lane, int comboId,
               int spinId, int textId, bool next) {
    unsigned char* model = ActiveModel(app);
    mdl::ModelRecord* modelRecord = mdl::Mdl(model);
    std::int32_t idx = modelRecord->selectedMorphs[lane];
    if (idx < 0)
        return;
    if (modelRecord->physicsMode != 2 && idx == 0)
        return;
    if (modelRecord->morphs == nullptr)
        return;
    const std::int32_t orig = idx;
    for (;;) {
        if (next) {
            ++idx;
            if (idx >= static_cast<std::int32_t>(
                           mdl::Mdl(ActiveModel(app))->morphCount))
                idx = 0;
        } else {
            --idx;
            if (idx < 0)
                idx = static_cast<std::int32_t>(
                          mdl::Mdl(ActiveModel(app))->morphCount) - 1;
        }
        unsigned char* m = ActiveModel(app);
        mdl::Mdl(m)->selectedMorphs[lane] = idx;
        m = ActiveModel(app);
        idx = mdl::Mdl(m)->selectedMorphs[lane];
        if (idx == orig)
            return;
        if (mdl::Mdl(m)->morphs[idx].panel == mdl::MorphPanel::eyebrow)
            break;
    }
    SyncMorphRow(app, hwnd, lane, comboId, spinId, textId, next);
}

void BonePosEdit(MMDApp* app, float* vec, std::size_t axis) {
    if (app->state.optflag[0] != 0) {
        vec[axis] = 0.0f;
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    const std::int32_t sel = mdl::Mdl(ActiveModel(app))->selectedBone;
    if (sel >= 0) {
        PushBoneEditUndo(app);   
        unsigned char* m = ActiveModel(app);
        rinmotiondance::mdl::Bones(m)[sel].trans[axis] = 0.0f;
        m = ActiveModel(app);
        mdl::Mdl(m)->bonePhysicsState[sel] = 1;
    }
    PostViewRefresh(app);
}

void BoneRotEdit(MMDApp* app, float* vec, int axis) {
    if (app->state.optflag[0] != 0) {
        vec[axis] = 0.0f;
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    const std::int32_t sel = mdl::Mdl(ActiveModel(app))->selectedBone;
    if (sel < 0) {
        PostViewRefresh(app);
        return;
    }
    PushBoneEditUndo(app);   
    const float rX = app->BoneRotationEditDegreesX();   
    const float rY = app->BoneRotationEditDegreesY();   
    const float rZ = app->BoneRotationEditDegreesZ();   
    constexpr double kPi = 3.1415926535897931;    
    constexpr double kPiF = 3.1415927410125732;   
    switch (axis) {
    case 0:  
        app->BoneRotationEditDegreesX() = 0.0f;
        app->BoneRotationEditDegreesY() =
            static_cast<float>(-(double)rY * kPi / 180.0);
        app->BoneRotationEditDegreesZ() =
            static_cast<float>(-(double)rZ * kPi / 180.0);
        break;
    case 1:  
        app->BoneRotationEditDegreesX() =
            static_cast<float>((double)rX * kPiF / 180.0);
        app->BoneRotationEditDegreesY() = 0.0f;
        app->BoneRotationEditDegreesZ() =
            static_cast<float>(-(double)rZ * kPi / 180.0);
        break;
    default:  
        app->BoneRotationEditDegreesX() =
            static_cast<float>((double)rX * kPiF / 180.0);
        app->BoneRotationEditDegreesY() =
            static_cast<float>(-(double)rY * kPi / 180.0);
        app->BoneRotationEditDegreesZ() = 0.0f;
        break;
    }
    auto* d3dx = &d3dx::Get();
    {
        d3dx::D3DXMATRIXF m, m2;
        d3dx->rotZ(&m, app->BoneRotationEditDegreesZ());
        d3dx->rotX(&m2, app->BoneRotationEditDegreesX());
        d3dx->multiply(&m, &m, &m2);
        d3dx->rotY(&m2, app->BoneRotationEditDegreesY());
        d3dx->multiply(&m, &m, &m2);
        unsigned char* model = ActiveModel(app);
        d3dx->quatFromMatrix(mdl::Mdl(model)->boneTable[sel].rotQuat, &m);
    }
    unsigned char* model = ActiveModel(app);
    mdl::Mdl(model)->bonePhysicsState[sel] = 1;
    PostViewRefresh(app);
}

void OptionFlagRefresh(MMDApp* app, HWND hwnd) {
    PostModelReload2(app);   
    HandleWindowSize(app);   
    InvalidateRect(hwnd, nullptr, FALSE);
}

}  

void CmdControl500(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify) {
    (void)notify;  
    switch (id) {

    case 500: {
        if (ActiveModel(app) == nullptr) {
            break;
        }
        app->SceneModified() = 1;
        RegisterSelectedBoneKeys(
            ActiveModel(app),
            app->state.currentFrame,
            app->PlaybackPhysicsMode());
        const std::int32_t frames =
            static_cast<std::int32_t>(mdl::Mdl(ActiveModel(app))->maxFrame);
        if (app->state.lastRegisteredFrame < frames)
            app->state.lastRegisteredFrame = frames;
        PanelPaint(app);              
        SelectionReeval(app);         
        break;
    }

    case 501: {
        if (ActiveModel(app) == nullptr) {
            break;
        }
        SendMessageA(GetDlgItem(hwnd, panel::kBoneSelectRadio), BM_SETCHECK, 1, 0);
        SendMessageA(hwnd, WM_COMMAND, 0x1EA, 0);
        mdl::Mdl(ActiveModel(app))->selectedBone = -1;
        const std::int32_t boneCount = mdl::Mdl(ActiveModel(app))->boneCount;
        for (int i = 0; i < boneCount; ++i) {
            unsigned char* m = ActiveModel(app);
            unsigned char* flags = mdl::Mdl(m)->bonePhysicsState;
            unsigned char* sel = mdl::Mdl(m)->boneSelection;
            sel[i] = (flags[i] != 0) ? 1 : 0;
        }
        PostLanguageSweep(app);       
        PostLanguageSweep2(app);      
        break;
    }

    case 502:  
        app->state.optflag[4] = 0;
        OptionFlagRefresh(app, hwnd);
        break;
    case 503:  
        app->state.optflag[4] = 1;
        OptionFlagRefresh(app, hwnd);
        break;


    case 507:  
        MorphStep(app, hwnd, 0, 0x1F8, 0x1F9, 0x1FA, false);
        break;
    case 508:  
        MorphStep(app, hwnd, 0, 0x1F8, 0x1F9, 0x1FA, true);
        break;


    case 512:  
        MorphStep(app, hwnd, 1, 0x1FD, 0x1FE, 0x1FF, false);
        break;
    case 513:  
        MorphStep(app, hwnd, 1, 0x1FD, 0x1FE, 0x1FF, true);
        break;


    case 517:  
        MorphStep(app, hwnd, 2, 0x202, 0x203, 0x204, false);
        break;
    case 518:  
        MorphStep(app, hwnd, 2, 0x202, 0x203, 0x204, true);
        break;


    case 522:  
        MorphStep(app, hwnd, 3, 0x207, 0x208, 0x209, false);
        break;
    case 523:  
        MorphStep(app, hwnd, 3, 0x207, 0x208, 0x209, true);
        break;

    case 524:  
    case 525:  
    case 526:  
    case 527:  
    {
        std::size_t lane = 3;
        switch (id) {
        case 525: lane = 0; break;
        case 524: lane = 1; break;
        case 527: lane = 2; break;
        default: break;
        }
        app->SceneModified() = 1;
        {
            unsigned char* m = ActiveModel(app);
            mdl::BoneKey* boneKeys = mdl::BoneKeys(m);
            for (std::size_t i = 0; i < mdl::kBoneKeyCapacity; ++i)
                boneKeys[i].allocated = 0;
        }
        {
            unsigned char* m = ActiveModel(app);
            mdl::MorphKey* morphKeys = mdl::MorphKeys(m);
            for (std::size_t i = 0; i < mdl::kMorphKeyCapacity; ++i)
                morphKeys[i].allocated = 0;
        }
        {
            unsigned char* m = ActiveModel(app);
            mdl::DisplayKey* displayKeys = mdl::DisplayKeys(m);
            for (std::size_t i = 0; i < mdl::kDisplayKeyCapacity; ++i)
                displayKeys[i].allocated = 0;
        }
        unsigned char* m = ActiveModel(app);
        RegisterMorphKeyCurrent(
            m,
            mdl::Mdl(m)->selectedMorphs[lane],
            app->state.currentFrame);
        const std::int32_t frames =
            static_cast<std::int32_t>(mdl::Mdl(ActiveModel(app))->maxFrame);
        if (app->state.lastRegisteredFrame < frames)
            app->state.lastRegisteredFrame = frames;
        PanelPaint(app);
        break;
    }

    case 528:  
        app->state.optflag[5] = 0;
        OptionFlagRefresh(app, hwnd);
        break;
    case 529:  
        app->state.optflag[5] = 1;
        OptionFlagRefresh(app, hwnd);
        break;

    case 530:
        break;

    case 531: {  
        if (SendMessageA(GetDlgItem(hwnd, panel::kCameraRefBoneCheckbox), BM_GETCHECK, 0, 0) == 1) {
            const int oldMode = static_cast<int>(app->CameraReferenceMode());
            app->CameraReferenceMode() =
                CameraAttachmentReference::SelectedBone;
            SendMessageA(GetDlgItem(hwnd, panel::kCameraRefModelCheckbox), BM_SETCHECK, 0, 0);
            if (app->state.optflag[0] == 0)
                ApplyCameraReferenceModeChange(app, oldMode);
        } else {
            const int oldMode = static_cast<int>(app->CameraReferenceMode());
            app->CameraReferenceMode() = CameraAttachmentReference::None;
            if (app->state.optflag[0] == 0)
                ApplyCameraReferenceModeChange(app, oldMode);
        }
        break;
    }

    case 532:  
        JumpNextKeyframe(app);   
        break;
    case 533:  
        JumpPrevKeyframe(app);   
        break;

    case 534:
        break;

    case 535: {  
        if (SendMessageA(GetDlgItem(hwnd, panel::kFollowCameraCheckbox), BM_GETCHECK, 0, 0) == 1) {
            app->state.followCameraEnabled = 1;
            app->ViewOffsetX() = 0.0f;
            app->ViewOffsetY() = 0.0f;
            CheckMenuItem(GetMenu(hwnd), 0xF7, MF_CHECKED);
            ReloadModels(app);   
            RefreshLightPanel(app);
            RefreshSelfShadowPanel(app);
            ApplyGravityTrack(app);
            for (int i = 0; i < 0xFF; ++i) {
                if (app->ObjectSlot(i) != nullptr)
                    ApplyAccessoryTrack(app, i);
            }
            SyncAccessoryEditPanel(app);
            if (app->state.optflag[0] == 0) {
                app->CameraAttachmentTransformSuppressed() = 0;
                PostModelReload(app);  
            }
            PostLanguageSweep2(app);
        } else {
            app->state.followCameraEnabled = 0;
            CheckMenuItem(GetMenu(hwnd), 0xF7, MF_UNCHECKED);
            if (app->state.optflag[0] == 0) {
                const std::int32_t sel = app->CameraParentModel();
                if (sel == app->SelectedModelSlot() &&
                    sel >= 0) {
                    app->CameraAttachmentTransformSuppressed() = 0;
                    PostModelReload(app);
                }
            }
            PostLanguageSweep2(app);
        }
        break;
    }

    case 536: {  
        if (app->state.optflag[0] != 0) {
            const std::int32_t sel =
                app->state.mainModelComboSelection;
            if (sel == 0) {
                SendMessageA(hwnd, WM_COMMAND, 0x1B3, 0);
                break;
            }
            SendMessageA(GetDlgItem(hwnd, panel::kMainComboModel), CB_SETCURSEL, sel, 0);
            ApplyModelComboSelection(app);
        } else {
            SendMessageA(GetDlgItem(hwnd, panel::kMainComboModel), CB_SETCURSEL, 0, 0);
            ApplyModelComboSelection(app);
        }
        break;
    }

    case 537:  
        BonePosEdit(app, app->CameraPosition(), 0);
        break;
    case 538:  
        BonePosEdit(app, app->CameraPosition(), 1);
        break;
    case 539:  
        BonePosEdit(app, app->CameraPosition(), 2);
        break;

    case 540:  
        BoneRotEdit(app, app->CameraRotation(), 0);
        break;
    case 541:  
        BoneRotEdit(app, app->CameraRotation(), 1);
        break;
    case 542:  
        BoneRotEdit(app, app->CameraRotation(), 2);
        break;

    case 543: {  
        if (app->state.optflag[0] != 0) {
            app->CameraDistance() = 0.0f;
            RefreshRequest(-1);
        }
        PostViewRefresh(app);
        break;
    }

    case 544:
    case 545:
    case 546:
    case 547:
    case 548:
    case 549:
    case 550:
        break;

    case 553: {  
        char buf[0x100];
        GetWindowTextA(GetDlgItem(hwnd, panel::kGotoFrameEdit), buf, 0xA);
        long v = atol(buf);
        if (v < 0)
            v = 0;
        app->state.currentFrame = static_cast<std::int32_t>(v);
        RefreshAfterFrameApply(app);   
        PostViewRefresh(app);
        sprintf_s(buf, 0x100u, "%d", app->state.currentFrame);
        SetWindowTextA(GetDlgItem(hwnd, panel::kCurrentFrameEdit), buf);
        break;
    }

    case 555: {  
        char buf[0x100];
        sprintf_s(buf, 0x100u, "%d", app->state.currentFrame);
        SetWindowTextA(GetDlgItem(hwnd, panel::kGotoFrameEdit), buf);
        break;
    }

    case 556: {  
        D3DRenderer* locale = app->Renderer();
        const std::uint32_t size =
            (SendMessageA(GetDlgItem(hwnd, panel::kSelfShadowCheckbox), BM_GETCHECK, 0, 0) == 1)
                ? 0x1000
                : 0x800;
        locale->renderTargetWidth = size;   
        locale->renderTargetHeight = size;  
        IUnknown* objects[] = {locale->hdrTexture,        
                               locale->shadowDepthSurface,  
                               locale->shadowSurface};      
        for (IUnknown* obj : objects) {
            if (obj != nullptr) {
                obj->Release();  
            }
        }
        locale->hdrTexture = nullptr;
        locale->shadowDepthSurface = nullptr;
        locale->shadowSurface = nullptr;
        break;
    }

    case 558: {  
        app->state.currentFrame = 0;
        RefreshAfterFrameApply(app);   
        PostViewRefresh(app);
        char buf[0x100];
        sprintf_s(buf, 0x100u, "%d", app->state.currentFrame);
        SetWindowTextA(GetDlgItem(hwnd, panel::kCurrentFrameEdit), buf);
        break;
    }

    case 559: {  
        app->state.currentFrame =
            app->state.lastRegisteredFrame;
        RefreshAfterFrameApply(app);   
        PostViewRefresh(app);
        char buf[0x100];
        sprintf_s(buf, 0x100u, "%d", app->state.currentFrame);
        SetWindowTextA(GetDlgItem(hwnd, panel::kCurrentFrameEdit), buf);
        break;
    }

    case 562: {  
        SendMessageA(GetDlgItem(hwnd, panel::kEditOffCheckbox), BM_SETCHECK, 1, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kEditMode1Checkbox), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kEditMode2Checkbox), BM_SETCHECK, 0, 0);
        app->SelfShadowMode() = 0;
        RefreshRequest(-3);
        break;
    }
    case 563: {  
        SendMessageA(GetDlgItem(hwnd, panel::kEditOffCheckbox), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kEditMode1Checkbox), BM_SETCHECK, 1, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kEditMode2Checkbox), BM_SETCHECK, 0, 0);
        app->SelfShadowMode() = 1;
        RefreshRequest(-3);
        break;
    }
    case 564: {  
        SendMessageA(GetDlgItem(hwnd, panel::kEditOffCheckbox), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kEditMode1Checkbox), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kEditMode2Checkbox), BM_SETCHECK, 1, 0);
        app->SelfShadowMode() = 2;
        RefreshRequest(-3);
        break;
    }

    case 565: {  
        char buf[0x100];
        GetWindowTextA(GetDlgItem(hwnd, panel::kSelfShadowRangeEdit), buf, 8);
        const float v = static_cast<float>(atof(buf));
        app->state.physicsInterval =
            static_cast<float>((10000.0 - (double)v) / 100000.0);
        SendMessageA(GetDlgItem(hwnd, panel::kSelfShadowRangeSlider), TBM_SETPOS, 1,
                     static_cast<LPARAM>(static_cast<std::int32_t>(v)));
        app->SceneModified() = 1;
        for (std::size_t key = 0; key < 10000; ++key) {
            app->CameraKeys()[key].selected = 0;
            app->LightKeys()[key].selected = 0;
            app->ShadowKeys()[key].selected = 0;
            app->GravityKeys()[key].selected = 0;
        }
        for (int s = 0; s < 0xFF; ++s) {
            auto* rec = reinterpret_cast<unsigned char*>(app->AccessoryKeys(s));
            for (std::size_t off = 0; off < 0x927C0; off += 0x3C)
                rec[off + 0x18] = 0;
        }
        RegisterSelfShadowState(app, app->state.currentFrame);
        RefreshRequest(-3);
        PanelPaint(app);
        break;
    }

    case 551: {  
        if (app->state.fpsOverlayEnabled != 0) {
            app->state.fpsOverlayEnabled = 0;
            CheckMenuItem(GetMenu(hwnd), 0xD3, MF_UNCHECKED);
        } else {
            app->state.fpsOverlayEnabled = 1;
            app->state.fpsOverlayElapsedSeconds = 0.0f;
            app->state.fpsOverlayFrameCount = 0;
            CheckMenuItem(GetMenu(hwnd), 0xD3, MF_CHECKED);
        }
        break;
    }

    case 552: {  
        if (GetMenuState(GetMenu(hwnd), 0x12B, 0) & MF_CHECKED) {
            CheckMenuItem(GetMenu(hwnd), 0x12B, MF_UNCHECKED);
            app->FrameVolumeControlEnabled() = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x12B, MF_CHECKED);
            app->FrameVolumeControlEnabled() = 1;
        }
        break;
    }

    case 554:
        break;

    case 557: {  
        if (app->state.groundGridEnabled != 0) {
            app->state.groundGridEnabled = 0;
            CheckMenuItem(GetMenu(hwnd), 0xD7, MF_UNCHECKED);
        } else {
            app->state.groundGridEnabled = 1;
            CheckMenuItem(GetMenu(hwnd), 0xD7, MF_CHECKED);
        }
        break;
    }

    case 560:
    case 561:
        break;

    case 566:  
        app->state.optflag[6] = 0;
        OptionFlagRefresh(app, hwnd);
        break;
    case 567:  
        app->state.optflag[6] = 1;
        OptionFlagRefresh(app, hwnd);
        break;

    default:
        break;
    }
}

}  
