#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>
#include <vfw.h>

#include <cstdint>
#include <cstring>
#include <cwchar>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

constexpr double kOverlayScale = 1.2000000476837158;

void ClearAviPathEcho(MMDApp* app) {
    swprintf_s(app->AviBackgroundPath(), 0x100, L"%s%s", L"", L"");
}

void ReleaseAviBackgroundHandles(MMDApp* app) {
    if (app->AviFrameReader() != nullptr) {                       
        AVIStreamGetFrameClose(static_cast<PGETFRAME>(app->AviFrameReader()));
        app->AviFrameReader() = nullptr;
    }
    if (app->AviStream() != nullptr) {                            
        AVIStreamRelease(static_cast<PAVISTREAM>(app->AviStream()));
        app->AviStream() = nullptr;
    }
    if (app->AviFile() != nullptr) {                              
        AVIFileRelease(static_cast<PAVIFILE>(app->AviFile()));
        app->AviFile() = nullptr;
    }
}

HWND OverlayHwnd(MMDApp* app) {
    return static_cast<HWND>(app->state.hwnd);
}

void WriteOverlayQuad(void* locked, double left, double top, double right,
                      double bottom) {
    auto* v = static_cast<float*>(locked);
    v[0x8C / 4] = static_cast<float>(left);
    v[0x54 / 4] = v[0x8C / 4];
    v[0x38 / 4] = v[0x54 / 4];
    v[0x70 / 4] = static_cast<float>(right);
    v[0x1C / 4] = v[0x70 / 4];
    v[0x00 / 4] = v[0x1C / 4];
    v[0x58 / 4] = static_cast<float>(top);
    v[0x3C / 4] = v[0x58 / 4];
    v[0x04 / 4] = v[0x3C / 4];
    v[0x90 / 4] = static_cast<float>(bottom);
    v[0x74 / 4] = v[0x90 / 4];
    v[0x20 / 4] = v[0x74 / 4];
    v[0x08 / 4] = 0.0f;
    v[0x24 / 4] = 0.0f;
    v[0x40 / 4] = 0.0f;
    v[0x5C / 4] = 0.0f;
    v[0x78 / 4] = 0.0f;
    v[0x94 / 4] = 0.0f;
    v[0x28 / 4] = 1.0f;
    v[0x44 / 4] = 1.0f;
    v[0x60 / 4] = 1.0f;
    v[0x7C / 4] = 1.0f;
    v[0x98 / 4] = 1.0f;
    std::memset(reinterpret_cast<unsigned char*>(locked) + 0x10, 0xFF, 4);
    std::memset(reinterpret_cast<unsigned char*>(locked) + 0x2C, 0xFF, 4);
    std::memset(reinterpret_cast<unsigned char*>(locked) + 0x48, 0xFF, 4);
    std::memset(reinterpret_cast<unsigned char*>(locked) + 0x64, 0xFF, 4);
    std::memset(reinterpret_cast<unsigned char*>(locked) + 0x80, 0xFF, 4);
    std::memset(reinterpret_cast<unsigned char*>(locked) + 0x9C, 0xFF, 4);
    v[0x14 / 4] = 1.0f;
    v[0x30 / 4] = 1.0f;
    v[0x84 / 4] = 1.0f;
    v[0x4C / 4] = 0.0f;
    v[0x68 / 4] = 0.0f;
    v[0xA0 / 4] = 0.0f;
    v[0x18 / 4] = 0.0f;
    v[0x50 / 4] = 0.0f;
    v[0x6C / 4] = 0.0f;
    v[0x34 / 4] = 1.0f;
    v[0x88 / 4] = 1.0f;
    v[0xA4 / 4] = 1.0f;
}

struct OverlayPlacement {
    std::int32_t x;
    std::int32_t y;
    float scale;
    std::int32_t width;
    std::int32_t height;
};

