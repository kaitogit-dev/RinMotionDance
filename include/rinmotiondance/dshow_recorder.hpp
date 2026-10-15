#pragma once

#include <cstdint>
#include <Windows.h>

struct IBaseFilter;
struct IFileSinkFilter;
struct IGraphBuilder;
struct IAMVfwCompressDialogs;
struct IMediaControl;
struct IMediaEvent;
struct IPin;

namespace rinmotiondance {

struct IPushSource : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetBitmapInfo(const void* pBitmapInfo,
                                                    int n, float fps) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamingState(void* pState) = 0;
    virtual HRESULT STDMETHODCALLTYPE StartStreaming(DWORD_PTR dwBits) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginStreaming() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRate(float* pRate) = 0;
};

struct DShowRecorder {
    IBaseFilter* compressor;
    IAMVfwCompressDialogs* compressorDialogs;
    void* compressorState;
    IGraphBuilder* graph;
    IBaseFilter* videoSource;
    IBaseFilter* videoGrabber;
    IBaseFilter* aviMux;
    IFileSinkFilter* fileWriter;
    IPin* sourceOutput;
    IPin* grabberInput;
    IPin* grabberOutput;
    IPin* compressorInput;
    IPin* compressorOutput;
    IPin* muxVideoInput;
    IPin* reservedInput0;
    IPin* reservedInput1;
    IPin* muxAudioInput;
    IMediaControl* mediaControl;
    IBaseFilter* waveSource;
    IPin* waveOutput;
    IBaseFilter* audioGrabber;
    IPin* audioGrabberInput;
    IPin* audioGrabberOutput;
    IMediaEvent* mediaEvent;
    std::int32_t compressorStateSize;
    std::int32_t selectedCodec;
    void* framePush;
};

#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(DShowRecorder) == 0x6C);
#endif

}  
