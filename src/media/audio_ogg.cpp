#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <cstdint>
#include <cwchar>
#include <string>
#include <vector>

namespace rinmotiondance {
namespace {

struct WaveHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    std::uint32_t size = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    std::uint32_t fmtSize = 16;
    std::uint16_t format = 1;
    std::uint16_t channels = 2;
    std::uint32_t sampleRate = 44100;
    std::uint32_t bytesPerSecond = 176400;
    std::uint16_t blockAlign = 4;
    std::uint16_t bits = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    std::uint32_t dataSize = 0;
};
static_assert(sizeof(WaveHeader) == 44);

struct AudioCache {
    std::wstring source;
    std::wstring pcm;
    std::vector<std::wstring> temporaryFiles;

    ~AudioCache() {
        for (const auto& file : temporaryFiles)
            DeleteFileW(file.c_str());
    }
};

AudioCache gAudioCache;

template <typename T>
void ReleaseAudioCom(T*& object) {
    if (object != nullptr) {
        object->Release();
        object = nullptr;
    }
}

bool IsOggPath(const wchar_t* path) {
    if (path == nullptr)
        return false;
    const wchar_t* dot = wcsrchr(path, L'.');
    return dot != nullptr && _wcsicmp(dot, L".ogg") == 0;
}

bool DecodeOggToWave(const wchar_t* source, std::wstring& output) {
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
        return false;

    IMFSourceReader* reader = nullptr;
    IMFMediaType* type = nullptr;
    IMFSample* sample = nullptr;
    IMFMediaBuffer* buffer = nullptr;
    HANDLE file = INVALID_HANDLE_VALUE;
    std::wstring temporary;
    WaveHeader header{};
    std::uint32_t audioBytes = 0;

    hr = MFCreateSourceReaderFromURL(source, nullptr, &reader);
    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&type);
    if (SUCCEEDED(hr))
        hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr))
        hr = type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (SUCCEEDED(hr))
        hr = type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    if (SUCCEEDED(hr))
        hr = reader->SetCurrentMediaType(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, type);
    ReleaseAudioCom(type);
    if (SUCCEEDED(hr))
        hr = reader->GetCurrentMediaType(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM, &type);
    UINT32 channels = 0;
    UINT32 rate = 0;
    if (SUCCEEDED(hr))
        hr = type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
    if (SUCCEEDED(hr))
        hr = type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
    if (SUCCEEDED(hr) && (channels < 1 || channels > 2 ||
                          rate == 0 || rate > 192000))
        hr = E_FAIL;
    if (SUCCEEDED(hr)) {
        header.channels = static_cast<std::uint16_t>(channels);
        header.sampleRate = rate;
        header.blockAlign = static_cast<std::uint16_t>(channels * 2);
        header.bytesPerSecond = rate * header.blockAlign;
        wchar_t directory[MAX_PATH]{};
        wchar_t name[MAX_PATH]{};
        if (!GetTempPathW(MAX_PATH, directory) ||
            !GetTempFileNameW(directory, L"RMD", 0, name)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        } else {
            temporary = name;
            std::wstring wav = temporary;
            const std::size_t dot = wav.find_last_of(L'.');
            if (dot != std::wstring::npos)
                wav.replace(dot, std::wstring::npos, L".wav");
            else
                wav += L".wav";
            if (!MoveFileExW(name, wav.c_str(), MOVEFILE_REPLACE_EXISTING))
                hr = HRESULT_FROM_WIN32(GetLastError());
            else
                temporary = wav;
        }
    }
    if (SUCCEEDED(hr)) {
        file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    if (SUCCEEDED(hr)) {
        DWORD written = 0;
        if (!WriteFile(file, &header, sizeof(header), &written, nullptr) ||
            written != sizeof(header))
            hr = E_FAIL;
    }

    while (SUCCEEDED(hr)) {
        DWORD flags = 0;
        hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0,
                                nullptr, &flags, nullptr, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM))
            break;
        if (sample == nullptr)
            continue;
        hr = sample->ConvertToContiguousBuffer(&buffer);
        BYTE* bytes = nullptr;
        DWORD size = 0;
        if (SUCCEEDED(hr))
            hr = buffer->Lock(&bytes, nullptr, &size);
        if (SUCCEEDED(hr)) {
            DWORD written = 0;
            if (size > 0xFFFFFFFFu - audioBytes - sizeof(WaveHeader) ||
                !WriteFile(file, bytes, size, &written, nullptr) ||
                written != size)
                hr = E_FAIL;
            else
                audioBytes += size;
            buffer->Unlock();
        }
        ReleaseAudioCom(buffer);
        ReleaseAudioCom(sample);
    }
    ReleaseAudioCom(buffer);
    ReleaseAudioCom(sample);
    if (SUCCEEDED(hr) && file != INVALID_HANDLE_VALUE && audioBytes != 0) {
        header.dataSize = audioBytes;
        header.size = audioBytes + sizeof(WaveHeader) - 8;
        if (SetFilePointer(file, 0, nullptr, FILE_BEGIN) ==
            INVALID_SET_FILE_POINTER)
            hr = E_FAIL;
        DWORD written = 0;
        if (SUCCEEDED(hr) &&
            (!WriteFile(file, &header, sizeof(header), &written, nullptr) ||
             written != sizeof(header)))
            hr = E_FAIL;
    } else if (SUCCEEDED(hr)) {
        hr = E_FAIL;
    }
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
    ReleaseAudioCom(type);
    ReleaseAudioCom(reader);
    MFShutdown();

    if (FAILED(hr)) {
        if (!temporary.empty())
            DeleteFileW(temporary.c_str());
        return false;
    }
    output = temporary;
    return true;
}

}

const wchar_t* ResolveAudioPlaybackFile(const wchar_t* source) {
    if (!IsOggPath(source))
        return source;
    if (gAudioCache.source == source && !gAudioCache.pcm.empty())
        return gAudioCache.pcm.c_str();
    std::wstring decoded;
    if (!DecodeOggToWave(source, decoded))
        return nullptr;
    gAudioCache.source = source;
    gAudioCache.pcm = decoded;
    gAudioCache.temporaryFiles.push_back(decoded);
    return gAudioCache.pcm.c_str();
}

}
