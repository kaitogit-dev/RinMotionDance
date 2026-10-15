#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

void FillPanelBottom(HDC hdc, int left, int top, int right, int bottom,
                     std::uint32_t color, std::uint32_t color2, int mode) {
    HDC dc = hdc;
    if (color == color2) {
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        HBRUSH brush = CreateSolidBrush(color);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, brush);
        Rectangle(dc, left, top, right, bottom);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
        SelectObject(dc, oldBrush);
        DeleteObject(brush);
    } else {
        TRIVERTEX vtx[2];
        GRADIENT_RECT mesh;
        vtx[0].x = left;
        vtx[0].y = top;
        vtx[0].Red = static_cast<COLOR16>(
            static_cast<std::uint8_t>(color) << 8);
        vtx[0].Green = static_cast<COLOR16>(
            static_cast<std::uint8_t>(color >> 8) << 8);
        vtx[0].Blue = static_cast<COLOR16>(
            static_cast<std::uint8_t>(color >> 16) << 8);
        vtx[0].Alpha = 0;
        vtx[1].x = right;
        vtx[1].y = bottom;
        vtx[1].Red = static_cast<COLOR16>(
            static_cast<std::uint8_t>(color2) << 8);
        vtx[1].Green = static_cast<COLOR16>(
            static_cast<std::uint8_t>(color2 >> 8) << 8);
        vtx[1].Blue = static_cast<COLOR16>(
            static_cast<std::uint8_t>(color2 >> 16) << 8);
        vtx[1].Alpha = 0;
        mesh.UpperLeft = 0;
        mesh.LowerRight = 1;
        GradientFill(dc, vtx, 2, &mesh, 1,
                     (mode & 0xFF) != 0 ? GRADIENT_FILL_RECT_V
                                        : GRADIENT_FILL_RECT_H);
    }

    HPEN border = CreatePen(PS_SOLID, 1, 0);
    HGDIOBJ old = SelectObject(dc, border);
    MoveToEx(dc, left, top, nullptr);
    LineTo(dc, right, top);
    LineTo(dc, right, bottom);
    LineTo(dc, left, bottom);
    LineTo(dc, left, top);
    SelectObject(dc, old);
    DeleteObject(border);
}

}  
