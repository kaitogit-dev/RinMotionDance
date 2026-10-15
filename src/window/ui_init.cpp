#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>
#include <d3d9.h>
#include <mmsystem.h>
#include <dsound.h>
#include <shellapi.h>   
#include <objbase.h>    

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include <cstdio>
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"
#include "ui_controls.inc"

namespace rinmotiondance {

static int EvalPos(const ui::PosExpr& e, int sidebar) {
    switch (e.kind) {
        case 1:  return sidebar + e.off;        
        case 2:  return sidebar / 2 + e.off;    
        default: return e.off;                  
    }
}

static unsigned char* NewZeroed(std::size_t bytes) {
    unsigned char* p = static_cast<unsigned char*>(::operator new(bytes));
    std::memset(p, 0, bytes);
    return p;
}

#ifdef RINMOTIONDANCE_DIAG
static void TraceInitialAccessoryTrack(const MMDApp& app) {
    const char* directory = std::getenv("RINMOTIONDANCE_PMM_TRACE_DIR");
    if (directory == nullptr || directory[0] == '\0')
        return;
    char path[MAX_PATH];
    sprintf_s(path, "%s\\pmm_model_load.log", directory);
    FILE* stream = nullptr;
    if (fopen_s(&stream, path, "ab") != 0 || stream == nullptr)
        return;
    fprintf(stream,
            "stage=ui-initial-accessory-track-174 image=%p track=%p\r\n",
            GetModuleHandleW(nullptr), app.AccessoryKeyTracks()[174]);
    fclose(stream);
}
#else
static inline void TraceInitialAccessoryTrack(const MMDApp&) {}
#endif

static bool InitTimelineAudio(MMDApp* app, HWND hwnd, HDC timeline,
                              bool english) {
    WaveAudioContext* audio = app->Audio();
    audio->englishUI = english ? 1 : 0;
    audio->mainWindow = hwnd;

#ifdef RINMOTIONDANCE_DIAG
    if (std::getenv("RINMOTIONDANCE_SKIP_DSOUND") != nullptr)
        return false;
#endif

    IDirectSound*& directSound = audio->directSound;
    if (FAILED(DirectSoundCreate(nullptr, &directSound, nullptr))) {
        static const char kJpCannotMakeDirectSound[] =
            "DirectSound\x82\xAA\x90\xB6\x90\xAC\x82\xC5\x82\xAB\x82\xDC"
            "\x82\xB9\x82\xF1\nDirectSound\x82\xCC\x8E\x67\x97\x70\x82\xAA"
            "\x95\x73\x89\xC2\x94\x5C\x82\xC5\x82\xB7";
        MessageBoxA(hwnd,
                    english ? "Cannot make DirectSound!!"
                            : kJpCannotMakeDirectSound,
                    "", MB_OK);
        return false;
    }
    if (FAILED(directSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))) {
        static const char kJpFailedCooperateDirectSound[] =
            "DirectSound\x82\xCC\x8B\xA2\x92\xB2\x82\xC9\x8E\xB8\x94\x73\x82\xB5\x82\xDC\x82\xB5\x82\xBD";
        MessageBoxA(hwnd,
                    english ? "Failed cooperate DirectSound!!"
                            : kJpFailedCooperateDirectSound,
                    "", MB_OK);
        return false;
    }

    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = 44100;
    format.nAvgBytesPerSec = 176400;
    format.nBlockAlign = 4;
    format.wBitsPerSample = 16;

    DSBUFFERDESC desc{};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x81E0;
    desc.dwBufferBytes = 352800;
    desc.lpwfxFormat = &format;
    audio->bufferBytes = static_cast<std::int32_t>(desc.dwBufferBytes);

    IDirectSoundBuffer*& buffer = audio->streamingBuffer;
    if (FAILED(directSound->CreateSoundBuffer(&desc, &buffer, nullptr)))
        buffer = nullptr;
    if (buffer != nullptr) {
        buffer->SetCurrentPosition(0);
        buffer->Play(0, 0, DSBPLAY_LOOPING);
        Sleep(1);
        buffer->Stop();
        buffer->Release();
        buffer = nullptr;
    }

