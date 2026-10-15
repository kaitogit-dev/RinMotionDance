#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>
#include <d3d9.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void RefreshAfterFrameApply(MMDApp* app);          
void PushBoneEditUndo(MMDApp* app);                       
void RefreshRequest(int area);                     

static constexpr double kDegToRad = 0.01745329238474369;
static constexpr double kPiLit = 3.141592025756836;

namespace {

unsigned char* SlotModel(MMDApp* app) {
    return app->SelectedModel();
}

mdl::AccessoryRecord* LightAccessory(MMDApp* app) {
    return app->AccessorySlot(app->SelectedAccessorySlot());
}

void ComposeEulerToBone(MMDApp* app, unsigned char* model, int sel) {
    d3dx::D3DXMATRIXF rot{};
    d3dx::D3DXMATRIXF tmp{};
    auto* d3dx = &d3dx::Get();
    d3dx->rotZ(&rot, app->state.eulerZ);
    d3dx->rotX(&tmp, app->state.eulerX);
    d3dx->multiply(&rot, &rot, &tmp);
    d3dx->rotY(&tmp, app->state.eulerY);
    d3dx->multiply(&rot, &rot, &tmp);
    rinmotiondance::mdl::BoneRecord* bones = rinmotiondance::mdl::Bones(model);
    d3dx->quatFromMatrix(reinterpret_cast<float*>(&bones[sel].rotQuat[0]),
                         &rot);
    rinmotiondance::mdl::Mdl(model)->bonePhysicsState[sel] = 1;
}

}  

void CommitEditControlTail(MMDApp* app, HWND edit) {
    HWND base = app->FloatingWindow();
    if (base == nullptr)
        base = app->MainWindow();
    const bool cameraMode = app->CameraMode() != 0;

    if (edit == GetDlgItem(base, panel::kReadoutPosXEdit)) {           
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (!cameraMode) {
            unsigned char* model = SlotModel(app);
            const int sel = rinmotiondance::mdl::Mdl(model)->selectedBone;
            if (sel >= 0) {
                PushBoneEditUndo(app);
                rinmotiondance::mdl::Bones(model)[sel].trans[0] = static_cast<float>(v);
                rinmotiondance::mdl::Mdl(model)->bonePhysicsState[sel] = 1;
            }
            PostViewRefresh(app);
            return;
        }
        app->CameraPositionX() = static_cast<float>(v);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, panel::kReadoutPosYEdit)) {           
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (!cameraMode) {
            unsigned char* model = SlotModel(app);
            const int sel = rinmotiondance::mdl::Mdl(model)->selectedBone;
            if (sel >= 0) {
                PushBoneEditUndo(app);
                rinmotiondance::mdl::Bones(model)[sel].trans[1] = static_cast<float>(v);
                rinmotiondance::mdl::Mdl(model)->bonePhysicsState[sel] = 1;
            }
            PostViewRefresh(app);
            return;
        }
        app->CameraPositionY() = static_cast<float>(v);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, panel::kReadoutPosZEdit)) {           
        char text[256];
        GetWindowTextA(edit, text, 100);
        double v = atof(text);
        if (!cameraMode) {
            unsigned char* model = SlotModel(app);
            const int sel = rinmotiondance::mdl::Mdl(model)->selectedBone;
            if (sel >= 0) {
                PushBoneEditUndo(app);
                rinmotiondance::mdl::Bones(model)[sel].trans[2] = static_cast<float>(v);
                rinmotiondance::mdl::Mdl(model)->bonePhysicsState[sel] = 1;
            }
            PostViewRefresh(app);
            return;
        }
        if (app->CameraParentModel() >= 0)         
            v = -v;
        app->CameraPositionZ() = static_cast<float>(v);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, panel::kReadoutRotXEdit)) {           
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (!cameraMode) {
            unsigned char* model = SlotModel(app);
            const int sel = rinmotiondance::mdl::Mdl(model)->selectedBone;
            if (sel >= 0) {
                PushBoneEditUndo(app);
                app->state.eulerX =
                    static_cast<float>(v * kPiLit / 180.0);
                app->state.eulerY =
                    static_cast<float>(-static_cast<double>(
                                           app->state.eulerY) *
                                       kPiLit / 180.0);
                app->state.eulerZ =
                    static_cast<float>(kPiLit *
                                       -static_cast<double>(
                                           app->state.eulerZ) /
                                       180.0);
                ComposeEulerToBone(app, model, sel);
            }
            PostViewRefresh(app);
            return;
        }
        app->CameraPitch() = static_cast<float>(-v * kPiLit / 180.0);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, panel::kReadoutRotYEdit)) {           
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (cameraMode) {
            app->CameraYaw() = static_cast<float>(v * kPiLit / 180.0);
            RefreshRequest(-1);
            PostViewRefresh(app);
            return;
        }
        unsigned char* model = SlotModel(app);
        const int sel = rinmotiondance::mdl::Mdl(model)->selectedBone;
        if (sel >= 0) {
            PushBoneEditUndo(app);
            app->state.eulerX =      
                static_cast<float>(                        
                    static_cast<double>(
                        app->state.eulerX) *
                    kPiLit / 180.0);
            app->state.eulerY =
                static_cast<float>(-v * kPiLit / 180.0);
            app->state.eulerZ =
                static_cast<float>(kPiLit *
                                   -static_cast<double>(
                                       app->state.eulerZ) /
                                   180.0);
            ComposeEulerToBone(app, model, sel);
        }
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, panel::kReadoutRotZEdit)) {           
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (cameraMode) {
            app->CameraRoll() = static_cast<float>(v * kPiLit / 180.0);
            RefreshRequest(-1);
            PostViewRefresh(app);
            return;
        }
        unsigned char* model = SlotModel(app);
        const int sel = rinmotiondance::mdl::Mdl(model)->selectedBone;
        if (sel >= 0) {
            PushBoneEditUndo(app);
            app->state.eulerX =
                static_cast<float>(
                    static_cast<double>(
                        app->state.eulerX) *
                    kPiLit / 180.0);
            app->state.eulerY =
                static_cast<float>(-static_cast<double>(
                                       app->state.eulerY) *
                                   kPiLit / 180.0);
            app->state.eulerZ =
                static_cast<float>(kPiLit * -v / 180.0);
            ComposeEulerToBone(app, model, sel);
        }
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, panel::kReadoutDistEdit) && cameraMode) {  
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        app->CameraDistance() = static_cast<float>(-v);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    PostViewRefresh(app);                           
}

