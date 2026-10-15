#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <cstring>

#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/d3d_wrapper.hpp"
#include "rinmotiondance/mme_bridge.hpp"
#include "rinmotiondance/path_workspace.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

constexpr UINT kD3dxDefault = 0xFFFFFFFFu;
struct ImgInfo {                    
    UINT Width, Height, Depth, MipLevels;
    UINT rest[16];
};

bool WsOpenOk(const wchar_t* path) {
    int fh = -1;
    if (_wsopen_s(&fh, path, _O_BINARY, _SH_DENYNO, _S_IREAD) == 0) {
        _close(fh);
        return true;
    }
    return false;
}

}  

const wchar_t* ResolveUserFilePath(PathResolutionWorkspace& workspace,
                                   const wchar_t* path) {      
    wchar_t* out = workspace.resolvedPath;
    const wchar_t* dirBase = workspace.executableDirectory;
    wchar_t tmp[256];
    if (path[0] == L'\0') {
        out[0] = L'\0';
        return out;
    }
    const wchar_t* uf = wcsstr(path, L"UserFile");
    if (uf != nullptr) {
        swprintf_s(tmp, 0x100, L"%s%s", dirBase, uf);
        bool ok = WsOpenOk(tmp);
        if (!ok &&
            workspace.projectDirectory[0] != L'\0') {
        swprintf_s(tmp, 0x100, L"%s%s",
                       workspace.projectDirectory, uf);
            ok = WsOpenOk(tmp);
        }
        if (ok) {
            wcscpy_s(out, 0x100, tmp);
            return out;
        }
    }
    if (WsOpenOk(path)) {
        wcscpy_s(out, 0x100, path);
        return out;
    }
    const size_t len = wcslen(path);
    const wchar_t* ext = &path[len - 3];
    const wchar_t* scan = ext;
    const wchar_t* sub = nullptr;
    if (wcscmp(scan, L"pmd") == 0 || wcscmp(scan, L"PMD") == 0)
        sub = L"UserFile\\Model";
    else if (wcscmp(&path[len - 3], L"wav") == 0 ||
             wcscmp(&path[len - 3], L"WAV") == 0)
        sub = L"UserFile\\Wave";
    else if ((wcscmp(&path[len - 3], L"x") == 0 ||
              wcscmp(&path[len - 3], L"X") == 0) ||
             (wcscmp(&path[len - 3], L"vac") == 0 ||
              wcscmp(&path[len - 3], L"VAC") == 0))
        sub = L"UserFile\\Accessory";
    if (sub == nullptr) {
        out[0] = L'\0';
        return out;
    }
    while (*scan != L'\\') {
        --scan;
        if (scan == path) {
            out[0] = L'\0';
            return out;
        }
    }
    swprintf_s(tmp, 0x100, L"%s%s%s", dirBase, sub, scan);
    if (WsOpenOk(tmp)) {
        wcscpy_s(out, 0x100, tmp);
        return out;
    }
    out[0] = L'\0';
    return out;
}

bool ResolveAnsiUserFile(unsigned char* sub, const char* mbName,
                         wchar_t* wideOut, rsize_t sizeWords,
                         PathResolutionWorkspace& paths) {     
    wcscpy_s(wideOut, sizeWords, L"");
    if (mbName == nullptr || mbName[0] == '\0')
        return false;
    wchar_t candidate[512] = {};
    auto resolve = [&]() {
        const wchar_t* value = ResolveUserFilePath(paths, candidate);
        if (value[0] == L'\0')
            return false;
        wcsncpy_s(wideOut, sizeWords, value, _TRUNCATE);
        return true;
    };
    MultiByteToWideChar(CP_ACP, 0, mbName, -1, candidate,
                        static_cast<int>(sizeWords));
    if (resolve())
        return true;
    for (int i = 0; i < 4; ++i) {
        _mbstowcs_s_l(nullptr, candidate, sizeWords, mbName, _TRUNCATE,
            reinterpret_cast<D3DRenderer*>(sub)->localeTable[i]);
        if (resolve())
            return true;
    }
    return false;
}

