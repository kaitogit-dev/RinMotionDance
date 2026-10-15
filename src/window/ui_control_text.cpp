#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

void DrawControlText(MMDApp* app, HWND hwnd, HDC hdc, RECT rc) {
    char text[0x100];
    GetWindowTextA(hwnd, text, 0x100);
    SetBkMode(hdc, TRANSPARENT);  

    LOGFONTA* lf = static_cast<LOGFONTA*>(GlobalAlloc(GPTR, sizeof(LOGFONTA)));
    if (app->EnglishUI() != 0) {                       
        strcpy_s(lf->lfFaceName, sizeof(lf->lfFaceName), "Tahoma");
        lf->lfHeight = 13;                             
    } else {
        strcpy_s(lf->lfFaceName, sizeof(lf->lfFaceName),
                 "\x82\x6c\x82\x72 \x82\x6f\x83\x53\x83\x56\x83\x62\x83\x4e");
        lf->lfHeight = 12;                             
    }
    lf->lfWeight = 700;                                
    lf->lfEscapement = 0;
    lf->lfWidth = 0;
    lf->lfItalic = FALSE;
    lf->lfUnderline = FALSE;
    lf->lfCharSet = 0x80;                              

    HFONT font = CreateFontIndirectA(lf);
    HGDIOBJ oldFont = SelectObject(hdc, font);
    DrawTextA(hdc, text, -1, &rc,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);  
    SelectObject(hdc, oldFont);
    DeleteObject(font);
    GlobalFree(lf);
}


void CloseDataFile(void* file);

void HandleTimer100(MMDApp* app) {
    CloseDataFile(app->Audio());                           
    app->AudioSeekReady() = 1;                             
}

}  
