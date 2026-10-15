#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

namespace {

void MoveCtrl(HWND parent, int id, int x, int y, int w, int h) {
    MoveWindow(GetDlgItem(parent, id), x, y, w, h, TRUE);
}

void ToggleVisible(HWND parent, int firstId, int lastId) {
    for (int id = firstId; id <= lastId; ++id) {
        HWND ctrl = GetDlgItem(parent, id);
        if (IsWindowVisible(ctrl)) {
            ShowWindow(ctrl, SW_HIDE);
            ShowWindow(ctrl, SW_SHOW);
        }
    }
}

}  

void PanelPaint(MMDApp* app);  
void LayoutViewportPanels(MMDApp* app);   

void HandleWindowSize(MMDApp* app) {
    auto& s = *app;
    const HWND hwnd = static_cast<HWND>(app->Hwnd());  

    RECT wr;
    GetWindowRect(hwnd, &wr);
    if (wr.right - wr.left > 2560)
        SetWindowPos(hwnd, nullptr, wr.left, wr.top, 2560,
                     wr.bottom - wr.top, SWP_NOZORDER);
    GetWindowRect(hwnd, &wr);
    if (wr.bottom - wr.top > 1920)
        SetWindowPos(hwnd, nullptr, wr.left, wr.top, wr.right - wr.left,
                     1920, SWP_NOZORDER);

    RECT cr1;
    GetClientRect(hwnd, &cr1);  
    std::int32_t& sidebar = s.SidebarWidth();
    if (s.FloatingWindow() != nullptr) {
        sidebar = cr1.right - 3;                               
        PostLanguageSweep(app);                                
    } else {
        std::int32_t newSidebar = static_cast<std::int32_t>(
            static_cast<double>(s.SidebarRatio()) *
            cr1.right);                                        
        if (newSidebar < 250) newSidebar = 250;                                
        const bool grew = sidebar < newSidebar;                        
        sidebar = newSidebar;                                          
        if (grew) PanelPaint(app);                             
    }
    const std::int32_t side = sidebar;
    RECT cb;                                                   
    GetClientRect(hwnd, &cb);                                  
    const std::int32_t bottom = cb.bottom;

    MoveCtrl(hwnd, 400, side / 2 - 80, 21, 70, 24);
    MoveCtrl(hwnd, 401, side / 2 + 10, 21, 70, 24);
    MoveCtrl(hwnd, 418, side / 2 - 49, 67, 20, 24);
    MoveCtrl(hwnd, 419, side / 2 + 29, 67, 20, 24);
    MoveCtrl(hwnd, 533, side / 2 - 79, 69, 20, 22);
    MoveCtrl(hwnd, 532, side / 2 + 59, 69, 20, 22);
    MoveCtrl(hwnd, 417, side / 2 - 27, 66, 54, 24);
    MoveCtrl(hwnd, 559, side - 30, 69, 20, 22);
    MoveCtrl(hwnd, 558, 15, 69, 20, 22);

    MoveCtrl(hwnd, 427, side - 19, 162, 16, bottom - 410);
    MoveCtrl(hwnd, 428, 97, bottom - 247, side - 116, 16);
    MoveCtrl(hwnd, 429, 8, bottom - 247, 80, 18);
    MoveCtrl(hwnd, 416, side / 2 + 15, bottom - 224, 50, 18);
    MoveCtrl(hwnd, 423, side / 2 + 70, bottom - 224, 50, 18);
    MoveCtrl(hwnd, 420, side / 2 - 119, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 421, side / 2 - 76, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 422, side / 2 - 33, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 425, side / 2 - 3, bottom - 201, 50, 14);
    MoveCtrl(hwnd, 426, side / 2 + 69, bottom - 201, 50, 14);
    MoveCtrl(hwnd, 434, side / 2 - 118, bottom - 193, 100, 400);
    MoveCtrl(hwnd, 415, side / 2 - 4, bottom - 181, 60, 18);
    MoveCtrl(hwnd, 424, side / 2 + 60, bottom - 181, 60, 18);
    MoveCtrl(hwnd, 432, 144, bottom - 26, 63, 18);
    MoveCtrl(hwnd, 431, 144, bottom - 52, 63, 18);
    MoveCtrl(hwnd, 430, 144, bottom - 78, 63, 18);
    MoveCtrl(hwnd, 433, 144, bottom - 135, 63, 120);
    MoveCtrl(hwnd, 530, 144, bottom - 105, 13, 13);
    MoveCtrl(hwnd, 436, 225, bottom - 138, 125, 200);
    MoveCtrl(hwnd, 435, 224, bottom - 112, 60, 21);
    MoveCtrl(hwnd, 437, 291, bottom - 112, 60, 21);
    MoveCtrl(hwnd, 438, 291, bottom - 35, 60, 28);
    MoveCtrl(hwnd, 439, 225, bottom - 84, 13, 13);
    MoveCtrl(hwnd, 440, 267, bottom - 86, 40, 18);
    MoveCtrl(hwnd, 441, 310, bottom - 86, 41, 18);
    MoveCtrl(hwnd, 442, 328, bottom - 62, 22, 22);
    MoveCtrl(hwnd, 443, 225, bottom - 61, 100, 100);
    MoveCtrl(hwnd, 444, 227, bottom - 36, 13, 13);
    MoveCtrl(hwnd, 445, 227, bottom - 19, 13, 13);
    MoveCtrl(hwnd, 451, 368, bottom - 139, 70, 23);
    MoveCtrl(hwnd, 446, 443, bottom - 139, 65, 23);
    MoveCtrl(hwnd, 448, 477, bottom - 111, 30, 14);
    MoveCtrl(hwnd, 447, 363, bottom - 95, 151, 20);
    MoveCtrl(hwnd, 452, 463, bottom - 40, 45, 33);
    MoveCtrl(hwnd, 449, 368, bottom - 53, 90, 100);
    MoveCtrl(hwnd, 450, 368, bottom - 28, 90, 300);
    MoveCtrl(hwnd, 454, 363, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 453, 363, bottom - 155, 14, 14);

    const std::uint8_t opt0 = s.CameraMode();     
    const std::uint8_t opt1 = s.UiOptionFlag(1);  
    const std::uint8_t opt2 = s.UiOptionFlag(2);  
    const std::uint8_t opt3 = s.UiOptionFlag(3);  
    const std::uint8_t opt4 = s.UiOptionFlag(4);  
    const std::uint8_t opt5 = s.UiOptionFlag(5);  
    const std::uint8_t opt6 = s.UiOptionFlag(6);  

    std::int32_t bandXBase = -18;
    if (opt0 != 0 && opt1 == 0) bandXBase = 117;                   
    MoveCtrl(hwnd, 455, 514 - bandXBase, bottom - 140, 136, 18);
    MoveCtrl(hwnd, 461, 650 - bandXBase, bottom - 140, 23, 14);
    MoveCtrl(hwnd, 456, 514 - bandXBase, bottom - 122, 136, 18);
    MoveCtrl(hwnd, 462, 650 - bandXBase, bottom - 122, 23, 14);
    MoveCtrl(hwnd, 457, 514 - bandXBase, bottom - 104, 136, 18);
    MoveCtrl(hwnd, 463, 650 - bandXBase, bottom - 104, 23, 14);
    MoveCtrl(hwnd, 458, 514 - bandXBase, bottom - 84, 132, 20);
    MoveCtrl(hwnd, 464, 646 - bandXBase, bottom - 84, 27, 14);
    MoveCtrl(hwnd, 459, 514 - bandXBase, bottom - 66, 132, 20);
    MoveCtrl(hwnd, 465, 646 - bandXBase, bottom - 66, 27, 14);
    MoveCtrl(hwnd, 460, 514 - bandXBase, bottom - 48, 132, 20);
    MoveCtrl(hwnd, 466, 646 - bandXBase, bottom - 48, 27, 14);
    MoveCtrl(hwnd, 468, 590 - bandXBase, bottom - 26, 75, 20);
    MoveCtrl(hwnd, 467, 524 - bandXBase, bottom - 26, 50, 20);
    MoveCtrl(hwnd, 470, 501 - bandXBase, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 469, 501 - bandXBase, bottom - 155, 14, 14);

    if (opt0 != 0 && opt2 == 0) bandXBase += 162;
    MoveCtrl(hwnd, 567, 684 - bandXBase, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 566, 684 - bandXBase, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 562, 688 - bandXBase, bottom - 135, 54, 22);
    MoveCtrl(hwnd, 563, 745 - bandXBase, bottom - 135, 54, 22);
    MoveCtrl(hwnd, 564, 802 - bandXBase, bottom - 135, 54, 22);
    MoveCtrl(hwnd, 560, 683 - bandXBase, bottom - 80, 176, 25);
    MoveCtrl(hwnd, 561, 820 - bandXBase, bottom - 100, 30, 14);
    MoveCtrl(hwnd, 565, 745 - bandXBase, bottom - 36, 54, 30);

    if (opt0 != 0 && opt6 == 0) bandXBase += 160;
    MoveCtrl(hwnd, 489, 864 - bandXBase, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 488, 864 - bandXBase, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 471, 868 - bandXBase, bottom - 140, 125, 150);
    MoveCtrl(hwnd, 472, 867 - bandXBase, bottom - 116, 60, 18);
    MoveCtrl(hwnd, 473, 934 - bandXBase, bottom - 116, 60, 18);
    MoveCtrl(hwnd, 476, 1000 - bandXBase, bottom - 142, 13, 13);
    MoveCtrl(hwnd, 486, 1000 - bandXBase, bottom - 127, 13, 13);
    MoveCtrl(hwnd, 477, 999 - bandXBase, bottom - 113, 53, 17);
    MoveCtrl(hwnd, 474, 868 - bandXBase, bottom - 95, 90, 100);
    MoveCtrl(hwnd, 475, 962 - bandXBase, bottom - 95, 90, 300);
    MoveCtrl(hwnd, 478, 877 - bandXBase, bottom - 70, 48, 14);
    MoveCtrl(hwnd, 479, 940 - bandXBase, bottom - 70, 48, 14);
    MoveCtrl(hwnd, 480, 1003 - bandXBase, bottom - 70, 48, 14);
    MoveCtrl(hwnd, 481, 877 - bandXBase, bottom - 50, 48, 14);
    MoveCtrl(hwnd, 482, 940 - bandXBase, bottom - 50, 48, 14);
    MoveCtrl(hwnd, 483, 1003 - bandXBase, bottom - 50, 48, 14);
    MoveCtrl(hwnd, 484, 877 - bandXBase, bottom - 30, 48, 14);
    MoveCtrl(hwnd, 485, 940 - bandXBase, bottom - 30, 48, 14);
    MoveCtrl(hwnd, 487, 995 - bandXBase, bottom - 32, 58, 26);

    if (opt0 != 0 && opt3 == 0) bandXBase += 178;
    MoveCtrl(hwnd, 503, 363, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 502, 363, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 490, 366, bottom - 140, 60, 28);
    MoveCtrl(hwnd, 493, 430, bottom - 140, 60, 28);
    MoveCtrl(hwnd, 492, 494, bottom - 140, 60, 28);
    MoveCtrl(hwnd, 491, 366, bottom - 105, 60, 24);
    MoveCtrl(hwnd, 494, 430, bottom - 105, 60, 24);
    MoveCtrl(hwnd, 501, 494, bottom - 105, 60, 24);
    MoveCtrl(hwnd, 496, 366, bottom - 70, 60, 24);
    MoveCtrl(hwnd, 497, 430, bottom - 70, 60, 24);
    MoveCtrl(hwnd, 498, 494, bottom - 70, 60, 24);
    MoveCtrl(hwnd, 499, 500, bottom - 36, 48, 28);
    MoveCtrl(hwnd, 495, 430, bottom - 32, 60, 24);
    MoveCtrl(hwnd, 500, 366, bottom - 36, 60, 28);

    if (opt4 == 0 && opt0 == 0) bandXBase += 179;                   
    std::int32_t bandXBase2 = opt0 != 0 ? bandXBase - 180 : bandXBase + 53;     
    MoveCtrl(hwnd, 529, 598 - bandXBase2, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 528, 598 - bandXBase2, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 509, 618 - bandXBase2, bottom - 120, 92, 500);
    MoveCtrl(hwnd, 512, 602 - bandXBase2, bottom - 121, 16, 22);
    MoveCtrl(hwnd, 513, 710 - bandXBase2, bottom - 121, 16, 22);
    MoveCtrl(hwnd, 510, 597 - bandXBase2, bottom - 100, 135, 25);
    MoveCtrl(hwnd, 511, 642 - bandXBase2, bottom - 138, 28, 14);
    MoveCtrl(hwnd, 504, 618 - bandXBase2, bottom - 50, 92, 500);
    MoveCtrl(hwnd, 507, 602 - bandXBase2, bottom - 51, 16, 22);
    MoveCtrl(hwnd, 508, 710 - bandXBase2, bottom - 51, 16, 22);
    MoveCtrl(hwnd, 505, 597 - bandXBase2, bottom - 30, 135, 25);
    MoveCtrl(hwnd, 506, 642 - bandXBase2, bottom - 68, 28, 14);
    MoveCtrl(hwnd, 514, 749 - bandXBase2, bottom - 120, 92, 500);
    MoveCtrl(hwnd, 517, 733 - bandXBase2, bottom - 121, 16, 22);
    MoveCtrl(hwnd, 518, 841 - bandXBase2, bottom - 121, 16, 22);
    MoveCtrl(hwnd, 515, 729 - bandXBase2, bottom - 100, 135, 25);
    MoveCtrl(hwnd, 516, 773 - bandXBase2, bottom - 138, 28, 14);
    MoveCtrl(hwnd, 519, 749 - bandXBase2, bottom - 50, 92, 500);
    MoveCtrl(hwnd, 522, 733 - bandXBase2, bottom - 51, 16, 22);
    MoveCtrl(hwnd, 523, 841 - bandXBase2, bottom - 51, 16, 22);
    MoveCtrl(hwnd, 520, 729 - bandXBase2, bottom - 30, 135, 25);
    MoveCtrl(hwnd, 521, 773 - bandXBase2, bottom - 68, 28, 14);
    MoveCtrl(hwnd, 524, 675 - bandXBase2, bottom - 141, 50, 19);
    MoveCtrl(hwnd, 525, 675 - bandXBase2, bottom - 71, 50, 19);
    MoveCtrl(hwnd, 526, 807 - bandXBase2, bottom - 71, 50, 19);
    MoveCtrl(hwnd, 527, 807 - bandXBase2, bottom - 141, 50, 19);

    if (opt5 == 0 && opt0 == 0) bandXBase2 += 251;                  
    if (opt0 == 0) bandXBase2 += 13;                                
    MoveCtrl(hwnd, 402, 886 - bandXBase2, bottom - 141, 38, 17);
    MoveCtrl(hwnd, 403, 925 - bandXBase2, bottom - 141, 37, 17);
    MoveCtrl(hwnd, 404, 963 - bandXBase2, bottom - 141, 37, 17);
    MoveCtrl(hwnd, 405, 886 - bandXBase2, bottom - 121, 38, 17);
    MoveCtrl(hwnd, 406, 925 - bandXBase2, bottom - 121, 37, 17);
    MoveCtrl(hwnd, 407, 963 - bandXBase2, bottom - 121, 37, 17);
    MoveCtrl(hwnd, 408, 887 - bandXBase2, bottom - 63, 50, 24);
    MoveCtrl(hwnd, 409, 888 - bandXBase2, bottom - 36, 50, 14);
    MoveCtrl(hwnd, 410, 952 - bandXBase2, bottom - 36, 50, 14);
    MoveCtrl(hwnd, 411, 952 - bandXBase2, bottom - 58, 13, 13);
    MoveCtrl(hwnd, 412, 902 - bandXBase2, bottom - 99, 13, 13);
    MoveCtrl(hwnd, 531, 971 - bandXBase2, bottom - 99, 13, 13);
    MoveCtrl(hwnd, 413, 952 - bandXBase2, bottom - 19, 13, 13);
    MoveCtrl(hwnd, 414, 888 - bandXBase2, bottom - 19, 13, 13);
    MoveCtrl(hwnd, 534, 1017 - bandXBase2, bottom - 63, 22, 57);
    MoveCtrl(hwnd, 535, 1002 - bandXBase2, bottom - 141, 35, 37);

    ToggleVisible(hwnd, 455, 469);
    ToggleVisible(hwnd, 471, 488);
    ToggleVisible(hwnd, 504, 528);
    ToggleVisible(hwnd, 402, 414);
    ToggleVisible(hwnd, 531, 535);

    RefreshMainWindowViewport(app);
    LayoutViewportPanels(app);  
}

