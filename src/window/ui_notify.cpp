#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void DrawControlText(MMDApp* app, HWND hwnd, HDC hdc, RECT rc);

struct NotifyCd {
    HWND hwndFrom;          
    std::uint32_t idFrom;   
    std::int32_t code;      
    std::uint32_t stage;    
    HDC hdc;                
    RECT rc;                
};

LRESULT HandleNotify(HWND h, UINT m, WPARAM w, LPARAM l) {
    MMDApp* app = g_Block;
    const NotifyCd* cd = reinterpret_cast<const NotifyCd*>(l);
    const HWND mainHwnd =
        static_cast<HWND>(app->state.hwnd);  

    if (cd->code != -12 || cd->stage != 1)
        return DefWindowProcA(h, m, w, l);

    const LRESULT mainShadowCheck =
        SendMessageA(GetDlgItem(mainHwnd, panel::kSelfShadowCheckbox), BM_GETCHECK, 0, 0);
    const std::uint32_t shadowMode =
        static_cast<std::uint32_t>(app->state.selfShadowMode);  
    const std::uint32_t editMode =
        static_cast<std::uint32_t>(app->EditMode());
    const std::uint8_t lParama = app->PlaybackActive();
    const HWND subWnd = app->FloatingWindow();
    const LRESULT v11res =
        SendMessageA(GetDlgItem(subWnd, panel::kSelfShadowCheckbox), BM_GETCHECK, 0, 0);

    const bool mainShadowChecked = (mainShadowCheck == 1);
    const std::uint32_t ctrlId = cd->idFrom;  

    const bool hit = static_cast<bool>(
        ((v11res == 1) | mainShadowChecked) & (ctrlId == 556)                       
        | (ctrlId == 446) & (app->CameraPerspective() == 0)
        | (ctrlId == 492) & (editMode == 4)
        | (ctrlId == 493) & (editMode == 3)
        | (ctrlId == 491) & (editMode == 1)
        | (ctrlId == 490) & (editMode == 0)
        | (ctrlId == 552) & app->FrameVolumeControlEnabled()  
        | (ctrlId == 557) & app->state.groundGridEnabled  
        | (ctrlId == 564) & (shadowMode == 2)
        | (ctrlId == 551) & app->state.fpsOverlayEnabled  
        | (ctrlId == 563) & (shadowMode == 1)
        | (ctrlId == 562) & (shadowMode == 0)
        | (ctrlId == 535) & app->state.followCameraEnabled  
        | (ctrlId == 408) & (lParama != 0));                          

    if (hit) {
        SetTextColor(cd->hdc, 0xC86400);
        DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
        return 4;
    }

    if (ctrlId == 499) {  
        if (IsDlgButtonChecked(mainHwnd, panel::kPhysicsCheckbox) == 1) {
            SetTextColor(cd->hdc, 0xC86400);
            DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
            return 4;
        }
        return DefWindowProcA(h, m, w, l);
    }

    if (ctrlId == 440) {  
        if (app->state.optflag[0] != 0)  
            return DefWindowProcA(h, m, w, l);
        const std::uint8_t idx = app->SelectedModelSlot();
        const mdl::ModelRecord& model = *mdl::Mdl(app->ModelSlot(idx));
        if (model.toonFlag == 0)
            return DefWindowProcA(h, m, w, l);
        SetTextColor(cd->hdc, 0xC86400);
        DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
        return 4;
    }

    if (ctrlId == 441) {  
        if (app->state.optflag[0] != 0)  
            return DefWindowProcA(h, m, w, l);
        const std::uint8_t idx = app->SelectedModelSlot();
        const mdl::ModelRecord& model = *mdl::Mdl(app->ModelSlot(idx));
        if (model.postLoadFlag2 == 0)
            return DefWindowProcA(h, m, w, l);
        SetTextColor(cd->hdc, 0xC86400);
        DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
        return 4;
    }

    if (ctrlId == 477) {  
        const std::uint8_t idx = app->SelectedAccessorySlot();
        const mdl::AccessoryRecord* rec = app->AccessorySlot(idx);
        if (rec == nullptr)
            return DefWindowProcA(h, m, w, l);
        if (rec->additiveBlend == 0)
            return DefWindowProcA(h, m, w, l);
        SetTextColor(cd->hdc, 0xC86400);
        DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
        return 4;
    }

    if (ctrlId == 543) {  
        SetTextColor(cd->hdc, 0);
        DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
        return 4;
    }

    if ((ctrlId == 537 || ctrlId == 540) && lParama == 0) {
        SetTextColor(cd->hdc, 0x96);  
        DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
        return 4;
    }
    if ((ctrlId == 538 || ctrlId == 541) && lParama == 0) {
        SetTextColor(cd->hdc, 0x6400);  
        DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
        return 4;
    }
    if ((ctrlId == 539 || ctrlId == 542) && lParama == 0) {
        SetTextColor(cd->hdc, 0x960000);  
        DrawControlText(app, cd->hwndFrom, cd->hdc, cd->rc);
        return 4;
    }

    return DefWindowProcA(h, m, w, l);
}

}  
