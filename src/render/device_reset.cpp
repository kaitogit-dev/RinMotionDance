#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/mme_bridge.hpp"
#include "rinmotiondance/ported_funcs.hpp"

#include "fx_slots.hpp"

namespace rinmotiondance {
namespace {

template <typename ComPtr>
void ReleaseSubSlot(ComPtr& slot) {
    if (slot != nullptr) {
        slot->Release();
        slot = nullptr;
    }
}

}  

void PostDeviceReset(MMDApp* app) {
    ReleaseSubSlot(app->CaptureTexture());                        
    ReleaseSubSlot(app->AviBackgroundTexture());                  
    ReleaseSubSlot(app->AviBackgroundSurface());                  
    ReleaseSubSlot(app->CaptureRenderTarget());                   
    ReleaseSubSlot(app->CaptureSystemSurface());                  

    D3DRenderer* r = app->Renderer();
    if (r == nullptr)
        return;
    ReleaseSubSlot(r->captureSurface);                            
    ReleaseSubSlot(r->depthStencilSurface);                       
    ReleaseSubSlot(r->backbufferSurface);                         
    ReleaseSubSlot(r->shadowSurface);                             
    ReleaseSubSlot(r->hdrTexture);                                
    ReleaseSubSlot(r->shadowDepthSurface);                        

    if (r->effect != nullptr) {
        fx::OnLostDevice(r->effect);
    }

    mme::OnLostDevice(app);

    IDirect3DDevice9* device = r->device;
    if (device == nullptr)
        return;
    device->Reset(&r->presentParameters);                          

    mme::OnResetDevice(app);

    InitRenderStates(app);                                        

    const HWND hwnd = static_cast<HWND>(app->state.hwnd);
    const UINT menuState = GetMenuState(GetMenu(hwnd), 0x115, 0);
    device->SetRenderState(                                       
        static_cast<D3DRENDERSTATETYPE>(0xA1),
        (menuState & 8) != 0 ? 1 : 0);

    if (r->effect != nullptr) {
        fx::OnResetDevice(r->effect);
    }

    if (app->FullscreenMode() != 0) {
        RefreshSeparateWindowViewport(app);                                           
    } else {
        RefreshMainWindowViewport(app);                                           
    }

    app->state.recentFile0[0] = 0;                                 
    app->state.recentFile1[0] = 0;
    app->state.recentFile2[0] = 0;
}

}  
