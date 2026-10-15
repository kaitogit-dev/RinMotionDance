#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>
#include <mmsystem.h>

#include <fcntl.h>
#include <io.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void RefreshAfterFrameApply(MMDApp* app);          
void DisablePlaybackMenus(HWND hwnd);              
void RestorePlaybackMenus(MMDApp* app);            

void KickRecordPhysics(MMDApp* app) {
    (void)app;
}

bool BuildRecordingGraph(DShowRecorder* recorder, HWND hwnd,
                         unsigned char english,
                         void* recStruct, void* config, float fps,
                         const wchar_t* wavPath, float seconds);

namespace {

const char kRecTitleJp[] = "\x98\x5E\x89\xE6\x92\x86\x81\x63";
const char kRecTitleEn[] = "Recording...";
const char kCaptionRecWndJp[] =
    "\x98\x5E\x89\xE6\x83\x45\x83\x42\x83\x93\x83\x68\x83\x45\x8D\xEC\x90\xAC";
const char kMsgSaveFailJp[] =
    "\x8F\x6F\x97\xCD\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xF0\x8D\xEC\x90\xAC"
    "\x82\xC5\x82\xAB\x82\xDC\x82\xB9\x82\xF1\x81\x42\x0A\x91\xBC\x82\xCC\x83"
    "\x41\x83\x76\x83\x8A\x83\x50\x81\x5B\x83\x56\x83\x87\x83\x93\x93\x99\x82"
    "\xC5\x8A\x4A\x82\xA2\x82\xC4\x82\xA2\x82\xC8\x82\xA2\x82\xA9\x8A\x6D\x94"
    "\x46\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2\x81\x42";
const char kCaptionAviOutJp[] = "\x41\x56\x49\x8F\x6F\x97\xCD";
const char kMsgSaveFailEn[] =
    "Cannot make save file.\nPlease check whether other application open it.";
const char kCaptionAviOutEn[] = "AVI save failed";
const char kMsg3dVisionJp[] =
    "\x33\x44\x20\x56\x69\x73\x69\x6F\x6E\x8F\x6F\x97\xCD\x8E\x9E\x82\xCD\x81"
    "\x41\x89\x66\x96\xCA\x89\xF0\x91\xFC\x93\x78\x88\xC8\x8F\xE3\x82\xCC\x83"
    "\x54\x83\x43\x83\x59\x82\xCC\x41\x56\x49\x82\xCD\x8F\x6F\x97\xCD\x82\xC5"
    "\x82\xAB\x82\xDC\x82\xB9\x82\xF1";

struct RecConfig {
    std::uint32_t size;    
    std::int32_t width;
    std::int32_t height;
    std::uint16_t flag1;   
    std::uint16_t bpp;     
    std::uint32_t zero;
    std::uint32_t bytes;   
    std::uint32_t zero2[4];
};
static_assert(sizeof(RecConfig) == 0x28);

bool ProbeWritable(const wchar_t* path) {  
    int fd = -1;
    const errno_t r =
        _wsopen_s(&fd, path, 0x8301  ,
                  0x40  , 0x80  );
    if (r != 0)
        return false;
    if (fd >= 0)
        _close(fd);
    return true;
}

void GrowRenderTarget(MMDApp* app, std::int32_t w, std::int32_t h) {
    auto& s = *app;
    D3DRenderer* r = s.Renderer();
    if (r == nullptr)
        return;
    s.state.recRTW = r->screenWidth;
    s.state.recRTH = r->screenHeight;
    if (w > s.state.recRTW ||
        h > s.state.recRTH) {
        r->presentParameters.BackBufferWidth = w;
        r->presentParameters.BackBufferHeight = h;
        r->screenWidth = w;
        r->screenHeight = h;
        PostDeviceReset(app);  
    }
}

void EnsureRecordRenderTarget(MMDApp* app) {
    auto& s = *app;
    D3DRenderer* r = s.Renderer();
    if (r == nullptr)
        return;
    if (r->multisampleAvailable == 0) {
        if (r->captureSurface == nullptr) {
            auto* device = r->device;
            if (device == nullptr)
                return;
            IDirect3DSurface9* surf = nullptr;
            const HRESULT hr = device->CreateRenderTarget(
                static_cast<std::uint32_t>(r->screenWidth),
                static_cast<std::uint32_t>(r->screenHeight),
                r->backbufferFormat,
                D3DMULTISAMPLE_NONE, 0, TRUE  , &surf, nullptr);
            r->captureSurface = surf;
            if (SUCCEEDED(hr) && surf != nullptr)
                r->multisampleAvailable = 1;
        }
        if (r->multisampleAvailable == 0) {
            auto* device = r->device;
            if (device == nullptr)
                return;
            device->SetRenderTarget(0, r->captureSurface);
        }
    }
}


void RecordStartTail(MMDApp* app) {
    auto& s = *app;
#ifdef RINMOTIONDANCE_DIAG
    if (getenv("RINMOTIONDANCE_TRACE_REC")) {
        FILE* tf = fopen(getenv("RINMOTIONDANCE_TRACE_REC"), "a");
        if (tf) {
            fputs("RecordStartTail enter\n", tf);
            fclose(tf);
        }
    }
#endif
    s.state.recordSavedFrame =
        s.state.currentFrame;               
    if (s.AviRecordStartFrame() !=
        s.state.currentFrame) {
        s.state.currentFrame =
            s.AviRecordStartFrame();                            
        RefreshAfterFrameApply(app);                           
        PostViewRefresh(app);                                  
    }
    s.state.recordPlaybackStartPending = 1;              
    timeBeginPeriod(1);                                        
    KickRecordPhysics(app);                                            
#ifdef RINMOTIONDANCE_DIAG
    if (getenv("RINMOTIONDANCE_TRACE_REC")) {
        FILE* tf = fopen(getenv("RINMOTIONDANCE_TRACE_REC"), "a");
        if (tf) {
            fputs("RecordStartTail done\n", tf);
            fclose(tf);
        }
    }
#endif
}

bool StartRecordGraph(MMDApp* app, std::int32_t outW, std::int32_t outH) {
    auto& s = *app;
    RecConfig config;
    std::memset(&config, 0, sizeof config);
    config.size = 0x28;
    config.width = outW;
    config.height = outH;
    config.flag1 = 1;
    config.bpp = 0x20;
    config.bytes =
        static_cast<std::uint32_t>(outW) * outH * 4;

    const std::int32_t frameStart = s.AviRecordStartFrame();
    const std::int32_t frameEnd = s.AviRecordEndFrame();
    std::int32_t frames = frameEnd - frameStart + 1;
    double f = static_cast<double>(frames);
    if (frames < 0)
        f += 4294967296.0f;  
    const float seconds = static_cast<float>(f / g_FrameScale);

    float fpsF;
    std::memcpy(&fpsF, &s.AviRecordFps(), sizeof fpsF);
    const wchar_t* wavPath =
        (s.AviIncludeWave() != 0 && frameStart == 0)
            ? s.WavePath()
            : nullptr;
    DShowRecorder* recorder = s.Recorder();
    if (recorder == nullptr)
        return false;  
    return BuildRecordingGraph(recorder,
                     static_cast<HWND>(s.state.hwnd),
                     static_cast<unsigned char>(s.EnglishUI() != 0),
                     s.AviOutputPath(), &config, fpsF, wavPath,
                     seconds);
}

}  

