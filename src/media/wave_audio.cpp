#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>   
#include <dsound.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/wave_audio_context.hpp"
#include "rinmotiondance/wave_data_chunk.hpp"

namespace rinmotiondance {
namespace {


#ifdef RINMOTIONDANCE_DIAG
bool RINMOTIONDANCE_WAVE_TRACE() { return getenv("RINMOTIONDANCE_TRACE_WAVE") != nullptr; }
void WaveTrace(const char* fmt, ...) {
    FILE* tf = fopen(getenv("RINMOTIONDANCE_TRACE_WAVE"), "a");
    if (!tf) return;
    va_list ap; va_start(ap, fmt);
    vfprintf(tf, fmt, ap);
    va_end(ap);
    fclose(tf);
}
#else
inline bool RINMOTIONDANCE_WAVE_TRACE() { return false; }
inline void WaveTrace(const char*, ...) {}
#endif

static const char kMsgWaveNoSoundJp[] =
    "DirectSound\x82\xCC\x8F\x89\x8A\xFA\x89\xBB\x82\xC9\x8E\xB8\x94\x73"
    "\x82\xB5\x82\xC4\x82\xA2\x82\xE9\x82\xBD\x82\xDF\x81\x41WAV\x82\xCD"
    "\x96\xC2\x82\xE7\x82\xB9\x82\xDC\x82\xB9\x82\xF1";
static const char kMsgWaveErrCaptionJp[] =
    "DirectSound\x83\x47\x83\x89\x81\x5B";
static const char kMsgWaveNotFoundJp[] =
    "WAVE\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xAA\x8C\xA9\x82\xC2\x82\xA9"
    "\x82\xE8\x82\xDC\x82\xB9\x82\xF1";
static const char kMsgWaveCaptionJp[] = "WAVE\x93\xC7\x8D\x9E";
static const char kMsgWaveNotPcmJp[] =
    "WAVE(PCM)\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xC5\x82\xCD\x82\xA0"
    "\x82\xE8\x82\xDC\x82\xB9\x82\xF1";

}  

void WaveCtxReset(void* obj) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    audio->parseStream = nullptr;               
    audio->feedThread = 0;                      
    audio->streamingBuffer = nullptr;           
    audio->fileHandle = nullptr;                
}

bool WaveFindDataChunk(void* obj, FILE* stream) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    if (!ScanWaveDataChunk(stream, audio->dataSize, audio->dataOffset))
        return false;
    audio->readCursor = 0;                                        
    return true;
}

bool WaveStreamRead(void* obj, void* buf, int bytes) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    const std::int32_t cursor = audio->readCursor;
    const std::int32_t size = audio->dataSize;
    std::int32_t toRead;                                   
    if (cursor + bytes < size)
        toRead = bytes;
    else
        toRead = size - cursor;
    DWORD got = 0;
    if (!ReadFile(audio->fileHandle, buf,
                  static_cast<DWORD>(toRead), &got, nullptr))       
        got = 0;
    audio->readCursor += static_cast<std::int32_t>(got);           
    if (static_cast<std::int32_t>(got) >= bytes)
        return true;                                               
    const unsigned short bits = audio->format.wBitsPerSample;
    if (bits == 16)                                                
        memset(static_cast<unsigned char*>(buf) + got, 0,
               static_cast<size_t>(bytes - got));
    else if (bits == 8)                                            
        memset(static_cast<unsigned char*>(buf) + got, 0x80,
               static_cast<size_t>(bytes - got));
    return false;
}

bool WaveStreamFeed(void* obj, void* buf, int bytes, void* buf2,
                    int bytes2) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    if (!WaveStreamRead(audio, buf, bytes))                        
        return false;
    if (buf2 != nullptr) {                                         
        if (!WaveStreamRead(audio, buf2, bytes2))                  
            return false;
    }
    return true;                                                   
}