    audio->timelineDC = timeline;
    return true;
}

void ClearTimelineAndCurveDCs(MMDApp* app) {
    HDC timeline = app->TimelineDC();
    HDC curve = app->CurveDC();
    D3DRenderer* r = app->Renderer();
    const int backbufferWidth = r->screenWidth;

    HPEN whitePen = CreatePen(PS_SOLID, 1, 0xFFFFFF);
    HBRUSH whiteBrush = CreateSolidBrush(0xFFFFFF);
    HGDIOBJ oldPen = SelectObject(timeline, whitePen);
    HGDIOBJ oldBrush = SelectObject(timeline, whiteBrush);
    SelectObject(curve, whitePen);
    SelectObject(curve, whiteBrush);
    Rectangle(timeline, 0, 0, backbufferWidth, 49);
    Rectangle(curve, 0, 0, 127, 127);
    SelectObject(timeline, oldPen);
    SelectObject(curve, oldPen);
    DeleteObject(whitePen);
    SelectObject(timeline, oldBrush);
    SelectObject(curve, oldBrush);
    DeleteObject(whiteBrush);

    HPEN blackPen = CreatePen(PS_SOLID, 1, 0);
    SelectObject(timeline, blackPen);
    MoveToEx(timeline, 0, 25, nullptr);
    LineTo(timeline, backbufferWidth, 25);
    SelectObject(timeline, oldPen);
    DeleteObject(blackPen);
}

static bool LoadPngTexture(MMDApp* app, IDirect3DDevice9* device,
                           WORD resourceId, IDirect3DTexture9** output) {
    HMODULE module = static_cast<HMODULE>(app->HInstance());
    HRSRC info = FindResourceA(module, MAKEINTRESOURCEA(resourceId), "PNG");
    if (info == nullptr)
        return false;
    HGLOBAL resource = LoadResource(module, info);
    const void* bytes = resource != nullptr ? LockResource(resource) : nullptr;
    const DWORD size = SizeofResource(module, info);
    auto& api = d3dx::Get();
    if (bytes == nullptr || size == 0)
        return false;
    return SUCCEEDED(api.fromMemEx(
        device, bytes, size, static_cast<UINT>(-1), static_cast<UINT>(-1),
        1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
        static_cast<DWORD>(-1), static_cast<DWORD>(-1), 0,
        nullptr, nullptr, output));
}

