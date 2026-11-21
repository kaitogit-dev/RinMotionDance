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
#include <gdiplus.h>
#include <string>
#include <cwchar>
#include <algorithm>
#include "rinmotiondance/editor_dock.hpp"

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

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kFovSlider))  
        return brushResult(9);

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kFrameVolumeSlider))  
        return brushResult(10);

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kLightColorSliderR))  
        return brushResult(0);

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kLightColorSliderG))  
        return brushResult(1);

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kLightColorSliderB))  
        return brushResult(2);

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kLightDirSliderX))  
        return brushResult(3);

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kLightDirSliderY))  
        return brushResult(4);

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kLightDirSliderZ))  
        return brushResult(5);

    const bool hit515 = ctrl == GetDlgItem(main, panel::kMorphSlider2);
    const bool hit510 = ctrl == GetDlgItem(main, panel::kMorphSlider1);
    if (message == WM_CTLCOLORSTATIC && (hit515 || hit510))
        return brushResult(6);

    const bool hit520 = ctrl == GetDlgItem(main, panel::kMorphSlider3);
    const bool hit505 = ctrl == GetDlgItem(main, panel::kMorphSlider0);
    if (message == WM_CTLCOLORSTATIC && (hit520 || hit505))
        return brushResult(7);

    if (message == WM_CTLCOLORSTATIC &&
        ctrl == GetDlgItem(main, panel::kSelfShadowRangeSlider))  
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
HBRUSH panelAltBrush = nullptr;
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
    const EditorDock dock = ComputeEditorDock(g_Block, client.right, client.bottom);
    if (dock.enabled && point.y >= dock.top) {
        for (int i = 0; i < 7; ++i) {
            if (dock.width[i] == 0 ||
                point.x < dock.left[i] ||
                point.x >= dock.left[i] + dock.width[i])
                continue;
            if (point.y < dock.top + 24)
                return Brush(headerBrush, Header);
            if ((i & 1) != 0)
                return Brush(panelAltBrush, SurfaceAlt);
            return Brush(panelBrush, Surface);
        }
    }
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

enum class SegmentPart {
    Single,
    First,
    Middle,
    Last
};

SegmentPart SegmentOf(HWND hwnd) {
    if (g_Block == nullptr)
        return SegmentPart::Single;
    RECT client{};
    GetClientRect(static_cast<HWND>(g_Block->Hwnd()), &client);
    if (!ComputeEditorDock(g_Block, client.right, client.bottom).enabled)
        return SegmentPart::Single;
    const int groups[][3] = {
        {435, 437, 0}, {440, 441, 0}, {451, 446, 0},
        {467, 468, 0}, {562, 563, 564}, {472, 473, 477},
        {402, 403, 404}, {405, 406, 407},
        {490, 493, 492}, {491, 494, 501},
        {496, 497, 498}, {500, 495, 499}
    };
    const int id = GetDlgCtrlID(hwnd);
    for (const auto& group : groups) {
        int n = 0;
        while (n < 3 && group[n] != 0)
            ++n;
        for (int i = 0; i < n; ++i) {
            if (id == group[i])
                return i == 0 ? SegmentPart::First :
                       i == n - 1 ? SegmentPart::Last :
                                    SegmentPart::Middle;
        }
    }
    return SegmentPart::Single;
}

struct GraphicsSession {
    ULONG_PTR token{};
    GraphicsSession() {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&token, &input, nullptr);
    }
    ~GraphicsSession() {
        if (token != 0)
            Gdiplus::GdiplusShutdown(token);
    }
};

Gdiplus::Color GraphicsColor(COLORREF color) {
    return Gdiplus::Color(255, GetRValue(color),
                         GetGValue(color), GetBValue(color));
}

