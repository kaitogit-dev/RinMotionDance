#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
namespace rinmotiondance {

void CreateUiFont(MMDApp* app, HWND hwnd) {
    auto& s = *app;

    NONCLIENTMETRICSA metrics;                                    
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoA(SPI_GETNONCLIENTMETRICS  , 0x154,
                          &metrics, 0);
    std::memcpy(s.state.logFont,
                &metrics.lfMessageFont, sizeof(LOGFONTA));

    const bool english = s.EnglishUI() != 0;                      
    HFONT font = CreateFontA(
        english ? 13 : 12, 0, 0, 0,
        400  , FALSE, FALSE, FALSE,
        SHIFTJIS_CHARSET  ,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY  ,
        FIXED_PITCH | FF_MODERN  ,
        english ? "Tahoma" : "MS PGothic");                       
    s.state.hFontUI = font;                                       

}

void ApplyUiFontToControl(MMDApp* app, HWND control, int id) {
        switch (id) {
        case 417:
        case 418:
        case 419:
        case 532:
        case 533:
        case 558:
        case 559:
            return;
        default:
            break;
        }
        if (control != nullptr)
            SendMessageA(control, WM_SETFONT,
                         reinterpret_cast<WPARAM>(app->state.hFontUI),
                         TRUE);
}

}  