bool CreateUIControls(MMDApp* app, HWND hwnd) {
    auto& s = *app;

    if (!InitD3D(app, hwnd, app->EnglishUI() != 0,
                 static_cast<HMODULE>(app->HInstance())))
        return false;

    if (!SceneConstruct(app->Physics(), app->Renderer()))
        return false;

    DragAcceptFiles(hwnd, TRUE);

    HDC hdc = GetDC(hwnd);
    D3DRenderer* wrap = app->Renderer();
    const int bbW = wrap->screenWidth;
    const int bbH = wrap->screenHeight;

    s.TimelineDC() = CreateCompatibleDC(hdc);                          
    HBITMAP ruler = CreateCompatibleBitmap(hdc, bbW, 0x32);            
    s.TimelineBitmap() = ruler;
    SelectObject(s.TimelineDC(), ruler);

    s.PanelDC() = CreateCompatibleDC(hdc);                            
    HBITMAP panel = CreateCompatibleBitmap(hdc, bbW, bbH);             
    s.PanelBitmap() = panel;
    HBITMAP spare = CreateCompatibleBitmap(hdc, bbW, bbH);             
    s.PanelSpareBitmap() = spare;
    SelectObject(s.PanelDC(), panel);

    s.CurveDC() = CreateCompatibleDC(hdc);                             
    HBITMAP curve = CreateCompatibleBitmap(hdc, 0x80, 0x80);           
    s.CurveBitmap() = curve;
    SelectObject(s.CurveDC(), curve);

    HDC scratch = CreateCompatibleDC(hdc);                             
    SelectObject(scratch, spare);

    auto fill = [&](COLORREF color, void (*rects)(HDC, int, int)) {
        HPEN pen = CreatePen(PS_SOLID, 1, color);                      
        HBRUSH brush = CreateSolidBrush(color);                        
        HGDIOBJ oldPen = SelectObject(scratch, pen);                   
        HGDIOBJ oldBrush = SelectObject(scratch, brush);               
        rects(scratch, bbW, bbH);
        SelectObject(scratch, oldBrush);                               
        SelectObject(scratch, oldPen);                                 
        DeleteObject(pen);                                             
        DeleteObject(brush);                                           
    };
    fill(s.ThemeColor(UiThemeColor::TimelineBase),                     
         [](HDC dc, int w, int h) { Rectangle(dc, 0, 0, w, h); });
    fill(s.ThemeColor(UiThemeColor::HeaderBand),                       
         [](HDC dc, int w, int h) {
             Rectangle(dc, 1, 0xF, 0x5A, h);      
             Rectangle(dc, 0x5A, 1, w, 0xF);      
         });
    fill(s.ThemeColor(UiThemeColor::RowBand),                          
         [](HDC dc, int w, int h) {
             for (int i = 0xF; i + 0xF < h; i += 0x1C)                
                 Rectangle(dc, 1, i, 0x5A, i + 0xF);
         });

    fill(ColorLerp(s.ThemeColor(UiThemeColor::ControlLight),
                   s.ThemeColor(UiThemeColor::ControlDark),
                   0.08281938f),
         [](HDC dc, int, int) { Rectangle(dc, 0, 0, 0x59, 0xF); });
    fill(s.ThemeColor(UiThemeColor::TimelineRows),
         [](HDC dc, int w, int h) {
             for (int y = 0xF; y < h; y += 0x1C)
                 Rectangle(dc, 0x5B, y, w, y + 0xF);
         });

    auto drawRows = [&](COLORREF color, int x0, int x1) {
        HPEN rowPen = CreatePen(PS_SOLID, 1, color);
        HGDIOBJ previous = SelectObject(scratch, rowPen);
        for (int y = 0xF; y < bbH; y += 0xE) {
            MoveToEx(scratch, x0, y, nullptr);
            LineTo(scratch, x1, y);
        }
        SelectObject(scratch, previous);
        DeleteObject(rowPen);
    };
    drawRows(s.ThemeColor(UiThemeColor::LabelGrid), 1, 0x5A);
    drawRows(s.ThemeColor(UiThemeColor::TimelineGrid), 0x5B, bbW);

    ReleaseDC(hwnd, hdc);                                             
    DeleteDC(scratch);                                                
    HDC copyDc = CreateCompatibleDC(nullptr);                          
    SelectObject(copyDc, spare);
    BitBlt(s.PanelDC(), 0, 0, bbW, bbH,
           copyDc, 0, 0, SRCCOPY);                                    
    DeleteDC(copyDc);

    ClearTimelineAndCurveDCs(app);  
    s.DirectSoundAvailable() = static_cast<std::uint8_t>(
        InitTimelineAudio(app, hwnd, s.TimelineDC(),
                          app->EnglishUI() != 0));

    SetCurrentDirectoryW(app->ExeDir());                              
    s.state.bmpRes101 = LoadBitmapA(
        static_cast<HMODULE>(s.HInstance()), MAKEINTRESOURCEA(0x65)); 
    s.state.bmpRes119 = LoadBitmapA(
        static_cast<HMODULE>(s.HInstance()), MAKEINTRESOURCEA(0x77)); 

    if (CoInitialize(nullptr) < 0) {
        MessageBoxA(hwnd, "Failed CoInitialize!", "DirectShowInit", 0);
        return false;
    }

    s.RecordingCompletionFlag() = new unsigned char(0);

    s.SetProjectedShadowAmbient(1.0f);
    s.ProjectedShadowDiffuseAlpha() = 0.5f;
    s.ProjectedShadowSpecularAlpha() = 1.0f;

    if (!MakeLineGeometry(app))
        return false;

    IDirect3DDevice9* device = wrap->device;
    if (device != nullptr) {
        if (FAILED(device->CreateVertexBuffer(
                0x6D60, D3DUSAGE_WRITEONLY, 0x144, D3DPOOL_MANAGED,
                &s.OverlayVertices(),
                nullptr)))
            return false;
        device->CreateVertexBuffer(6 * 28, D3DUSAGE_WRITEONLY, 0x144,
                                   D3DPOOL_MANAGED,
                                   &s.LeftViewportVertices(),
                                   nullptr);
        device->CreateVertexBuffer(6 * 28, D3DUSAGE_WRITEONLY, 0x144,
                                   D3DPOOL_MANAGED,
                                   &s.RightViewportVertices(),
                                   nullptr);
        device->CreateVertexBuffer(0xA8, D3DUSAGE_WRITEONLY, 0x144,
                                   D3DPOOL_MANAGED,
                                   &s.AviOverlayVertices(), nullptr);
        device->CreateVertexBuffer(0xA8, D3DUSAGE_WRITEONLY, 0x144,
                                   D3DPOOL_MANAGED,
                                   &s.PictureOverlayVertices(), nullptr);
        device->CreateVertexBuffer(
            0x445C0, D3DUSAGE_WRITEONLY, 0x144, D3DPOOL_MANAGED,
            &s.SpriteOverlayVertices(), nullptr);
        device->CreateVertexBuffer(
            0x30D40, D3DUSAGE_WRITEONLY, 0x44, D3DPOOL_MANAGED,
            &wrap->lineVertexBuffer,
            nullptr);

        if (!LoadPngTexture(app, device, 0x66, &s.OverlayTexture())) {
            static const char kJpHudSheetLoadFailed[] =
                "splite.tga\x93\xC7\x8D\x9E\x8E\xB8\x94\x73";
            MessageBoxA(hwnd,
                        app->EnglishUI() != 0
                            ? "cannot load splite.tga"
                            : kJpHudSheetLoadFailed,
                        "InitFont", MB_OK);
        }
        LoadPngTexture(app, device, 0x72,
            &s.ProjectedShadowRestoreTexture());
    }

    HMENU menu = GetMenu(hwnd);
    CheckMenuItem(menu, 0xD7, MF_CHECKED);
    CheckMenuItem(menu, 0xDD, MF_CHECKED);
    CheckMenuItem(menu, 0xEC, MF_CHECKED);
    CheckMenuItem(menu, 0xF3, MF_CHECKED);
    CheckMenuItem(menu, 0xFE, MF_CHECKED);

    unsigned char* cam = NewZeroed(0xCD140);                    
    s.state.cameraKeyTrack = cam;                               
    unsigned char* trkLight = NewZeroed(0x61A80);               
    s.state.lightKeyTrack = trkLight;                           
    unsigned char* trkShadow = NewZeroed(0x3A980);              
    s.state.selfShadowKeyTrack = trkShadow;                     
    unsigned char* trkPhys = NewZeroed(0x57E40);                
    s.state.gravityKeyTrack = trkPhys;                          
    for (int i = 0; i < 255; ++i) {                             
        unsigned char* acc = NewZeroed(0x927C0);                
        s.AccessoryKeys(i) = reinterpret_cast<mdl::AccessoryKey*>(acc);
        acc[12] = 1;                                            
        *reinterpret_cast<std::int32_t*>(acc + 16) = -1;
        *reinterpret_cast<float*>(acc + 52) = 1.0f;             
        *reinterpret_cast<float*>(acc + 56) = 1.0f;             
        s.ObjectSlot(i) = nullptr;                              
    }
    TraceInitialAccessoryTrack(s);
#ifdef RINMOTIONDANCE_DIAG
    if (std::getenv("RINMOTIONDANCE_PMM_GUARD_ACCESSORY_TRACKS") != nullptr) {
        SYSTEM_INFO systemInfo{};
        GetSystemInfo(&systemInfo);
        const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(
            &s.AccessoryKeyTracks()[174]);
        const std::uintptr_t page = address & ~(
            static_cast<std::uintptr_t>(systemInfo.dwPageSize) - 1);
        DWORD previousProtection = 0;
        VirtualProtect(reinterpret_cast<void*>(page), systemInfo.dwPageSize,
                       PAGE_READONLY, &previousProtection);
    }
#endif

    *reinterpret_cast<float*>(cam + 12) = -45.0f;   
    *reinterpret_cast<float*>(cam + 20) = 10.0f;    
    cam[64] = 0;                                    
    *reinterpret_cast<std::int32_t*>(cam + 68) = 30;            
    for (int k = 0; k < 840000; k += 84) {                      
        *reinterpret_cast<std::int32_t*>(cam + k + 76) = -1;    
        for (int j = 0; j < 6; ++j) {
            cam[k + 40 + j] = 20;                    
            cam[k + 46 + j] = 20;                    
        }
        std::memset(cam + k + 52, 107, 12);          
    }

    s.CameraFov() = 30.0f;
    float* direction = s.LightDirection();
    direction[0] = -0.5f;
    direction[1] = -1.0f;
    direction[2] = 0.5f;
    D3DLIGHT9& light = s.SceneLight();
    std::memset(&light, 0, sizeof(light));
    light.Type = D3DLIGHT_DIRECTIONAL;
    for (int j = 0; j < 3; ++j) {
        reinterpret_cast<float*>(&light.Specular)[j] = 0.602f;
        reinterpret_cast<float*>(&light.Ambient)[j] = 0.602f;
    }
    std::memcpy(&light.Direction, direction, sizeof(light.Direction));
    D3DRenderer* w = app->Renderer();
    IDirect3DDevice9* dev = w->device;                          
    dev->SetLight(0, &light);
    dev->LightEnable(0, TRUE);                       

    *reinterpret_cast<float*>(trkLight + 24) = 0.602f;          
    *reinterpret_cast<float*>(trkLight + 28) = 0.602f;
    *reinterpret_cast<float*>(trkLight + 32) = 0.602f;
    *reinterpret_cast<float*>(trkLight + 12) = -0.5f;           
    *reinterpret_cast<float*>(trkLight + 16) = -1.0f;
    *reinterpret_cast<float*>(trkLight + 20) = 0.5f;
    trkShadow[12] = w->postProcessEnabled != 0 ? 1 : 0;  
    *reinterpret_cast<float*>(trkShadow + 16) = 0.01125f;       
    *reinterpret_cast<std::int32_t*>(trkPhys + 28) = 10;        
    *reinterpret_cast<float*>(trkPhys + 12) = 9.8000002f;       
    *reinterpret_cast<float*>(trkPhys + 20) = -1.0f;            

    const int sidebar = app->SidebarWidth();                            
    HINSTANCE hInst = static_cast<HINSTANCE>(app->HInstance());          

    CreateUiFont(app, hwnd);
    const auto initializeTrackRange = [](HWND control, int id) {
        int lo = 0, hi = 0;
        switch (id) {
        case 447: lo = 1; hi = 125; break;
        case 455: case 456: case 457: lo = 0; hi = 255; break;
        case 458: case 459: case 460: lo = -100; hi = 100; break;
        case 505: case 510: case 515: case 520: case 534:
            lo = 0; hi = 100; break;
        case 560: lo = 0; hi = 9999; break;
        default: return;
        }
        SendMessageA(control, TBM_SETRANGEMIN, 0, lo);
        SendMessageA(control, TBM_SETRANGEMAX, 0, hi);

        if (id == 458 || id == 459 || id == 460 || id == 560)
            SendMessageA(control, TBM_SETTICFREQ, 1000, 0);
    };

    for (const ui::ControlSpec& c : ui::kControls) {
        HWND control = c.wide
            ? CreateWindowExW(c.ex, c.wcls, c.wtext, c.style,
                              EvalPos(c.x, sidebar), EvalPos(c.y, sidebar),
                              c.w, c.h, hwnd,
                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(c.id)),
                              hInst, nullptr)
            : CreateWindowExA(c.ex, c.acls, c.atext, c.style,
                              EvalPos(c.x, sidebar), EvalPos(c.y, sidebar),
                              c.w, c.h, hwnd,
                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(c.id)),
                              hInst, nullptr);
        if (control == nullptr)
            return false;   
        ApplyUiFontToControl(app, control, c.id);
        initializeTrackRange(control, c.id);
        InstallControlSubclass(app, control, c.id);
    }

    SetWindowTextA(GetDlgItem(hwnd, panel::kGotoFrameEdit), "0");

    InitToonTextures(app);
    InitSceneFontTexture(app);
    if (!InitAxisMesh(app))
        return false;

    char text[0x100];
    auto setEdit = [&](int id, const char* t) {
        SendMessageA(GetDlgItem(hwnd, id), EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(t));
    };
    auto setTrack = [&](int id, int pos) {
        SendMessageA(GetDlgItem(hwnd, id), TBM_SETPOS, 1, pos);
    };
    sprintf_s(text, 0x100, "%3d",
              static_cast<int>(static_cast<double>(s.LightColor()[0]) *
                               256.0));
    setEdit(461, text);
    sprintf_s(text, 0x100, "%3d",
              static_cast<int>(static_cast<double>(s.LightColor()[1]) *
                               256.0));
    setEdit(462, text);
    sprintf_s(text, 0x100, "%3d",
              static_cast<int>(static_cast<double>(s.LightColor()[2]) *
                               256.0));
    setEdit(463, text);
    sprintf_s(text, 0x100, "%+3.1f", direction[0]);
    setEdit(464, text);
    sprintf_s(text, 0x100, "%+3.1f", direction[1]);
    setEdit(465, text);
    sprintf_s(text, 0x100, "%+3.1f", direction[2]);
    setEdit(466, text);
    sprintf_s(text, 0x100, "%3d",
              static_cast<int>(s.CameraFov()));
    setEdit(448, text);
    setTrack(447, static_cast<int>(s.CameraFov()));
    setEdit(417, "0");
    setTrack(455, static_cast<int>(static_cast<double>(s.LightColor()[0]) *
                                  256.0));
    setTrack(456, static_cast<int>(static_cast<double>(s.LightColor()[1]) *
                                  256.0));
    setTrack(457, static_cast<int>(static_cast<double>(s.LightColor()[2]) *
                                  256.0));
    setTrack(458, static_cast<int>(static_cast<double>(direction[0]) *
                                  100.0));
    setTrack(459, static_cast<int>(static_cast<double>(direction[1]) *
                                  100.0));
    setTrack(460, static_cast<int>(static_cast<double>(direction[2]) *
                                  100.0));

    static constexpr UINT kInitiallyDisabled[] = {
        0xFD, 0xD9, 0xDC, 0xDA, 0xCA, 0xCB, 0xDB, 0xDE,
        0xF9, 0xFA, 0xFB, 0xFC, 0x120, 0x121, 0x124,
    };
    for (UINT id : kInitiallyDisabled)
        EnableMenuItem(menu, id, MF_BYCOMMAND | MF_GRAYED);
    for (UINT id = 0xE0; id <= 0xE7; ++id)
        EnableMenuItem(menu, id, MF_BYCOMMAND | MF_GRAYED);
    for (UINT id = 0x111; id <= 0x113; ++id)
        EnableMenuItem(menu, id, MF_BYCOMMAND | MF_GRAYED);

    if (w->multisampleAvailable == 0)
        EnableMenuItem(menu, 0x115, MF_BYCOMMAND | MF_GRAYED);
    else
        CheckMenuItem(menu, 0x115, MF_BYCOMMAND | MF_CHECKED);

    if (w->postProcessEnabled == 0) {
        EnableMenuItem(menu, 0x117, MF_BYCOMMAND | MF_GRAYED);
        s.SelfShadowMode() = 0;
        s.state.selfShadowEnabled = 0;
    } else {
        CheckMenuItem(menu, 0x117, MF_BYCOMMAND | MF_CHECKED);
        s.SelfShadowMode() = 1;
        s.state.selfShadowEnabled = 1;
    }

    if (s.FrameVolumeControlEnabled() != 0) {
        CheckMenuItem(menu, 0x12B, MF_BYCOMMAND | MF_CHECKED);
        SendMessageA(GetDlgItem(hwnd, panel::kFrameVolumeCheckbox), BM_SETCHECK, BST_CHECKED, 0);
    }
    static constexpr UINT kInitiallyChecked[] = {
        0x11D, 0x125, 0x127, 0x12A, 0x10D,
    };
    for (UINT id : kInitiallyChecked)
        CheckMenuItem(menu, id, MF_BYCOMMAND | MF_CHECKED);

    setTrack(0x216, 100 - s.FrameNormalization());
    SendMessageA(GetDlgItem(hwnd, panel::kCoordAxisCheckbox), BM_SETCHECK, BST_CHECKED, 0);
    SendMessageA(GetDlgItem(hwnd, panel::kPhysicsCheckbox), BM_SETCHECK, BST_CHECKED, 0);

    MENUITEMINFOA mii{};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE;
    mii.fState = MFS_DISABLED | MFS_GRAYED;
    SetMenuItemInfoA(GetSubMenu(menu, 7), 2, TRUE, &mii);
    DrawMenuBar(hwnd);

    static const char kJpFullScreen[] =
        "\x83\x74\x83\x8B\x83\x58\x83\x4E\x83\x8A\x81\x5B"
        "\x83\x93\x95\x5C\x8E\xA6(Alt+Enter)";
    static const char kJpNvidia3D[] =
        "NVIDIA 3D Vision\x95\x5C\x8E\xA6(Alt+Enter)";
    const bool stereoCapable = w->stereoEnabled != 0;
    const char* fullScreenText;
    if (app->EnglishUI() != 0)
        fullScreenText = stereoCapable
            ? "NDIVIA 3D Vision(Alt+Enter)"
            : "full screen(Alt+Enter)";
    else
        fullScreenText = stereoCapable ? kJpNvidia3D : kJpFullScreen;
    mii = {};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = const_cast<char*>(fullScreenText);
    SetMenuItemInfoA(GetSubMenu(menu, 2), 0x1D, TRUE, &mii);

    SendMessageA(GetDlgItem(hwnd, panel::kEditMode1Checkbox), BM_SETCHECK, BST_CHECKED, 0);
    const double rangeValue =
        10000.0 - static_cast<double>(s.state.physicsInterval) *
                      100000.0;
    const int rangePos = static_cast<int>(rangeValue + 0.5);
    setTrack(0x230, rangePos);
    sprintf_s(text, 0x100, "%d", rangePos);
    SetWindowTextA(GetDlgItem(hwnd, panel::kSelfShadowRangeEdit), text);
    return true;
}

