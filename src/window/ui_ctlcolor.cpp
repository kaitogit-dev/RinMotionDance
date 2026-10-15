#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdlib>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

void FreeTimelineSelectionRecords(MMDApp* app, TimelineSelectionBand band) {
    TimelineSelectionRecord*& p = app->TimelineSelectionRecords(band);
    if (p != nullptr) {
        std::free(p);
        p = nullptr;
    }
}

}  

void PanelPaint(MMDApp* app);       
void SelectionReeval(MMDApp* app);  

LRESULT HandleCtlColor(HWND control, HDC dc) {
    MMDApp* app = g_Block;
    (void)dc;
    const HWND ctrl = control;
    const HWND main = reinterpret_cast<HWND>(app->state.hwnd);  
    const auto brushResult = [app](int index) {
        return static_cast<LRESULT>(
            reinterpret_cast<std::intptr_t>(app->UiBrush(index)));
    };

    if (ctrl == GetDlgItem(main, panel::kFovSlider))  
        return brushResult(9);

    if (ctrl == GetDlgItem(main, panel::kFrameVolumeSlider))  
        return brushResult(10);

    if (ctrl == GetDlgItem(main, panel::kLightColorSliderR))  
        return brushResult(0);

    if (ctrl == GetDlgItem(main, panel::kLightColorSliderG))  
        return brushResult(1);

    if (ctrl == GetDlgItem(main, panel::kLightColorSliderB))  
        return brushResult(2);

    if (ctrl == GetDlgItem(main, panel::kLightDirSliderX))  
        return brushResult(3);

    if (ctrl == GetDlgItem(main, panel::kLightDirSliderY))  
        return brushResult(4);

    if (ctrl == GetDlgItem(main, panel::kLightDirSliderZ))  
        return brushResult(5);

    const bool hit515 = ctrl == GetDlgItem(main, panel::kMorphSlider2);
    const bool hit510 = ctrl == GetDlgItem(main, panel::kMorphSlider1);
    if (hit515 || hit510)
        return brushResult(6);

    const bool hit520 = ctrl == GetDlgItem(main, panel::kMorphSlider3);
    const bool hit505 = ctrl == GetDlgItem(main, panel::kMorphSlider0);
    if (hit520 || hit505)
        return brushResult(7);

    if (ctrl == GetDlgItem(main, panel::kSelfShadowRangeSlider))  
        return brushResult(8);

    return 0;
}

void HandleLButtonUp(MMDApp* app) {
    const bool dragActive = app->SelectionBoxDragging() != 0;
    app->SidebarResizeDragging() = 0;
    app->WindowLayoutReady() = 1;

    if (dragActive) {
        FreeTimelineSelectionRecords(app, TimelineSelectionBand::Accessory);
        FreeTimelineSelectionRecords(app, TimelineSelectionBand::ModelIk);
        FreeTimelineSelectionRecords(app, TimelineSelectionBand::Camera);
        FreeTimelineSelectionRecords(app, TimelineSelectionBand::Light);
        FreeTimelineSelectionRecords(app, TimelineSelectionBand::ModelBone);
        FreeTimelineSelectionRecords(app, TimelineSelectionBand::SelfShadow);
        FreeTimelineSelectionRecords(app, TimelineSelectionBand::ModelMorph);

        app->SelectionBoxDragging() = 0;
        app->TimelineSelectionChanged() = 0;
        PanelPaint(app);               
        SelectionReeval(app);          
    }

    app->PendingTimelineSelectionRow() = TimelineSelectionRow::None;
}

}  
