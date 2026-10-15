#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <wchar.h>

#include <cstdint>
#include <cstring>
#include <new>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

const int kThemeFileOrder[] = {
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 22, 23, 24, 27, 28, 29, 25, 26, 32, 31, 30, 33, 34, 20, 21,
};

const COLORREF kDefaultThemeColors[] = {
    8421504, 0x545454, 0x464646, 1315860, 3289700, 1315880,
    0x464646, 1315860, 3289700, 1315880, 6566450, 1971220,
    3302450, 1318420, 3302500, 1321000, 6566450, 2626580,
    0x464646, 1315860, 0x464646, 1315860, 1250067, 3946295,
    868768, 1250132, 3946345, 5401478, 4418967, 100,
    9895830, 16750230, 4934475, 0, 6579455,
};

void StripNewline(wchar_t* s) {
    while (*s != L'\n')
        ++s;
    *s = L'\0';
}

constexpr const char kJpCaptionCreateMainWindow[] =
    "\x83\x81\x83\x43\x83\x93\x83\x45\x83\x42\x83\x93\x83\x68\x83\x45"
    "\x8d\xec\x90\xac";

constexpr const char kJpRecWindowRegFailed[] =
    "\x98\x5e\x89\xe6\x97\x70\x83\x45\x83\x42\x83\x93\x83\x68\x83\x45"
    "\x93\x6f\x98\x5e\x8e\xb8\x94\x73";

constexpr const char kJpMicWindowRegFailed[] =
    "\x95\xca\x91\x8b\x93\x6f\x98\x5e\x8e\xb8\x94\x73";

}  


void InitDShowRecorder(DShowRecorder* recorder) {
    const std::int32_t selectedCodec = recorder->selectedCodec;
    std::memset(recorder, 0, sizeof(*recorder));
    recorder->selectedCodec = selectedCodec;
}

