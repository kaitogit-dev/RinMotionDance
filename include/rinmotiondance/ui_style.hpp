#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace rinmotiondance::ui_style {

inline constexpr COLORREF Window = RGB(22, 24, 28);
inline constexpr COLORREF Surface = RGB(32, 35, 41);
inline constexpr COLORREF SurfaceAlt = RGB(38, 42, 49);
inline constexpr COLORREF Header = RGB(28, 31, 37);
inline constexpr COLORREF Input = RGB(25, 28, 34);
inline constexpr COLORREF Button = RGB(47, 52, 60);
inline constexpr COLORREF ButtonDown = RGB(34, 38, 44);
inline constexpr COLORREF Border = RGB(61, 67, 76);
inline constexpr COLORREF Text = RGB(216, 221, 228);
inline constexpr COLORREF Muted = RGB(154, 165, 178);
inline constexpr COLORREF Accent = RGB(115, 160, 171);
inline constexpr COLORREF AccentDim = RGB(57, 78, 87);
inline constexpr COLORREF Disabled = RGB(106, 116, 128);

HBRUSH InputBrush();
HBRUSH AreaBrush(bool header, bool bottom);
bool PaintButton(const DRAWITEMSTRUCT* item);
bool PaintCombo(const DRAWITEMSTRUCT* item);
bool MeasureCombo(MEASUREITEMSTRUCT* item);
bool PaintTrackbar(NMCUSTOMDRAW* draw, LRESULT* result);
void InstallNativeTheme(HWND control);
void InstallToggle(HWND control, DWORD style);
void ReleaseBrushes();

}
