#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>   
#include <dsound.h>

#include <cstdint>
#include <process.h>   

#include "rinmotiondance/physics_scene.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/wave_audio_context.hpp"

namespace rinmotiondance {
namespace {

void __cdecl WaveFeedThread(void* ctx) {
    auto* audio = static_cast<WaveAudioContext*>(ctx);
    IDirectSoundBuffer* buffer = audio->streamingBuffer;
    const DWORD avgBytes = audio->format.nAvgBytesPerSec;
    const std::int32_t bufBytes = audio->bufferBytes;
    auto lock = [&](DWORD off, void** p1, DWORD* l1, void** p2,
                    DWORD* l2) -> HRESULT {
        HRESULT hr = buffer->Lock(off, avgBytes, p1, l1, p2, l2, 0);
        if (hr == DSERR_BUFFERLOST) {                          
            buffer->Restore();                                 
            hr = buffer->Lock(off, avgBytes, p1, l1, p2, l2, 0);
        }
        return hr;
    };
    auto exitThread = [&]() {
        buffer->Stop();                                        
        buffer->Release();                                     
        audio->streamingBuffer = nullptr;                       
        audio->WriteStopFlag(2);                                              
        _endthread();
    };

    void* p1 = nullptr;
    void* p2 = nullptr;
    DWORD l1 = 0;
    DWORD l2 = 0;
    if (lock(0, &p1, &l1, &p2, &l2) >= 0) {                    
        if (!WaveStreamFeed(ctx, p1, static_cast<int>(l1), p2,
                            static_cast<int>(l2)))              
            ++audio->failureCount;                              
        buffer->Unlock(p1, l1, p2, l2);                        
        buffer->Play(0, 0, DSBPLAY_LOOPING);                   
        bool wrapped = false;                                  
        if (audio->ReadStopFlag() != 1) {
            DWORD writeOff = l1;  
            for (;;) {
                if (lock(writeOff, &p1, &l1, &p2, &l2) < 0)    
                    break;
                if (l1 == 0 && l2 == 0) {                      
                    buffer->Unlock(p1, 0, p2, 0);              
                    break;
                }
                if (!WaveStreamFeed(ctx, p1, static_cast<int>(l1), p2,
                                    static_cast<int>(l2)))      
                    ++audio->failureCount;                      
                buffer->Unlock(p1, l1, p2, l2);                
                if (audio->failureCount > 2) {                  
                    buffer->Stop();                            
                    break;
                }
                DWORD last = 0;
                do {
                    DWORD cur = 0;
                    buffer->GetCurrentPosition(&cur, nullptr); 
                    if (wrapped) {                             
                        if (cur <= last)
                            wrapped = false;
                    } else if (cur > writeOff) {
                        break;                                 
                    }
                    last = cur;
                    for (int i = 0; i < 10; ++i) {             
                        Sleep(10);
                        if (audio->ReadStopFlag() == 1)
                            break;
                    }
                } while (audio->ReadStopFlag() != 1);
                if (audio->ReadStopFlag() != 1) {
                    if (l2 != 0) {
                        writeOff = l2;
                        wrapped = true;
                    } else {
                        writeOff += l1;
                        if (writeOff == static_cast<DWORD>(bufBytes)) {
                            writeOff = 0;
                            wrapped = true;
                        } else {
                            wrapped = false;
                        }
                    }
                    if (audio->ReadStopFlag() != 1)
                        continue;
                }
                break;
            }
        }
    }
    exitThread();
}

}  

void InitAudioContext(void* sub) {
    auto* audio = static_cast<WaveAudioContext*>(sub);
    audio->path[0] = L'\0';                                    
    audio->englishUI = 0;                                      
    audio->waveformMax = nullptr;                              
    audio->waveformMin = nullptr;                              
    audio->volume = 0;                                         
}

void PhysicsSceneInit(PhysicsScene* scene) {
    scene->gizmoSphereVB = nullptr;
    scene->gizmoSphereIB = nullptr;
    scene->gizmoCubeVB = nullptr;
    scene->gizmoCubeIB = nullptr;
    scene->gizmoSphere33VB = nullptr;
    scene->gizmoSphere33IB = nullptr;
    scene->gizmoArrowVB = nullptr;
    scene->gizmoIdentityIB = nullptr;
    scene->gizmoBoxSelVB = nullptr;
    scene->gizmoBoxSelIB = nullptr;
    scene->collisionConfig = nullptr;
    scene->dispatcher = nullptr;
    scene->broadphase = nullptr;
    scene->solver = nullptr;
    scene->world = nullptr;
    scene->constraintId = 0;
}

void WaveSeekAndFeed(void* obj, double seconds) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    if (audio->streamingBuffer == nullptr)                      
        return;                                                 
    const std::int32_t avgBytes = audio->format.nAvgBytesPerSec; 
    const std::int32_t blkAlign = audio->format.nBlockAlign;     
    audio->WriteStopFlag(0);                                        
    audio->failureCount = 0;                                    
    double avg = static_cast<double>(avgBytes);                 
    if (avgBytes < 0)                                           
        avg += 4294967296.0;                                    
    const double t = static_cast<double>(blkAlign) *
                     ((avg * seconds) / static_cast<double>(blkAlign));
    const std::int32_t whole = static_cast<std::int32_t>(t);    
    const std::int32_t rem = whole % blkAlign;                  
    const std::int32_t dataOff = audio->dataOffset;              
    HANDLE file = audio->fileHandle;                             
    SetFilePointer(file, dataOff - rem + whole, nullptr, FILE_BEGIN);
    audio->feedThread = _beginthread(WaveFeedThread, 0, obj);   
}

void WaveRestartAt(void* obj, double seconds) {  
    auto* audio = static_cast<WaveAudioContext*>(obj);
    HWND hwnd = audio->mainWindow;                              
    KillTimer(hwnd, 0x64);                                      
    CloseDataFile(obj);                                         
    WaveStartPlayback(obj);                                     
    IDirectSoundBuffer* buffer = audio->streamingBuffer;
    buffer->SetFrequency(static_cast<DWORD>(audio->volume));      
    WaveSeekAndFeed(obj, seconds);                              
    SetTimer(hwnd, 0x64, 0x21, nullptr);                        
}

}  