void StartAviRecordWindow(MMDApp* app) {
    auto& s = *app;
    if (!ProbeWritable(
            s.AviOutputPath())) {                               
        const HWND main = static_cast<HWND>(s.state.hwnd);
        MessageBoxA(main, s.EnglishUI() != 0 ? kMsgSaveFailEn
                                             : kMsgSaveFailJp,
                    s.EnglishUI() != 0 ? kCaptionAviOutEn : kCaptionAviOutJp,
                    0);                                         
        return;
    }

    GrowRenderTarget(app, s.RenderWidth(), s.RenderHeight());

    RECT rc{0, 0, s.RenderWidth(), s.RenderHeight()};
    AdjustWindowRect(&rc, WS_POPUP | WS_CAPTION, FALSE);                  
    char title[0x32];
    strcpy_s(title, 0x32, s.EnglishUI() != 0 ? kRecTitleEn : kRecTitleJp);
    HINSTANCE hInst = static_cast<HINSTANCE>(s.HInstance());  
    HWND recWnd = CreateWindowExA(                             
        0, "RecWindow", title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 100, 100,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInst,
        nullptr);
    s.RecordingWindow() = recWnd;                              
    if (recWnd == nullptr) {
        const HWND main = s.MainWindow();
        MessageBoxA(main, "CreateWindow failed",
                    s.EnglishUI() != 0 ? "create main window"
                                       : kCaptionRecWndJp,
                    0);                                         
        return;
    }
    ShowWindow(recWnd, SW_SHOW);                               
    UpdateWindow(recWnd);

    EnsureRecordRenderTarget(app);
    RefreshMainWindowViewport(app);                                            
    InvalidateRect(s.MainWindow(), &s.ViewportRect(), FALSE);   

    if (!StartRecordGraph(app, s.RenderWidth(), s.RenderHeight())) {
        return;  
    }
    if (s.state.pictureBackgroundEnabled != 0) {
        PicBgOverlayRefresh(app);                              
    }
    HWND main = static_cast<HWND>(s.state.hwnd);
    EnableWindow(GetDlgItem(main, panel::kPlayButton), FALSE);             
    ShowWindow(main, SW_HIDE);                                
    ShowWindow(main, SW_HIDE);                                
    if (s.state.floatingWindow != 0) {
        ShowWindow(
            static_cast<HWND>(s.state.floatingWindow),
            SW_HIDE);                                          
    }
    RecordStartTail(app);
}

