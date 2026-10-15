#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void SetFrameNormalized(int frame);   

void HandleVScroll(LPARAM lParam, WPARAM wParam) {
    MMDApp* app = g_Block;
    HWND hwnd = static_cast<HWND>(app->Hwnd());
    const HWND ctrl = reinterpret_cast<HWND>(lParam);          
    const std::uint32_t code = LOWORD(wParam);                 

    if (ctrl == GetDlgItem(hwnd, panel::kTimelineVScroll)) {
        if (app->state.optflag[0] != 0) {
            switch (code) {
            case 0:  
                --app->DisplayObjectListScrollPosition();
                break;
            case 1:  
                ++app->DisplayObjectListScrollPosition();
                break;
            case 2:  
                app->DisplayObjectListScrollPosition() -=
                    app->state.scrollNPage;
                break;
            case 3:  
                app->DisplayObjectListScrollPosition() +=
                    app->state.scrollNPage;
                break;
            case 5:  
                app->DisplayObjectListScrollPosition() +=
                    static_cast<std::int32_t>(HIWORD(wParam)) -
                    app->state.scrollNPos;
                break;
            default:
                break;
            }
            if (app->DisplayObjectListScrollPosition() < 0)
                app->DisplayObjectListScrollPosition() = 0;
            const int upper = app->DisplayObjectListMatchCount() - 2;
            if (app->DisplayObjectListScrollPosition() > upper)
                app->DisplayObjectListScrollPosition() = upper;
        } else {
            unsigned char* model = app->SelectedModel();
            rinmotiondance::mdl::ModelRecord& record = *rinmotiondance::mdl::Mdl(model);
            switch (code) {
            case 0:  
                --record.boneListPos;
                break;
            case 1:  
                ++record.boneListPos;
                break;
            case 2:  
                record.boneListPos -=
                    app->state.scrollNPage;
                break;
            case 3:  
                record.boneListPos +=
                    app->state.scrollNPage;
                break;
            case 5:  
                record.boneListPos +=
                    static_cast<std::int32_t>(HIWORD(wParam)) -
                    app->state.scrollNPos;
                break;
            default:
                break;
            }
            if (record.boneListPos < 0)
                record.boneListPos = 0;
            if (record.boneListPos > record.boneListRows - 2)
                record.boneListPos = record.boneListRows - 2;
        }
        PostLanguageSweep(app);                                   
        return;
    }

    if (ctrl == GetDlgItem(hwnd, panel::kFrameVolumeSlider)) {
        HWND slider = GetDlgItem(hwnd, panel::kFrameVolumeSlider);
        LRESULT pos = SendMessageA(slider, TBM_GETPOS, 0, 0);
        app->FrameNormalization() = 100 - static_cast<int>(pos);
        SetFrameNormalized(100 - static_cast<int>(pos));          
    }
}

void HandleMouseWheel(int delta) {
    MMDApp* app = g_Block;
    HWND hwnd = static_cast<HWND>(app->Hwnd());
    const std::int16_t wheel = static_cast<std::int16_t>(delta);  

    if (app->MouseX() <=
        app->SidebarWidth()) {
        if (wheel <= 0) {
            HandleVScroll(reinterpret_cast<LPARAM>(GetDlgItem(hwnd, panel::kTimelineVScroll)), 1);
            HandleVScroll(reinterpret_cast<LPARAM>(GetDlgItem(hwnd, panel::kTimelineVScroll)), 1);
            HandleVScroll(reinterpret_cast<LPARAM>(GetDlgItem(hwnd, panel::kTimelineVScroll)), 1);
        } else {
            HandleVScroll(reinterpret_cast<LPARAM>(GetDlgItem(hwnd, panel::kTimelineVScroll)), 0);
            HandleVScroll(reinterpret_cast<LPARAM>(GetDlgItem(hwnd, panel::kTimelineVScroll)), 0);
            HandleVScroll(reinterpret_cast<LPARAM>(GetDlgItem(hwnd, panel::kTimelineVScroll)), 0);
        }
        return;
    }

    const std::int32_t axis = app->CameraParentModel();
    const std::uint8_t mode = app->state.optflag[0];  

    if ((mode & (axis >= 0 ? 1u : 0u)) != 0) {
        app->CameraPosition()[2] =
            static_cast<float>(wheel) * 0.05f + app->CameraPosition()[2];
        if (app->GlobalTrackSelected(GlobalTimelineTrack::Camera) == 0) {
            app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
            for (int slot = 0; slot < 0xFF; ++slot) {
                mdl::AccessoryRecord* accessory = app->AccessorySlot(slot);
                if (accessory != nullptr)
                    accessory->rowSelected = 0;
            }
            PostLanguageSweep(app);                             
        }
    } else {
        const bool morphFollow =
            mode == 0 &&
            app->state.followCameraEnabled != 0 &&
            app->PlaybackActive() == 0 &&
            axis == static_cast<int>(app->SelectedModelSlot()) &&
            axis >= 0;
        if (morphFollow) {
            app->state.viewDirty = 1;                       
            app->WindowLayoutReady() = 0;
            PostLanguageSweep2(app);                              
            app->WindowLayoutReady() = 1;
            app->state.b6568482 = 1;                       
            PostViewRefresh(app);                                 
            return;
        }
        app->CameraDistance() =
            static_cast<float>(wheel) * 0.05f + app->CameraDistance();
        if (app->GlobalTrackSelected(GlobalTimelineTrack::Camera) == 0) {
            app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
            for (int slot = 0; slot < 0xFF; ++slot) {
                mdl::AccessoryRecord* accessory = app->AccessorySlot(slot);
                if (accessory != nullptr)
                    accessory->rowSelected = 0;
            }
            PostLanguageSweep(app);                             
        }
    }
    PostViewRefresh(app);     
}

}  
