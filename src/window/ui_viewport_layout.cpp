// ===========================================================================
// VA 0x0040CAC0 - LayoutViewportPanels  (original: sub_40CAC0, 0x5A9 bytes,
// )
// ===========================================================================
// Repositions the camera/light/accessory overlay strip at the top of the 3D
// viewport (ids 536..550) and the window-top button row (ids 551..557).
// Called from HandleWindowSize (0x443300 tail), the command dispatch
// (0x47E8A0 case 212), sub_442EB0 and MicWndProc (0x466A10).
//
// Base rect: the separate-viewport window when this+0xA0D38 is a live HWND,
// otherwise the main window with a left inset of sidebar(this+0xA06C8)+9.
// The camera strip anchors left (x = inset + K, shown while inset+K <=
// right), the button row anchors right (x = right - K, shown while
// right-K >= inset); controls that do not fit are parked off-screen at
// (-9000,-9000) with size 1x1, exactly like the original.
// Y bases come from the viewport hide-rect fields: strip rows sit at
// this+0xA0D4C + 3..11, buttons at this+0xA0D44 - 22 (or -19 for id 554).
// Tail: ids 536..557 get the IsWindowVisible -> SW_HIDE -> SW_SHOW
// re-toggle so repainted overlays pick up their new positions.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

void LayoutViewportPanels(MMDApp* app) {  // VA 0x0040CAC0
    auto& s = *app;

    HWND separate = s.FloatingWindow();                           // 658744
    HWND base;
    int inset;
    RECT rc;
    if (separate != nullptr) {                                    // 0x40CADB
        base = separate;
        GetClientRect(base, &rc);
        inset = 0;
    } else {
        base = s.MainWindow();                                    // 657080
        GetClientRect(base, &rc);
        inset = s.SidebarWidth() + 9;                             // 657096
    }
    const int right = rc.right;
    const RECT& viewport = s.ViewportRect();
    const int stripY = viewport.bottom;                           // 658764
    const int rowY = viewport.top;                                // 658756

    // Camera strip: x = inset + K, shown while inset + K <= right.
    auto moveLeft = [&](int id, int k, int dy, int w, int h) {
        if (inset + k <= right)
            MoveWindow(GetDlgItem(base, id), inset + k, stripY + dy, w, h, TRUE);
        else
            MoveWindow(GetDlgItem(base, id), -9000, -9000, 1, 1, TRUE);
    };
    // Top button row: x = right - K, shown while right - K >= inset.
    auto moveRight = [&](int id, int k, int dy, int w, int h) {
        if (right - k >= inset)
            MoveWindow(GetDlgItem(base, id), right - k, rowY + dy, w, h, TRUE);
        else
            MoveWindow(GetDlgItem(base, id), -9000, -9000, 1, 1, TRUE);
    };

    moveLeft(536,   5,  3, 55, 27);   // 0x40CB25 model-select strip label
    moveLeft(537, 140,  9, 16, 17);   // X
    moveLeft(544, 157, 11, 53, 14);   // X value
    moveLeft(538, 213,  9, 16, 17);   // Y
    moveLeft(545, 230, 11, 53, 14);   // Y value
    moveLeft(539, 286,  9, 16, 17);   // Z
    moveLeft(546, 303, 11, 53, 14);   // Z value
    moveLeft(540, 400,  9, 16, 17);   // scale label
    moveLeft(547, 417, 11, 38, 14);
    moveLeft(541, 458,  9, 16, 17);
    moveLeft(548, 475, 11, 38, 14);
    moveLeft(542, 516,  9, 16, 17);
    moveLeft(549, 533, 11, 38, 14);
    moveLeft(543, 579,  8, 36, 21);   // distance
    moveLeft(550, 616, 11, 53, 14);

    moveRight(557, 140, -22, 50, 20); // 0x40D018 coordinate-axis toggle
    moveRight(552, 195, -22, 50, 20); // power-save
    moveRight(551, 250, -22, 50, 20); // info display
    moveRight(556, 305, -22, 50, 20);
    moveRight(553, 360, -22, 40, 20);
    moveRight(554, 398, -19, 35, 14); // frame edit box
    moveRight(555, 441, -22, 40, 20);

    for (int i = 536; i <= 557; ++i) {                            // 0x40D053
        HWND overlay = GetDlgItem(base, i);
        if (IsWindowVisible(overlay)) {
            ShowWindow(overlay, SW_HIDE);
            ShowWindow(overlay, SW_SHOW);
        }
    }
}

}  // namespace rinmotiondance