void CommitEditControl(MMDApp* app, HWND edit) {
    auto& s = *app;
    const HWND main = s.MainWindow();
    s.state.enterKeyState = 1;    

    if (edit == GetDlgItem(main, panel::kCurrentFrameEdit)) {            
        char text[256];
        GetWindowTextA(edit, text, 8);
        const long v = atol(text);
        s.CurrentFrame() = static_cast<std::int32_t>(v);
        if (static_cast<unsigned long>(v) > 0x80000000UL) {
            s.CurrentFrame() = 0;
            SetWindowTextA(edit, "0");
        }
        RefreshAfterFrameApply(app);
        PostViewRefresh(app);
        CommitEditControlTail(app, edit);
        return;
    }
    if (edit == GetDlgItem(main, panel::kLightColorEditR) ||            
        edit == GetDlgItem(main, panel::kLightColorEditG) || edit == GetDlgItem(main, panel::kLightColorEditB)) {
        char text[256];
        GetWindowTextA(edit, text, 8);
        const double v = static_cast<double>(atol(text)) * 0.00390625;
        const int id = edit == GetDlgItem(main, panel::kLightColorEditR) ? 461
                     : edit == GetDlgItem(main, panel::kLightColorEditG) ? 462 : 463;
        s.LightColor()[id - 461] = static_cast<float>(v);
        SendMessageA(GetDlgItem(main, 455 + (id - 461)), TBM_SETPOS, 1,
                     static_cast<LPARAM>(static_cast<int>(v * 256.0)));
        RefreshRequest(-2);
        CommitEditControlTail(app, edit);
        return;
    }
    if (edit == GetDlgItem(main, panel::kLightDirEditX) ||            
        edit == GetDlgItem(main, panel::kLightDirEditY) || edit == GetDlgItem(main, panel::kLightDirEditZ)) {
        char text[256];
        GetWindowTextA(edit, text, 8);
        const double v = atof(text);
        const int id = edit == GetDlgItem(main, panel::kLightDirEditX) ? 464
                     : edit == GetDlgItem(main, panel::kLightDirEditY) ? 465 : 466;
        s.LightDirection()[id - 464] = static_cast<float>(v);
        SendMessageA(GetDlgItem(main, 458 + (id - 464)), TBM_SETPOS, 1,
                     static_cast<LPARAM>(static_cast<int>(v * 100.0)));
        RefreshRequest(-2);
        CommitEditControlTail(app, edit);
        return;
    }
    if (edit == GetDlgItem(main, panel::kFovEdit)) {            
        char text[256];
        GetWindowTextA(edit, text, 8);
        const float v = static_cast<float>(atol(text));
        s.CameraFov() = v;
        SendMessageA(GetDlgItem(main, panel::kFovSlider), TBM_SETPOS, 1,
                     static_cast<LPARAM>(static_cast<int>(v)));
        D3DRenderer* r = s.Renderer();
        const float fovRad = static_cast<float>(
            static_cast<double>(s.CameraFov()) * kDegToRad);
        d3dx::D3DXMATRIXF mat{};
        auto* d3dx = &d3dx::Get();
        d3dx->perspectiveFovLH(&mat, fovRad, r->aspectRatio, 1.0f,
                               100000.0f);
        IDirect3DDevice9* dev = r->device;
        dev->SetTransform(D3DTS_PROJECTION,
                          reinterpret_cast<const D3DMATRIX*>(&mat));
        RefreshRequest(-1);
        CommitEditControlTail(app, edit);
        return;
    }
    {
        mdl::AccessoryRecord* acc = LightAccessory(app);
        int lightId = 0;
        if (edit == GetDlgItem(main, panel::kAccPosXEdit)) lightId = 478;
        else if (edit == GetDlgItem(main, panel::kAccPosYEdit)) lightId = 479;
        else if (edit == GetDlgItem(main, panel::kAccPosZEdit)) lightId = 480;
        else if (edit == GetDlgItem(main, panel::kAccRotXEdit)) lightId = 481;
        else if (edit == GetDlgItem(main, panel::kAccRotYEdit)) lightId = 482;
        else if (edit == GetDlgItem(main, panel::kAccRotZEdit)) lightId = 483;
        else if (edit == GetDlgItem(main, panel::kAccScaleXEdit)) lightId = 484;
        else if (edit == GetDlgItem(main, panel::kAccScaleYEdit)) lightId = 485;
        if (lightId != 0 && acc != nullptr) {
            char text[256];
            GetWindowTextA(edit, text, 8);
            const double v = atof(text);
            if (lightId <= 480) {
                acc->position[lightId - 478] = static_cast<float>(v);
            } else if (lightId <= 483) {
                acc->rotation[lightId - 481] =
                    static_cast<float>(v / 180.0 * kPiLit);
            } else if (lightId == 484) {
                acc->scale = static_cast<float>(v);
            } else {                                 
                double out = v;
                if (v < 0.0)
                    out = 0.0;
                else if (v > 1.0)
                    out = 1.0;
                acc->opacity = static_cast<float>(out);
                const HWND same = GetDlgItem(main, panel::kAccScaleYEdit);
                const LPARAM len = GetWindowTextLengthA(same);
                SendMessageA(same, EM_SETSEL, 0, len);
                char fmt[256];
                sprintf_s(fmt, 0x100, "%3.2f", out);
                SendMessageA(same, EM_REPLACESEL, 0,
                             reinterpret_cast<LPARAM>(fmt));
            }
            RefreshRequest(s.SelectedAccessorySlot());
            CommitEditControlTail(app, edit);
            return;
        }
        if (lightId != 0) {                          
            RefreshRequest(s.SelectedAccessorySlot());
            CommitEditControlTail(app, edit);
            return;
        }
    }
    if (edit == GetDlgItem(main, panel::kSelfShadowRangeEdit)) {            
        char text[256];
        GetWindowTextA(edit, text, 8);
        const double v = atof(text);
        s.state.physicsInterval =   
            static_cast<float>((10000.0 - v) / 100000.0);
        SendMessageA(GetDlgItem(main, panel::kSelfShadowRangeSlider), TBM_SETPOS, 1,
                     static_cast<LPARAM>(static_cast<int>(v)));
        RefreshRequest(-3);
        CommitEditControlTail(app, edit);
        return;
    }
    {
        int morphEdit = 0;
        if (edit == GetDlgItem(main, panel::kMorphEdit0)) morphEdit = 506;
        else if (edit == GetDlgItem(main, panel::kMorphEdit1)) morphEdit = 511;
        else if (edit == GetDlgItem(main, panel::kMorphEdit2)) morphEdit = 516;
        else if (edit == GetDlgItem(main, panel::kMorphEdit3)) morphEdit = 521;
        if (morphEdit != 0) {
            char text[256];
            GetWindowTextA(edit, text, 8);
            const double v = atof(text);
            unsigned char* model = SlotModel(app);
            const int selector =
                rinmotiondance::mdl::Mdl(model)->selectedMorphs[
                    (morphEdit - 506) / 5];
            if (rinmotiondance::mdl::Morphs(model) != nullptr && selector >= 0)
                rinmotiondance::mdl::Morphs(model)[selector].value =
                    static_cast<float>(v);
            SendMessageA(GetDlgItem(main, morphEdit - 1), TBM_SETPOS, 1,
                         static_cast<LPARAM>(static_cast<int>(v * 100.0)));
            CommitEditControlTail(app, edit);
            return;
        }
    }
    CommitEditControlTail(app, edit);                           
}

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT Msg, WPARAM wParam,
                                  LPARAM lParam) {
    MMDApp* app = g_Block;
    if (Msg == WM_KEYDOWN) {
        if (wParam == 13) {                          
            CommitEditControl(app, hWnd);
            SetFocus(app->MainWindow());
            return 0;
        }
    } else if (Msg == WM_KILLFOCUS) {
        CommitEditControl(app, reinterpret_cast<HWND>(wParam));
    }
    return CallWindowProcA(
        app->OriginalEditProc(),
        hWnd, Msg, wParam, lParam);
}

