#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

IDirect3DDevice9* DeviceOf(MMDApp* app) {
    D3DRenderer* sub = app->Renderer();
    if (sub == nullptr)
        return nullptr;
    return sub->device;
}

}  

void InitRenderStates(MMDApp* app) {
    IDirect3DDevice9* dev = DeviceOf(app);
    if (dev == nullptr)
        return;
    D3DRenderer* sub = app->Renderer();

    dev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);                 
    dev->LightEnable(0, TRUE);                                      
    dev->SetRenderState(D3DRS_LIGHTING, TRUE);                      
    dev->SetRenderState(D3DRS_SPECULARENABLE, TRUE);                

    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);   
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);   
    dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);   
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);   
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);   
    dev->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);   

    const DWORD maxAniso = static_cast<DWORD>(sub->shaderModelCaps);
    const DWORD minMagFilter = sub->runtimeToggle != 0 ? D3DTEXF_ANISOTROPIC
                                                       : D3DTEXF_LINEAR;
    const DWORD mipFilter = sub->runtimeToggle != 0 ? D3DTEXF_ANISOTROPIC
                                                    : D3DTEXF_NONE;
    const DWORD maxAnisoAll = sub->runtimeToggle != 0 ? maxAniso : 1;
    for (DWORD s = 0; s < 3; ++s) {
        dev->SetSamplerState(s, D3DSAMP_MINFILTER, minMagFilter);   
    }
    for (DWORD s = 0; s < 3; ++s) {
        dev->SetSamplerState(s, D3DSAMP_MAGFILTER, minMagFilter);   
    }
    for (DWORD s = 0; s < 3; ++s) {
        dev->SetSamplerState(s, D3DSAMP_MIPFILTER, mipFilter);      
    }
    dev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, maxAnisoAll);    
    dev->SetSamplerState(1, D3DSAMP_MAXANISOTROPY, maxAnisoAll);    
    dev->SetSamplerState(2, D3DSAMP_MAXANISOTROPY, maxAnisoAll);    

    D3DMATRIX halfTexel = {
        0.5f,  0.0f,  0.0f, 0.0f,
        0.0f, -0.5f,  0.0f, 0.0f,
        0.0f,  0.0f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.0f, 1.0f,
    };
    dev->SetTransform(D3DTS_TEXTURE1, &halfTexel);                  
    dev->SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_MODULATE);   
    dev->SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);   
    dev->SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_TEXTURE);   
    dev->SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);   
    dev->SetTransform(D3DTS_TEXTURE2, &halfTexel);                   
    dev->SetTextureStageState(2, D3DTSS_TEXTURETRANSFORMFLAGS,
                              D3DTTFF_COUNT2);                       
    dev->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 0x10000);      

    if (sub->d3dInitialized != 0) {  
        dev->SetRenderState(D3DRS_STENCILENABLE, TRUE);              
        dev->SetRenderState(D3DRS_STENCILMASK, 255);                 
    }
    dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);       
    dev->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_MAX);         
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);          
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);      

    D3DCAPS9 caps;
    dev->GetDeviceCaps(&caps);
    if (caps.AlphaCmpCaps & D3DPCMPCAPS_GREATEREQUAL) {  
        dev->SetRenderState(D3DRS_ALPHAREF, 1);                     
        dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);           
        dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);  
    }
}

}  
