#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <string>

namespace rinmotiondance {
namespace {

struct Mp4Session {
    IMFSinkWriter* writer = nullptr;
    DWORD videoStream = 0;
    std::wstring path;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint32_t frameRateNumerator = 0;
    std::uint32_t frameRateDenominator = 1000;
    std::uint64_t frameIndex = 0;
    HRESULT failure = S_OK;
    bool started = false;
};

Mp4Session gMp4;

template <typename T>
void ReleaseMp4(T*& object) {
    if (object != nullptr) {
        object->Release();
        object = nullptr;
    }
}

void ShowMp4Error(HWND owner, const wchar_t* operation, HRESULT error) {
    wchar_t message[256]{};
    swprintf_s(message, L"%s (0x%08X)", operation,
               static_cast<unsigned>(error));
    MessageBoxW(owner, message, L"MP4 export", MB_OK | MB_ICONERROR);
}

void ResetMp4Session(bool deleteOutput) {
    ReleaseMp4(gMp4.writer);
    if (gMp4.started)
        MFShutdown();
    if (deleteOutput && !gMp4.path.empty())
        DeleteFileW(gMp4.path.c_str());
    gMp4 = {};
}

HRESULT ConfigureMp4Writer(const wchar_t* path,
                           std::int32_t width, std::int32_t height,
                           std::uint32_t rate) {
    HRESULT hr = MFCreateSinkWriterFromURL(path, nullptr, nullptr, &gMp4.writer);
    if (FAILED(hr))
        return hr;

    IMFMediaType* output = nullptr;
    IMFMediaType* input = nullptr;
    hr = MFCreateMediaType(&output);
    if (SUCCEEDED(hr))
        hr = output->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr))
        hr = output->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    if (SUCCEEDED(hr))
        hr = output->SetUINT32(MF_MT_AVG_BITRATE,
            static_cast<UINT32>(std::clamp(
                static_cast<double>(width) * height * rate * 0.00014,
                2500000.0, 20000000.0)));
    if (SUCCEEDED(hr))
        hr = output->SetUINT32(MF_MT_INTERLACE_MODE,
                               MFVideoInterlace_Progressive);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeSize(output, MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(output, MF_MT_FRAME_RATE, rate, 1000);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(output, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(hr))
        hr = gMp4.writer->AddStream(output, &gMp4.videoStream);

    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&input);
    if (SUCCEEDED(hr))
        hr = input->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr))
        hr = input->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (SUCCEEDED(hr))
        hr = input->SetUINT32(MF_MT_INTERLACE_MODE,
                              MFVideoInterlace_Progressive);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeSize(input, MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(input, MF_MT_FRAME_RATE, rate, 1000);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(input, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(hr))
        hr = gMp4.writer->SetInputMediaType(gMp4.videoStream, input, nullptr);
    if (SUCCEEDED(hr))
        hr = gMp4.writer->BeginWriting();

    ReleaseMp4(input);
    ReleaseMp4(output);
    return hr;
}

}

bool IsMp4OutputPath(const wchar_t* path) {
    if (path == nullptr)
        return false;
    const wchar_t* dot = wcsrchr(path, L'.');
    return dot != nullptr && _wcsicmp(dot, L".mp4") == 0;
}

bool Mp4RecordingActive() {
    return gMp4.writer != nullptr && gMp4.started;
}

bool StartMp4Recording(HWND owner, const wchar_t* path,
                       std::int32_t width, std::int32_t height, float fps) {
    if (Mp4RecordingActive())
        return false;
    if (width <= 0 || height <= 0 || (width & 1) || (height & 1) ||
        !std::isfinite(fps) || fps <= 0.0f || fps > 240.0f) {
        MessageBoxW(owner,
            L"MP4 requires even video dimensions and a frame rate between 1 and 240.",
            L"MP4 export", MB_OK | MB_ICONERROR);
        return false;
    }
    const auto rate = static_cast<std::uint32_t>(std::round(fps * 1000.0f));
    if (rate == 0)
        return false;
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        ShowMp4Error(owner, L"Could not start the MP4 encoder", hr);
        return false;
    }
    gMp4.started = true;
    gMp4.path = path;
    hr = ConfigureMp4Writer(path, width, height, rate);
    if (FAILED(hr)) {
        ShowMp4Error(owner, L"Could not initialize H.264 encoding", hr);
        ResetMp4Session(true);
        return false;
    }
    gMp4.width = width;
    gMp4.height = height;
    gMp4.frameRateNumerator = rate;
    gMp4.frameRateDenominator = 1000;
    return true;
}

bool WriteMp4Frame(const void* pixels, std::int32_t width,
                   std::int32_t height) {
    if (!Mp4RecordingActive() || pixels == nullptr ||
        width != gMp4.width || height != gMp4.height)
        return false;
    if (FAILED(gMp4.failure))
        return false;

    const auto byteCount = static_cast<DWORD>(
        static_cast<std::uint64_t>(width) * height * 4);
    IMFMediaBuffer* buffer = nullptr;
    IMFSample* sample = nullptr;
    HRESULT hr = MFCreateMemoryBuffer(byteCount, &buffer);
    if (SUCCEEDED(hr)) {
        BYTE* destination = nullptr;
        hr = buffer->Lock(&destination, nullptr, nullptr);
        if (SUCCEEDED(hr)) {
            std::memcpy(destination, pixels, byteCount);
            buffer->Unlock();
            hr = buffer->SetCurrentLength(byteCount);
        }
    }
    if (SUCCEEDED(hr))
        hr = MFCreateSample(&sample);
    if (SUCCEEDED(hr))
        hr = sample->AddBuffer(buffer);
    if (SUCCEEDED(hr)) {
        const auto timestamp = [=](std::uint64_t frame) {
            return static_cast<LONGLONG>(
                (frame * 10000000ull * gMp4.frameRateDenominator) /
                gMp4.frameRateNumerator);
        };
        const LONGLONG time = timestamp(gMp4.frameIndex);
        hr = sample->SetSampleTime(time);
        if (SUCCEEDED(hr))
            hr = sample->SetSampleDuration(
                timestamp(gMp4.frameIndex + 1) - time);
        if (SUCCEEDED(hr))
            hr = gMp4.writer->WriteSample(gMp4.videoStream, sample);
        if (SUCCEEDED(hr))
            ++gMp4.frameIndex;
    }
    ReleaseMp4(sample);
    ReleaseMp4(buffer);
    if (FAILED(hr))
        gMp4.failure = hr;
    return SUCCEEDED(hr);
}

bool FinishMp4Recording(HWND owner) {
    if (!gMp4.started)
        return true;
    HRESULT hr = gMp4.failure;
    if (SUCCEEDED(hr) && gMp4.writer != nullptr)
        hr = gMp4.writer->Finalize();
    if (FAILED(hr))
        ShowMp4Error(owner, L"MP4 encoding failed", hr);
    ResetMp4Session(FAILED(hr));
    return SUCCEEDED(hr);
}

}
