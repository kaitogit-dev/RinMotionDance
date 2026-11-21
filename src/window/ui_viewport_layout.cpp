#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

void LayoutViewportPanels(MMDApp* app) {  
    auto& s = *app;

    HWND separate = s.FloatingWindow();                           
    HWND base;
    int inset;
    RECT rc;
    if (separate != nullptr) {                                    
        base = separate;
        GetClientRect(base, &rc);
        inset = 0;
    } else {
        base = s.MainWindow();                                    
        GetClientRect(base, &rc);
        inset = s.SidebarWidth() + 9;                             
    }
    const int right = rc.right;
    const RECT& viewport = s.ViewportRect();
    const int stripY = viewport.bottom;                           
    const int rowY = viewport.top;                                

    auto moveLeft = [&](int id, int k, int dy, int w, int h) {
        if (inset + k + w <= right - 8)
            MoveWindow(GetDlgItem(base, id), inset + k, stripY + dy, w, h, TRUE);
        else
            MoveWindow(GetDlgItem(base, id), -9000, -9000, 1, 1, TRUE);
    };
    auto moveRight = [&](int id, int k, int dy, int w, int h) {
        if (right - k >= inset)
            MoveWindow(GetDlgItem(base, id), right - k, rowY + dy, w, h, TRUE);
        else
            MoveWindow(GetDlgItem(base, id), -9000, -9000, 1, 1, TRUE);
    };

    moveLeft(536,   5,  3, 112, 27);
    moveLeft(537, 210,  9, 16, 17);
    moveLeft(544, 227, 11, 53, 14);
    moveLeft(538, 287,  9, 16, 17);
    moveLeft(545, 304, 11, 53, 14);
    moveLeft(539, 364,  9, 16, 17);
    moveLeft(546, 381, 11, 53, 14);
    moveLeft(540, 520,  9, 16, 17);
    moveLeft(547, 537, 11, 53, 14);
    moveLeft(541, 597,  9, 16, 17);
    moveLeft(548, 614, 11, 53, 14);
    moveLeft(542, 674,  9, 16, 17);
    moveLeft(549, 691, 11, 53, 14);
    moveLeft(543, 760,  8, 36, 21);
    moveLeft(550, 797, 11, 53, 14);

    moveRight(557, 140, -22, 50, 20);
    moveRight(552, 235, -22, 90, 20);
    moveRight(551, 290, -22, 50, 20);
    moveRight(556, 405, -22, 110, 20);
    moveRight(553, 450, -22, 40, 20);
    moveRight(554, 510, -19, 55, 18);
    moveRight(555, 560, -22, 45, 20);

    for (int i = 536; i <= 557; ++i) {                            
        HWND overlay = GetDlgItem(base, i);
        if (IsWindowVisible(overlay)) {
            ShowWindow(overlay, SW_HIDE);
            ShowWindow(overlay, SW_SHOW);
        }
    }
}

}  