bool WaveLoadFile(void* obj, const wchar_t* path,
                  PathResolutionWorkspace& paths) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    HWND hwnd = audio->mainWindow;
    const unsigned char english = audio->englishUI;

    if (audio->parseStream != nullptr)                              
        CloseDataFile(audio);                                      
    std::memset(&audio->format, 0, sizeof audio->format);           

    ResolveUserFilePath(paths, path);                                  
    wcscpy_s(audio->path, 0x100, paths.resolvedPath);              

    FILE*& stream = audio->parseStream;
    _wfopen_s(&stream, audio->path, L"rb");                         
    if (stream == nullptr) {                                       
        MessageBoxA(hwnd,
                    english ? "Cannot find Wave file!!"
                            : kMsgWaveNotFoundJp,                  
                    english ? "open wave" : kMsgWaveCaptionJp,     
                    0);
        return false;                                              
    }

    char riff[4] = {};
    std::fread(riff, 4, 1, stream);                                
    bool pcm = riff[0] == 'R' && riff[1] == 'I' &&                 
               riff[2] == 'F' && riff[3] == 'F';
    if (pcm) {
        std::fseek(stream, 0x10, SEEK_CUR);                        
        unsigned short tag = 0;
        std::fread(&tag, 2, 1, stream);                            
        pcm = tag == 1;                                            
    }
    if (!pcm) {
        MessageBoxA(hwnd,
                    english ? "This is not WAVE(PCM) file"
                            : kMsgWaveNotPcmJp,
                    "WAVE", 0);
        return false;                                              
    }

    std::memset(&audio->format, 0, sizeof audio->format);           
    audio->format.wFormatTag = WAVE_FORMAT_PCM;                     
    audio->format.cbSize = 0x12;                                    
    std::fread(&audio->format.nChannels, 2, 1, stream);             
    std::fread(&audio->format.nSamplesPerSec, 4, 1, stream);        
    std::fread(&audio->format.nAvgBytesPerSec, 4, 1, stream);
    std::fread(&audio->format.nBlockAlign, 2, 1, stream);
    std::fread(&audio->format.wBitsPerSample, 2, 1, stream);        
    if (!WaveFindDataChunk(audio, stream)) {
        std::fclose(stream);
        stream = nullptr;
        return false;
    }

    const float avgF = static_cast<float>(
        static_cast<std::uint32_t>(audio->format.nAvgBytesPerSec));
    const int blockAlign = audio->format.nBlockAlign;
    if (blockAlign == 0) {
        std::fclose(stream);
        stream = nullptr;
        return false;
    }
    const int colUnits = static_cast<int>(
        avgF * (1.0f / 30.0f) / 13.0f / static_cast<float>(blockAlign));
    const int colBytes = colUnits * blockAlign;
    if (colBytes <= 0) {
        std::fclose(stream);
        stream = nullptr;
        return false;
    }

    audio->bufferBytes = static_cast<std::int32_t>(
        audio->format.nAvgBytesPerSec * 2);                         
    HANDLE file = CreateFileW(audio->path,
                              GENERIC_READ, 1, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);                            
    audio->fileHandle = file == INVALID_HANDLE_VALUE ? nullptr : file;
    if (file != INVALID_HANDLE_VALUE)
        SetFilePointer(file, audio->dataOffset, nullptr, FILE_BEGIN);
    std::fclose(stream);                                           
    stream = nullptr;
    if (file == INVALID_HANDLE_VALUE)
        return false;                                              

    free(audio->waveformMax);                                      
    free(audio->waveformMin);                                      

    unsigned char* scratch = static_cast<unsigned char*>(malloc(colBytes));
    memset(scratch, 0, static_cast<size_t>(colBytes));              

    const std::int32_t columns =
        audio->dataSize / colBytes + 1;                             
    audio->waveformColumns = columns;                              
    unsigned char* arrMax = static_cast<unsigned char*>(malloc(columns));
    memset(arrMax, 0, static_cast<size_t>(columns));
    audio->waveformMax = arrMax;                                   
    unsigned char* arrMin = static_cast<unsigned char*>(malloc(columns));
    memset(arrMin, 0, static_cast<size_t>(columns));
    audio->waveformMin = arrMin;                                   

    const unsigned short channels = audio->format.nChannels;
    const int k = channels == 1 ? (blockAlign != 1 ? 1 : 0)
                                : (blockAlign != 2 ? 3 : 2);

    if (columns > 0) {                                             
        int i = 0;                                                 
        int bytesSoFar = 0;                                        
        do {
            const float d = static_cast<float>(
                static_cast<std::uint32_t>(audio->format.nAvgBytesPerSec));
            const int t = static_cast<int>(
                (static_cast<float>(i) / 390.0f) * d);
            const unsigned skipBlocks =
                static_cast<unsigned>(t - bytesSoFar) /
                static_cast<unsigned>(blockAlign);                 
            if (static_cast<int>(skipBlocks) > 0) {                
                const int skip = blockAlign * static_cast<int>(skipBlocks);
                if (!WaveStreamFeed(audio, scratch, skip, nullptr, 0))  
                    ++audio->failureCount;                         
                bytesSoFar += skip;                                
            }
            const bool ok =
                WaveStreamFeed(audio, scratch, colBytes, nullptr, 0); 
            if (!ok)                                               
                ++audio->failureCount;
            bytesSoFar += colBytes;                                

            int maxV = 0, minV = 0;                                
            for (int off = 0; off < colBytes; off += blockAlign) { 
                int sampleL, sampleR;
                if (k == 3) {          
                    sampleL = (static_cast<signed char>(scratch[off + 1]) << 8) +
                         static_cast<signed char>(scratch[off]);
                    sampleR = (static_cast<signed char>(scratch[off + 3]) << 8) +
                         static_cast<signed char>(scratch[off + 2]);
                } else if (k == 2) {   
                    sampleL = static_cast<signed char>(scratch[off]);
                    sampleR = static_cast<signed char>(scratch[off + 1]);
                } else if (k == 1) {   
                    sampleL = (static_cast<signed char>(scratch[off + 1]) << 8) +
                         static_cast<signed char>(scratch[off]);
                    sampleR = sampleL;
                } else {               
                    sampleL = static_cast<signed char>(scratch[off]);
                    sampleR = sampleL;
                }
                if (maxV < sampleL) maxV = sampleL;                          
                if (minV > sampleL) minV = sampleL;
                if (maxV < sampleR) maxV = sampleR;
                if (minV > sampleR) minV = sampleR;
            }
            const int shift = (k == 0 || k == 2) ? 7 : 15;
            arrMax[i] = static_cast<unsigned char>(
                (maxV * 0x19 >> shift) + 0x19);
            arrMin[i] = static_cast<unsigned char>(
                (minV * 0x19 >> shift) + 0x19);
            if (!ok)                                                
                break;
            ++i;                                                    
        } while (i < audio->waveformColumns);
    }
    free(scratch);                                                  
    return true;                                                    
}

