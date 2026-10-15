#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdio>
#include <cstdint>
#include <cstring>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

constexpr int kListLeft = 91;     
constexpr int kIconLeft = 95;     
constexpr int kRowH = 13;         
constexpr int kBandH = 14;        
constexpr int kBandLastY = 2816;  
constexpr int kGridX0 = 100;      
constexpr int kGridTopY = 16;     
constexpr int kIcon = 11;         
constexpr int kAccY0 = 73;        
constexpr int kAccLastY = 2873;   
constexpr int kSheetMaskX = 44;   
constexpr int kSheetOnX = 22;     
constexpr int kSheetNegX = 33;    
constexpr int kSheetAltX = 11;    
constexpr int kSheetOffX = 0;     




unsigned char* CurrentModel(MMDApp* app) {
    return app->SelectedModel();
}

template <typename Key>
void DrawBandList(MMDApp* app, HDC panel, HDC icons, Key* keys,
                  int bandY, std::int32_t (&map)[200], int rowCount) {
    if (keys == nullptr)
        return;
    const std::int32_t scroll = app->state.timelineStartFrame;
    const std::int32_t limit = scroll + rowCount;
    if (static_cast<std::int32_t>(keys[0].frame) >= limit)
        return;
    int record = 0;
    for (;;) {
        const Key& key = keys[record];
        const int row = static_cast<std::int32_t>(key.frame) - scroll;
        if (row >= 0) {
            const int x = kRowH * row + kIconLeft;
            BitBlt(panel, x, bandY, kIcon, kIcon, icons, kSheetMaskX, 0, SRCAND);
            BitBlt(panel, x, bandY, kIcon, kIcon, icons,
                   key.selected ? kSheetOnX : kSheetOffX, 0, SRCPAINT);
            map[row] = record;
        }
        const int next = static_cast<int>(key.next);
        record = next;
        if (next == 0 ||
            static_cast<std::int32_t>(keys[next].frame) >= limit)
            return;
    }
}

}  

