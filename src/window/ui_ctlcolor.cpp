#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdlib>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"
#include "rinmotiondance/ui_style.hpp"
#include <commctrl.h>

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

LRESULT HandleCtlColor(HWND control, HDC dc, UINT message) {
    MMDApp* app = g_Block;
    if (app == nullptr || control == nullptr || dc == nullptr)
        return 0;
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

    if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX) {
        SetTextColor(dc, ui_style::Text);
        SetBkColor(dc, ui_style::Input);
        return reinterpret_cast<LRESULT>(ui_style::InputBrush());
    }

    RECT windowRect{};
    RECT client{};
    GetWindowRect(control, &windowRect);
    GetClientRect(main, &client);
    POINT point{windowRect.left, windowRect.top};
    ScreenToClient(main, &point);
    const bool header = point.y < 50 && point.x < app->SidebarWidth();
    const bool bottom = point.y >= client.bottom - 160;
    const COLORREF background = header ? ui_style::Header :
        bottom ? ui_style::Surface : ui_style::SurfaceAlt;
    SetTextColor(dc, IsWindowEnabled(control) ? ui_style::Text : ui_style::Disabled);
    SetBkColor(dc, background);
    return reinterpret_cast<LRESULT>(ui_style::AreaBrush(header, bottom));
}


