#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <stdio.h>
#include <cstdlib>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

static const char kMsgQuitEnhancedJp[] =                 
    "\x27\x8A\x67\x92\xA3\x83\x82\x83\x66\x83\x8B\x95\xDB\x91\xB6\x27"
    "\x82\xB5\x82\xC4\x82\xA2\x82\xC8\x82\xA2\x8A\x67\x92\xA3\x95\xD2\x8F\x57"
    "\x82\xB5\x82\xBD\x83\x82\x83\x66\x83\x8B\x82\xAA\x82\xA0\x82\xE8\x82\xDC"
    "\x82\xB7\n\n"
    "\x8F\x49\x97\xB9\x82\xB5\x82\xC4\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5"
    "\x82\xB7\x82\xA9\x81\x48";
static const char kMsgQuitModifiedJp[] =                 
    "\x95\xDB\x91\xB6\x82\xB5\x82\xC4\x82\xA2\x82\xC8\x82\xA2\x95\xCF\x8D\x58"
    "\x93\x5F\x82\xAA\x82\xA0\x82\xE8\x82\xDC\x82\xB7\n\n"
    "\x8F\x49\x97\xB9\x82\xB5\x82\xC4\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5"
    "\x82\xB7\x82\xA9\x81\x48";
static const char kCaptionQuitJp[] = "\x8F\x49\x97\xB9";  

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    auto& s = *app;
    s.MessageSeen() = 1;                                           

    if (msg <= WM_COMMAND  ) {
        if (msg == WM_COMMAND) {
            CommandDispatch(reinterpret_cast<HWND>(lParam),
                            wParam);  
            return 0;
        }
        switch (msg) {
        case WM_CREATE:
            if (!CreateUIControls(app, hwnd))                     
                exit(1);
            return 0;

        case WM_DESTROY: {
            WINDOWPLACEMENT wndpl;
            wndpl.length = sizeof(wndpl);
            GetWindowPlacement(hwnd, &wndpl);
            SetCurrentDirectoryW(app->ExeDir());                  
            FILE* f = nullptr;
            if (fopen_s(&f, "Data\\mmconfig.ini", "wt") == 0) {
                fprintf(f, "%d\n", wndpl.rcNormalPosition.left);
                fprintf(f, "%d\n", wndpl.rcNormalPosition.top);
                fprintf(f, "%d\n", wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left);
                fprintf(f, "%d\n", wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top);
                fprintf(f, wndpl.showCmd == 3 ? "1\n" : "0\n");
                fprintf(f, "%d\n", s.RenderWidth());
                fprintf(f, "%d\n", s.RenderHeight());
                if (s.FloatingWindow() != nullptr)
                    fprintf(f, "%d\n", s.SeparateWindowSidebarWidth());
                else
                    fprintf(f, "%d\n", s.SidebarWidth());
                fprintf(f, s.EnglishUI() ? "1\n" : "0\n");
                if (s.FloatingWindow() != nullptr) {
                    fprintf(f, "1\n");
                    SaveFlagSubsystem(app);                       
                } else {
                    fprintf(f, "0\n");
                }
                fprintf(f, "%d\n", s.SeparateWindowMaximized());
                fprintf(f, "%d\n", s.SeparateWindowX());
                fprintf(f, "%d\n", s.SeparateWindowY());
                fprintf(f, "%d\n", s.SeparateWindowWidth());
                fprintf(f, "%d\n", s.SeparateWindowHeight());
                HWND dlg530 = GetDlgItem(hwnd, panel::kPhysicsFrameCheckbox);
                fprintf(f, SendMessageA(dlg530, BM_GETCHECK, 0, 0) == 1 ? "1\n" : "0\n");
                fprintf(f, "1\n");
                fwprintf(f, L"%s\n", app->DirModel());
                fwprintf(f, L"%s\n", app->DirAccs());
                fwprintf(f, L"%s\n", app->DirWave());
                fwprintf(f, L"%s\n", app->DirPose());
                fwprintf(f, L"%s\n", app->DirMotion());
                fwprintf(f, L"%s\n", app->DirUser());
                fprintf(f, "1\n");
                fprintf(f, s.FrameVolumeControlEnabled() ? "1\n" : "0\n");
                fprintf(f, "%d\n", s.FrameNormalization());
                fprintf(f, "1\n");
                fprintf(f, "%f\n", s.SidebarRatio());
                fprintf(f, "1\n");
                fwprintf(f, L"%s\n", app->DirBg());
                fprintf(f, "1\n");
                HMENU menu = GetMenu(hwnd);
                fprintf(f, (GetMenuState(menu, 0x119, 0) & 8) ? "1\n" : "0\n");
                fprintf(f, "1\n");
                menu = GetMenu(hwnd);
                fprintf(f, (GetMenuState(menu, 0x12D, 0) & 8) ? "1\n" : "0\n");
                fclose(f);
            }
            PostQuitMessage(0);
            return 0;
        }

        case WM_SIZE:
            s.WindowLayoutReady() = 0;                            
            HandleWindowSize(app);                                
            s.WindowLayoutReady() = 1;
            return 0;

        case WM_PAINT:
            HandleWindowPaint(app);                               
            return 0;

        case WM_CLOSE:
            if (s.EnhancedModelDirty() != 0) {                    
                int r = s.EnglishUI()
                    ? MessageBoxA(hwnd,
                        "There is a enhanced model not preserved by 'save enhanced model'.\n\nDo you realy quit?",
                        "quit", 0x40001)
                    : MessageBoxA(hwnd, kMsgQuitEnhancedJp, kCaptionQuitJp,
                                 (MB_OKCANCEL | MB_TOPMOST));  
                if (r == IDOK)                                    
                    return DefWindowProcA(hwnd, msg, wParam, lParam);
            } else {
                if (s.SceneModified() == 0)                       
                    return DefWindowProcA(hwnd, msg, wParam, lParam);
                int r = s.EnglishUI()
                    ? MessageBoxA(hwnd,
                        "There is a change point not preserved.\n\nDo you realy quit?",
                        "quit", 0x40001)
                    : MessageBoxA(hwnd, kMsgQuitModifiedJp, kCaptionQuitJp,
                                 (MB_OKCANCEL | MB_TOPMOST));  
                if (r == IDOK)
                    return DefWindowProcA(hwnd, msg, wParam, lParam);
            }
            return 0;

        case WM_ERASEBKGND:
            return 0;

        case WM_NOTIFY:
            return HandleNotify(hwnd, msg, wParam, lParam);       

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
        }
    }

    if (msg > 0x233) {
        if (msg != 792  )
            return DefWindowProcA(hwnd, msg, wParam, lParam);
        HandlePaletteChanged(reinterpret_cast<HDC>(wParam));      
        HandlePaletteChanged2(reinterpret_cast<HDC>(wParam));     
        return 0;
    }

    if (msg == WM_DROPFILES  ) {
        HandleDropFiles(reinterpret_cast<HDROP>(wParam));         
        return 0;
    }

    switch (msg) {
    case WM_TIMER:
        if (wParam == 100) {
            KillTimer(hwnd, 100);
            HandleTimer100(app);                                  
            return 0;
        }
        if (wParam != 101)
            return 0;
        KillTimer(hwnd, 101);
        ++s.AutoRepeatCount();                                    
        if (s.AutoRepeatCount() >= 4 || s.AutoRepeatCount() <= 0)
            return 0;
        SetTimer(hwnd, 101, 0x5DC, nullptr);                      
        return 0;

    case WM_HSCROLL:
        HandleHScroll(lParam, wParam);                            
        return 0;
    case WM_VSCROLL:
        HandleVScroll(lParam, wParam);                            
        return 0;
    case 0x138:  
        return HandleCtlColor(reinterpret_cast<HWND>(lParam),
                              reinterpret_cast<HDC>(wParam));  
    case WM_MOUSEMOVE:
        HandleMouseMove(static_cast<std::uint32_t>(lParam),
                        HIWORD(lParam));                          
        return 0;
    case WM_LBUTTONDOWN:
        SetCapture(static_cast<HWND>(s.Hwnd()));
        HandleLButtonDown(app);                                   
        return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        HandleLButtonUp(app);                                     
        return 0;
    case WM_LBUTTONDBLCLK:
        HandleLButtonDblClk(app);                                 
        return 0;
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        SetCapture(static_cast<HWND>(s.Hwnd()));
        return 0;
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        ReleaseCapture();
        return 0;
    case WM_RBUTTONDBLCLK:
        HandleMouseActivate(app);                                 
        return 0;
    case WM_MOUSEWHEEL:
        HandleMouseWheel(HIWORD(wParam));                         
        return 0;
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

}  
