#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace rinmotiondance::ui_style {

inline constexpr COLORREF Window = RGB(19, 23, 29);
inline constexpr COLORREF Surface = RGB(31, 38, 47);
inline constexpr COLORREF SurfaceAlt = RGB(36, 44, 54);
inline constexpr COLORREF Header = RGB(26, 33, 42);
inline constexpr COLORREF Input = RGB(22, 28, 36);
inline constexpr COLORREF Button = RGB(46, 56, 68);
inline constexpr COLORREF ButtonDown = RGB(30, 38, 47);
inline constexpr COLORREF Border = RGB(62, 73, 85);
inline constexpr COLORREF Text = RGB(228, 234, 240);
inline constexpr COLORREF Muted = RGB(158, 172, 187);
inline constexpr COLORREF Accent = RGB(110, 186, 202);
inline constexpr COLORREF AccentDim = RGB(49, 90, 103);
inline constexpr COLORREF Disabled = RGB(104, 118, 133);

HBRUSH InputBrush();
HBRUSH AreaBrush(bool header, bool bottom);
bool PaintButton(const DRAWITEMSTRUCT* item);
void InstallToggle(HWND control, DWORD style);
void ReleaseBrushes();

}