errno_t ConvertMaterialName(unsigned char* sub, const char* mbName,
                            wchar_t* wideOut, rsize_t sizeWords,
                            const wchar_t* dirW) {             
    wchar_t probe[512];
    errno_t result = wcscpy_s(wideOut, sizeWords, L"");
    if (mbName[0] == '\0')
        return result;

    int cw = MultiByteToWideChar(0, 0, mbName, -1, nullptr, 0);
    wchar_t* tmp = static_cast<wchar_t*>(malloc(2 * cw));
    if (MultiByteToWideChar(0, 0, mbName,
                            static_cast<int>(strlen(mbName)) + 1, tmp, cw))
        wcsncpy_s(wideOut, sizeWords, tmp, _TRUNCATE);
    free(tmp);

    for (int li = 0; li < 5; ++li) {
        swprintf_s(probe, 0x200, L"%s%s", dirW, wideOut);
        if (WsOpenOk(probe))
            return 0;
        if (li < 4)
            _mbstowcs_s_l(nullptr, wideOut, sizeWords, mbName, _TRUNCATE,
                reinterpret_cast<D3DRenderer*>(sub)->localeTable[li]);
    }
    return result;
}

int LoadTextureShared(unsigned char* sub, wchar_t* path) {     
    if (path[0] == L'\0')
        return 0;
    D3DRenderer* r = reinterpret_cast<D3DRenderer*>(sub);
    IDirect3DDevice9* dev = r->device;
    int used = 0;
    while (used < 10000) {
        wchar_t* nm = static_cast<wchar_t*>(
            r->resourcePool[used].heapBuffer);
        if (nm == nullptr)
            break;
        if (wcscmp(nm, path) == 0)
            return 1;
        ++used;
    }
    if (used >= 10000)
        return 0;
    unsigned char* tagBytes =
        reinterpret_cast<unsigned char*>(&r->resourcePool[used].tag);
    auto* entryTex = reinterpret_cast<IDirect3DTexture9**>(
        &r->resourcePool[used].comObject);
    auto* entryName = reinterpret_cast<wchar_t**>(
        &r->resourcePool[used].heapBuffer);
    ImgInfo info = {};
    HRESULT hr;
    auto* d3dx = &d3dx::Get();
    hr = d3dx->fromFileExW(dev, path, kD3dxDefault, kD3dxDefault, 1, 1024,
                           D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, kD3dxDefault,
                           kD3dxDefault, 0, &info, nullptr, entryTex);
    if (info.Width == 1 && info.Height == 1 && hr == 0) {
        if (*entryTex != nullptr) {
            (*entryTex)->Release();
            *entryTex = nullptr;
        }
        hr = d3dx->fromFileExW(dev, path, kD3dxDefault, kD3dxDefault, 1, 0,
                               D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                               kD3dxDefault, kD3dxDefault, 0, &info, nullptr,
                               entryTex);
    }
    if (FAILED(hr) &&
        FAILED(d3dx->fromFileExW(dev, path, kD3dxDefault, kD3dxDefault, 1, 0,
                                 D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                 kD3dxDefault, kD3dxDefault, 0, &info,
                                 nullptr, entryTex))) {
        *entryTex = nullptr;
        return 0;
    }
    const size_t plen = wcslen(path) + 1;
    *entryName = static_cast<wchar_t*>(malloc(2 * plen));
    wcscpy_s(*entryName, plen, path);

    mme::RecordTexture(path, *entryTex);

    D3DLOCKED_RECT lr;
    if (SUCCEEDED((*entryTex)->LockRect(0, &lr, nullptr,
                                        D3DLOCK_READONLY))) {
        const unsigned char* px =
            static_cast<const unsigned char*>(lr.pBits) +
            lr.Pitch * (info.Height ? info.Height - 1 : 0);
        tagBytes[2] = px[0];  
        tagBytes[1] = px[1];  
        tagBytes[0] = px[2];  
        (*entryTex)->UnlockRect(0);
    }
    return 1;
}

}  
