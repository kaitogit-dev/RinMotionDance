#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <algorithm>
#include <cwchar>

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

    moveLeft(536, 6, 2, 100, 23);
    moveLeft(537, 174, 6, 17, 18);
    moveLeft(544, 193, 5, 57, 20);
    moveLeft(538, 257, 6, 17, 18);
    moveLeft(545, 276, 5, 57, 20);
    moveLeft(539, 340, 6, 17, 18);
    moveLeft(546, 359, 5, 57, 20);
    moveLeft(540, 490, 6, 17, 18);
    moveLeft(547, 509, 5, 57, 20);
    moveLeft(541, 573, 6, 17, 18);
    moveLeft(548, 592, 5, 57, 20);
    moveLeft(542, 656, 6, 17, 18);
    moveLeft(549, 675, 5, 57, 20);
    moveLeft(543, 757, 3, 43, 22);
    moveLeft(550, 801, 5, 66, 20);

    int toolbarRight = right - 80;
    const auto placeToolbar = [&](int id, int minimum, int dy, int height) {
        const HWND control = GetDlgItem(base, id);
        int width = minimum;
        if (control != nullptr && id != 554) {
            wchar_t caption[160]{};
            GetWindowTextW(control, caption, 160);
            HDC dc = GetDC(control);
            if (dc != nullptr) {
                const HFONT font = reinterpret_cast<HFONT>(
                    SendMessageW(control, WM_GETFONT, 0, 0));
                HGDIOBJ previous = font != nullptr ?
                    SelectObject(dc, font) : nullptr;
                SIZE size{};
                GetTextExtentPoint32W(dc, caption, lstrlenW(caption), &size);
                width = std::max(minimum, size.cx + 10);
                if (previous != nullptr)
                    SelectObject(dc, previous);
                ReleaseDC(control, dc);
            }
        }
        toolbarRight -= width;
        moveRight(id, right - toolbarRight, dy, width, height);
        toolbarRight -= 3;
    };
    placeToolbar(557, 36, -22, 20);
    placeToolbar(552, 62, -22, 20);
    placeToolbar(551, 34, -22, 20);
    placeToolbar(556, 80, -22, 20);
    placeToolbar(553, 29, -22, 20);
    placeToolbar(554, 52, -19, 18);
    placeToolbar(555, 31, -22, 20);

    for (int i = 536; i <= 557; ++i) {                            
        HWND overlay = GetDlgItem(base, i);
        if (IsWindowVisible(overlay)) {
            ShowWindow(overlay, SW_HIDE);
            ShowWindow(overlay, SW_SHOW);
        }
    }
}

}  