bool WaveStartPlayback(void* obj) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    FILE*& stream = audio->parseStream;
    if (stream != nullptr)                                          
        CloseDataFile(audio);

    std::memset(&audio->format, 0, sizeof audio->format);            
    _wfopen_s(&stream, audio->path, L"rb");                          
    if (stream == nullptr)                                          
        return false;
    {
        char riff[8] = {};                                          
        std::fread(riff, 4, 1, stream);                             
        std::fseek(stream, 0x10, SEEK_CUR);                         
        std::fread(riff, 2, 1, stream);   
        std::memset(&audio->format, 0, sizeof audio->format);        
        audio->format.wFormatTag = WAVE_FORMAT_PCM;                 
        audio->format.cbSize = 0x12;                                
        std::fread(&audio->format.nChannels, 2, 1, stream);
        std::fread(&audio->format.nSamplesPerSec, 4, 1, stream);
        std::fread(&audio->format.nAvgBytesPerSec, 4, 1, stream);
        std::fread(&audio->format.nBlockAlign, 2, 1, stream);
        std::fread(&audio->format.wBitsPerSample, 2, 1, stream);
    }
    if (!WaveFindDataChunk(audio, stream) ||
        audio->format.nBlockAlign == 0 ||
        audio->format.nAvgBytesPerSec == 0) {
        std::fclose(stream);
        stream = nullptr;
        return false;
    }

    const std::int32_t bufBytes = static_cast<std::int32_t>(
        audio->format.nAvgBytesPerSec * 2);                         
    audio->bufferBytes = bufBytes;                                  
    HANDLE file = CreateFileW(audio->path,
                              GENERIC_READ, 1, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);                             
    audio->fileHandle = file == INVALID_HANDLE_VALUE ? nullptr : file;
    if (file != INVALID_HANDLE_VALUE)
        SetFilePointer(file, audio->dataOffset, nullptr, FILE_BEGIN);
    std::fclose(stream);                                            
    stream = nullptr;                                               
    if (file == INVALID_HANDLE_VALUE)
        return false;

    DSBUFFERDESC desc = {};                                         
    desc.dwSize = sizeof(desc);                                     
    desc.dwFlags = 0x81E0;
    desc.dwBufferBytes = static_cast<DWORD>(bufBytes);
    desc.lpwfxFormat = &audio->format;                              
    IDirectSound*& dsound = audio->directSound;
    IDirectSoundBuffer*& buffer = audio->streamingBuffer;
    const HRESULT csbHr = dsound->CreateSoundBuffer(&desc, &buffer,
                                                     nullptr);      
    if (RINMOTIONDANCE_WAVE_TRACE())
        WaveTrace("CSB hr=%08lx ds=%p tag=%02x%02x ch=%02x%02x "
                  "rate=%08lx avg=%08lx align=%04x bits=%04x cb=%04x "
                  "bytes=%lu\n",
                  (unsigned long)csbHr, (void*)dsound,
                  reinterpret_cast<unsigned char*>(&audio->format)[0],
                  reinterpret_cast<unsigned char*>(&audio->format)[1],
                  reinterpret_cast<unsigned char*>(&audio->format)[2],
                  reinterpret_cast<unsigned char*>(&audio->format)[3],
                  (unsigned long)audio->format.nSamplesPerSec,
                  (unsigned long)audio->format.nAvgBytesPerSec,
                  (unsigned)audio->format.nBlockAlign,
                  (unsigned)audio->format.wBitsPerSample,
                  (unsigned)audio->format.cbSize,
                  (unsigned long)desc.dwBufferBytes);
    if (FAILED(csbHr)) {  
        buffer = nullptr;                                           
        if (stream != nullptr) {                                    
            std::fclose(stream);
            stream = nullptr;
        }
        return false;                                               
    }
    return true;                                                    
}

