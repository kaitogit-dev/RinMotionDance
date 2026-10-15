#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

void TimelineDrawTicks(int frameOffset, int width) {
    MMDApp* app = g_Block;
    WaveAudioContext* audio = app->Audio();

    int sampleCursor = 13 * frameOffset - 101;  
    int xMax = width - 10;

    HDC strip = audio->timelineDC;
    char* bufA = reinterpret_cast<char*>(audio->waveformMax);
    char* bufB = reinterpret_cast<char*>(audio->waveformMin);

    HPEN whitePen = CreatePen(PS_SOLID, 1, 0xFFFFFF);       
    HBRUSH whiteBrush = CreateSolidBrush(0xFFFFFF);
    HGDIOBJ oldPen = SelectObject(strip, whitePen);         
    HGDIOBJ oldBrush = SelectObject(strip, whiteBrush);     
    Rectangle(strip, 0, 0, xMax, 50);                       
    SelectObject(strip, oldPen);
    DeleteObject(whitePen);

    HPEN redPen = CreatePen(PS_SOLID, 1, 0xFF0000);         
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

    HPEN blackPen = CreatePen(PS_SOLID, 1, 0);              
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
