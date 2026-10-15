#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

int DrawGlyph(MMDApp* app, const char* text, HDC hdc, int size, int x, int y,
              unsigned char r, unsigned char g, unsigned char b, int bold) {
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE  , 0x3C);
    auto* lf = static_cast<LOGFONTA*>(GlobalLock(mem));
    const bool english = app->EnglishUI() != 0;                   
    strcpy_s(lf->lfFaceName, 0x20, english ? "Arial" : "MS PGothic");
    lf->lfWidth = 0;
    lf->lfEscapement = 0;
    lf->lfOrientation = 0;
    lf->lfItalic = FALSE;
    lf->lfUnderline = FALSE;
    lf->lfStrikeOut = FALSE;
    lf->lfCharSet = SHIFTJIS_CHARSET  ;
    lf->lfWeight = bold == 1 ? 700 : 400;
    lf->lfHeight = -size;

    HFONT font = CreateFontIndirectA(lf);
    HGDIOBJ prev = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT  );
    SetTextColor(hdc, RGB(r, g, b));                              
    const int len = static_cast<int>(strlen(text));
    TextOutA(hdc, x, y, text, len);
    SIZE extent;
    GetTextExtentPoint32A(hdc, text, len, &extent);
    SelectObject(hdc, prev);
    DeleteObject(font);
    GlobalUnlock(mem);
    GlobalFree(mem);
    return extent.cx;
}

}  
