#include "rinmotiondance/mme_bridge.hpp"

#include <Windows.h>
#include <string.h>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/d3d_wrapper.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "mme_host_api.h"
#include "mesh_subset.hpp"

extern "C" const wchar_t* MmdModelPathW(int index);
extern "C" const wchar_t* MmdAcsPathW(int index);

namespace rinmotiondance {
namespace mme {
namespace {

bool g_deviceRegistered = false;
HWND g_registeredWindow = nullptr;

HWND g_editModeControl = nullptr;
bool g_recWindowSticky = false;

BOOL CALLBACK FindRecWindowProc(HWND candidate, LPARAM found) {
    char className[32];
    if (GetClassNameA(candidate, className, 32) != 0 &&
        strcmp(className, "RecWindow") == 0) {
        *reinterpret_cast<bool*>(found) = true;
        return FALSE;
    }
    return TRUE;
}

int ProbeNotEditMode(HWND mainWindow) {
    if (g_editModeControl == nullptr) {
        char className[32];
        HWND probe = GetDlgItem(mainWindow, 317);
        if (probe == nullptr ||
            GetClassNameA(probe, className, 32) == 0 ||
            _stricmp(className, "EDIT") != 0)
            probe = nullptr;
        if (probe == nullptr) {
            probe = GetDlgItem(mainWindow, 417);
            if (probe != nullptr &&
                (GetClassNameA(probe, className, 32) == 0 ||
                 _stricmp(className, "EDIT") != 0))
                probe = nullptr;
        }
        g_editModeControl = probe;
    }
    bool recPresent = false;
    EnumThreadWindows(GetCurrentThreadId(), FindRecWindowProc,
                      reinterpret_cast<LPARAM>(&recPresent));
    if (!recPresent)
        g_recWindowSticky = false;
    if (IsWindowEnabled(g_editModeControl) == FALSE)
        return 1;
    if (recPresent) {
        if (g_recWindowSticky)
            return 1;
        g_recWindowSticky = true;
    }
    return 0;
}

}  

void OnDeviceCreated(MMDApp* app, HWND hwnd) {
    g_deviceRegistered = false;
    g_registeredWindow = nullptr;
    D3DRenderer* renderer = app->Renderer();
    if (renderer == nullptr || renderer->device == nullptr)
        return;
    HWND window = hwnd;
    if (window == nullptr)
        window = static_cast<HWND>(app->Hwnd());
    MmeHostSetPathProviders(&MmdModelPathW, &MmdAcsPathW);
    MmeHostSetMainWindow(window);
    MmeHostSetDrawnWindow(window);
    MmeHostSetStandardEffect(renderer->effect);
    g_registeredWindow = window;
    g_deviceRegistered = true;
}

void OnDeviceDestroyed(MMDApp* app) {
    if (!g_deviceRegistered)
        return;
    D3DRenderer* renderer = app->Renderer();
    MmeHostDestroyDevice(renderer != nullptr ? renderer->device : nullptr);
    g_deviceRegistered = false;
}

void OnLostDevice(MMDApp* app) {
    if (!g_deviceRegistered)
        return;
    D3DRenderer* renderer = app->Renderer();
    if (renderer != nullptr)
        MmeHostOnLostDevice(renderer->device);
}

void OnResetDevice(MMDApp* app) {
    if (!g_deviceRegistered)
        return;
    D3DRenderer* renderer = app->Renderer();
    if (renderer != nullptr)
        MmeHostOnResetDevice(renderer->device);
}

HRESULT ClearScene(MMDApp* app, IDirect3DDevice9* device, unsigned long flags,
                   D3DCOLOR color, float z, unsigned long stencil) {
    if (!g_deviceRegistered)
        return device->Clear(0, nullptr, flags, color, z, stencil);
    return MmeHostClear(device, 0, nullptr, flags, color, z, stencil);
}

HRESULT BeginScene(MMDApp* app, IDirect3DDevice9* device) {
    if (!g_deviceRegistered)
        return device->BeginScene();
    HWND window = static_cast<HWND>(app->Hwnd());
    if (window != nullptr && window != g_registeredWindow) {
        MmeHostSetMainWindow(window);
        g_registeredWindow = window;
    }
    const int notEditMode = ProbeNotEditMode(g_registeredWindow);
    return MmeHostBeginScene(device, notEditMode);
}

HRESULT EndScene(MMDApp* app, IDirect3DDevice9* device) {
    if (!g_deviceRegistered)
        return device->EndScene();
    return MmeHostEndScene(device);
}

HRESULT Present(MMDApp* app, IDirect3DDevice9* device,
                const RECT* source, const RECT* destination,
                HWND destinationWindow, const RGNDATA* dirtyRegion) {
    if (g_deviceRegistered) {
        MmeHostSetDrawnWindow(destinationWindow != nullptr
            ? destinationWindow : static_cast<HWND>(app->Hwnd()));
    }
    return device->Present(source, destination, destinationWindow, dirtyRegion);
}

void PreRenderTargetCopy(MMDApp* app, IDirect3DDevice9* device,
                         IDirect3DSurface9* source) {
    if (!g_deviceRegistered)
        return;
    MmeHostPreRenderTargetCopy(device, source);
}

HRESULT DrawPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type,
                      unsigned int start_vertex, unsigned int primitive_count) {
    if (!g_deviceRegistered)
        return device->DrawPrimitive(type, start_vertex, primitive_count);
    return MmeHostDrawPrimitive(device, type, start_vertex, primitive_count);
}

HRESULT DrawIndexedPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type,
                             int base_vertex_index, unsigned int min_vertex_index,
                             unsigned int vertex_count, unsigned int start_index,
                             unsigned int primitive_count) {
    if (!g_deviceRegistered)
        return device->DrawIndexedPrimitive(type, base_vertex_index, min_vertex_index,
                                            vertex_count, start_index, primitive_count);
    return MmeHostDrawIndexedPrimitive(device, type, base_vertex_index,
                                       min_vertex_index, vertex_count, start_index,
                                       primitive_count);
}