void ComputeOverlayRect(MMDApp* app, const OverlayPlacement& placement,
                        double* leftOut, double* topOut,
                        double* rightOut, double* bottomOut) {
    auto& s = *app;
    D3DRenderer* wrapper = s.Renderer();  
    const double ratio = wrapper->viewScale;
    const double k = kOverlayScale;
    const double offXv = static_cast<double>(placement.x);
    const double offYv = static_cast<double>(placement.y);
    const double zoomV = static_cast<double>(placement.scale);
    const double wv = static_cast<double>(placement.width);
    const double hv = static_cast<double>(placement.height);
    const RECT& viewport = s.ViewportRect();
    const std::int32_t rectL = viewport.left;
    const std::int32_t rectT = viewport.top;
    const std::int32_t rectR = viewport.right;
    const std::int32_t rectB = viewport.bottom;

    if (s.RecordingWindow() == nullptr) {                       
        const double left = rectL + offXv * ratio / k - 1.0;
        const double top = offYv * ratio / k + rectT - 1.0;
        *leftOut = left;
        *topOut = top;
        *rightOut = wv * zoomV * ratio / k + left + 1.0;
        *bottomOut = hv * zoomV * ratio / k + top + 1.0;
        return;
    }

    if (wrapper->multisampleAvailable != 0) {  
        const float scale = static_cast<float>(
            static_cast<double>(s.state.renderW) /
            static_cast<double>(rectR - rectL));
        const double sc = scale;
        const double left = sc * offXv * ratio / k - 1.0;
        const double top = sc * offYv * ratio / k - 1.0;
        *leftOut = left;
        *topOut = top;
        *rightOut = wv * zoomV * ratio * sc / k + left + 1.0;
        *bottomOut = hv * zoomV * ratio * sc / k + top + 1.0;
        return;
    }

    const float sxF = static_cast<float>(
        static_cast<double>(wrapper->screenWidth) /   
        static_cast<double>(rectR - rectL));
    const float syF = static_cast<float>(
        static_cast<double>(wrapper->screenHeight) /  
        static_cast<double>(rectB - rectT));
    const double left = static_cast<double>(sxF) * offXv * ratio / k - 1.0;
    const double top = static_cast<double>(syF) * offYv * ratio / k - 1.0;
    *leftOut = left;
    *topOut = top;
    *rightOut =
        wv * zoomV * ratio * sxF / k + left + 1.0;
    *bottomOut =
        hv * zoomV * ratio * syF / k + top + 1.0;
}

void RefreshOverlayVertexBuffer(MMDApp* app, IDirect3DVertexBuffer9* vb,
                                const OverlayPlacement& placement) {
    if (vb == nullptr)
        return;
    void* locked = nullptr;
    if (FAILED(vb->Lock(0, 0xA8, &locked, 0)) || locked == nullptr)  
        return;
    double left, top, right, bottom;
    ComputeOverlayRect(app, placement, &left, &top, &right, &bottom);
    WriteOverlayQuad(locked, left, top, right, bottom);
    vb->Unlock();                                                
}

}  

