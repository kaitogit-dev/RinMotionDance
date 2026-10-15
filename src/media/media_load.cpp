#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>
#include <vfw.h>

#include <cstdint>
#include <cstdio>
#include <cwchar>

#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

static const char kMsgAviCaptionJp[] =
    "AVI\x83\x66\x81\x5B\x83\x5E\x93\xC7\x8D\x9E";
static const char kMsgAviOpenFailJp[] =
    "AVI\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xAA\x93\xC7\x82\xDD\x8D\x9E"
    "\x82\xDF\x82\xDC\x82\xB9\x82\xF1";
static const char kMsgAviOpenFail2Jp[] =
    "AVI\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xF0\x93\xC7\x82\xDD\x8D\x9E"
    "\x82\xDF\x82\xDC\x82\xB9\x82\xF1";
static const char kMsgAviInfoFailJp[] =
    "AVIFILEINFO\x82\xF0\x93\xC7\x82\xDD\x8D\x9E\x82\xDF\x82\xDC\x82\xB9"
    "\x82\xF1";
static const char kMsgAviGetStreamFailJp[] =
    "AVIFileGetStream\x8E\xB8\x94\x73";
static const char kMsgAviStreamInfoFailJp[] =
    "AVIStreamInfo\x8E\xB8\x94\x73";
static const char kMsgBmpFailJp[] =
    "BMP\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xF0\x93\xC7\x82\xDD\x8D\x9E"
    "\x82\xDF\x82\xDC\x82\xB9\x82\xF1\x82\xC5\x82\xB5\x82\xBD";
static const char kMsgBmpCaptionJp[] = "BMP\x83\x74\x83\x40\x83\x43\x83\x8B"
                                       "\x93\xC7\x8D\x9E";

struct ImgInfo {                    
    UINT Width, Height, Depth, MipLevels;
};

constexpr float kOverlaySpan = 1.2f;   
constexpr float kOverlayHalf = 0.5f;   

bool WsOpenOk(const wchar_t* path) {
    int fh = -1;
    if (_wsopen_s(&fh, path, _O_BINARY, _SH_DENYNO, _S_IREAD) == 0) {
        _close(fh);
        return true;
    }
    return false;
}

void AviFailTail(MMDApp* app) {
    swprintf_s(app->AviBackgroundPath(), 0x100, L"");              
    app->AviBackgroundEnabled() = 0;                                
}

void MediaAspect(MMDApp* app, std::int32_t width, std::int32_t height,
                 float& scaleOut, std::int32_t& posOut) {
    auto& s = *app;
    const std::int32_t w = s.state.hideLeft - s.state.hideRight;   
    const std::int32_t h2 =
        (s.state.hideBottom - s.state.hideTop) / 2;                
    const float ratio = s.Renderer()->viewScale;                   
    const float spanW = static_cast<float>(w) / ratio;             
    const float zoom = (spanW * kOverlaySpan) / static_cast<float>(width);
    const float drawnH =
        (static_cast<float>(height) * zoom * ratio) / kOverlaySpan;
    scaleOut = zoom;
    posOut = static_cast<std::int32_t>(
        static_cast<float>(h2) - drawnH * kOverlayHalf);           
}

}  

void CopyDirPathW(wchar_t* dest, const wchar_t* src) {
    wcscpy_s(dest, 0x3E8, src);                                     
}

void CopyPathW(wchar_t* dest, const wchar_t* src) {
    wcscpy_s(dest, 0x100, src);                                     
}

