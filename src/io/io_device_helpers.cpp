#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <objbase.h>    

#include <cstdint>
#include <cwchar>

#include "rinmotiondance/d3d_wrapper.hpp"

namespace rinmotiondance {


void* TextureTableFind(D3DRenderer* wrapper, const wchar_t* name) {
    if (name[0] == L'\0')                                       
        return nullptr;
    for (int index = 0;;) {
        const wchar_t* entryName = static_cast<const wchar_t*>(
            wrapper->resourcePool[index].heapBuffer);
        if (entryName == nullptr)                               
            return nullptr;
        if (wcscmp(entryName, name) == 0)                       
            return wrapper->resourcePool[index].comObject;
        if (++index >= 10000)                                   
            return nullptr;
    }
}

float* TextureTableLookupColor(D3DRenderer* wrapper, float* outRgb,
                               const wchar_t* name) {
    outRgb[2] = 1.0f;                                           
    outRgb[1] = 1.0f;                                           
    outRgb[0] = 1.0f;                                           
    if (name[0] == L'\0')                                       
        return outRgb;
    for (int index = 0;;) {
        const wchar_t* entryName = static_cast<const wchar_t*>(
            wrapper->resourcePool[index].heapBuffer);
        if (entryName == nullptr)                               
            return outRgb;
        if (wcscmp(entryName, name) == 0) {                     
            const unsigned char* spill =                        
                reinterpret_cast<const unsigned char*>(
                    &wrapper->resourcePool[index].tag);
            outRgb[0] = static_cast<float>(                    
                static_cast<double>(spill[0]) * 0.00390625);
            outRgb[1] = static_cast<float>(                    
                static_cast<double>(spill[1]) * 0.00390625);
            outRgb[2] = static_cast<float>(                    
                static_cast<double>(spill[2]) * 0.00390625);
            return outRgb;
        }
        if (++index >= 10000)                                   
            return outRgb;
    }
}

namespace {

constexpr std::uint32_t kNvapiStereoReverseBlitControlId = 0x3CD58F89u;

int NvapiStereo3CD58F89(void* stereoHandle, unsigned enable) {
    using QueryInterface = void*(__cdecl*)(std::uint32_t);
    using StereoCall = int(__cdecl*)(void*, unsigned);
    static StereoCall call = nullptr;    
    static bool resolved = false;        
    if (!resolved) {
        resolved = true;
        HMODULE module = LoadLibraryA(sizeof(void*) == 8 ? "nvapi64.dll"
                                                         : "nvapi.dll");
        if (module != nullptr) {
            auto query = reinterpret_cast<QueryInterface>(
                GetProcAddress(module, "nvapi_QueryInterface"));
            if (query != nullptr)
                call = reinterpret_cast<StereoCall>(
                    query(kNvapiStereoReverseBlitControlId));
        }
    }
    if (call == nullptr)
        return -3;                                               
    return call(stereoHandle, enable);                           
}

}  

int StereoActivationToggle(unsigned char* wrapper, std::uint8_t enable) {
    void* stereoHandle =
        reinterpret_cast<D3DRenderer*>(wrapper)->stereoHandle;  
    return NvapiStereo3CD58F89(stereoHandle, enable);           
}

bool DirectShowInit(HWND hWnd) {
    if (CoInitialize(nullptr) >= 0)                              
        return true;                                             
    MessageBoxA(hWnd, "Failed CoInitialize!", "DirectShowInit", 0);
    return false;                                                
}


}  
