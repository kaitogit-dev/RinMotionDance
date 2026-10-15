#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

using QueryInterface = void* (__cdecl*)(std::uint32_t);

constexpr std::uint32_t kNvapiInitializeId = 0x0150E828u;
constexpr std::uint32_t kNvapiStereoCreateHandleId = 0xAC7E37F4u;
constexpr std::uint32_t kNvapiStereoIsActivatedId = 0x1FB0BC30u;
constexpr std::uint32_t kNvapiStereoActivateId = 0xF6A1AD68u;
constexpr std::uint32_t kNvapiStereoSetConvergenceId = 0x3DD6B54Bu;
constexpr std::uint32_t kNvapiStereoCapsId = 0xBE7692ECu;
constexpr std::uint32_t kNvapiStereoSupportId = 0x239C4545u;

HMODULE g_nvapiModule = nullptr;
QueryInterface g_nvapiQuery = nullptr;
bool g_nvapiAttempted = false;

int NvapiInitImpl() {                                         
    if (g_nvapiModule == nullptr) {
        g_nvapiModule = LoadLibraryA(sizeof(void*) == 8 ? "nvapi64.dll"
                                                        : "nvapi.dll");
        if (g_nvapiModule == nullptr)
            return -2;                                        
    }
    g_nvapiQuery = reinterpret_cast<QueryInterface>(
        GetProcAddress(g_nvapiModule, "nvapi_QueryInterface"));
    if (g_nvapiQuery != nullptr) {
        using Initialize = int (__cdecl*)();
        auto initialize = reinterpret_cast<Initialize>(
            g_nvapiQuery(kNvapiInitializeId));
        if (initialize == nullptr || initialize() != 0) {
            g_nvapiQuery = nullptr;
            FreeLibrary(g_nvapiModule);                       
            g_nvapiModule = nullptr;
            return -1;                                        
        }
        return 0;
    }
    FreeLibrary(g_nvapiModule);                               
    g_nvapiModule = nullptr;
    return -1;                                                
}

QueryInterface NvQuery() {
    if (!g_nvapiAttempted) {
        g_nvapiAttempted = true;
        NvapiInitImpl();
    }
    return g_nvapiQuery;
}

template <typename Fn>
Fn Resolve(std::uint32_t id) {
    QueryInterface query = NvQuery();
    return query != nullptr ? reinterpret_cast<Fn>(query(id)) : nullptr;
}

bool ActivateStereo(D3DRenderer* wrapper) {  
    void* handle = wrapper->stereoHandle;  
    if (handle == nullptr)
        return false;
    using IsActivated = int (__cdecl*)(void*, std::uint8_t*);
    using Activate = int (__cdecl*)(void*);
    auto isActivated = Resolve<IsActivated>(kNvapiStereoIsActivatedId);
    auto activate = Resolve<Activate>(kNvapiStereoActivateId);
    std::uint8_t active = 0;
    if (isActivated != nullptr)
        isActivated(handle, &active);
    return active != 0 || (activate != nullptr && activate(handle) == 0);
}

void SetStereoConvergence(D3DRenderer* wrapper, float value) {  
    void* handle = wrapper->stereoHandle;  
    using SetConvergence = int (__cdecl*)(void*, float);
    auto set = Resolve<SetConvergence>(kNvapiStereoSetConvergenceId);
    if (handle != nullptr && set != nullptr)
        set(handle, value);
}

}  

bool ProbeStereo3D(void* device, void* handleSlot) {  
    using CreateHandle = int (__cdecl*)(void*, void**);
    auto create = Resolve<CreateHandle>(kNvapiStereoCreateHandleId);
    return create == nullptr || create(device, static_cast<void**>(handleSlot)) != 0;
}

int NvapiStereoCaps(int arg) {
    using Caps = int (__cdecl*)(int);
    static Caps call = nullptr;
    static bool resolved = false;
    if (!resolved) {
        resolved = true;
        QueryInterface query = NvQuery();
        if (query != nullptr)
            call = reinterpret_cast<Caps>(query(kNvapiStereoCapsId));
    }
    if (call == nullptr)
        return -3;                                            
    return call(arg);                                         
}

int NvapiStereoSupportGate() {
    using Gate = int (__cdecl*)();
    static Gate call = nullptr;
    static bool resolved = false;
    if (!resolved) {
        resolved = true;
        QueryInterface query = NvQuery();
        if (query != nullptr)
            call = reinterpret_cast<Gate>(query(kNvapiStereoSupportId));
    }
    if (call == nullptr)
        return -3;                                            
    return call();                                            
}

int NvapiInitChain() {
    return NvapiInitImpl();
}

void UpdateFrameStereo(MMDApp* app) {  
    D3DRenderer* wrapper = app->Renderer();
    if (wrapper == nullptr || wrapper->stereoEnabled == 0)
        return;
    if (app->state.stereoActivated == 0 &&
        app->state.recordFullscreenActive != 0) {
        app->state.stereoActivated =
            ActivateStereo(wrapper) ? 1 : 0;
    }
    if (app->state.stereoActivated != 0)
        SetStereoConvergence(wrapper, -app->CameraDistance());
}

}  