LRESULT CALLBACK TrackbarSubclassProc(HWND hWnd, UINT Msg, WPARAM wParam,
                                      LPARAM lParam) {
    MMDApp* app = g_Block;
    if (Msg == WM_LBUTTONUP)
        SetFocus(app->MainWindow());
    return CallWindowProcA(
        app->OriginalTrackbarProc(),
        hWnd, Msg, wParam, lParam);
}

void InstallControlSubclasses(MMDApp* app, HWND hwnd) {
    static const int kEditIds[] = {
        417, 448, 461, 462, 463, 464, 465, 466,
        478, 479, 480, 481, 482, 483, 484, 485,
        506, 511, 516, 521, 544, 545, 546, 547, 548, 549, 550, 554, 561,
    };
    static const int kTrackIds[] = {
        447, 455, 456, 457, 458, 459, 460,
        505, 510, 515, 520, 534, 560,
    };

    HWND firstEdit = GetDlgItem(hwnd, panel::kCurrentFrameEdit);
    app->OriginalEditProc() = reinterpret_cast<WNDPROC>(
        GetWindowLongPtrA(firstEdit, GWLP_WNDPROC));     
    SetWindowLongPtrA(firstEdit, GWLP_WNDPROC,
                   reinterpret_cast<LONG_PTR>(EditSubclassProc));

    HWND firstTrack = GetDlgItem(hwnd, panel::kFovSlider);
    app->OriginalTrackbarProc() = reinterpret_cast<WNDPROC>(
        GetWindowLongPtrA(firstTrack, GWLP_WNDPROC));    
    SetWindowLongPtrA(firstTrack, GWLP_WNDPROC,
                   reinterpret_cast<LONG_PTR>(TrackbarSubclassProc));

    for (int id : kEditIds) {
        if (id == 417)
            continue;
        SetWindowLongPtrA(GetDlgItem(hwnd, id), GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(EditSubclassProc));
    }
    for (int id : kTrackIds) {
        if (id == 447)
            continue;
        SetWindowLongPtrA(GetDlgItem(hwnd, id), GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(TrackbarSubclassProc));
    }
}

void InstallControlSubclass(MMDApp* app, HWND control, int id) {
    static const int kEditIds[] = {
        417, 448, 461, 462, 463, 464, 465, 466,
        478, 479, 480, 481, 482, 483, 484, 485,
        506, 511, 516, 521, 544, 545, 546, 547, 548, 549, 550, 554, 561,
    };
    static const int kTrackIds[] = {
        447, 455, 456, 457, 458, 459, 460,
        505, 510, 515, 520, 534, 560,
    };
    if (control == nullptr)
        return;
    for (int editId : kEditIds) {
        if (id != editId)
            continue;
        if (id == 417) {
            app->OriginalEditProc() = reinterpret_cast<WNDPROC>(
                GetWindowLongPtrA(control, GWLP_WNDPROC));
        }
        SetWindowLongPtrA(control, GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(EditSubclassProc));
        return;
    }
    for (int trackId : kTrackIds) {
        if (id != trackId)
            continue;
        if (id == 447) {
            app->OriginalTrackbarProc() = reinterpret_cast<WNDPROC>(
                GetWindowLongPtrA(control, GWLP_WNDPROC));
        }
        SetWindowLongPtrA(control, GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(TrackbarSubclassProc));
        return;
    }
}

}  