void LoadAviFile(MMDApp* app) {
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());

    if (s.AviFrameReader() != nullptr) {
        AVIStreamGetFrameClose(static_cast<PGETFRAME>(s.AviFrameReader()));
        s.AviFrameReader() = nullptr;
    }
    if (s.AviStream() != nullptr) {
        AVIStreamRelease(static_cast<PAVISTREAM>(s.AviStream()));
        s.AviStream() = nullptr;
    }
    if (s.AviFile() != nullptr) {
        AVIFileRelease(static_cast<PAVIFILE>(s.AviFile()));
        s.AviFile() = nullptr;
    }

    PathResolutionWorkspace& paths = app->PathWorkspace();
    ResolveUserFilePath(paths, s.AviBackgroundPath());            
    wchar_t path[0x100];
    wcscpy_s(path, 0x100,
             paths.resolvedPath);                                  

    PAVIFILE file = nullptr;
    if (AVIFileOpenW(&file, path, 0x40  ,
                     nullptr) != 0) {                               
        MessageBoxA(hwnd,
                    s.EnglishUI() != 0 ? "Cannot open AVI file!!"
                                       : kMsgAviOpenFailJp,
                    s.EnglishUI() != 0 ? "open AVI file" : kMsgAviCaptionJp,
                    0);                                             
        s.AviFile() = nullptr;                                      
        AviFailTail(app);
        return;
    }
    s.AviFile() = file;

    wcscpy_s(s.AviBackgroundPath(), 0x100, path);

    AVIFILEINFOA info{};
    if (AVIFileInfoA(file, &info, 0x6C) != 0) {                     
        MessageBoxA(hwnd,
                    s.EnglishUI() != 0 ? "Cannot read AVIFILEINFO!!"
                                       : kMsgAviInfoFailJp,
                    s.EnglishUI() != 0 ? "open AVI file" : kMsgAviCaptionJp,
                    0);                                             
        AVIFileRelease(file);                                       
        s.AviFile() = nullptr;
        AviFailTail(app);
        return;
    }

    PAVISTREAM best = nullptr;
    unsigned short bestPrio = 0xFFFF;
    for (DWORD i = 0; i < info.dwStreams; ++i) {
        PAVISTREAM stream = nullptr;
        if (AVIFileGetStream(file, &stream, 0, i) != 0) {           
            MessageBoxA(hwnd,
                        s.EnglishUI() != 0 ? "Failed AVIFileGetStream"
                                           : kMsgAviGetStreamFailJp,
                        s.EnglishUI() != 0 ? "open AVI file"
                                           : kMsgAviCaptionJp,
                        0);                                         
            AVIFileRelease(file);
            s.AviFile() = nullptr;
            AviFailTail(app);
            return;
        }
        AVISTREAMINFOA si{};
        if (AVIStreamInfoA(stream, &si, 0x8C) != 0) {               
            MessageBoxA(hwnd,
                        s.EnglishUI() != 0 ? "Failed AVIStreamInfo"
                                           : kMsgAviStreamInfoFailJp,
                        s.EnglishUI() != 0 ? "open AVI file"
                                           : kMsgAviCaptionJp,
                        0);                                         
            AVIFileRelease(file);
            s.AviFile() = nullptr;
            AviFailTail(app);
            return;
        }
        if (si.fccType == 0x73646976  ) {                 
            if (best == nullptr || si.wPriority < bestPrio) {       
                if (best != nullptr)
                    AVIStreamRelease(best);                         
                best = stream;
                bestPrio = si.wPriority;                            
            } else {
                AVIStreamRelease(stream);
            }
        } else {
            AVIStreamRelease(stream);                               
        }
    }

    if (best == nullptr) {                                          
        MessageBoxA(hwnd,
                    s.EnglishUI() != 0 ? "Cannot open AVI file"
                                       : kMsgAviOpenFail2Jp,
                    s.EnglishUI() != 0 ? "open AVI file" : kMsgAviCaptionJp,
                    0);                                             
        AVIFileRelease(file);
        s.AviFile() = nullptr;
        AviFailTail(app);
        return;
    }
    s.AviStream() = best;

    PGETFRAME pgf = AVIStreamGetFrameOpen(best, nullptr);  
    s.AviFrameReader() = pgf;                                      
    if (pgf == nullptr) {
        AVIStreamRelease(best);                                     
        s.AviStream() = nullptr;
        MessageBoxA(hwnd,
                    s.EnglishUI() != 0 ? "Cannot open AVI file"
                                       : kMsgAviOpenFail2Jp,
                    s.EnglishUI() != 0 ? "open AVI file" : kMsgAviCaptionJp,
                    0);                                             
        AVIFileRelease(file);
        s.AviFile() = nullptr;
        AviFailTail(app);
        return;
    }

    s.AviStreamStartFrame() = AVIStreamStart(best);                  
    s.AviStreamEndFrame() =
        s.AviStreamStartFrame() + AVIStreamLength(best);             
    const LONG frameMs = AVIStreamSampleToTime(best, 1);            
    s.AviUsesThirtyFpsTiming() =
        (frameMs > 0x20 && frameMs < 0x22) ? 1 : 0;                 

    LPBITMAPINFOHEADER header =
        static_cast<LPBITMAPINFOHEADER>(AVIStreamGetFrame(pgf, 0)); 
    if (header == nullptr) {                                        
        AVIStreamGetFrameClose(pgf);                                
        s.AviFrameReader() = nullptr;
        AVIStreamRelease(best);
        s.AviStream() = nullptr;
        AVIFileRelease(file);
        s.AviFile() = nullptr;
        MessageBoxA(hwnd,
                    s.EnglishUI() != 0 ? "Cannot open AVI file"
                                       : kMsgAviOpenFail2Jp,
                    s.EnglishUI() != 0 ? "open AVI file" : kMsgAviCaptionJp,
                    0);                                             
        s.AviBackgroundEnabled() = 0;                               
        swprintf_s(s.AviBackgroundPath(), 0x100, L"");
        return;
    }

    s.AviFrameWidth() = header->biWidth;                            
    s.AviFrameHeight() = header->biHeight;                          
    s.AviOffsetX() = 0;                                             
    float scale = 0.0f;
    std::int32_t pos = 0;
    MediaAspect(app, header->biWidth, header->biHeight, scale, pos);
    s.AviScale() = scale;
    s.AviOffsetY() = pos;
    CheckMenuItem(GetMenu(hwnd), 0xD8, MF_CHECKED);                 

    if (s.AviBackgroundSurface() != nullptr) {                       
        s.AviBackgroundSurface()->Release();
        s.AviBackgroundSurface() = nullptr;
    }
    s.AviBackgroundEnabled() = 1;                                   
    AviBgOverlayRefresh(app);                                                 
}