void LoadWaveFile(MMDApp* app) {
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());

    if (s.DirectSoundAvailable() == 0) {                            
        if (s.EnglishUI() != 0)                                     
            MessageBoxA(hwnd,
                        "You cannot play WAVE because failed "
                        "initialization of DirectSound!",            
                        "DirectSound error", 0);                    
        else
            MessageBoxA(hwnd, kMsgWaveNoSoundJp, kMsgWaveErrCaptionJp,
                        0);                                         
        return;
    }

    auto* audio = s.Audio();                                      
    unsigned char* ctx = reinterpret_cast<unsigned char*>(audio);
    WaveCtxReset(ctx);                                              
    const bool ok = WaveLoadFile(ctx, s.WavePath(),
                                 app->PathWorkspace());             

    if (!ok) {                                                      
        s.WaveEnabled() = 0;                                        
        swprintf_s(s.WavePath(), 0x100, L"");                       
        HDC hdc = s.TimelineDC();
        HPEN white = CreatePen(PS_SOLID, 1, 0xFFFFFF);
        HBRUSH brush = CreateSolidBrush(0xFFFFFF);
        HGDIOBJ oldPen = SelectObject(hdc, white);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        Rectangle(hdc, 0, 0, 0xEF, 0x31);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(white);
        DeleteObject(brush);
        HPEN black = CreatePen(PS_SOLID, 1, 0);
        HGDIOBJ oldPen2 = SelectObject(hdc, black);
        MoveToEx(hdc, 0, 0x19, nullptr);
        LineTo(hdc, 0xEF, 0x19);
        SelectObject(hdc, oldPen2);
        DeleteObject(black);
        RECT rect{6, 0x64, 0xF7, 0x97};                             
        InvalidateRect(hwnd, &rect, FALSE);
        return;
    }

    wcscpy_s(s.WavePath(), 0x100, audio->path);                    
    WaveStartPlayback(ctx);                                         
    s.WaveEnabled() = 1;                                            
    TimelineDrawTicks(s.TimelineStartFrame(), s.SidebarWidth());
    RECT rect{6, 0x5F,
              s.SidebarWidth() - 3,
              0x92};                                                
    InvalidateRect(hwnd, &rect, FALSE);
    PanelPaint(app);                                                
    SetFrameNormalized(s.FrameNormalization());                     
}

}  
