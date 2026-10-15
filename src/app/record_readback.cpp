#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/mme_bridge.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

int StereoActivationToggle(unsigned char* wrapper, std::uint8_t enable);

namespace {

constexpr long kEFail = static_cast<long>(0x80004005);

#ifdef RINMOTIONDANCE_DIAG
void TraceRec(const char* fmt, ...) {
    if (getenv("RINMOTIONDANCE_TRACE_REC") == nullptr)
        return;
    if (FILE* tf = fopen(getenv("RINMOTIONDANCE_TRACE_REC"), "a")) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(tf, fmt, ap);
        va_end(ap);
        fclose(tf);
    }
}
#else
inline void TraceRec(const char*, ...) {}
#endif

long CallStreamingState(IPushSource* push, void* flagPtr) {
    return push->GetStreamingState(flagPtr);
}

}  

bool RecordingReadbackPass(MMDApp* app) {
    auto& s = *app;
    D3DRenderer* wrapper = s.Renderer();
    if (wrapper == nullptr)
        return true;  
    IDirect3DDevice9* device = wrapper->device;  
    if (device == nullptr)
        return true;

    const bool stereo3d = s.AviStereoOutput() != 0;
    const std::int32_t capW = stereo3d
        ? s.AviStereoWidthMultiplier() * s.RenderWidth()
        : s.RenderWidth();                                      
    const std::int32_t capH = s.RenderHeight();
    const D3DFORMAT fmt = wrapper->backbufferFormat;  

    IDirect3DSurface9** rtSlot = &s.CaptureRenderTarget();
    if (*rtSlot == nullptr) {                                    
        IDirect3DSurface9* surf = nullptr;
        const HRESULT hr = device->CreateRenderTarget(
            capW, capH, fmt, D3DMULTISAMPLE_NONE, 0, FALSE, &surf,
            nullptr);
        *rtSlot = surf;
        if (FAILED(hr) || surf == nullptr) {
            TraceRec("readback: CreateRenderTarget hr=%ld\n", (long)hr);
            FinishAviRecord(app);                                      
            return false;
        }
    }
    IDirect3DSurface9** sysSlot = &s.CaptureSystemSurface();
    if (*sysSlot == nullptr) {                                   
        IDirect3DSurface9* surf = nullptr;
        const HRESULT hr = device->CreateOffscreenPlainSurface(
            capW, capH, fmt, D3DPOOL_SYSTEMMEM, &surf, nullptr);
        *sysSlot = surf;
        if (FAILED(hr)) {
            TraceRec("readback: sysmem hr=%ld\n", (long)hr);
            FinishAviRecord(app);
            return false;
        }
    }
    if (s.CaptureReadbackPixels() == nullptr) {                  
        const std::size_t bytes = static_cast<std::size_t>(capW) *
                                  static_cast<std::size_t>(capH) * 4;
        s.CaptureReadbackPixels() = ::operator new(bytes);
    }

#ifdef RINMOTIONDANCE_DIAG
    if (const char* pace = getenv("RINMOTIONDANCE_REC_PACE_MS"))
        Sleep(atoi(pace));
#endif
    DShowRecorder* recorder = s.Recorder();
    IPushSource* push = recorder != nullptr && !Mp4RecordingActive()
        ? static_cast<IPushSource*>(recorder->framePush)
        : nullptr;
    unsigned char* flagPtr = s.RecordingCompletionFlag();
    if (push != nullptr)
        CallStreamingState(push, flagPtr);                       
    while (flagPtr != nullptr && *flagPtr == 0) {                
        if (push == nullptr)
            break;
        if (CallStreamingState(push, flagPtr) == kEFail)
            break;  
        Sleep(1);
    }

    IDirect3DSurface9** backBuffer = &wrapper->backbufferSurface;  
    if (*backBuffer == nullptr)
        device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
                              backBuffer);
    if (mme::Present(app, device, nullptr, nullptr, nullptr, nullptr) ==
        D3DERR_DEVICELOST) {
        while (device->TestCooperativeLevel() !=
               D3DERR_DEVICENOTRESET) {
            Sleep(1);
            MSG msg;
            if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        PostDeviceReset(app);                                    
    }
    StereoActivationToggle(
        reinterpret_cast<unsigned char*>(wrapper), 1);           

    HRESULT stretch = 0;                                         
    if (stereo3d && s.AviStereoWidthMultiplier() == 2) {
        const std::int32_t screenW = GetSystemMetrics(SM_CXSCREEN);
        const std::int32_t screenH = GetSystemMetrics(SM_CYSCREEN);
        IDirect3DSurface9* wide = nullptr;
        device->CreateRenderTarget(screenW * 2, screenH, fmt,
                                   D3DMULTISAMPLE_NONE, 0, FALSE, &wide,
                                   nullptr);
        if (wide != nullptr) {
            RECT srcR{0, 0, screenW, screenH};
            RECT dstR{0, 0, screenW * 2, screenH};
            stretch = device->StretchRect(*backBuffer, &srcR, wide, &dstR,
                                          D3DTEXF_LINEAR);
            if (stretch != 0)
                stretch = device->StretchRect(*backBuffer, &srcR, wide,
                                              &dstR, D3DTEXF_NONE);
            RECT eyeL{0, 0, capW, capH};
            HRESULT eye = device->StretchRect(wide, &eyeL, *rtSlot, &eyeL,
                                              D3DTEXF_LINEAR);
            if (eye != 0)
                device->StretchRect(wide, &eyeL, *rtSlot, &eyeL,
                                    D3DTEXF_NONE);
            RECT eyeRsrc{screenW, 0, screenW + capW, capH};
            RECT eyeRdst{capW, 0, capW * 2, capH};
            eye = device->StretchRect(wide, &eyeRsrc, *rtSlot, &eyeRdst,
                                      D3DTEXF_LINEAR);
            if (eye != 0)
                device->StretchRect(wide, &eyeRsrc, *rtSlot, &eyeRdst,
                                    D3DTEXF_NONE);
            wide->Release();
        }
    } else {
        RECT rect{0, 0, capW, capH};
        stretch = device->StretchRect(*backBuffer, &rect, *rtSlot, &rect,
                                      D3DTEXF_LINEAR);
        if (stretch != 0)
            stretch = device->StretchRect(*backBuffer, &rect, *rtSlot,
                                          &rect, D3DTEXF_NONE);
    }
    if (stereo3d)
        StereoActivationToggle(
            reinterpret_cast<unsigned char*>(wrapper), 0);       
    if (stretch != 0) {
        TraceRec("readback: StretchRect hr=%ld\n", (long)stretch);
        FinishAviRecord(app);                                          
        return false;
    }

    const HRESULT grtdHr = device->GetRenderTargetData(*rtSlot, *sysSlot);
    if (FAILED(grtdHr)) {
        TraceRec("readback: GetRenderTargetData hr=%ld\n",
                 (long)grtdHr);
        FinishAviRecord(app);                                          
        return false;
    }
    D3DLOCKED_RECT locked;
    if (FAILED((*sysSlot)->LockRect(&locked, nullptr,
                                    D3DLOCK_READONLY))) {        
        TraceRec("readback: LockRect failed\n");
        FinishAviRecord(app);
        return false;
    }
    {
        const std::size_t rowBytes =
            static_cast<std::size_t>(capW) * 4;
        unsigned char* dst = static_cast<unsigned char*>(
                                 s.CaptureReadbackPixels()) +
                             (static_cast<std::size_t>(capH) - 1) *
                                 rowBytes;
        const unsigned char* src =
            static_cast<const unsigned char*>(locked.pBits);
        for (std::int32_t y = 0; y < capH; ++y) {                
            std::memcpy(dst, src, rowBytes);
            src += locked.Pitch;
            dst -= rowBytes;                                     
        }
    }
    static LONG passCount = 0;
    if (++passCount <= 5 || passCount % 60 == 0)
        TraceRec("readback pass#%ld push=%p capW=%d capH=%d" "\n",
                 passCount, (void*)push, (int)capW, (int)capH);
    if (Mp4RecordingActive()) {
        if (!WriteMp4Frame(s.CaptureReadbackPixels(), capW, capH)) {
            (*sysSlot)->UnlockRect();
            FinishAviRecord(app);
            return false;
        }
    } else if (push != nullptr) {
        push->StartStreaming(
            reinterpret_cast<DWORD_PTR>(s.CaptureReadbackPixels()));
    }
    (*sysSlot)->UnlockRect();                                    
    return true;
}

}  
