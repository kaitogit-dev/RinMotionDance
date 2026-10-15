#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdlib>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

void DispatchSeparateWindowCommand(MMDApp* app, unsigned short id);  
void LayoutViewportPanels(MMDApp* app);  

int FilterSeparateWindowMouseMove(MMDApp* app, unsigned short x, unsigned short y) {
    auto& s = *app;
    if (s.state.a06B5 != 0) {          
        const int edgeBand = static_cast<int>(s.Renderer()->viewScale * 200.0f);
        if (x > s.ViewportRect().right - edgeBand)                  
            return edgeBand;
        s.state.a06B5 = 0;              
    }
    int result = y;
    if (s.state.a06B4 != 0) {                                       
        if (y > s.ViewportRect().bottom)                            
            return result;
        s.state.a06B4 = 0;                                          
    }
    if (s.MouseX() != x || s.MouseY() != y) {                      
        const bool first = s.state.separateWindowMouseSeen == 0;
        s.MouseX() = x;                                             
        s.MouseY() = y;                                             
        if (!first) {
            const int dy = std::abs(s.PreviousMouseY() - static_cast<int>(y));
            const int dx = std::abs(s.PreviousMouseX() - static_cast<int>(x));
            if (dx > 50 || dy > 50)                                
                s.state.mouseJumped = 1;                              
            s.PreviousMouseX() = x;                                
            s.PreviousMouseY() = y;                                
            s.state.separateWindowMouseSeen = 0;          
        }
        if (s.ViewportToolHovered() == 1) {                      
            SetCursor(LoadCursorA(nullptr,
                                  reinterpret_cast<LPCSTR>(0x7F89))  
            );                                                     
            result = 1;
        }
    }
    return result;
}

LRESULT CALLBACK MicWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                            LPARAM lParam) {
    MMDApp* app = g_Block;
    auto& s = *app;
    switch (msg) {
    case WM_MOUSEMOVE:                                             
        s.MessageSeen() = 1;                                      
        FilterSeparateWindowMouseMove(app, LOWORD(lParam), HIWORD(lParam));  
        return 0;
    case WM_CLOSE:                                                 
        SaveFlagSubsystem(app);                                    
        return 0;
    case WM_COMMAND:                                               
        DispatchSeparateWindowCommand(app, LOWORD(wParam));  
        return 0;
    case WM_PAINT: {                                               
        RECT rc;
        GetClientRect(hwnd, &rc);
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        const RECT& viewport = s.ViewportRect();
        BitBlt(hdc, 0, 0, rc.right, viewport.top, 0, 0, 0, 0x42);
        BitBlt(hdc, 0, 0, viewport.left, rc.bottom,
               0, 0, 0, 0x42);
        BitBlt(hdc, viewport.right, 0,
               rc.right, rc.bottom, 0, 0, 0, 0x42);
        BitBlt(hdc, 0, viewport.bottom,
               rc.right, rc.bottom, 0, 0, 0, 0x42);
        HandlePaletteChanged2(hdc);                                
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOVE:                                                  
        RefreshSeparateWindowViewport(app);                                            
        return 0;
    case WM_SIZE:                                                  
        s.MessageSeen() = 1;                                      
        RefreshSeparateWindowViewport(app);                                            
        LayoutViewportPanels(app);                                 
        return 0;
    case WM_RBUTTONDBLCLK:                                           
        HandleMouseActivate(app);
        return 0;
    case WM_LBUTTONDOWN:                                           
        SetCapture(s.FloatingWindow());
        return 0;
    case WM_LBUTTONUP:                                             
        ReleaseCapture();
        return 0;
    case WM_MOUSEWHEEL:                                            
        s.CameraDistance() = static_cast<float>(
            static_cast<double>(static_cast<short>(HIWORD(wParam))) *
                0.05000000074505806 +
            static_cast<double>(s.CameraDistance()));
        PostViewRefresh(app);                                      
        return 0;
    case 792  :                                
        HandlePaletteChanged2(reinterpret_cast<HDC>(wParam));
        return 0;
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);          
    }
}

void InitFlagSubsystem(MMDApp* app) {
    auto& s = *app;
    HWND mic = CreateWindowExA(                                   
        8  , "MicWindow", "MMD",
        0x80CF0000u,      
        s.SeparateWindowX(), s.SeparateWindowY(),
        s.SeparateWindowWidth(), s.SeparateWindowHeight(),
        nullptr, nullptr,
        static_cast<HINSTANCE>(s.HInstance()), nullptr);
    s.FloatingWindow() = mic;                                     
    if (!mic) {
        HWND main = s.MainWindow();
        if (s.EnglishUI())                                        
            MessageBoxA(main, "CreateWindow failed",
                        "create separate window", 0);
        else
            MessageBoxA(main, "CreateWindow failed",
                        "\x95\xca\x91\x8b\x8d\xec\x90\xac", 0);  
        return;
    }

    ShowWindow(mic, s.SeparateWindowMaximized() == 1
                     ? SW_MAXIMIZE
                     : SW_SHOW);                                  
    UpdateWindow(s.FloatingWindow());                              
    CheckMenuItem(GetMenu(s.MainWindow()), 0x118, 8  );  

    HWND main = s.MainWindow();
    s.SeparateWindowSidebarWidth() = s.SidebarWidth();
    RECT rc;                                                       
    GetClientRect(main, &rc);
    s.SidebarWidth() = rc.right - 3;                              
    if ((GetMenuState(GetMenu(main), 0x119, 0) & 8) == 0)         
        SetWindowPos(s.FloatingWindow(),
                     reinterpret_cast<HWND>(static_cast<intptr_t>(-2))  ,
                     0, 0, 0, 0, 0x43  );  
    for (int id = 536; id <= 557; ++id)                            
        SetParent(GetDlgItem(main, id),
                  s.FloatingWindow());                            

    RelayoutSidebarControls(app);                                                
    PanelPaint(app);                                               
    InvalidateRect(main, nullptr, FALSE);                          
    InvalidateRect(s.FloatingWindow(),
                   nullptr, FALSE);                                
}

void SaveFlagSubsystem(MMDApp* app) {
    auto& s = *app;
    HWND main = s.MainWindow();
    for (int id = 536; id <= 557; ++id)                            
        SetParent(GetDlgItem(s.FloatingWindow(), id),
                  main);                                           

    WINDOWPLACEMENT wndpl;                                         
    wndpl.length = 44;
    GetWindowPlacement(s.FloatingWindow(), &wndpl);
    s.SeparateWindowX() = wndpl.rcNormalPosition.left;
    s.SeparateWindowY() = wndpl.rcNormalPosition.top;
    s.SeparateWindowWidth() = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
    s.SeparateWindowHeight() = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
    s.SeparateWindowMaximized() = wndpl.showCmd == SW_SHOWMAXIMIZED;
    DestroyWindow(s.FloatingWindow());                             
    s.FloatingWindow() = nullptr;

    CheckMenuItem(GetMenu(main), 0x118, 0  );       
    s.SidebarWidth() = s.SeparateWindowSidebarWidth();
    PanelPaint(app);                                               
    InvalidateRect(main, nullptr, FALSE);                          
    RelayoutSidebarControls(app);                                                
}

}  