namespace ui_style {
namespace {
HBRUSH inputBrush = nullptr;
HBRUSH panelBrush = nullptr;
HBRUSH canvasBrush = nullptr;
HBRUSH headerBrush = nullptr;

HBRUSH Brush(HBRUSH& brush, COLORREF color) {
    if (brush == nullptr)
        brush = CreateSolidBrush(color);
    return brush;
}

HBRUSH Background(HWND hwnd) {
    if (g_Block == nullptr)
        return AreaBrush(false, true);
    const HWND main = static_cast<HWND>(g_Block->Hwnd());
    RECT window{}, client{};
    GetWindowRect(hwnd, &window);
    GetClientRect(main, &client);
    POINT point{window.left, window.top};
    ScreenToClient(main, &point);
    return AreaBrush(point.y < 50 && point.x < g_Block->SidebarWidth(),
                     point.y >= client.bottom - 160);
}

void DrawLabel(HDC dc, HWND hwnd, RECT bounds, COLORREF color, UINT align) {
    wchar_t label[256]{};
    GetWindowTextW(hwnd, label, 256);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ old = font != nullptr ? SelectObject(dc, font) : nullptr;
    DrawTextW(dc, label, -1, &bounds, align | DT_SINGLELINE |
              DT_VCENTER | DT_END_ELLIPSIS);
    if (old != nullptr)
        SelectObject(dc, old);
}

void PaintToggle(HDC dc, HWND hwnd) {
    RECT r{};
    GetClientRect(hwnd, &r);
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    const bool pushLike = (style & BS_PUSHLIKE) != 0;
    const bool checked = SendMessageW(hwnd, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
    const bool enabled = IsWindowEnabled(hwnd) != FALSE;
    const COLORREF fg = enabled ? Text : Disabled;

    if (pushLike) {
        HBRUSH b = CreateSolidBrush(checked ? AccentDim : Button);
        FillRect(dc, &r, b);
        DeleteObject(b);
        if (checked) {
            RECT bar{r.left, r.bottom - 2, r.right, r.bottom};
            HBRUSH a = CreateSolidBrush(Accent);
            FillRect(dc, &bar, a);
            DeleteObject(a);
        }
        DrawLabel(dc, hwnd, r, fg, DT_CENTER);
    } else {
        FillRect(dc, &r, Background(hwnd));
        RECT indicator{r.left + 2, (r.top + r.bottom - 12) / 2,
                       r.left + 14, (r.top + r.bottom + 12) / 2};
        const DWORD type = style & BS_TYPEMASK;
        const bool radio = type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
        HBRUSH fill = CreateSolidBrush(checked ? AccentDim : Input);
        HPEN pen = CreatePen(PS_SOLID, 1, checked ? Accent : Border);
        HGDIOBJ oldBrush = SelectObject(dc, fill);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        if (radio)
            Ellipse(dc, indicator.left, indicator.top, indicator.right, indicator.bottom);
        else
            Rectangle(dc, indicator.left, indicator.top, indicator.right, indicator.bottom);
        if (checked) {
            HPEN mark = CreatePen(PS_SOLID, 2, Accent);
            SelectObject(dc, mark);
            if (radio) {
                Ellipse(dc, indicator.left + 4, indicator.top + 4,
                        indicator.right - 4, indicator.bottom - 4);
            } else {
                MoveToEx(dc, indicator.left + 3, indicator.top + 6, nullptr);
                LineTo(dc, indicator.left + 5, indicator.top + 9);
                LineTo(dc, indicator.right - 2, indicator.top + 3);
            }
            SelectObject(dc, pen);
            DeleteObject(mark);
        }
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(fill);
        DeleteObject(pen);
        RECT text = r;
        text.left += 19;
        DrawLabel(dc, hwnd, text, fg, DT_LEFT);
    }

    if (GetFocus() == hwnd) {
        RECT focus = r;
        InflateRect(&focus, -2, -2);
        DrawFocusRect(dc, &focus);
    }
}

LRESULT CALLBACK ToggleProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam, UINT_PTR id, DWORD_PTR) {
    if (message == WM_PAINT) {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        PaintToggle(dc, hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (message == WM_PRINTCLIENT) {
        PaintToggle(reinterpret_cast<HDC>(wParam), hwnd);
        return 0;
    }
    if (message == WM_ERASEBKGND)
        return 1;
    if (message == WM_NCDESTROY)
        RemoveWindowSubclass(hwnd, ToggleProc, id);
    const LRESULT value = DefSubclassProc(hwnd, message, wParam, lParam);
    if (message == BM_SETCHECK || message == WM_SETTEXT ||
        message == WM_ENABLE || message == WM_SETFOCUS ||
        message == WM_KILLFOCUS)
        InvalidateRect(hwnd, nullptr, FALSE);
    return value;
}
}

HBRUSH InputBrush() {
    return Brush(inputBrush, Input);
}

HBRUSH AreaBrush(bool header, bool bottom) {
    if (header)
        return Brush(headerBrush, Header);
    if (bottom)
        return Brush(panelBrush, Surface);
    return Brush(canvasBrush, SurfaceAlt);
}

bool PaintButton(const DRAWITEMSTRUCT* item) {
    if (item == nullptr || item->CtlType != ODT_BUTTON)
        return false;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    const bool disabled = (item->itemState & ODS_DISABLED) != 0;
    const bool primary = item->CtlID == 408 || item->CtlID == 500 ||
                         item->CtlID == 452;
    const COLORREF face = pressed ? ButtonDown :
        primary && !disabled ? AccentDim : Button;
    HBRUSH brush = CreateSolidBrush(face);
    FillRect(item->hDC, &item->rcItem, brush);
    DeleteObject(brush);
    if (primary && !disabled) {
        RECT bar{item->rcItem.left, item->rcItem.bottom - 2,
                 item->rcItem.right, item->rcItem.bottom};
        HBRUSH accent = CreateSolidBrush(Accent);
        FillRect(item->hDC, &bar, accent);
        DeleteObject(accent);
    }
    DrawLabel(item->hDC, item->hwndItem, item->rcItem,
              disabled ? Disabled : Text, DT_CENTER);
    if ((item->itemState & ODS_FOCUS) != 0) {
        RECT focus = item->rcItem;
        InflateRect(&focus, -2, -2);
        DrawFocusRect(item->hDC, &focus);
    }
    return true;
}

void InstallToggle(HWND control, DWORD style) {
    const DWORD type = style & BS_TYPEMASK;
    if (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX ||
        type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON ||
        type == BS_3STATE || type == BS_AUTO3STATE)
        SetWindowSubclass(control, ToggleProc, 1, 0);
}

void ReleaseBrushes() {
    HBRUSH* brushes[] = {&inputBrush, &panelBrush, &canvasBrush, &headerBrush};
    for (HBRUSH* item : brushes) {
        if (*item != nullptr) {
            DeleteObject(*item);
            *item = nullptr;
        }
    }
}
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