void RoundedPath(Gdiplus::GraphicsPath& path,
                 Gdiplus::RectF rect, float radius, SegmentPart part) {
    const bool left = part == SegmentPart::Single || part == SegmentPart::First;
    const bool right = part == SegmentPart::Single || part == SegmentPart::Last;
    const float x = rect.X;
    const float y = rect.Y;
    const float r = x + rect.Width;
    const float b = y + rect.Height;
    const float d = radius * 2.0f;
    if (left)
        path.AddArc(x, y, d, d, 180.0f, 90.0f);
    else
        path.AddLine(x, y, x, y);
    path.AddLine(x + (left ? radius : 0.0f), y,
                 r - (right ? radius : 0.0f), y);
    if (right)
        path.AddArc(r - d, y, d, d, 270.0f, 90.0f);
    path.AddLine(r, y + (right ? radius : 0.0f),
                 r, b - (right ? radius : 0.0f));
    if (right)
        path.AddArc(r - d, b - d, d, d, 0.0f, 90.0f);
    path.AddLine(r - (right ? radius : 0.0f), b,
                 x + (left ? radius : 0.0f), b);
    if (left)
        path.AddArc(x, b - d, d, d, 90.0f, 90.0f);
    path.AddLine(x, b - (left ? radius : 0.0f),
                 x, y + (left ? radius : 0.0f));
    path.CloseFigure();
}

bool IsHovered(HWND hwnd) {
    return GetPropW(hwnd, L"RinMotionDance.ButtonHover") != nullptr;
}

void PaintRoundedButton(HDC dc, HWND hwnd, RECT bounds,
                        bool pressed, bool checked,
                        bool enabled, bool focused) {
    static GraphicsSession session;
    FillRect(dc, &bounds, Background(hwnd));
    const int id = GetDlgCtrlID(hwnd);
    const bool primary = id == 408 || id == 500 || id == 452;
    COLORREF fill = checked ? AccentDim :
                    pressed ? ButtonDown :
                    primary && enabled ? RGB(54, 77, 87) :
                    IsHovered(hwnd) && enabled ? RGB(61, 67, 77) :
                    Button;
    if (!enabled)
        fill = RGB(39, 43, 50);
    if (session.token != 0) {
        Gdiplus::Graphics graphics(dc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        Gdiplus::RectF rectangle(
            static_cast<float>(bounds.left) + 0.5f,
            static_cast<float>(bounds.top) + 0.5f,
            static_cast<float>(bounds.right - bounds.left) - 1.0f,
            static_cast<float>(bounds.bottom - bounds.top) - 1.0f);
        const float radius = std::min(5.0f, rectangle.Height * 0.22f);
        const SegmentPart segment = SegmentOf(hwnd);
        Gdiplus::GraphicsPath path;
        RoundedPath(path, rectangle, radius, segment);
        Gdiplus::SolidBrush brush(GraphicsColor(fill));
        graphics.FillPath(&brush, &path);
        if (segment == SegmentPart::First || segment == SegmentPart::Middle) {
            Gdiplus::Pen seam(Gdiplus::Color(120, 22, 26, 32), 1.0f);
            graphics.DrawLine(&seam, rectangle.X + rectangle.Width,
                              rectangle.Y + 4.0f,
                              rectangle.X + rectangle.Width,
                              rectangle.Y + rectangle.Height - 4.0f);
        }
        if (focused) {
            Gdiplus::Pen keyboardFocus(
                Gdiplus::Color(200, GetRValue(Accent),
                               GetGValue(Accent), GetBValue(Accent)), 1.0f);
            graphics.DrawPath(&keyboardFocus, &path);
        }
    } else {
        HBRUSH brush = CreateSolidBrush(fill);
        HGDIOBJ old = SelectObject(dc, brush);
        HGDIOBJ pen = SelectObject(dc, GetStockObject(NULL_PEN));
        RoundRect(dc, bounds.left, bounds.top, bounds.right,
                  bounds.bottom, 12, 12);
        SelectObject(dc, pen);
        SelectObject(dc, old);
        DeleteObject(brush);
    }
    RECT label = bounds;
    label.left += 2;
    label.right -= 2;
    const int navId = GetDlgCtrlID(hwnd);
    if (navId == 418 || navId == 419 || navId == 532 ||
        navId == 533 || navId == 558 || navId == 559) {
        const bool leftArrow = navId == 418 || navId == 533 || navId == 558;
        const bool edge = navId == 558 || navId == 559;
        const int centerX = (bounds.left + bounds.right) / 2 +
                            (edge ? (leftArrow ? 3 : -3) : 0);
        const int centerY = (bounds.top + bounds.bottom) / 2;
        POINT triangle[3] = {
            {centerX + (leftArrow ? 3 : -3), centerY - 4},
            {centerX + (leftArrow ? -3 : 3), centerY},
            {centerX + (leftArrow ? 3 : -3), centerY + 4}
        };
        HBRUSH ink = CreateSolidBrush(enabled ? Text : Disabled);
        HGDIOBJ oldBrush = SelectObject(dc, ink);
        HGDIOBJ oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
        Polygon(dc, triangle, 3);
        if (edge) {
            HPEN bar = CreatePen(PS_SOLID, 1, enabled ? Text : Disabled);
            SelectObject(dc, bar);
            const int x = centerX + (leftArrow ? -5 : 5);
            MoveToEx(dc, x, centerY - 5, nullptr);
            LineTo(dc, x, centerY + 6);
            SelectObject(dc, oldPen);
            DeleteObject(bar);
        } else {
            SelectObject(dc, oldPen);
        }
        SelectObject(dc, oldBrush);
        DeleteObject(ink);
    } else {
        DrawLabel(dc, hwnd, label, enabled ? Text : Disabled, DT_CENTER);
    }
}

void UpdateHover(HWND hwnd, UINT message) {
    if (message == WM_MOUSEMOVE && IsWindowEnabled(hwnd) && !IsHovered(hwnd)) {
        TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, hwnd, 0};
        if (TrackMouseEvent(&tracking)) {
            SetPropW(hwnd, L"RinMotionDance.ButtonHover",
                     reinterpret_cast<HANDLE>(static_cast<INT_PTR>(1)));
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    } else if (message == WM_MOUSELEAVE || message == WM_NCDESTROY) {
        RemovePropW(hwnd, L"RinMotionDance.ButtonHover");
        if (message == WM_MOUSELEAVE)
            InvalidateRect(hwnd, nullptr, FALSE);
    }
}

LRESULT CALLBACK ButtonProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam, UINT_PTR id, DWORD_PTR) {
    UpdateHover(hwnd, message);
    if (message == WM_NCDESTROY)
        RemoveWindowSubclass(hwnd, ButtonProc, id);
    const LRESULT value = DefSubclassProc(hwnd, message, wParam, lParam);
    if (message == WM_SETTEXT || message == WM_ENABLE ||
        message == WM_SETFOCUS || message == WM_KILLFOCUS)
        InvalidateRect(hwnd, nullptr, FALSE);
    return value;
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
        PaintRoundedButton(dc, hwnd, r, false, checked, enabled,
                           GetFocus() == hwnd);
        return;
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
            RoundRect(dc, indicator.left, indicator.top, indicator.right, indicator.bottom, 5, 5);
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
    UpdateHover(hwnd, message);
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
    PaintRoundedButton(item->hDC, item->hwndItem, item->rcItem,
                       (item->itemState & ODS_SELECTED) != 0, false,
                       (item->itemState & ODS_DISABLED) == 0,
                       (item->itemState & ODS_FOCUS) != 0);
    return true;
}