void AviBgOverlayRefresh(MMDApp* app) {
    auto& s = *app;
    D3DRenderer* wrapper = s.Renderer();
    if (wrapper == nullptr)
        return;
    IDirect3DDevice9* device = wrapper->device;  
    if (device == nullptr)
        return;

    if (s.AviBackgroundTexture() == nullptr) {                    
        IDirect3DTexture9* tex = nullptr;
        const HRESULT hr = device->CreateTexture(                 
            0x400, 0x400, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, &tex, nullptr);
        s.AviBackgroundTexture() = tex;
        if (hr != 0) {
            ReleaseAviBackgroundHandles(app);
            MessageBoxA(OverlayHwnd(app),                         
                        app->EnglishUI() != 0
                            ? "cannot make AVIdispTxtr"
                            : "\x41\x56\x49\x95\x5C\x8E\xA6\x97\x70\x83\x65"
                              "\x83\x4E\x83\x58\x83\x60\x82\xAA\x8D\xEC\x90"
                              "\xAC\x82\xC5\x82\xAB\x82\xDC\x82\xB9\x82\xF1",
                        "InitFont", 0);
            s.AviBackgroundEnabled() = 0;
            ClearAviPathEcho(app);                                
            return;
        }
    }
    if (s.AviBackgroundSurface() == nullptr) {                    
        IDirect3DSurface9* surf = nullptr;
        const HRESULT hr = device->CreateOffscreenPlainSurface(
            s.AviFrameWidth(), s.AviFrameHeight(), D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, &surf, nullptr);
        s.AviBackgroundSurface() = surf;
        if (hr != 0) {
            ReleaseAviBackgroundHandles(app);
            MessageBoxA(OverlayHwnd(app),                         
                        app->EnglishUI() != 0
                            ? "Cannot make Surface for AVI file"
                            : "\x41\x56\x49\x97\x70\x83\x54\x81\x5B\x83\x74"
                              "\x83\x46\x83\x43\x83\x58\x82\xF0\x8D\xEC\x90"
                              "\xAC\x82\xC5\x82\xAB\x82\xDC\x82\xB9\x82\xF1",
                        app->EnglishUI() != 0
                            ? "open AVI file"
                            : "\x41\x56\x49\x83\x66\x81\x5B\x83\x5E\x93\xC7"
                              "\x8D\x9E",
                        0);
            s.AviBackgroundEnabled() = 0;
            ClearAviPathEcho(app);                                
            return;
        }
    }

    PAVISTREAM stream = static_cast<PAVISTREAM>(s.AviStream());
    PGETFRAME getFrame = static_cast<PGETFRAME>(s.AviFrameReader());
    LONG sample;
    if (s.PlaybackActive() == 0) {
        if (s.AviUsesThirtyFpsTiming() == 0) {
            sample = stream != nullptr
                ? AVIStreamTimeToSample(
                      stream, s.state.currentFrame *
                                   1000 / 30)                       
                : 0;
        } else {
            sample = s.state.currentFrame;
        }
    } else if (s.FrameStepPlayback() == 0 ||
               s.AviUsesThirtyFpsTiming() == 0 ||
               s.state.aviRecordFps != g_FrameScale) {
        float secondsF;
        std::memcpy(&secondsF, &s.state.playbackCursorSeconds, sizeof secondsF);
        const LONG t = static_cast<LONG>(
            static_cast<double>(secondsF) * 1000.0);
        sample = (stream != nullptr ? AVIStreamTimeToSample(stream, t)
                                    : 0) +
                 static_cast<std::int32_t>(s.state.a0B10);
    } else {
        sample = s.state.aviBackgroundSample;      
        s.state.aviBackgroundSample = sample + 1;
    }
    if (sample < s.AviStreamStartFrame())
        sample = s.AviStreamStartFrame();
    const LONG last = s.AviStreamEndFrame() - 1;
    if (last < sample)
        sample = last;

    void* bits = getFrame != nullptr ? AVIStreamGetFrame(getFrame, sample)
                                     : nullptr;                   
    if (bits == nullptr) {
        ReleaseAviBackgroundHandles(app);
        MessageBoxA(OverlayHwnd(app),                             
                    app->EnglishUI() != 0
                        ? "Cannot open AVI file"
                        : "\x41\x56\x49\x83\x74\x83\x40\x83\x43\x83\x8B\x82"
                          "\xF0\x93\xC7\x82\xDD\x8D\x9E\x82\xDF\x82\xDC\x82"
                          "\xB9\x82\xF1",
                    app->EnglishUI() != 0
                        ? "open AVI file"
                        : "\x41\x56\x49\x83\x66\x81\x5B\x83\x5E\x93\xC7\x8D"
                          "\x9E",
                    0);
        s.AviBackgroundEnabled() = 0;
        ClearAviPathEcho(app);                                    
        return;
    }

    IDirect3DSurface9* surface = s.AviBackgroundSurface();
    HDC hdc = nullptr;
    if (FAILED(surface->GetDC(&hdc)))                             
        return;
    DrawDibDraw(static_cast<HDRAWDIB>(s.AviDrawDib()), hdc, 0, 0, -1, -1,
                static_cast<LPBITMAPINFOHEADER>(bits),
                static_cast<LPVOID>(
                    static_cast<unsigned char*>(bits) + 0x28),    
                0, 0, -1, -1, 0);
    surface->ReleaseDC(hdc);                                      
    DeleteDC(hdc);  

    IDirect3DTexture9* texture = s.AviBackgroundTexture();
    IDirect3DSurface9* texSurface = nullptr;
    if (FAILED(texture->GetSurfaceLevel(0, &texSurface)) ||       
        texSurface == nullptr)
        return;
    if (FAILED(device->StretchRect(surface, nullptr, texSurface, nullptr,
                                   D3DTEXF_LINEAR))) {            
        device->StretchRect(surface, nullptr, texSurface, nullptr,
                            D3DTEXF_NONE);                        
    }
    texSurface->Release();

    RefreshOverlayVertexBuffer(
        app, s.AviOverlayVertices(),
        {s.AviOffsetX(), s.AviOffsetY(), s.AviScale(),
         s.AviFrameWidth(), s.AviFrameHeight()});
}

void PicBgOverlayRefresh(MMDApp* app) {
    auto& s = *app;
    RefreshOverlayVertexBuffer(
        app, s.PictureOverlayVertices(),
        {s.PictureOffsetX(), s.PictureOffsetY(), s.PictureScale(),
         s.PictureWidth(), s.PictureHeight()});
}


}  
