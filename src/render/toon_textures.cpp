#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstdio>
#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mme_bridge.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

struct D3dxImageInfo {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;
    D3DRESOURCETYPE ResourceType;
    UINT ImageFileFormat;
};

auto& g_d3dx = d3dx::Get();

IDirect3DDevice9* DeviceOf(MMDApp* app) {
    D3DRenderer* sub = app->Renderer();                           
    if (sub == nullptr)
        return nullptr;
    return sub->device;                                           
}

}  

bool InitToonTextures(MMDApp* app) {
    auto& s = *app;
    IDirect3DDevice9* device = DeviceOf(app);
    if (device == nullptr)
        return false;   

    SetCurrentDirectoryW(app->ExeDir());                          

    for (int i = 0; i < 11; ++i) {
        IDirect3DTexture9* tex = s.ToonTexture(i);                
        if (tex != nullptr) {
            static_cast<IUnknown*>(tex)->Release();
            s.ToonTexture(i) = nullptr;
        }
    }

    HRSRC res = FindResourceA(static_cast<HMODULE>(s.HInstance()),
                              MAKEINTRESOURCEA(0x67), "PNG");
    DWORD size = SizeofResource(nullptr, res);
    HGLOBAL glob = LoadResource(static_cast<HMODULE>(s.HInstance()), res);
    void* data = LockResource(glob);
    IDirect3DTexture9* tex0 = nullptr;
    if (FAILED(g_d3dx.fromMemEx(device, data, size,
                                static_cast<UINT>(-1), static_cast<UINT>(-1),
                                1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                static_cast<DWORD>(-1), static_cast<DWORD>(-1),
                                0, nullptr, nullptr, &tex0)))
        return false;
    s.ToonTexture(0) = tex0;

    const float defaults[14] = {
        0.80078101f, 0.80078101f, 0.80078101f, 0.95703101f, 0.87890601f,
        0.87890601f, 0.60156298f, 0.60156298f, 0.60156298f, 0.96875f,
        0.93359399f, 0.91796899f, 1.0f, 0.90234399f,
    };
    for (int i = 0; i < 14; ++i)
        s.state.toonEdgeTable[i] = defaults[i];
    const float tail[4] = {0.86328101f, 0.76171899f, 0.671875f,
                           0.011719f};
    for (int i = 0; i < 4; ++i)
        s.state.toonEdgeTable[14 + i] = tail[i];
    for (int i = 18; i < 30; ++i)
        s.state.toonEdgeTable[i] = 1.0f;

    for (int i = 1; i < 11; ++i) {
        char path[256];
        sprintf_s(path, 0x100, "data\\toon%02d.bmp", i);
        wchar_t widePath[256] = {};
        MultiByteToWideChar(CP_ACP, 0, path, -1, widePath, 256);
        IDirect3DTexture9** slot = &s.ToonTexture(i);
        D3dxImageInfo info{};
        if (FAILED(g_d3dx.fromFileExA(device, path,
                                      static_cast<UINT>(-1),
                                      static_cast<UINT>(-1), 1, 0,
                                      D3DFMT_A8R8G8B8  ,
                                      D3DPOOL_MANAGED  ,
                                      static_cast<DWORD>(-1),
                                      static_cast<DWORD>(-1), 0,
                                      &info, nullptr, slot))) {
            HRSRC r = FindResourceA(static_cast<HMODULE>(s.HInstance()),
                                    MAKEINTRESOURCEA(i + 103), "PNG");
            DWORD sz = SizeofResource(nullptr, r);
            HGLOBAL g = LoadResource(static_cast<HMODULE>(s.HInstance()), r);
            void* d = LockResource(g);
            g_d3dx.fromMemEx(device, d, sz, static_cast<UINT>(-1),
                             static_cast<UINT>(-1), 1, 0, D3DFMT_UNKNOWN,
                             D3DPOOL_MANAGED, static_cast<DWORD>(-1),
                             static_cast<DWORD>(-1), 0, nullptr, nullptr,
                             slot);
        } else {
            D3DLOCKED_RECT rect;
            if (SUCCEEDED((*slot)->LockRect(0, &rect, nullptr, 0))) {
                auto* row = static_cast<unsigned char*>(rect.pBits) +
                            rect.Pitch * (info.Height - 1);
                float* out = &s.state.toonEdgeTable[3 * (i - 1)];
                out[0] = row[2] * 0.00390625f;     
                out[1] = row[1] * 0.00390625f;     
                out[2] = row[0] * 0.00390625f;     
                (*slot)->UnlockRect(0);
            }
        }
        if (*slot != nullptr) {
            mme::RecordTexture(widePath, *slot);
        }
    }
    return true;
}

}  