bool MeasureCombo(MEASUREITEMSTRUCT* item) {
    if (item == nullptr || item->CtlType != ODT_COMBOBOX)
        return false;
    item->itemHeight = 19;
    return true;
}

bool PaintCombo(const DRAWITEMSTRUCT* item) {
    if (item == nullptr || item->CtlType != ODT_COMBOBOX ||
        item->hDC == nullptr || item->hwndItem == nullptr)
        return false;

    const bool disabled = !IsWindowEnabled(item->hwndItem);
    const bool selected = (item->itemState & ODS_SELECTED) != 0;
    HBRUSH background = CreateSolidBrush(selected ? AccentDim : Input);
    FillRect(item->hDC, &item->rcItem, background);
    DeleteObject(background);

    if (item->itemID != static_cast<UINT>(-1)) {
        const LRESULT length = SendMessageA(item->hwndItem, CB_GETLBTEXTLEN,
                                            item->itemID, 0);
        if (length >= 0 && length < 65536) {
            std::string label(static_cast<std::size_t>(length) + 1, '\0');
            SendMessageA(item->hwndItem, CB_GETLBTEXT, item->itemID,
                         reinterpret_cast<LPARAM>(&label[0]));
            RECT area = item->rcItem;
            area.left += 7;
            area.right -= 5;
            HFONT font = reinterpret_cast<HFONT>(
                SendMessageA(item->hwndItem, WM_GETFONT, 0, 0));
            HGDIOBJ previous = font != nullptr
                ? SelectObject(item->hDC, font) : nullptr;
            SetBkMode(item->hDC, TRANSPARENT);
            SetTextColor(item->hDC, disabled ? Disabled : Text);
            DrawTextA(item->hDC, label.c_str(), -1, &area,
                      DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
            if (previous != nullptr)
                SelectObject(item->hDC, previous);
        }
    }

    if ((item->itemState & ODS_FOCUS) != 0 &&
        (item->itemState & ODS_COMBOBOXEDIT) == 0) {
        RECT focus = item->rcItem;
        InflateRect(&focus, -2, -2);
        DrawFocusRect(item->hDC, &focus);
    }
    return true;
}

bool PaintTrackbar(NMCUSTOMDRAW* draw, LRESULT* result) {
    if (draw == nullptr || result == nullptr || draw->hdr.hwndFrom == nullptr ||
        draw->hdr.code != NM_CUSTOMDRAW)
        return false;
    wchar_t className[64]{};
    GetClassNameW(draw->hdr.hwndFrom, className, 64);
    if (std::wcscmp(className, TRACKBAR_CLASSW) != 0)
        return false;
    if (draw->dwDrawStage == CDDS_PREPAINT) {
        *result = CDRF_NOTIFYITEMDRAW;
        return true;
    }
    if (draw->dwDrawStage != CDDS_ITEMPREPAINT)
        return false;

    const DWORD item = static_cast<DWORD>(draw->dwItemSpec);
    if (item == TBCD_TICS) {
        *result = CDRF_SKIPDEFAULT;
        return true;
    }
    if (item == TBCD_CHANNEL) {
        const HWND hwnd = draw->hdr.hwndFrom;
        RECT channel = draw->rc;
        const bool vertical = (GetWindowLongPtrW(hwnd, GWL_STYLE) & TBS_VERT) != 0;
        const int minValue = static_cast<int>(SendMessageW(hwnd, TBM_GETRANGEMIN, 0, 0));
        const int maxValue = static_cast<int>(SendMessageW(hwnd, TBM_GETRANGEMAX, 0, 0));
        const int position = static_cast<int>(SendMessageW(hwnd, TBM_GETPOS, 0, 0));
        const double percent = maxValue > minValue ?
            static_cast<double>(position - minValue) /
            static_cast<double>(maxValue - minValue) : 0.0;
        HBRUSH base = CreateSolidBrush(Border);
        FillRect(draw->hdc, &channel, base);
        DeleteObject(base);
        RECT active = channel;
        if (vertical)
            active.top = channel.bottom - static_cast<int>(
                (channel.bottom - channel.top) * percent);
        else
            active.right = channel.left + static_cast<int>(
                (channel.right - channel.left) * percent);
        HBRUSH fill = CreateSolidBrush(Accent);
        FillRect(draw->hdc, &active, fill);
        DeleteObject(fill);
        *result = CDRF_SKIPDEFAULT;
        return true;
    }
    if (item == TBCD_THUMB) {
        HBRUSH knob = CreateSolidBrush(Accent);
        FillRect(draw->hdc, &draw->rc, knob);
        DeleteObject(knob);
        *result = CDRF_SKIPDEFAULT;
        return true;
    }
    return false;
}

void InstallNativeTheme(HWND control) {
    using SetWindowThemePtr = HRESULT (WINAPI*)(HWND, LPCWSTR, LPCWSTR);
    static const auto theme = []() -> SetWindowThemePtr {
        const HMODULE library = LoadLibraryW(L"uxtheme.dll");
        return library != nullptr
            ? reinterpret_cast<SetWindowThemePtr>(
                GetProcAddress(library, "SetWindowTheme"))
            : nullptr;
    }();
    if (theme != nullptr)
        theme(control, L"DarkMode_Explorer", nullptr);
}

void InstallToggle(HWND control, DWORD style) {
    const DWORD type = style & BS_TYPEMASK;
    if (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX ||
        type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON ||
        type == BS_3STATE || type == BS_AUTO3STATE)
        SetWindowSubclass(control, ToggleProc, 1, 0);
    else if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON)
        SetWindowSubclass(control, ButtonProc, 2, 0);
}

void ReleaseBrushes() {
    HBRUSH* brushes[] = {&inputBrush, &panelBrush, &panelAltBrush,
                        &canvasBrush, &headerBrush};
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
