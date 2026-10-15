#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>
#include <mmsystem.h>   

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <locale.h>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void TeardownDShowGraph(DShowRecorder* rec);   
void RefreshAfterFrameApply(MMDApp* app);      




void FinishAviRecord(MMDApp* app) {
    auto& s = *app;
    timeEndPeriod(1);                                       
    s.state.playbackActive = 0;             
    StopPlayback(app);                                         
    unsigned char* flag = s.RecordingCompletionFlag();      
    s.state.frameStepPlayback = 0;           
    if (flag != nullptr)
        *flag = 1;                                          
    FinishMp4Recording(static_cast<HWND>(s.state.hwnd));
    TeardownDShowGraph(s.Recorder());                       
    D3DRenderer* r = s.Renderer();
    if (r != nullptr) {
        IDirect3DDevice9* device = r->device;               
        IDirect3DSurface9** backBuffer = &r->backbufferSurface;
        if (*backBuffer == nullptr)                         
            device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
                                  backBuffer);              
        if (r->multisampleAvailable == 0)
            device->SetRenderTarget(0, *backBuffer);        
    }
    if (s.state.aviStereoOutput != 0) {                     
        s.state.fullscreenMode = 0;                    
        ApplyFullscreenWindowState(app);                                     
        PostDeviceReset(app);                               
        s.state.aviStereoOutput = 0;                    
    } else {
        const std::int32_t winW = s.state.recRTW;
        const std::int32_t winH = s.state.recRTH;
        if (r != nullptr &&
            (r->screenWidth > winW || r->screenHeight > winH)) {
            r->presentParameters.BackBufferWidth = winW;   
            r->presentParameters.BackBufferHeight = winH;  
            r->screenWidth = winW;                          
            r->screenHeight = winH;                         
            PostDeviceReset(app);                           
        }
        DestroyWindow(s.RecordingWindow());                 
    }
    s.RecordingWindow() = nullptr;                          
    EnableWindow(GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kPlayButton), 1);  
    RefreshMainWindowViewport(app);                                         
    RefreshAfterFrameApply(app);                            
    PostViewRefresh(app);                                   
    if (IDirect3DSurface9* com = s.CaptureRenderTarget()) {        
        com->Release();
        s.CaptureRenderTarget() = nullptr;
    }
    if (IDirect3DSurface9* com = s.CaptureSystemSurface()) {       
        com->Release();
        s.CaptureSystemSurface() = nullptr;
    }
    if (void* buf = s.state.captureReadbackPixels) {                 
        free(buf);
        s.state.captureReadbackPixels = nullptr;
    }
    if (s.state.aviStereoOutput == 0) {                 
        ShowWindow(static_cast<HWND>(s.Hwnd()), SW_SHOW);   
        HWND separate = s.FloatingWindow();
        if (separate != nullptr)
            ShowWindow(separate, SW_SHOW);                  
    }
}



int GetPmdNum(MMDApp* app) {
    int count = 0;
    for (int slot = 0; slot < kModelSlotCount; ++slot) {
        if (app->ModelSlot(slot) != nullptr)
            ++count;
    }
    return count;
}

void RendererInit(D3DRenderer* r) {
    r->d3d9 = nullptr;
    r->device = nullptr;
    r->lineVertexBuffer = nullptr;
    r->captureSurface = nullptr;
    r->backbufferSurface = nullptr;
    r->depthStencilSurface = nullptr;
    r->hdrTexture = nullptr;
    r->spriteTexture = nullptr;
    r->shadowSurface = nullptr;
    r->shadowDepthSurface = nullptr;
    r->effect = nullptr;
    r->screenWidth = 0;
    r->screenHeight = 0;
    r->multisampleAvailable = 0;
    r->postProcessEnabled = 0;
    for (auto& entry : r->resourcePool) {
        entry.heapBuffer = nullptr;
        entry.comObject = nullptr;
    }
    r->d3dInitialized = 1;
    r->shaderModelCaps = 1;
    r->runtimeToggle = 1;
    r->localeTable[0] = _create_locale(LC_ALL, "");
    r->localeTable[1] = _create_locale(LC_ALL, ".OCP");
    r->localeTable[2] = _create_locale(LC_ALL, ".ACP");
    r->localeTable[3] = _create_locale(LC_ALL, "JPN");
    {
        int status = NvapiInitChain();                        
        if (status != 0)
            status = NvapiStereoCaps(1);                      
        if (status != 0) {
            r->stereoEnabled = 0;                             
        } else {
            r->stereoEnabled = (NvapiStereoSupportGate() == 0) ? 1 : 0;  
        }
    }
}





void IdentityCtor(void* obj)            { (void)obj; }



INT_PTR CALLBACK DialogFuncStub(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp) {
    (void)lp;
    if (msg == WM_INITDIALOG) {                                    
        MMDApp* app = g_Block;
        SetWindowTextA(GetDlgItem(hDlg, panel::kScaleFromEdit),
                       app->state.statusText);
        return 0;
    }
    if (msg == WM_COMMAND) {                                       
        switch (LOWORD(wp)) {
        case 0x2AF: EndDialog(hDlg, 1); return 0;                  
        case 0x2B0: EndDialog(hDlg, 3); return 0;                  
        case 0x2B1: EndDialog(hDlg, 4); return 0;                  
        case 0x2B2: EndDialog(hDlg, 5); return 0;                  
        case 2:     EndDialog(hDlg, 2); return 0;                  
        default: break;
        }
    }
    return 0;                                                      
}
}  