void StartAviRecordFullscreen(MMDApp* app) {
    auto& s = *app;
    const std::int32_t screenW = GetSystemMetrics(SM_CXSCREEN);
    const std::int32_t screenH = GetSystemMetrics(SM_CYSCREEN);
    const std::int32_t w = s.RenderWidth();
    const std::int32_t h = s.RenderHeight();
    const HWND main = static_cast<HWND>(s.state.hwnd);
    if ((h != screenH && screenH <= h) || (w != screenW && screenW <= w)) {
        MessageBoxA(main, s.EnglishUI() != 0
                              ? "output size must less than screen "
                                "resolution in 3D Vision."
                              : kMsg3dVisionJp,
                    s.EnglishUI() != 0 ? "over size" : "3D Vision",
                    0);                                         
        return;
    }

    s.FullscreenMode() = 1;
    ApplyFullscreenWindowState(app);                                            
    PostDeviceReset(app);                                  
    s.RecordingWindow() = main;                                

    EnsureRecordRenderTarget(app);
    RefreshMainWindowViewport(app);                                            

    const std::int32_t scale = s.AviStereoWidthMultiplier();
    const std::int32_t scaledW = w * scale;
    if (!StartRecordGraph(app, scaledW, h)) {
        return;
    }
    ProbeWritable(
        s.AviOutputPath());
    if (s.state.pictureBackgroundEnabled != 0) {
        PicBgOverlayRefresh(app);                              
    }
    RecordStartTail(app);
}

void ApplyFullscreenWindowState(MMDApp* app) {
    auto& s = *app;
    const HWND main = static_cast<HWND>(s.state.hwnd);
    if (main == nullptr)
        return;

    if (s.FullscreenMode() != 0) {
        if (s.FloatingWindow() == nullptr) {
            s.SeparateWindowSidebarWidth() = s.SidebarWidth();
        } else {
            SaveFlagSubsystem(app);                              
            s.state.fullscreenFlagsSaved = 1;
        }
        GetWindowPlacement(main,
                           &s.SavedPlacement());
        s.state.savedMenu = GetMenu(main);
        SetWindowLongA(main, GWL_STYLE, WS_POPUP | WS_VISIBLE);             
        SetWindowPos(main, reinterpret_cast<HWND>(static_cast<LONG_PTR>(
                               0xFFFFFFFE  )),
                     0, 0, GetSystemMetrics(SM_CXSCREEN),
                     GetSystemMetrics(SM_CYSCREEN),
                     SWP_SHOWWINDOW);                   
        SetMenu(main, nullptr);
        for (int id = 400; id <= 0x213; ++id) {                  
            ShowWindow(GetDlgItem(main, id), SW_HIDE);
        }
        ValidateRect(main, nullptr);
        RECT rc;
        GetClientRect(main, &rc);                                
        if (D3DRenderer* r = s.Renderer()) {
            r->presentParameters.BackBufferWidth = rc.right;
            r->presentParameters.BackBufferHeight = rc.bottom;
            r->presentParameters.Windowed = 0;
        }
        s.state.recordFullscreenActive = 1;
        return;
    }

    if (s.state.fullscreenFlagsSaved != 0) {
        s.state.fullscreenFlagsSaved = 0;
        InitFlagSubsystem(app);                                  
    }
    s.SidebarWidth() = s.SeparateWindowSidebarWidth();
    SetWindowLongA(main, GWL_STYLE,
                  WS_VISIBLE | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU |
                      WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);                 
    SetWindowPlacement(main,
                       &s.SavedPlacement());
    SetWindowPos(main, reinterpret_cast<HWND>(static_cast<LONG_PTR>(
                           0xFFFFFFFE  )),
                 0, 0, 0, 0, (SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW)); 
    SetMenu(main, s.state.savedMenu);
    for (int id = 400; id <= 0x213; ++id) {                      
        ShowWindow(GetDlgItem(main, id), SW_SHOW);
    }
    PostModelReload2(app);                                       
    if (s.PlaybackActive() == 0) {
        RestorePlaybackMenus(app);                               
    } else {
        DisablePlaybackMenus(main);                              
    }
    InvalidateRect(main, nullptr, FALSE);
    if (D3DRenderer* r = s.Renderer()) {
        r->presentParameters.BackBufferWidth = r->screenWidth;
        r->presentParameters.BackBufferHeight = r->screenHeight;
        r->presentParameters.Windowed = 1;
        r->presentParameters.FullScreen_RefreshRateInHz = 0;
    }
    s.state.stereoActivated = 0;
    s.state.recordFullscreenActive = 0;
}

}  
