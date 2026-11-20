#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/editor_dock.hpp"
#include <algorithm>

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

void LayoutEditorDock(MMDApp* app) {
    if (app->FloatingWindow() != nullptr)
        return;
    const HWND hwnd = app->MainWindow();
    RECT client{};
    GetClientRect(hwnd, &client);
    const EditorDock dock = ComputeEditorDock(app, client.right, client.bottom);
    if (!dock.enabled)
        return;

    const auto put = [&](DockSection section, int id, int x, int y,
                         int w, int h) {
        const int i = DockIndex(section);
        if (dock.width[i] <= 0)
            return;
        const HWND control = GetDlgItem(hwnd, id);
        if (control == nullptr)
            return;
        const int available = dock.width[i] - x - 9;
        if (available <= 0)
            return;
        MoveCtrl(hwnd, id, dock.left[i] + x, dock.top + y,
                 std::min(w, available), h);
    };
    const auto w = [&](DockSection section) -> int {
        return dock.width[DockIndex(section)];
    };

    const DockSection interp = DockSection::Interpolation;
    const int interpW = w(interp);
    put(interp, 433, 143, 31, interpW - 154, 170);
    put(interp, 530, 145, 58, 15, 15);
    put(interp, 430, 143, 82, interpW - 154, 23);
    put(interp, 431, 143, 109, interpW - 154, 23);
    put(interp, 432, 143, 136, interpW - 154, 21);

    const DockSection model = DockSection::Model;
    const int modelW = w(model);
    const int halfModel = (modelW - 29) / 2;
    put(model, 436, 10, 28, modelW - 20, 180);
    put(model, 435, 10, 56, halfModel, 23);
    put(model, 437, 19 + halfModel, 56, halfModel, 23);
    put(model, 439, 10, 85, 15, 15);
    put(model, 440, 92, 83, 49, 20);
    put(model, 441, 148, 83, modelW - 158, 20);
    put(model, 443, 10, 109, modelW - 20, 160);
    put(model, 444, 12, 139, 15, 15);
    put(model, 445, 74, 139, 15, 15);
    put(model, 438, modelW - 88, 131, 76, 25);
    put(model, 442, modelW - 31, 108, 19, 19);

    if (dock.camera) {
        const DockSection camera = DockSection::Edit;
        const int camW = w(camera);
        const int camHalf = (camW - 30) / 2;
        put(camera, 451, 10, 29, camHalf, 24);
        put(camera, 446, 20 + camHalf, 29, camHalf, 24);
        put(camera, 448, camW - 64, 59, 52, 20);
        put(camera, 447, 10, 79, camW - 20, 19);
        put(camera, 449, 10, 116, camW - 101, 160);
        put(camera, 450, 10, 137, camW - 101, 160);
        put(camera, 452, camW - 86, 132, 74, 24);
        put(camera, 453, camW - 21, 4, 15, 15);
        put(camera, 454, camW - 21, 4, 15, 15);

        const DockSection light = DockSection::Secondary;
        const int lightW = w(light);
        const int sliders[] = {455, 456, 457, 458, 459, 460};
        const int values[] = {461, 462, 463, 464, 465, 466};
        for (int i = 0; i < 6; ++i) {
            put(light, sliders[i], 28, 27 + 17 * i,
                lightW - 93, 18);
            put(light, values[i], lightW - 59, 27 + 17 * i,
                47, 19);
        }
        put(light, 467, 11, 133, (lightW - 35) / 2, 23);
        put(light, 468, 22 + (lightW - 35) / 2, 133,
            (lightW - 35) / 2, 23);
        put(light, 469, lightW - 21, 4, 15, 15);
        put(light, 470, lightW - 21, 4, 15, 15);

        const DockSection shadow = DockSection::Shadow;
        const int shadowW = w(shadow);
        const int modeW = (shadowW - 28) / 3;
        put(shadow, 562, 9, 30, modeW, 24);
        put(shadow, 563, 14 + modeW, 30, modeW, 24);
        put(shadow, 564, 19 + 2 * modeW, 30, modeW, 24);
        put(shadow, 561, shadowW - 65, 66, 52, 19);
        put(shadow, 560, 10, 91, shadowW - 20, 20);
        put(shadow, 565, 10, 130, shadowW - 20, 25);
        put(shadow, 566, shadowW - 20, 4, 14, 14);
        put(shadow, 567, shadowW - 20, 4, 14, 14);

        const DockSection accessory = DockSection::Accessory;
        const int accW = w(accessory);
        put(accessory, 471, 10, 28, accW - 105, 150);
        put(accessory, 476, accW - 86, 31, 15, 15);
        put(accessory, 486, accW - 46, 31, 15, 15);
        const int accButtonW = (accW - 34) / 3;
        put(accessory, 472, 10, 54, accButtonW, 22);
        put(accessory, 473, 17 + accButtonW, 54, accButtonW, 22);
        put(accessory, 477, 24 + 2 * accButtonW, 54, accButtonW, 22);
        put(accessory, 474, 10, 79, (accW - 30) / 2, 130);
        put(accessory, 475, 20 + (accW - 30) / 2, 79,
            (accW - 30) / 2, 130);
        const int fields[8] = {478, 479, 480, 484, 481, 482, 483, 485};
        for (int i = 0; i < 8; ++i) {
            const int column = i % 4;
            const int row = i / 4;
            const int fieldW = (accW - 34) / 4;
            put(accessory, fields[i], 10 + column * (fieldW + 4),
                108 + row * 26, fieldW, 20);
        }
        put(accessory, 487, accW - 89, 3, 65, 20);
        put(accessory, 488, accW - 20, 4, 14, 14);
        put(accessory, 489, accW - 20, 4, 14, 14);
    } else {
        const DockSection bone = DockSection::Edit;
        const int boneW = w(bone);
        const int cell = (boneW - 40) / 3;
        const int controls[4][3] = {
            {490, 493, 492},
            {491, 494, 501},
            {496, 497, 498},
            {500, 495, 499}
        };
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 3; ++col)
                put(bone, controls[row][col], 12 + col * (cell + 8),
                    30 + row * 31, cell, 26);
        put(bone, 502, boneW - 21, 4, 14, 14);
        put(bone, 503, boneW - 21, 4, 14, 14);

        const DockSection facial = DockSection::Secondary;
        const int faceW = w(facial);
        const int colWidth = faceW / 2;
        const int comboIds[4] = {509, 514, 504, 519};
        const int sliderIds[4] = {510, 515, 505, 520};
        const int valueIds[4] = {511, 516, 506, 521};
        const int prevIds[4] = {512, 517, 507, 522};
        const int nextIds[4] = {513, 518, 508, 523};
        const int registerIds[4] = {524, 527, 525, 526};
        for (int i = 0; i < 4; ++i) {
            const int x = 11 + (i % 2) * colWidth;
            const int y = 41 + (i / 2) * 68;
            put(facial, comboIds[i], x, y, colWidth - 79, 150);
            put(facial, prevIds[i], x + colWidth - 70, y, 26, 23);
            put(facial, nextIds[i], x + colWidth - 38, y, 26, 23);
            put(facial, sliderIds[i], x, y + 25, colWidth - 142, 20);
            put(facial, valueIds[i], x + colWidth - 131, y + 25, 47, 20);
            put(facial, registerIds[i], x + colWidth - 76,
                y + 24, 64, 24);
        }
        put(facial, 528, faceW - 21, 4, 14, 14);
        put(facial, 529, faceW - 21, 4, 14, 14);
    }

    const DockSection playback = DockSection::Playback;
    const int playW = w(playback);
    const int viewW = (playW - 36) / 3;
    for (int i = 0; i < 6; ++i)
        put(playback, 402 + i, 10 + (i % 3) * (viewW + 7),
            29 + (i / 3) * 26, viewW, 23);
    put(playback, 534, 10, 82, 19, 72);
    put(playback, 408, 37, 84, 75, 26);
    put(playback, 535, 119, 84, 64, 26);
    put(playback, 411, 190, 89, 16, 16);
    put(playback, 412, 37, 115, 16, 16);
    put(playback, 531, 119, 115, 16, 16);
    put(playback, 414, 37, 137, 16, 16);
    put(playback, 409, 98, 134, 51, 20);
    put(playback, 413, 154, 137, 16, 16);
    put(playback, 410, 199, 134, playW - 210, 20);
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
        if (newSidebar < 300 && cr1.right >= 1480) newSidebar = 300;
        else if (newSidebar < 250) newSidebar = 250;                                
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

    LayoutEditorDock(app);
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
