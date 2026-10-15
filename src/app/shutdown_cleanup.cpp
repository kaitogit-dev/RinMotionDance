#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <objbase.h>  
#include <dshow.h>    
#include <vfw.h>      

#include <cstdint>
#include <cstdlib>
#include <new>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/mme_bridge.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/dshow_recorder.hpp"

namespace rinmotiondance {

void DisposeAccessory(void* accessory);

namespace {

inline void FreeTimelineSelectionRecords(MMDApp& app,
                                         TimelineSelectionBand band) {
    TimelineSelectionRecord*& records = app.TimelineSelectionRecords(band);
    if (records != nullptr) {
        std::free(records);
        records = nullptr;
    }
}

template <typename ComSlot>
inline void ReleaseSlot(ComSlot& slot) {
    if (slot != nullptr) {
        reinterpret_cast<IUnknown*>(slot)->Release();  
        slot = nullptr;
    }
}


template <typename ComSlot>
void ReleaseRecorderCom(ComSlot& slot) {
    if (slot != nullptr) {
        reinterpret_cast<IUnknown*>(slot)->Release();
        slot = nullptr;
    }
}


void DisposeAudioContext(WaveAudioContext* audio) {
    CloseDataFile(audio);                                       
    if (audio->streamingBuffer != nullptr)
        audio->streamingBuffer->Release();                      
    if (audio->directSound != nullptr)
        audio->directSound->Release();                          
    std::free(audio->waveformMax);                              
    std::free(audio->waveformMin);                              
}


constexpr std::uint32_t kNvapiStereoDestroyHandleId = 0x3A153134u;

int NvapiStereoDestroyHandle(void* stereoHandle) {
    using QueryInterface = void*(__cdecl*)(std::uint32_t);
    using DestroyHandle = int(__cdecl*)(void*);
    static DestroyHandle destroy = nullptr;   
    static bool resolved = false;             
    if (!resolved) {
        resolved = true;
        HMODULE module = LoadLibraryA(sizeof(void*) == 8 ? "nvapi64.dll"
                                                         : "nvapi.dll");
        if (module != nullptr) {
            auto query = reinterpret_cast<QueryInterface>(
                GetProcAddress(module, "nvapi_QueryInterface"));
            if (query != nullptr)
                destroy = reinterpret_cast<DestroyHandle>(
                    query(kNvapiStereoDestroyHandleId));
        }
    }
    if (destroy == nullptr)
        return -3;                                              
    return destroy(stereoHandle);                               
}

void DisposeRenderSubsystem(D3DRenderer* sub) {
    ReleaseSlot(sub->effect);             
    ReleaseSlot(sub->shadowSurface);      
    ReleaseSlot(sub->hdrTexture);         
    ReleaseSlot(sub->spriteTexture);      
    ReleaseSlot(sub->shadowDepthSurface); 
    ReleaseSlot(sub->depthStencilSurface);
    ReleaseSlot(sub->backbufferSurface);  
    ReleaseSlot(sub->captureSurface);     
    ReleaseSlot(sub->lineVertexBuffer);            
    ReleaseSlot(sub->device);             
    ReleaseSlot(sub->d3d9);               

    for (int k = 0; k < 10000; ++k) {    
        ResourcePoolEntry& entry = sub->resourcePool[k];
        if (entry.heapBuffer != nullptr) {   
            std::free(entry.heapBuffer);
            entry.heapBuffer = nullptr;
        }
        if (entry.comObject != nullptr) {    
            entry.comObject->Release();      
            entry.comObject = nullptr;
        }
    }

    if (sub->stereoEnabled != 0)         
        NvapiStereoDestroyHandle(sub->stereoHandle);  
}

}  

void TeardownDShowGraph(DShowRecorder* rec) {
    if (IPushSource* push = static_cast<IPushSource*>(rec->framePush))  
        push->BeginStreaming();

    IMediaEvent* mediaEvent = rec->mediaEvent;                
    if (mediaEvent != nullptr) {
        long code = 0;
        LONG_PTR param1 = 0;
        LONG_PTR param2 = 0;
        char done = 0;
        do {
            mediaEvent->GetEvent(&code, &param1, &param2, 0);          
            mediaEvent->FreeEventParams(code, param1, param2);         
            if (code > 0 && code <= 3)
                done = 1;                                      
            MSG msg;                                           
            while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        } while (!done);
        rec->mediaControl->Stop();
    }

    ReleaseRecorderCom(rec->mediaEvent);
    ReleaseRecorderCom(rec->mediaControl);
    ReleaseRecorderCom(rec->audioGrabber);
    ReleaseRecorderCom(rec->audioGrabberInput);
    ReleaseRecorderCom(rec->audioGrabberOutput);
    ReleaseRecorderCom(rec->waveOutput);
    ReleaseRecorderCom(rec->waveSource);
    ReleaseRecorderCom(rec->mediaControl);  
    ReleaseRecorderCom(rec->muxAudioInput);
    ReleaseRecorderCom(rec->reservedInput1);
    ReleaseRecorderCom(rec->reservedInput0);
    ReleaseRecorderCom(rec->muxVideoInput);
    ReleaseRecorderCom(rec->compressorOutput);
    ReleaseRecorderCom(rec->compressorInput);
    ReleaseRecorderCom(rec->grabberOutput);
    ReleaseRecorderCom(rec->grabberInput);
    ReleaseRecorderCom(rec->sourceOutput);
    ReleaseRecorderCom(rec->fileWriter);
    ReleaseRecorderCom(rec->aviMux);
    ReleaseRecorderCom(rec->videoGrabber);
    ReleaseRecorderCom(rec->framePush);
    ReleaseRecorderCom(rec->videoSource);
    ReleaseRecorderCom(rec->graph);
    std::free(rec->compressorState);        
    rec->compressorState = nullptr;
    ReleaseRecorderCom(rec->compressorDialogs);
    ReleaseRecorderCom(rec->compressor);
}

void TeardownDShowGraphCoUninit(DShowRecorder* rec) {
    TeardownDShowGraph(rec);                                    
    CoUninitialize();                                           
}

void ShutdownCleanup(MMDApp* app) {
    auto& s = *app;

    delete[] s.state.selectNavRecords;
    s.state.selectNavRecords = nullptr;

    if (s.state.depthDeviceEnabled != 0) {        
        reinterpret_cast<void(*)()>(
            s.state.oniExportSlot1)();                                   
        s.state.depthDeviceEnabled = 0;
    }
    if (HMODULE mod = s.state.oniModule) {                          
        FreeLibrary(mod);                                       
        s.state.oniModule = nullptr;
    }

    if (s.RecordingWindow() != nullptr)                         
        TeardownDShowGraph(s.Recorder());                        
    if (s.FloatingWindow() != nullptr)                           
        SaveFlagSubsystem(app);                                 

    if (s.AviFrameReader() != nullptr)                           
        AVIStreamGetFrameClose(                                 
            static_cast<PGETFRAME>(s.AviFrameReader()));
    if (s.AviStream() != nullptr)                                
        AVIStreamRelease(                                       
            static_cast<PAVISTREAM>(s.AviStream()));
    if (s.AviFile() != nullptr)                                  
        AVIFileRelease(                                         
            static_cast<PAVIFILE>(s.AviFile()));
    AVIFileExit();                                              
    DrawDibClose(static_cast<HDRAWDIB>(s.AviDrawDib()));        

    ::operator delete(s.CaptureReadbackPixels());               
    s.CaptureReadbackPixels() = nullptr;
    ReleaseSlot(s.LeftViewportVertices());                     
    ReleaseSlot(s.RightViewportVertices());                    
    ReleaseSlot(s.GroundPlaneVertices());                       
    for (int i = 0; i < 11; ++i)                                
        ReleaseSlot(s.ToonTexture(i));                          
    ReleaseSlot(s.ProjectedShadowRestoreTexture());             

    delete s.RecordingCompletionFlag();                         
    s.RecordingCompletionFlag() = nullptr;
    if (s.state.rigidScratchArray != nullptr) {                   
        std::free(s.state.rigidScratchArray);
        s.state.rigidScratchArray = nullptr;
    }

    ReleaseSlot(s.GroundGridIndices());                         
    ReleaseSlot(s.GroundGridVertices());                        
    ReleaseSlot(s.OverlayVertices());                           
    ReleaseSlot(s.SpriteOverlayVertices());                     
    ReleaseSlot(s.SceneFontTexture());                          
    ReleaseSlot(s.OverlayTexture());                            
    ReleaseSlot(s.CaptureRenderTarget());                       
    ReleaseSlot(s.CaptureSystemSurface());                      
    ReleaseSlot(s.CaptureTexture());                            
    ReleaseSlot(s.AviBackgroundSurface());                      
    ReleaseSlot(s.AviOverlayVertices());                        
    ReleaseSlot(s.PictureOverlayVertices());                    
    ReleaseSlot(s.PictureBackgroundTexture());                  
    if (s.AviBackgroundTexture() != nullptr) {                   
        s.AviBackgroundTexture()->Release();
        s.AviBackgroundTexture() = nullptr;
    }

    ::operator delete(s.CameraKeys());                                  
    s.CameraKeys() = nullptr;
    ::operator delete(s.LightKeys());                                   
    s.LightKeys() = nullptr;
    ::operator delete(s.ShadowKeys());                                  
    s.ShadowKeys() = nullptr;
    ::operator delete(s.GravityKeys());                                 
    s.GravityKeys() = nullptr;

    for (int i = 0; i < 255; ++i) {                             
        mdl::AccessoryRecord*& accessory = s.AccessorySlot(i);  
        if (accessory != nullptr) {
            DisposeAccessory(accessory);                        
            ::operator delete(accessory);                               
            accessory = nullptr;
        }
        mdl::AccessoryKey*& track = s.AccessoryKeys(i);
        if (track != nullptr) {
            ::operator delete(track);                                   
            track = nullptr;
        }
    }

    for (int i = 0; i < kModelSlotCount; ++i) {                
        unsigned char*& model = s.ModelSlot(i);
        if (model != nullptr) {                                 
            ModelDispose(model);                                
            ::operator delete(model);                                   
            model = nullptr;
        }
    }

    mdl::DisplayClipboardRecord*& displayRecords = s.DisplayClipboard();
    if (displayRecords != nullptr) {
        std::int32_t nRec =
            static_cast<std::int32_t>(s.ClipboardCounts().displays);
        if (nRec > 0) {                                         
            for (std::int32_t k = 0; k < nRec; ++k) {           
                std::free(displayRecords[k].ikStates);          
                displayRecords[k].ikStates = nullptr;
                std::free(displayRecords[k].selectorStates);    
                displayRecords[k].selectorStates = nullptr;
            }
        }
    }
    std::free(displayRecords);                                  
    displayRecords = nullptr;
    std::free(s.LightClipboard());                              
    s.LightClipboard() = nullptr;
    std::free(s.CameraClipboard());                             
    s.CameraClipboard() = nullptr;
    std::free(s.MorphClipboard());                              
    s.MorphClipboard() = nullptr;
    std::free(s.ShadowClipboard());                             
    s.ShadowClipboard() = nullptr;
    std::free(s.GravityClipboard());                            
    s.GravityClipboard() = nullptr;
    std::free(s.AccessoryClipboard());                          
    s.AccessoryClipboard() = nullptr;
    std::free(s.BoneClipboard());                               
    s.BoneClipboard() = nullptr;
    std::free(s.BoneCopyRecords());                               
    s.BoneCopyRecords() = nullptr;

    FreeTimelineSelectionRecords(s, TimelineSelectionBand::Accessory);   
    FreeTimelineSelectionRecords(s, TimelineSelectionBand::ModelIk);     
    FreeTimelineSelectionRecords(s, TimelineSelectionBand::Camera);      
    FreeTimelineSelectionRecords(s, TimelineSelectionBand::Light);       
    FreeTimelineSelectionRecords(s, TimelineSelectionBand::ModelBone);   
    FreeTimelineSelectionRecords(s, TimelineSelectionBand::SelfShadow);  
    FreeTimelineSelectionRecords(s, TimelineSelectionBand::ModelMorph);  

    DeleteDC(s.TimelineDC());                                   
    DeleteDC(s.PanelDC());                                      
    DeleteDC(s.CurveDC());                                      

    if (PhysicsScene* phys = s.Physics()) {                    
        DisposePhysicsWorld(phys);                              
        ::operator delete(phys);                                        
        s.Physics() = nullptr;
    }
    if (void* acc = s.AxisMeshObject()) {                              
        DisposeAccessory(acc);                                  
        ::operator delete(acc);                                         
        s.AxisMeshObject() = nullptr;
    }
    if (DShowRecorder* rec = s.Recorder()) {                    
        TeardownDShowGraphCoUninit(rec);                         
        ::operator delete(rec);                                         
        s.Recorder() = nullptr;
    }
    if (WaveAudioContext* audio = s.Audio()) {                  
        DisposeAudioContext(audio);                             
        ::operator delete(audio);                                       
        s.Audio() = nullptr;
    }
    if (D3DRenderer* render = s.Renderer()) {                   
        mme::OnDeviceDestroyed(app);
        DisposeRenderSubsystem(render);                         
        ::operator delete(render);                                      
        s.Renderer() = nullptr;
    }

}

}  