bool EffectsActive() {
    return g_deviceRegistered && MmeHostEffectsActive() != 0;
}

HRESULT DrawAccessorySubset(MMDApp* app, void* accessory, unsigned long material) {
    IDirect3DDevice9* device = app->Renderer()->device;
    auto* mesh = static_cast<ID3DXMesh*>(mdl::Accessory(accessory)->mesh);
    if (mesh == nullptr)
        return E_FAIL;
    if (!EffectsActive())
        return mesh->DrawSubset(material);

    IDirect3DVertexBuffer9* vb = nullptr;
    IDirect3DIndexBuffer9* ib = nullptr;
    HRESULT hr = mesh->GetVertexBuffer(&vb);
    if (FAILED(hr) || vb == nullptr)
        return FAILED(hr) ? hr : E_FAIL;
    hr = mesh->GetIndexBuffer(&ib);
    if (FAILED(hr) || ib == nullptr) {
        vb->Release();
        return FAILED(hr) ? hr : E_FAIL;
    }

    hr = DrawMeshSubsetWithFallback(*mesh, *device, *vb, *ib, material, [&] {
        MeshSubsetPlan plan;
        HRESULT planned = PlanMeshSubset(*mesh, material, plan);
        if (FAILED(planned))
            return planned;
        device->SetFVF(mesh->GetFVF());
        device->SetStreamSource(0, vb, 0, mesh->GetNumBytesPerVertex());
        device->SetIndices(ib);
        return DrawMeshSubsetPlan(plan, mesh->GetNumFaces(), (mesh->GetOptions() & 1) != 0,
            [device](const D3DXATTRIBUTERANGE& range) {
                return DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0,
                    range.VertexStart, range.VertexCount, range.FaceStart * 3,
                    range.FaceCount);
            });
    });
    vb->Release();
    ib->Release();
    return hr;
}

void NotifyPmmLoaded(MMDApp* app) {
    if (!g_deviceRegistered)
        return;
    MmeHostNotifyPmmLoaded(app->EnvFileName());
}

void NotifyPmmSaved(MMDApp* app) {
    if (!g_deviceRegistered)
        return;
    MmeHostNotifyPmmSaved(app->EnvFileName());
}

void RecordTexture(const wchar_t* path, IDirect3DBaseTexture9* texture) {
    if (!g_deviceRegistered || path == nullptr || texture == nullptr)
        return;
    MmeHostRecordTextureFile(path, texture);
}

}  
}  