void PanelPaint(MMDApp* app) {
    const std::int32_t sidebar = app->SidebarWidth();
    D3DRenderer* sub = app->Renderer();
    const std::int32_t listW = sub->screenWidth;   
    const std::int32_t listH = sub->screenHeight;  
    HDC panel = app->PanelDC();
    const std::int32_t scroll = app->state.timelineStartFrame;

    HDC dc = CreateCompatibleDC(nullptr);
    SelectObject(dc, reinterpret_cast<HGDIOBJ>(app->state.bmpPanelSpare));
    BitBlt(panel, kListLeft, 0, sidebar, listH, dc, kListLeft, 0, SRCCOPY);
    DeleteDC(dc);

    HPEN pen = CreatePen(
        PS_SOLID, 1,
        app->ThemeColor(UiThemeColor::PanelHeader));
    HBRUSH brush = CreateSolidBrush(
        app->ThemeColor(UiThemeColor::PanelHeader));
    HGDIOBJ oldPen = SelectObject(panel, pen);
    HGDIOBJ oldBrush = SelectObject(panel, brush);
    unsigned char* flags = app->PanelRowFlags() + 1;
    for (int y = 30; y < kBandLastY; y += kBandH * 2, flags += 2) {
        if (*flags)
            Rectangle(panel, kListLeft, y, listW, y + kRowH);
    }
    SelectObject(panel, oldPen);
    DeleteObject(pen);
    SelectObject(panel, oldBrush);
    DeleteObject(brush);

    HPEN pen2 = CreatePen(
        PS_SOLID, 1,
        app->ThemeColor(UiThemeColor::PanelBody));
    HBRUSH brush2 = CreateSolidBrush(
        app->ThemeColor(UiThemeColor::PanelBody));
    SelectObject(panel, pen2);
    SelectObject(panel, brush2);
    flags = app->PanelRowFlags();
    for (int y = 16; y < kBandLastY; y += kBandH * 2, flags += 2) {
        if (*flags)
            Rectangle(panel, kListLeft, y, listW, y + kRowH);
    }
    SelectObject(panel, oldPen);
    DeleteObject(pen2);
    SelectObject(panel, oldBrush);
    DeleteObject(brush2);

    const int rows = (sidebar - 84) / kRowH;      
    if (rows < 0)
        return;
    if (rows > 0) {
        char label[0x100];
        char sel[0x100];
        int x = kGridX0;
        for (int idx = 0; idx < rows; ++idx) {
            const std::int32_t n = scroll + idx;
            COLORREF color;
            if (n == app->state.currentFrame) {
                sprintf_s(sel, sizeof(sel), "%-4d", n);
                DrawGlyph(app, sel, panel, 12, x - 3, 3,
                          app->state.themeColors[30],
                          reinterpret_cast<const unsigned char*>(
                              &app->state.themeColors[30])[1],
                          reinterpret_cast<const unsigned char*>(
                              &app->state.themeColors[30])[2], 1);
                color = static_cast<COLORREF>(
                    app->state.themeColors[30]);
            } else if (static_cast<std::uint32_t>(n) % 5 != 0) {
                color = static_cast<COLORREF>(
                    app->state.themeColors[32]);
            } else {
                sprintf_s(label, sizeof(label), "%-4d", n);
                DrawGlyph(app, label, panel, 12, x - 3, 3,
                          app->state.themeColors[33],
                          reinterpret_cast<const unsigned char*>(&app->state.themeColors[33])[1],
                          reinterpret_cast<const unsigned char*>(&app->state.themeColors[33])[2], 1);
                color = static_cast<COLORREF>(
                    app->state.themeColors[31]);
            }
            HPEN line = CreatePen(PS_SOLID, 1, color);
            SelectObject(panel, line);
            MoveToEx(panel, x, kGridTopY, nullptr);
            LineTo(panel, x, listH);
            SelectObject(panel, oldPen);
            DeleteObject(line);
            x += kRowH;
        }
    }

    if (rows > 0) {
        std::int32_t* const ikMap = app->state.rowHitBone;
        std::int32_t* const morphMap = app->state.rowHitMorph;
        std::int32_t* const boneMap = app->state.rowHitIk;
        for (int column = 0; column < rows; ++column) {
            const std::size_t base =
                static_cast<std::size_t>(column) * 200;
            for (int index = 0; index < 200; ++index) {
                const std::size_t slot = base +
                    static_cast<std::size_t>(index);
                ikMap[slot] = 0;
                morphMap[slot] = 0;
                boneMap[slot] = 0;
            }
        }
    }
    {
        std::int32_t* const band0Map = app->state.rowHitBand0;
        std::int32_t* const band1Map = app->state.rowHitBand1;
        std::int32_t* const band2Map = app->state.rowHitBand2;
        std::int32_t* const band3Map = app->state.rowHitBand3;
        unsigned char* acc = reinterpret_cast<unsigned char*>(app->state.rowHitAcc);
        for (int row = 0; row < 200; ++row) {
            band0Map[row] = -1;
            band1Map[row] = -1;
            band2Map[row] = -1;
            band3Map[row] = -1;
            memset(acc, 0xFF, 0x320);
            acc += 800;
        }
    }

    HDC icons = CreateCompatibleDC(nullptr);
    SelectObject(icons, reinterpret_cast<HGDIOBJ>(app->state.bmpRes101));

    auto FinishPanel = [&](std::uint32_t a, std::uint32_t b) {
        const std::uint32_t pos = static_cast<std::uint32_t>(scroll);
        SCROLLINFO scrollInfo{};
        scrollInfo.cbSize = sizeof(scrollInfo);
        scrollInfo.fMask = SIF_ALL;
        scrollInfo.nMin = 0;
        std::uint32_t nMax = pos + static_cast<std::uint32_t>(rows);
        if (nMax < a) nMax = a;
        if (nMax < b) nMax = b;
        const std::uint32_t mid =
            static_cast<std::uint32_t>(app->state.currentFrame) +
            static_cast<std::uint32_t>(rows / 2);
        if (nMax < mid) nMax = mid;
        scrollInfo.nMax = static_cast<int>(nMax) - 1;
        scrollInfo.nPage = static_cast<DWORD>(rows);
        scrollInfo.nPos = static_cast<int>(pos);
        app->state.timelineScrollNPage =
            static_cast<std::int32_t>(scrollInfo.nPage);
        app->state.timelineScrollNPos = scrollInfo.nPos;
        SetScrollInfo(GetDlgItem(static_cast<HWND>(app->Hwnd()), panel::kTimelineHScroll), SB_CTL,
                      &scrollInfo,
                      TRUE);

        RECT rc;
        GetClientRect(static_cast<HWND>(app->Hwnd()), &rc);
        if (app->SelectionBoxDragging() != 0 &&
            app->TimelineSelectionChanged() == 0) {
            HPEN box = CreatePen(PS_SOLID, 1, 0x969696);
            SelectObject(panel, box);
            int x2 = app->MouseX() - 6;
            int y2 = app->MouseY() - 145;
            if (x2 < kListLeft) x2 = kListLeft;
            if (y2 < 17) y2 = 17;
            if (x2 > sidebar - 28) x2 = sidebar - 28;
            if (y2 > rc.bottom - 396) y2 = rc.bottom - 396;
            const int ax = app->SelectionBoxAnchorX();
            const int ay = app->SelectionBoxAnchorY();
            MoveToEx(panel, ax, ay, nullptr);
            LineTo(panel, ax, y2);
            LineTo(panel, x2, y2);
            LineTo(panel, x2, ay);
            LineTo(panel, ax, ay);
            SelectObject(panel, oldPen);
            DeleteObject(box);
        }
        DeleteDC(icons);
        PostViewRefresh(app);
        rc.bottom -= 248;
        rc.left = 6;
        rc.top = 145;
        rc.right = sidebar - 19;
        InvalidateRect(static_cast<HWND>(app->Hwnd()), &rc, FALSE);
    };

    if (app->state.optflag[0] != 0) {
        const std::int32_t limit = scroll + rows;
        DrawBandList(app, panel, icons,
                     app->CameraKeys(),
                     17, app->state.rowHitBand0, rows);
        DrawBandList(app, panel, icons,
                     app->LightKeys(),
                     31, app->state.rowHitBand1, rows);
        DrawBandList(app, panel, icons,
                     app->ShadowKeys(),
                     45, app->state.rowHitBand2, rows);
        DrawBandList(app, panel, icons,
                     app->GravityKeys(),
                     59, app->state.rowHitBand3, rows);
        const std::int32_t* idxArr = app->state.jointLineMap;
        int band = 0;
        int bandY = kAccY0;
        while (bandY < kAccLastY) {
            const int idx = idxArr[band];
            if (idx < 0)
                break;
            mdl::AccessoryKey* list = app->AccessoryKeys(idx);
            if (list == nullptr)  
                break;
            int record = 0;
            if (static_cast<std::int32_t>(list[0].frame) < limit) {
                for (;;) {
                    const mdl::AccessoryKey& key = list[record];
                    const int row = static_cast<std::int32_t>(key.frame) - scroll;
                    if (row >= 0) {
                        const int x = kRowH * row + kIconLeft;
                        BitBlt(panel, x, bandY, kIcon, kIcon, icons, kSheetMaskX, 0,
                               SRCAND);
                        BitBlt(panel, x, bandY, kIcon, kIcon, icons,
                               key.selected ? kSheetOnX : kSheetOffX,
                               0, SRCPAINT);
                        app->state.rowHitAcc[static_cast<unsigned>(band) +
                                             200u * row] = record;
                    }
                    const int next = static_cast<int>(key.next);
                    record = next;
                    if (next == 0 ||
                        static_cast<std::int32_t>(list[next].frame) >= limit)
                        break;
                }
            }
            ++band;
            bandY += kBandH;
        }
        const std::uint32_t len =
            static_cast<std::uint32_t>(app->LastRegisteredFrame()) + 20;
        FinishPanel(len, len);
    } else {
        unsigned char* model = CurrentModel(app);
        const std::int32_t limit = scroll + rows;

        mdl::DisplayKey* displayKeys = mdl::DisplayKeys(model);
        if (static_cast<std::int32_t>(displayKeys[0].frame) < limit) {
            int record = 0;
            for (;;) {
                const mdl::DisplayKey& key = displayKeys[record];
                const int row =
                    static_cast<std::int32_t>(key.frame) - scroll;
                if (row >= 0) {
                    const int link = mdl::Mdl(model)->boneListSelLine;
                    if (link != 0) {
                        const int x = kRowH * row + kIconLeft;
                        const int y = kBandH * link + 17;
                        BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0, SRCAND);
                        BitBlt(panel, x, y, kIcon, kIcon, icons,
                               key.allocated ? kSheetOnX : kSheetOffX,
                               0, SRCPAINT);
                        app->state.rowHitIk[link + 200 * row] =
                            record != 0 ? record : -10;
                    }
                }
                const int next = static_cast<int>(key.next);
                record = next;
                if (next == 0 ||
                    static_cast<std::int32_t>(displayKeys[next].frame) >= limit)
                    break;
            }
        }

        mdl::ModelRecord* modelRecord = mdl::Mdl(model);
        if (static_cast<std::int32_t>(modelRecord->morphCount) > 0) {
            mdl::MorphKey* morphList = mdl::MorphKeys(model);
            std::uint32_t* morphLinks = modelRecord->morphKeyIndices;
            std::int32_t* mapMorph = app->state.rowHitMorph;
            const std::int32_t morphCnt =
                static_cast<std::int32_t>(modelRecord->morphCount);
            for (int head = 0; head < morphCnt; ++head) {
                if (static_cast<std::int32_t>(morphList[head].frame) >= limit)
                    continue;
                int rec = head;
                for (;;) {
                    mdl::MorphKey& key = morphList[rec];
                    const int row =
                        static_cast<std::int32_t>(key.frame) - scroll;
                    if (row >= 0) {
                        const int link = morphLinks[head];
                        if (link != 0) {
                            if (app->SelectionBoxDragging() != 0 &&
                                app->TimelineSelectionChanged() == 0 &&
                                link < 0) {
                                const int neg = -link;
                                if (app->TimelineRangeFirstOffset() <= row &&
                                    row < app->TimelineRangeLastOffset() &&
                                    neg < app->TimelineRangeLastBase() &&
                                    app->TimelineRangeFirstBase() <= neg) {
                                    key.allocated = static_cast<std::uint8_t>(
                                        !app->CtrlModifierActive());
                                } else if (!app->ShiftModifierActive()) {
                                    key.allocated = 0;
                                }
                            }
                            const int x = kRowH * row + kIconLeft;
                            if (key.allocated != 0) {
                                if (link >= 0) {
                                    const int y = kBandH * link + 17;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0,
                                           SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetOnX, 0,
                                           SRCPAINT);
                                } else {
                                    const int y = 17 - kBandH * link;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0,
                                           SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetNegX, 0,
                                           SRCPAINT);
                                    mapMorph[200 * row - link] = -1;
                                }
                            } else if (link >= 0) {
                                if (mapMorph[200 * row + link] >= 0) {
                                    const int y = kBandH * link + 17;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0,
                                           SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetOffX, 0,
                                           SRCPAINT);
                                }
                            } else if (mapMorph[200 * row - link] >= 0) {
                                const int y = 17 - kBandH * link;
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0,
                                       SRCAND);
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetAltX, 0,
                                       SRCPAINT);
                                mapMorph[200 * row - link] = -2;
                            }
                            if (link >= 0)
                                mapMorph[200 * row + link] = rec != 0 ? rec : -10;
                        }
                    }
                    const int next = static_cast<int>(key.next);
                    rec = next;
                    if (next == 0 ||
                        static_cast<std::int32_t>(morphList[next].frame) >= limit)
                        break;
                }
            }
        }

        const std::int32_t ikCnt =
            static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);
        if (ikCnt > 0) {
            mdl::BoneKey* ikList = mdl::BoneKeys(model);
            mdl::BoneRecord* bones = mdl::Bones(model);
            std::uint32_t* ikLinks = mdl::Mdl(model)->boneKeyIndices;
            std::int32_t* mapIk = app->state.rowHitBone;  
            for (int head = 0; head < ikCnt; ++head) {    
                if (static_cast<std::int32_t>(ikList[head].frame) >= limit)
                    continue;
                int rec = head;
                for (;;) {
                    mdl::BoneKey& key = ikList[rec];
                    const int row = static_cast<std::int32_t>(key.frame) - scroll;
                    if (row >= 0) {
                        const int link = ikLinks[head];
                        if (link != 0 ||
                            head == mdl::Mdl(model)->displayRootBone) {
                            if (app->SelectionBoxDragging() != 0 &&
                                app->TimelineSelectionChanged() == 0 &&
                                link < 0) {
                                const int neg = -link;
                                if (neg < app->TimelineRangeLastBase() &&
                                    row < app->TimelineRangeLastOffset() &&
                                    app->TimelineRangeFirstBase() <= neg &&
                                    app->TimelineRangeFirstOffset() <= row) {
                                    key.allocated =
                                        app->TimelineRangeApplyEnabled() != 0
                                            ? 0
                                            : static_cast<std::uint8_t>(
                                                  !app->CtrlModifierActive());
                                } else if (!app->ShiftModifierActive()) {
                                    key.allocated = 0;
                                }
                            }
                            const bool mask =
                                key.physicsDisabled == 0 && bones[head].hasRigidBody != 0;
                            const int x = kRowH * row + kIconLeft;
                            if (key.allocated != 0) {
                                if (link >= 0) {
                                    const int y = kBandH * link + 17;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                           mask ? kIcon : 0, SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetOnX,
                                           mask ? kIcon : 0, SRCPAINT);
                                } else {
                                    const int y = 17 - kBandH * link;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                           mask ? kIcon : 0, SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetNegX, 0,
                                           SRCPAINT);
                                    mapIk[200 * row - link] = -1;
                                }
                            } else if (link >= 0) {
                                if (mapIk[200 * row + link] >= 0) {
                                    const int y = kBandH * link + 17;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                           mask ? kIcon : 0, SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetOffX,
                                           mask ? kIcon : 0, SRCPAINT);
                                }
                            } else if (mapIk[200 * row - link] == -1) {
                                const int y = 17 - kBandH * link;
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                       mask ? kIcon : 0, SRCAND);
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetNegX,
                                       mask ? kIcon : 0, SRCPAINT);
                            } else {
                                const int y = 17 - kBandH * link;
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                       mask ? kIcon : 0, SRCAND);
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetAltX,
                                       mask ? kIcon : 0, SRCPAINT);
                                mapIk[200 * row - link] = -2;
                            }
                            if (link >= 0)
                                mapIk[200 * row + link] = rec != 0 ? rec : -10;
                        }
                    }
                    const int next = static_cast<int>(key.next);
                    rec = next;
                    if (next == 0 ||
                        static_cast<std::int32_t>(ikList[next].frame) >= limit)
                        break;
                }
            }
        }
        const std::uint32_t a =
            static_cast<std::uint32_t>(mdl::Mdl(model)->maxFrame) + 20;
        const std::uint32_t b =
            static_cast<std::uint32_t>(app->LastRegisteredFrame()) + 20;
        FinishPanel(a, b);
    }
}

}  