bool InitMainWindowAndD3D(MMDApp* app, void* hInstanceIn, int nShowCmd) {
    (void)nShowCmd;
    HINSTANCE hInstance = static_cast<HINSTANCE>(hInstanceIn);
    auto& s = *app;

    auto* renderer = static_cast<D3DRenderer*>(
        operator new(sizeof(D3DRenderer), std::nothrow));
    s.Renderer() = renderer;
    std::memset(renderer, 0, sizeof(D3DRenderer));
    RendererInit(renderer);                                    

    auto* audioContext = static_cast<WaveAudioContext*>(
        operator new(sizeof(WaveAudioContext), std::nothrow));  
    s.Audio() = audioContext;
    std::memset(audioContext, 0, sizeof(WaveAudioContext));
    InitAudioContext(audioContext);                                      

    auto* recorder = static_cast<DShowRecorder*>(
        operator new(sizeof(DShowRecorder), std::nothrow));     
    s.Recorder() = recorder;
    recorder->compressor = nullptr;
    InitDShowRecorder(recorder);                                       

    void* sub04b0 = operator new(sizeof(mdl::AccessoryRecord),
                                 std::nothrow);                 
    s.AxisMeshObject() = sub04b0;
    *static_cast<std::uint32_t*>(sub04b0) = 0;
    InitAccessoryRecord(sub04b0);                                      

    auto* physics = static_cast<PhysicsScene*>(
        operator new(sizeof(PhysicsScene), std::nothrow));
    s.Physics() = physics;
    std::memset(physics, 0, sizeof(PhysicsScene));
    PhysicsSceneInit(physics);                                  

    wchar_t exePath[MAX_PATH];                                  
    GetModuleFileNameW(nullptr, exePath, 0x104);
    wchar_t drive[4], dir[256], fname[256], ext[256];
    _wsplitpath_s(exePath, drive, 3, dir, 0x100, fname, 0x100, ext, 0x100);
    swprintf_s(app->ExeDir(), 0x100, L"%s%s", drive, dir);
    SetCurrentDirectoryW(app->ExeDir());
    FontSubInit(app, app->ExeDir());                            
    _wsetlocale(0, L"japanese");                                

    wcscpy_s(app->DirModel(), 0x3E8, L"UserFile\\Model");       
    wcscpy_s(app->DirAccs(), 0x3E8, L"UserFile\\Accessory");
    wcscpy_s(app->DirBg(), 0x3E8, L"UserFile\\BackGround");
    wcscpy_s(app->DirWave(), 0x3E8, L"UserFile\\Wave");
    wcscpy_s(app->DirPose(), 0x3E8, L"UserFile\\Pose");
    wcscpy_s(app->DirMotion(), 0x3E8, L"UserFile\\Motion");
    wcscpy_s(app->DirUser(), 0x3E8, L"UserFile");

    s.UiTextRed() = 0xFF;
    s.UiTextGreen() = 0xFF;
    s.UiTextBlue() = 0xFF;
    for (int i = 0; i < static_cast<int>(_countof(kDefaultThemeColors)); ++i)
        s.ThemeColorAt(i) = kDefaultThemeColors[i];

    FILE* stream = nullptr;                                     
    if (fopen_s(&stream, "Data\\color.txt", "rt") == 0) {
        char line[256];
        int colorIndex = -1;
        const int colorCount = static_cast<int>(_countof(kThemeFileOrder));
        while (colorIndex < colorCount && fgets(line, sizeof(line), stream)) {
            int r = 0, g = 0, b = 0;
            if (sscanf_s(line, " %d,%d,%d", &r, &g, &b) != 3)
                continue;

            if (colorIndex < 0) {
                s.UiTextRed() = static_cast<unsigned char>(r);
                s.UiTextGreen() = static_cast<unsigned char>(g);
                s.UiTextBlue() = static_cast<unsigned char>(b);
            } else {
                s.ThemeColorAt(kThemeFileOrder[colorIndex]) =
                    static_cast<std::uint32_t>((r & 0xFF) | ((g & 0xFF) << 8) |
                                               ((b & 0xFF) << 16));
            }
            ++colorIndex;
        }
        fclose(stream);
    }

    const float factors[8] = {0.16883117f, 0.2857143f, 0.4025974f,
                              0.53246754f, 0.64935064f, 0.76623374f,
                              0.49350649f, 0.90909094f};
    for (int i = 0; i < 8; ++i)                                 
        s.SetUiBrush(i, CreateSolidBrush(
            ColorLerp(s.ThemeColorAt(12), s.ThemeColorAt(13), factors[i])));
    s.SetUiBrush(8, CreateSolidBrush(
        ColorLerp(s.ThemeColorAt(20), s.ThemeColorAt(21), 0.58441556f)));
    s.SetUiBrush(9, CreateSolidBrush(
        ColorLerp(s.ThemeColorAt(10), s.ThemeColorAt(11), 0.4025974f)));
    s.SetUiBrush(10, CreateSolidBrush(
        ColorLerp(s.ThemeColorAt(18), s.ThemeColorAt(19), 0.21052632f)));

    int screenW = GetSystemMetrics(0);                          
    int screenH = GetSystemMetrics(1);
    RECT rect = {100, 100, 870, 580};
    int runFlagSubsystem = 0;        
    char checkMenu119 = 0;       
    AdjustWindowRect(&rect, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                          WS_MINIMIZEBOX | WS_MAXIMIZEBOX, FALSE);
    LONG winL = rect.left, winT = rect.top, winR = rect.right;
    s.SeparateWindowMaximized() = 0;
    s.SeparateWindowX() = winL;
    s.SeparateWindowY() = winT;
    s.SeparateWindowWidth() = winR - winL;
    s.SeparateWindowHeight() = rect.bottom - winT;
    char checkMenu12D = 1;       
    int checkPhysicsFrame = 1;        
    int startMaximized = 0;        

    int X = 0, Y = 0, nWidth = 0, nHeight = 0;
    int tmp = 0;
    if (fopen_s(&stream, "Data\\mmconfig.ini", "rt") != 0) {    
        nWidth = screenW;
        nHeight = screenH - 30;
        s.Renderer()->screenWidth = screenW;
        s.Renderer()->screenHeight = screenH;
        s.RenderWidth() = 640;
        s.RenderHeight() = 360;
        s.SidebarWidth() = 250;
        if (GetUserDefaultLCID() == 1041)                       
            s.EnglishUI() = 0;
        s.FrameVolumeControlEnabled() = 0;
        s.FrameNormalization() = 100;
    } else {
        fscanf_s(stream, "%d\n", &X);                           
        fscanf_s(stream, "%d\n", &Y);
        fscanf_s(stream, "%d\n", &nWidth);
        fscanf_s(stream, "%d\n", &nHeight);
        fscanf_s(stream, "%d\n", &tmp);
        startMaximized = (tmp != 0);
        fscanf_s(stream, "%d\n", &s.RenderWidth());
        fscanf_s(stream, "%d\n", &s.RenderHeight());
        fscanf_s(stream, "%d\n", &s.SidebarWidth());
        if (fscanf_s(stream, "%d\n", &tmp) == -1) {
            if (GetUserDefaultLCID() == 1041)
                s.EnglishUI() = 0;
        } else {
            if (tmp == 0)
                s.EnglishUI() = 0;
            if (fscanf_s(stream, "%d\n", &runFlagSubsystem) != -1) {
                int maximized = 0;
                fscanf_s(stream, "%d\n", &maximized);
                s.SeparateWindowMaximized() = static_cast<std::uint8_t>(maximized);
                fscanf_s(stream, "%d\n", &s.SeparateWindowX());
                fscanf_s(stream, "%d\n", &s.SeparateWindowY());
                fscanf_s(stream, "%d\n", &s.SeparateWindowWidth());
                fscanf_s(stream, "%d\n", &s.SeparateWindowHeight());
                if (fscanf_s(stream, "%d\n", &tmp) != -1) {
                    if (tmp == 0)
                        checkPhysicsFrame = 0;
                    if (fscanf_s(stream, "%d\n", &tmp) != -1) {
                        fgetws(app->DirModel(), 1000, stream);  
                        StripNewline(app->DirModel());
                        fgetws(app->DirAccs(), 1000, stream);
                        StripNewline(app->DirAccs());
                        fgetws(app->DirWave(), 1000, stream);
                        StripNewline(app->DirWave());
                        fgetws(app->DirPose(), 1000, stream);
                        StripNewline(app->DirPose());
                        fgetws(app->DirMotion(), 1000, stream);
                        StripNewline(app->DirMotion());
                        fgetws(app->DirUser(), 1000, stream);
                        StripNewline(app->DirUser());
                        if (fscanf_s(stream, "%d\n", &tmp) != -1) {
                            fscanf_s(stream, "%d\n", &tmp);
                            s.FrameVolumeControlEnabled() = (tmp != 0);
                            fscanf_s(stream, "%d\n", &s.FrameNormalization());
                            if (fscanf_s(stream, "%d\n", &tmp) != -1) {
                                fscanf_s(stream, "%f\n", &s.SidebarRatio());
                                if (fscanf_s(stream, "%d\n", &tmp) != -1) {
                                    fgetws(app->DirBg(), 1000, stream);
                                    StripNewline(app->DirBg());
                                    if (fscanf_s(stream, "%d\n", &tmp) != -1) {
                                        fscanf_s(stream, "%d\n", &tmp);
                                        if (tmp == 1)
                                            checkMenu119 = 1;
                                        if (fscanf_s(stream, "%d\n", &tmp) != -1) {
                                            fscanf_s(stream, "%d\n", &tmp);
                                            if (tmp == 0)
                                                checkMenu12D = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        fclose(stream);
    }

    if (!nWidth)  nWidth = screenW;                             
    if (!nHeight) nHeight = screenH - 30;
    if (s.RenderWidth() == 0) s.RenderWidth() = 640;
    if (s.RenderHeight() == 0) s.RenderHeight() = 360;
    if (s.SidebarWidth() < 250) s.SidebarWidth() = 250;
    if (s.SidebarRatio() < 0.0f)
        s.SidebarRatio() =
            static_cast<float>(static_cast<double>(s.SidebarWidth()) /
                               static_cast<double>(nWidth));

    InitCommonControls();                                       
    s.HInstance() = hInstance;                                  

    WNDCLASSA wc;                                               
    std::memset(&wc, 0, sizeof(wc));
    wc.style = 11;                       
    wc.lpfnWndProc = MainWndProc;                               
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(0x64));
    wc.hCursor = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7F00));  
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(4));   
    wc.lpszClassName = "Polygon Movie Maker";
    wc.lpszMenuName = s.EnglishUI() ? "SAMPLE02E" : "SAMPLE02";

    if (!RegisterClassA(&wc)) {                                 
        MessageBoxA(nullptr, "RegisterClass failed",
                    s.EnglishUI() ? "create main window"
                                  : kJpCaptionCreateMainWindow,
                    MB_OK);
        return false;
    }

    std::memset(&wc, 0, sizeof(wc));
    wc.style = 11;
    wc.lpfnWndProc = RecWndProc;                                
    wc.hInstance = hInstance;
    wc.lpszClassName = "RecWindow";
    if (!RegisterClassA(&wc)) {
        MessageBoxA(nullptr,
                    s.EnglishUI() ? "RecWindow failed"
                                  : kJpRecWindowRegFailed,
                    s.EnglishUI() ? "create main window"
                                  : kJpCaptionCreateMainWindow,
                    MB_OK);
        return false;
    }

    std::memset(&wc, 0, sizeof(wc));
    wc.style = 11;
    wc.lpfnWndProc = MicWndProc;                                
    wc.hInstance = hInstance;
    wc.lpszClassName = "MicWindow";
    if (!RegisterClassA(&wc)) {
        MessageBoxA(nullptr,
                    s.EnglishUI() ? "MicWindow failed"
                                  : kJpMicWindowRegFailed,
                    s.EnglishUI() ? "create main window"
                                  : kJpCaptionCreateMainWindow,
                    MB_OK);
        return false;
    }

    char windowName[52];                                        
    strcpy_s(windowName, 0x32, "RinMotionDance");
    HWND hwnd = CreateWindowExA(                                
        0, "Polygon Movie Maker", windowName, 0xCF0000u  ,
        X, Y, nWidth, nHeight, nullptr, nullptr, hInstance, nullptr);
    s.Hwnd() = hwnd;
    if (!hwnd) {
        MessageBoxA(nullptr, "RegisterClass failed", "create main window", MB_OK);
        return false;
    }

    LocalizeUI(app);                                            

    if (app->EnvFileName()[0] != L'\0') {                       
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t destination[1000];  
        wcscpy_s(destination, 0x100, app->EnvFileName());
        if (app->EnvFileName()[0] == 34  ) {
            wchar_t* close = wcsstr(app->EnvFileName() + 1, L"\"");
            if (close)
                *close = L'\0';
            wcscpy_s(destination, 0x100, app->EnvFileName() + 1);
        }
        wchar_t dirBuf[1000];
        if (wcsstr(destination, L".pmm") || wcsstr(destination, L".PMM")) {
            wcscpy_s(app->DirUser(), 0x3E8,
                     ExtractDirFromPath(dirBuf, destination));
            wcscpy_s(app->EnvFileName(), 0x100, destination);
            LoadSceneFile();                                    
        } else if (wcsstr(destination, L".pmd") || wcsstr(destination, L".PMD") ||
                   wcsstr(destination, L".pmx") || wcsstr(destination, L".PMX")) {
            wcscpy_s(app->DirModel(), 0x3E8,
                     ExtractDirFromPath(dirBuf, destination));
            LoadModelFile(app, destination);                   
        } else if (wcsstr(destination, L".x") || wcsstr(destination, L".X")) {
            wcscpy_s(app->DirAccs(), 0x3E8,
                     ExtractDirFromPath(dirBuf, destination));
            LoadAccessoryFile(destination);                     
        }
    }

    WINDOWPLACEMENT wndpl;                                      
    std::memset(&wndpl, 0, sizeof(wndpl));
    wndpl.length = 44;
    wndpl.flags = 2;                    
    wndpl.showCmd = 2 * startMaximized + 1;       
    wndpl.rcNormalPosition.left = X;
    wndpl.rcNormalPosition.top = Y;
    wndpl.rcNormalPosition.right = X + nWidth;
    wndpl.rcNormalPosition.bottom = Y + nHeight;
    SetWindowPlacement(static_cast<HWND>(s.Hwnd()), &wndpl);
    UpdateWindow(static_cast<HWND>(s.Hwnd()));

    if (checkMenu119) {                                                 
        HMENU menu = GetMenu(static_cast<HWND>(s.Hwnd()));
        CheckMenuItem(menu, 0x119, MF_CHECKED);
    }
    if (runFlagSubsystem)
        InitFlagSubsystem(app);                                 
    if (checkMenu12D) {                                                 
        HMENU menu = GetMenu(static_cast<HWND>(s.Hwnd()));
        CheckMenuItem(menu, 0x12D, MF_CHECKED);
    }
    if (checkPhysicsFrame == 1) {                                            
        HWND item = GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kPhysicsFrameCheckbox);
        SendMessageA(item, BM_SETCHECK, 1, 0);
    }
    InvalidateRect(static_cast<HWND>(s.Hwnd()), nullptr, FALSE);
    return true;
}

}  
