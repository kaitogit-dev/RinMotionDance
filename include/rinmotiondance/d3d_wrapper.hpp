#pragma once

#include <cstddef>
#include <cstdint>

#include <Windows.h>
#include <locale.h>

#include <d3d9.h>

struct ID3DXEffect;  

namespace rinmotiondance {

struct ResourcePoolEntry {
    void* heapBuffer;    
    IUnknown* comObject; 
    std::int32_t tag;    
};

class D3DRenderer {
public:
    HWND hwnd;                    

    ResourcePoolEntry resourcePool[10000];  

    _locale_t localeTable[4];     
    void* stereoHandle;          

    std::int32_t shaderModelCaps; 
    IDirect3D9* d3d9;            
    IDirect3DDevice9* device;    
    std::int32_t screenWidth;    
    std::int32_t screenHeight;   
    float aspectRatio;           
    float viewScale;             
    IDirect3DVertexBuffer9* lineVertexBuffer;  
    unsigned char multisampleAvailable;  

    D3DPRESENT_PARAMETERS presentParameters;  

    IDirect3DSurface9* captureSurface;       
    IDirect3DSurface9* backbufferSurface;    
    IDirect3DSurface9* depthStencilSurface;  
    D3DFORMAT backbufferFormat;              
    unsigned char postProcessEnabled;        
    IDirect3DTexture9* hdrTexture;           
    IDirect3DTexture9* spriteTexture;        
    IDirect3DSurface9* shadowSurface;        
    IDirect3DSurface9* shadowDepthSurface;   
    std::int32_t renderTargetWidth;          
    std::int32_t renderTargetHeight;         
    ID3DXEffect* effect;                     
    unsigned char d3dInitialized;            
    unsigned char shaderModel3;              
    unsigned char stereoEnabled;             
    std::int32_t maxTextureWidth;            
    std::int32_t maxTextureHeight;           
    unsigned char runtimeToggle;             
};

static_assert(sizeof(D3DRenderer) == 0x1D574 ||
              sizeof(D3DRenderer) == 0x3AA88,
              "size must be the x86 or x64 original allocation size");

#define RINMOTIONDANCE_WRAPPER_OFF(f, x86off, x64off)                            \
    static_assert(offsetof(D3DRenderer, f) ==                             \
                      (sizeof(void*) == 8 ? (x64off) : (x86off)),         \
                  #f " offset must match the original binary")

RINMOTIONDANCE_WRAPPER_OFF(hwnd, 0, 0);
RINMOTIONDANCE_WRAPPER_OFF(resourcePool, 4, 8);
RINMOTIONDANCE_WRAPPER_OFF(localeTable, 120004, 240008);
RINMOTIONDANCE_WRAPPER_OFF(stereoHandle, 120020, 240040);
RINMOTIONDANCE_WRAPPER_OFF(shaderModelCaps, 120024, 240048);
RINMOTIONDANCE_WRAPPER_OFF(d3d9, 120028, 240056);
RINMOTIONDANCE_WRAPPER_OFF(device, 120032, 240064);
RINMOTIONDANCE_WRAPPER_OFF(screenWidth, 120036, 240072);
RINMOTIONDANCE_WRAPPER_OFF(screenHeight, 120040, 240076);
RINMOTIONDANCE_WRAPPER_OFF(aspectRatio, 120044, 240080);
RINMOTIONDANCE_WRAPPER_OFF(viewScale, 120048, 240084);
RINMOTIONDANCE_WRAPPER_OFF(lineVertexBuffer, 120052, 240088);
RINMOTIONDANCE_WRAPPER_OFF(multisampleAvailable, 120056, 240096);
RINMOTIONDANCE_WRAPPER_OFF(presentParameters, 120060, 240104);
RINMOTIONDANCE_WRAPPER_OFF(captureSurface, 120116, 240168);
RINMOTIONDANCE_WRAPPER_OFF(backbufferSurface, 120120, 240176);
RINMOTIONDANCE_WRAPPER_OFF(depthStencilSurface, 120124, 240184);
RINMOTIONDANCE_WRAPPER_OFF(backbufferFormat, 120128, 240192);
RINMOTIONDANCE_WRAPPER_OFF(postProcessEnabled, 120132, 240196);
RINMOTIONDANCE_WRAPPER_OFF(hdrTexture, 120136, 240200);
RINMOTIONDANCE_WRAPPER_OFF(spriteTexture, 120140, 240208);
RINMOTIONDANCE_WRAPPER_OFF(shadowSurface, 120144, 240216);
RINMOTIONDANCE_WRAPPER_OFF(shadowDepthSurface, 120148, 240224);
RINMOTIONDANCE_WRAPPER_OFF(renderTargetWidth, 120152, 240232);
RINMOTIONDANCE_WRAPPER_OFF(renderTargetHeight, 120156, 240236);
RINMOTIONDANCE_WRAPPER_OFF(effect, 120160, 240240);
RINMOTIONDANCE_WRAPPER_OFF(d3dInitialized, 120164, 240248);
RINMOTIONDANCE_WRAPPER_OFF(shaderModel3, 120165, 240249);
RINMOTIONDANCE_WRAPPER_OFF(stereoEnabled, 120166, 240250);
RINMOTIONDANCE_WRAPPER_OFF(maxTextureWidth, 120168, 240252);
RINMOTIONDANCE_WRAPPER_OFF(maxTextureHeight, 120172, 240256);
RINMOTIONDANCE_WRAPPER_OFF(runtimeToggle, 120176, 240260);

#undef RINMOTIONDANCE_WRAPPER_OFF

}  