void RelayoutSidebarControls(MMDApp* app) {
    const HWND hwnd = static_cast<HWND>(app->Hwnd());
    const int side = app->SidebarWidth();
    RECT client{};
    GetClientRect(hwnd, &client);

    MoveCtrl(hwnd, 400, side / 2 - 80, 21, 70, 24);
    MoveCtrl(hwnd, 401, side / 2 + 10, 21, 70, 24);
    MoveCtrl(hwnd, 418, side / 2 - 49, 67, 20, 24);
    MoveCtrl(hwnd, 419, side / 2 + 29, 67, 20, 24);
    MoveCtrl(hwnd, 533, side / 2 - 79, 69, 20, 22);
    MoveCtrl(hwnd, 532, side / 2 + 59, 69, 20, 22);
    MoveCtrl(hwnd, 417, side / 2 - 27, 66, 54, 24);
    MoveCtrl(hwnd, 559, side - 30, 69, 20, 22);
    MoveCtrl(hwnd, 558, 15, 69, 20, 22);

    const int bottom = client.bottom;
    MoveCtrl(hwnd, 427, side - 19, 162, 16, bottom - 410);
    MoveCtrl(hwnd, 428, 97, bottom - 247, side - 116, 16);
    MoveCtrl(hwnd, 429, 8, bottom - 247, 80, 18);
    MoveCtrl(hwnd, 416, side / 2 + 15, bottom - 224, 50, 18);
    MoveCtrl(hwnd, 423, side / 2 + 70, bottom - 224, 50, 18);
    MoveCtrl(hwnd, 420, side / 2 - 119, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 421, side / 2 - 76, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 422, side / 2 - 33, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 425, side / 2 - 3, bottom - 201, 50, 14);
    MoveCtrl(hwnd, 426, side / 2 + 69, bottom - 201, 50, 14);
    MoveCtrl(hwnd, 434, side / 2 - 118, bottom - 193, 100, 400);
    MoveCtrl(hwnd, 415, side / 2 - 4, bottom - 181, 60, 18);
    MoveCtrl(hwnd, 424, side / 2 + 60, bottom - 181, 60, 18);

    ToggleVisible(hwnd, 400, 401);
    ToggleVisible(hwnd, 415, 426);
    ToggleVisible(hwnd, 558, 559);
    ToggleVisible(hwnd, 434, 434);
    RefreshMainWindowViewport(app);
    LayoutViewportPanels(app);
}

}  
