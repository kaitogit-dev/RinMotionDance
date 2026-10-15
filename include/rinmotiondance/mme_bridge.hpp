#pragma once

#include <d3d9.h>

namespace rinmotiondance {

class MMDApp;

namespace mme {

void OnDeviceCreated(MMDApp* app, HWND hwnd);

void OnDeviceDestroyed(MMDApp* app);

void OnLostDevice(MMDApp* app);
void OnResetDevice(MMDApp* app);

HRESULT ClearScene(MMDApp* app, IDirect3DDevice9* device, unsigned long flags,
                   D3DCOLOR color, float z, unsigned long stencil);
HRESULT BeginScene(MMDApp* app, IDirect3DDevice9* device);
HRESULT EndScene(MMDApp* app, IDirect3DDevice9* device);
HRESULT Present(MMDApp* app, IDirect3DDevice9* device,
                const RECT* source, const RECT* destination,
                HWND destinationWindow, const RGNDATA* dirtyRegion);
void PreRenderTargetCopy(MMDApp* app, IDirect3DDevice9* device,
                         IDirect3DSurface9* source);

HRESULT DrawPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type,
                      unsigned int start_vertex, unsigned int primitive_count);
HRESULT DrawIndexedPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type,
                             int base_vertex_index, unsigned int min_vertex_index,
                             unsigned int vertex_count, unsigned int start_index,
                             unsigned int primitive_count);

bool EffectsActive();

HRESULT DrawAccessorySubset(MMDApp* app, void* accessory, unsigned long material);

void NotifyPmmLoaded(MMDApp* app);
void NotifyPmmSaved(MMDApp* app);

void RecordTexture(const wchar_t* path, IDirect3DBaseTexture9* texture);

}  
}  