void RefillBoneRegisterCombo(MMDApp* app, int slot) {
    HWND combo = GetDlgItem(static_cast<HWND>(app->Hwnd()), panel::kBoneRegisterCombo);
    SendMessageA(combo, CB_RESETCONTENT, 0, 0);                  
    if (slot >= 0) {
        unsigned char* model = app->ModelSlot(slot);
        if (model != nullptr) {
            const std::int32_t boneCount =
                *reinterpret_cast<std::int32_t*>(model + 11652);
            rinmotiondance::mdl::BoneRecord* bones =
                rinmotiondance::mdl::Bones(model);
            for (std::int32_t i = 0; i < boneCount; ++i) {       
                rinmotiondance::mdl::BoneRecord* bone = &bones[i];
                const mdl::BoneType type = bone->type;
                if (type < mdl::BoneType::InertTip ||
                    type == mdl::BoneType::FixedAxis) {
                    const char* name =
                        app->EnglishUI() != 0
                            ? reinterpret_cast<const char*>(bone->nameEn)
                            : reinterpret_cast<const char*>(bone);
                    SendMessageA(combo, CB_ADDSTRING, 0,
                                 reinterpret_cast<LPARAM>(name));
                }
            }
        }
    }
    PostLanguageSweep2(app);                                     
}

}  
