#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/ui_style.hpp"

namespace rinmotiondance {

void PanelPaint(MMDApp* app);

namespace {
MMDApp* previousApp = nullptr;
int previousX = -1;

int CurrentPlayheadX(MMDApp* app) {
    const int column = app->state.currentFrame -
                       app->state.timelineStartFrame;
    const int columns = std::max(1, (app->SidebarWidth() - 125) / 13);
    if (column < 0 || column >= columns)
        return -1;
    return 106 + column * 13;
}

void DrawPlayhead(MMDApp* app, HDC dc, int x) {
    if (x < 0 || dc == nullptr)
        return;
    RECT client{};
    GetClientRect(static_cast<HWND>(app->Hwnd()), &client);
    const int bottom = client.bottom - 248;
    if (bottom <= 157)
        return;
    HPEN pen = CreatePen(PS_SOLID, 2, ui_style::Accent);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    MoveToEx(dc, x, 154, nullptr);
    LineTo(dc, x, bottom);
    SelectObject(dc, oldPen);
    DeleteObject(pen);

    POINT triangle[3] = {{x - 6, 146}, {x + 6, 146}, {x, 157}};
    HBRUSH brush = CreateSolidBrush(ui_style::Accent);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
    Polygon(dc, triangle, 3);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(brush);
}
}

void DrawTimelinePlayhead(MMDApp* app, HDC dc) {
    if (app != nullptr && dc != nullptr)
        DrawPlayhead(app, dc, CurrentPlayheadX(app));
}

void UpdatePlaybackPlayhead(MMDApp* app, float seconds) {
    if (app == nullptr || app->FloatingWindow() != nullptr ||
        app->FullscreenMode() != 0 ||
        !std::isfinite(seconds) || seconds < 0.0f)
        return;
    const int frame = static_cast<int>(std::min(
        static_cast<double>(seconds) * 30.0,
        static_cast<double>(INT_MAX - 1)));
    if (app->state.currentFrame == frame)
        return;
    const HWND hwnd = static_cast<HWND>(app->Hwnd());
    if (previousApp != app) {
        previousApp = app;
        previousX = -1;
    }
    HDC dc = GetDC(hwnd);
    if (dc == nullptr)
        return;
    RECT client{};
    GetClientRect(hwnd, &client);
    if (previousX >= 0) {
        const int restoreY = 145;
        const int restoreH = std::max(0, static_cast<int>(client.bottom) - 248 - restoreY);
        BitBlt(dc, previousX - 7, restoreY, 15, restoreH,
               app->PanelDC(), previousX - 13, 0, SRCCOPY);
    }
    app->state.currentFrame = frame;
    const int columns = std::max(1, (app->SidebarWidth() - 125) / 13);
    if (frame < app->state.timelineStartFrame ||
        frame >= app->state.timelineStartFrame + columns) {
        app->state.timelineStartFrame = std::max(0, frame - columns / 2);
        PanelPaint(app);
    }
    previousX = CurrentPlayheadX(app);
    DrawPlayhead(app, dc, previousX);
    ReleaseDC(hwnd, dc);
    char label[24];
    sprintf_s(label, "%d", frame);
    SetWindowTextA(GetDlgItem(hwnd, 417), label);
}

void TimelineDrawTicks(int frameOffset, int width) {
    MMDApp* app = g_Block;
    WaveAudioContext* audio = app->Audio();

    int sampleCursor = 13 * frameOffset - 101;  
    int xMax = width - 10;

    HDC strip = audio->timelineDC;
    char* bufA = reinterpret_cast<char*>(audio->waveformMax);
    char* bufB = reinterpret_cast<char*>(audio->waveformMin);

    HPEN whitePen = CreatePen(PS_SOLID, 1, ui_style::Input);       
    HBRUSH whiteBrush = CreateSolidBrush(ui_style::Input);
    HGDIOBJ oldPen = SelectObject(strip, whitePen);         
    HGDIOBJ oldBrush = SelectObject(strip, whiteBrush);     
    Rectangle(strip, 0, 0, xMax, 50);                       
    SelectObject(strip, oldPen);
    DeleteObject(whitePen);

    HPEN redPen = CreatePen(PS_SOLID, 1, ui_style::Accent);         
    SelectObject(strip, redPen);                            
    int xa = 0;                                             
    int columnX = 0;                                        
    for (xa = 0; columnX < xMax; xa = columnX) {            
        if (sampleCursor >= 0 && sampleCursor < audio->waveformColumns) {
            MoveToEx(strip, columnX, bufA[sampleCursor], nullptr);   
            LineTo(strip, xa, bufB[sampleCursor]);           
            columnX = xa;
        }
        ++columnX;
        ++sampleCursor;
    }
    SelectObject(strip, oldPen);
    DeleteObject(redPen);

    HPEN blackPen = CreatePen(PS_SOLID, 1, ui_style::Border);              
    SelectObject(strip, blackPen);
    MoveToEx(strip, 0, 25, nullptr);
    LineTo(strip, xMax, 25);                                
    SelectObject(strip, oldPen);
    SelectObject(strip, oldBrush);
    DeleteObject(blackPen);
    DeleteObject(whiteBrush);                               
}

void SetFrameNormalized(int frame) {
    MMDApp* app = g_Block;
    WaveAudioContext* audio = app->Audio();

    IDirectSoundBuffer* player = audio->streamingBuffer;
    if (player == nullptr)                                  
        return;

    float seconds = static_cast<float>(static_cast<double>(frame) / 100.0);

    if (seconds < 1.0f) {
        if (seconds > 0.0f) {
            float log10Val = static_cast<float>(std::log10(static_cast<double>(seconds)));
            audio->volume =
                static_cast<int>(static_cast<double>(log10Val) *
                                 33.20000076293945 *          
                                 100.0);
            player->SetFrequency(static_cast<DWORD>(audio->volume));
        } else {
            audio->volume = -10000;                         
            player->SetFrequency(static_cast<DWORD>(audio->volume)); 
        }
    } else {
        audio->volume = 0;                                   
        player->SetFrequency(static_cast<DWORD>(audio->volume)); 
    }
}

}  