void LoadBackgroundPicture(MMDApp* app) {
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());
    wchar_t* stored = s.PictureBackgroundPath();

    PathResolutionWorkspace& paths = app->PathWorkspace();
    ResolveUserFilePath(paths, stored);                             
    wchar_t path[0x100];
    wcscpy_s(path, 0x100,
             paths.resolvedPath);                                  
    if (!WsOpenOk(path)) {                                          
        MessageBoxA(hwnd,
                    s.EnglishUI() != 0 ? "Cannot open BMP file"
                                       : kMsgBmpFailJp,
                    s.EnglishUI() != 0 ? "open BMP file" : kMsgBmpCaptionJp,
                    0);                                             
        swprintf_s(stored, 0x100, L"");                             
        return;
    }
    wcscpy_s(stored, 0x100, path);                                  

    if (s.PictureBackgroundTexture() != nullptr) {                   
        s.PictureBackgroundTexture()->Release();
        s.PictureBackgroundTexture() = nullptr;
    }

    IDirect3DDevice9* device = s.Renderer()->device;                
    auto& api = d3dx::Get();
    IDirect3DTexture9** slot = &s.PictureBackgroundTexture();
    ImgInfo info = {};
    constexpr UINT kD3dxDefault = 0xFFFFFFFFu;
    HRESULT hr = E_FAIL;
    hr = api.fromFileExW(device, stored, 0x400, 0x400, 0, 0,
                         D3DFMT_UNKNOWN, D3DPOOL_MANAGED, kD3dxDefault,
                         kD3dxDefault, 0, &info, nullptr, slot);  
    if (hr != 0) {
        hr = api.fromFileExW(device, stored, 0x200, 0x200, 1, 0,
                             D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                             kD3dxDefault, kD3dxDefault, 0, &info,
                             nullptr, slot);                    
    }
    if (hr != 0) {                                                  
        MessageBoxA(hwnd,
                    s.EnglishUI() != 0 ? "Cannot open BMP file"
                                       : kMsgBmpFailJp,
                    s.EnglishUI() != 0 ? "open BMP file" : kMsgBmpCaptionJp,
                    0);                                             
        swprintf_s(stored, 0x100, L"");                             
        return;
    }

    s.PictureWidth() = static_cast<std::int32_t>(info.Width);
    s.PictureHeight() = static_cast<std::int32_t>(info.Height);
    s.PictureOffsetX() = 0;                                         
    float scale = 0.0f;
    std::int32_t pos = 0;
    MediaAspect(app, static_cast<std::int32_t>(info.Width),
                static_cast<std::int32_t>(info.Height), scale, pos);
    s.PictureScale() = scale;
    s.PictureOffsetY() = pos;
    s.PictureBackgroundEnabled() = 1;                               
    CheckMenuItem(GetMenu(hwnd), 0xE9, MF_CHECKED);                 
    PicBgOverlayRefresh(app);                                                 
}

}  
