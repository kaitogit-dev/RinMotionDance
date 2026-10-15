#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <objbase.h>
#include <dshow.h>   
#include <ocidl.h>   

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/dshow_recorder.hpp"

namespace rinmotiondance {

void TeardownDShowGraph(DShowRecorder* rec);

namespace {

const GUID kMmdxShowClsid = {
    0x2f1713b8, 0xdd1f, 0x4186, {0x93, 0xbe, 0xfd, 0xa5, 0x0b, 0xf6, 0x87, 0xc7}};
const GUID kIidPushSource = {
    0xecfab031, 0x72ba, 0x4120, {0xb9, 0xf7, 0x8a, 0x3d, 0x5f, 0xd3, 0x8d, 0xec}};
const GUID kSubtypeArgb32 = {
    0x773c9ac0, 0x3274, 0x11d0, {0xb7, 0x24, 0x00, 0xaa, 0x00, 0x6c, 0x1a, 0x01}};
const GUID kClsidSampleGrabber = {
    0xc1f400a0, 0x3f08, 0x11d3, {0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37}};
const IID kIidISampleGrabber = {
    0x6b652fff, 0x11fe, 0x4fce, {0x92, 0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f}};

template <typename T>
void ReleaseCom(T*& object) {
    if (object != nullptr) {
        object->Release();
        object = nullptr;
    }
}

struct ISampleGrabberCB;
MIDL_INTERFACE("6B652FFF-11FE-4FCE-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL bOneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType(
        const AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(
        AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL bBufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(LONG* pBufferSize,
                                                       LONG* pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(
        IMediaSample** ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback(
        ISampleGrabberCB* pCallback, LONG WhichMethodToCallback) = 0;
};

#include "dshow_record_graph_jp.inc"

}  

HRESULT FindPinDShow(IBaseFilter* filter, int dir, int skip, IPin** outPin) {
    if (filter == nullptr)
        return 0x80004003;                                       
    *outPin = nullptr;
    IEnumPins* enumPins = nullptr;
    HRESULT hr = filter->EnumPins(&enumPins);
    if (hr < 0)
        return hr;
    HRESULT result = 0x80004005;                                 
    IPin* pin = nullptr;
    ULONG fetched = 0;
    if (enumPins->Next(1, &pin, &fetched) == 0) {
        for (;;) {
            PIN_DIRECTION d = static_cast<PIN_DIRECTION>(3);
            pin->QueryDirection(&d);                            
            if (d == dir) {
                if (skip == 0) {
                    *outPin = pin;                               
                    result = 0;
                    break;
                }
                --skip;
            }
            pin->Release();                                      
            if (enumPins->Next(1, &pin, &fetched) != 0)
                break;
        }
    }
    enumPins->Release();
    return result;
}

IPin* FindInputPin(DShowRecorder*  , IBaseFilter* filter, int skip) {
    IPin* pin = nullptr;
    FindPinDShow(filter, 0  , skip, &pin);
    return pin;
}

IPin* FindOutputPin(DShowRecorder*  , IBaseFilter* filter, int skip) {
    IPin* pin = nullptr;
    FindPinDShow(filter, 1  , skip, &pin);
    return pin;
}

void BindSelectedCompressor(DShowRecorder* rec) {
    ReleaseCom(rec->compressor);
    ReleaseCom(rec->compressorDialogs);
    int idx = rec->selectedCodec;

    ICreateDevEnum* devEnum = nullptr;
    if (CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,
                         CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
                         reinterpret_cast<void**>(&devEnum)) < 0 ||
        devEnum == nullptr)
        return;
    IEnumMoniker* enumMon = nullptr;
    if (devEnum->CreateClassEnumerator(CLSID_VideoCompressorCategory,
                                       &enumMon, 0) < 0 ||
        enumMon == nullptr) {
        devEnum->Release();
        return;
    }
    enumMon->Reset();                                            
    IMoniker* mon = nullptr;
    ULONG fetched = 0;
    enumMon->Next(1, &mon, &fetched);
    while (mon != nullptr) {                                     
        if (idx == 0)
            break;                                               
        --idx;
        mon->Release();
        mon = nullptr;
        enumMon->Next(1, &mon, &fetched);
    }
    if (mon != nullptr) {
        mon->BindToObject(nullptr, nullptr, IID_IBaseFilter,
                          reinterpret_cast<void**>(&rec->compressor));
        if (rec->compressor != nullptr)
            rec->compressor->QueryInterface(
                IID_IAMVfwCompressDialogs,
                reinterpret_cast<void**>(&rec->compressorDialogs));
        return;  
    }
    enumMon->Release();
    devEnum->Release();
}

static const wchar_t kJpNoCompression[] = {0x672A, 0x5727, 0x7E2E, 0x0000};
int FillCodecCombo(DShowRecorder*  , HWND hCombo, char english) {
    SendMessageA(hCombo, CB_RESETCONTENT, 0, 0);
    ICreateDevEnum* devEnum = nullptr;
    if (CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,
                         CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
                         reinterpret_cast<void**>(&devEnum)) < 0) {
        MessageBoxA(hCombo, "Failed to create system enumerator",
                    "DirectShow", 0);
        return 0;
    }
    IEnumMoniker* enumMon = nullptr;
    if (devEnum->CreateClassEnumerator(CLSID_VideoCompressorCategory,
                                       &enumMon, 0) < 0) {
        MessageBoxA(hCombo, "Failed to create class enumerator",
                    "DirectShow", 0);
        devEnum->Release();
        return 0;
    }
    enumMon->Reset();                                            
    IMoniker* mon = nullptr;
    ULONG fetched = 0;
    enumMon->Next(1, &mon, &fetched);
    while (mon != nullptr) {
        IPropertyBag* bag = nullptr;
        if (mon->BindToStorage(nullptr, nullptr, IID_IPropertyBag,
                               reinterpret_cast<void**>(&bag)) >= 0) {
            VARIANT var;
            var.vt = VT_BSTR;
            var.bstrVal = nullptr;                               
            if (bag->Read(L"FriendlyName", &var, nullptr) == 0) {
                const int len = WideCharToMultiByte(
                    3  , 0, var.bstrVal, -1, nullptr, 0,
                    nullptr, nullptr);
                char* name = static_cast<char*>(std::malloc(len));
                WideCharToMultiByte(
                    3, 0, var.bstrVal, lstrlenW(var.bstrVal) + 1, name, len,
                    nullptr, nullptr);
                SendMessageA(hCombo, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(name));
                std::free(name);
                SysFreeString(var.bstrVal);
            }
            if (bag != nullptr)
                bag->Release();
        }
        mon->Release();
        mon = nullptr;
        enumMon->Next(1, &mon, &fetched);
    }
    LRESULT index;
    if (english)
        index = SendMessageA(hCombo, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>("AVI Raw"));
    else
        index = SendMessageW(hCombo, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(kJpNoCompression));
    SendMessageA(hCombo, CB_SETCURSEL, index, 0);
    enumMon->Release();
    devEnum->Release();
    return static_cast<int>(index);
}

void RebindCodecOnComboChange(DShowRecorder* rec, int sel, HWND hButton,
                              char english) {
    rec->selectedCodec = sel;
    rec->compressorStateSize = 0;
    ReleaseCom(rec->compressor);
    ReleaseCom(rec->compressorInput);
    ReleaseCom(rec->compressorDialogs);
    ReleaseCom(rec->sourceOutput);
    ReleaseCom(rec->videoSource);
    ReleaseCom(rec->graph);

    CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                     IID_IGraphBuilder, reinterpret_cast<void**>(&rec->graph));
    CoCreateInstance(kMmdxShowClsid, nullptr, CLSCTX_INPROC_SERVER,
                     IID_IBaseFilter, reinterpret_cast<void**>(&rec->videoSource));
    IGraphBuilder* graph = rec->graph;
    IBaseFilter* source = rec->videoSource;
    if (graph != nullptr && source != nullptr)
        graph->AddFilter(source, L"File Source");
    rec->sourceOutput = FindOutputPin(rec, source, 0);

    IPushSource* push = nullptr;
    if (source != nullptr)
        source->QueryInterface(kIidPushSource,
                               reinterpret_cast<void**>(&push));   
    if (push != nullptr) {
        BITMAPINFOHEADER cfg = {};
        cfg.biSize = sizeof(cfg);        
        cfg.biWidth = 10;
        cfg.biHeight = 10;
        cfg.biPlanes = 1;                
        cfg.biBitCount = 32;             
        cfg.biCompression = BI_RGB;      
        cfg.biSizeImage = 400;           
        push->SetBitmapInfo(&cfg, sizeof(cfg), 30.0f);             
        push->Release();
    }

    ICreateDevEnum* devEnum = nullptr;
    CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                     IID_ICreateDevEnum, reinterpret_cast<void**>(&devEnum));
    IEnumMoniker* enumMon = nullptr;
    if (devEnum != nullptr)
        devEnum->CreateClassEnumerator(CLSID_VideoCompressorCategory,
                                       &enumMon, 0);
    IMoniker* mon = nullptr;
    ULONG fetched = 0;
    if (enumMon != nullptr) {
        enumMon->Reset();
        enumMon->Next(1, &mon, &fetched);
    }
    while (mon != nullptr && sel != 0) {
        --sel;
        mon->Release();
        mon = nullptr;
        enumMon->Next(1, &mon, &fetched);
    }
    if (mon != nullptr) {
        if (mon->BindToObject(nullptr, nullptr, IID_IBaseFilter,
                              reinterpret_cast<void**>(&rec->compressor)) >= 0) {
            IBaseFilter* comp = rec->compressor;
            IAMVfwCompressDialogs* vfw = nullptr;
            if (comp != nullptr &&
                comp->QueryInterface(IID_IAMVfwCompressDialogs,
                                     reinterpret_cast<void**>(&vfw)) >= 0 &&
                vfw != nullptr) {
                rec->compressorDialogs = vfw;
                if (vfw->ShowDialog(4, hButton) == 0) {
                    EnableWindow(hButton, TRUE);
                    if (graph != nullptr && comp != nullptr)
                        graph->AddFilter(comp, L"Compressor");
                    rec->compressorInput = FindInputPin(rec, comp, 0);
                    if (graph != nullptr)
                        graph->Connect(
                            rec->sourceOutput, rec->compressorInput);
                    if (enumMon != nullptr)
                        enumMon->Release();
                    if (devEnum != nullptr)
                        devEnum->Release();
                    return;
                }
                vfw->Release();
                rec->compressorDialogs = nullptr;
            }
        } else if (english) {
            MessageBoxA(hButton, "This codec cannot use in DirectShow.",
                        "DirectShow", 0);
        } else {
            MessageBoxA(hButton, JP_CANNOT_DS, "DirectShow", 0);
        }
    }
    EnableWindow(hButton, FALSE);                                
    if (enumMon != nullptr)
        enumMon->Release();
    if (devEnum != nullptr)
        devEnum->Release();
}

void ShowCodecConfigDialog(DShowRecorder* rec, HWND hDlg) {
    if (rec->compressorState != nullptr) {
        std::free(rec->compressorState);
        rec->compressorState = nullptr;
    }
    IAMVfwCompressDialogs* vfw = rec->compressorDialogs;
    if (vfw == nullptr)
        return;  
    vfw->ShowDialog(1, hDlg);                                    
    long size = vfw->SendDriverMessage(0x5000, 0, 0);
    rec->compressorStateSize = static_cast<std::int32_t>(size);
    if (size > 0) {
        void* blob = std::malloc(static_cast<std::size_t>(size));
        rec->compressorState = blob;
        std::memset(blob, 0, static_cast<std::size_t>(size));
        vfw->SendDriverMessage(
            0x5000,
            static_cast<long>(reinterpret_cast<UINT_PTR>(blob)),
            size);
    }
}

bool BuildRecordingGraph(DShowRecorder* rec, HWND hwnd, unsigned char english,
                         void* outPath,
               void* config, float fps,
               const wchar_t* wavPath, float seconds) {
    ReleaseCom(rec->compressorInput);
    ReleaseCom(rec->sourceOutput);
    ReleaseCom(rec->videoSource);
    ReleaseCom(rec->graph);
    if (rec->compressor != nullptr)
        BindSelectedCompressor(rec);                             

    CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                     IID_IGraphBuilder, reinterpret_cast<void**>(&rec->graph));
    IGraphBuilder* graph = rec->graph;
    if (graph == nullptr) {
        MessageBoxA(hwnd, english ? "failed create a filter graph"
                                  : JP_FILTERGRAPH, "DirectShow", 0);
        return false;
    }
    CoCreateInstance(kMmdxShowClsid, nullptr, CLSCTX_INPROC_SERVER,
                     IID_IBaseFilter, reinterpret_cast<void**>(&rec->videoSource));
    IBaseFilter* source = rec->videoSource;
    if (source == nullptr) {
        MessageBoxA(hwnd, english
                    ? "cannot read MMDxShow->dll\n\nthere is not "
                      "'MMDxShow->dll' in 'Data' folder.\nplease download "
                      "the newest ver. MikuMikuDance."
                    : JP_CANNOT_READ_DLL, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }
    if (graph->AddFilter(source, L"File Source") < 0) {
        MessageBoxA(hwnd, english
                    ? "Could not add DIB Sequential source filter to the "
                      "graph."
                    : JP_DIB_ADD, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }

    IPushSource* push = nullptr;
    if (source->QueryInterface(kIidPushSource,
                               reinterpret_cast<void**>(&push)) < 0) {
        MessageBoxA(hwnd, english
                    ? "Could not get DIB Sequential source filter "
                      "interface."
                    : JP_DIB_IF, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }
    rec->framePush = push;
    float ver = 0.0f;
    HRESULT hr = push->GetRate(&ver);                            
    if (ver < 1.02f || hr < 0) {
        MessageBoxA(hwnd, english
                    ? "MMDxShow->dll is too old.\n\nPlease get newest "
                      "MikuMikuDance and put MMDxShow->dll to "
                      "'Data'folder."
                    : JP_DLL_OLD, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }
    push->SetBitmapInfo(config, 40, fps);                        

    if (CoCreateInstance(kClsidSampleGrabber, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IBaseFilter, reinterpret_cast<void**>(&rec->videoGrabber)) < 0) {
        MessageBoxA(hwnd, english ? "Failed create a SampleGrabber filter"
                                  : JP_SG_FILTER, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }
    IBaseFilter* grabber = rec->videoGrabber;
    ISampleGrabber* sg = nullptr;
    if (grabber->QueryInterface(kIidISampleGrabber,
                                reinterpret_cast<void**>(&sg)) < 0) {
        MessageBoxA(hwnd, english ? "Failed create a SampleGrabber Interface"
                                  : JP_SG_IF, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }
    {
        AM_MEDIA_TYPE mt = {};
        mt.bFixedSizeSamples = TRUE;
        mt.lSampleSize = 1;
        mt.majortype = MEDIATYPE_Video;                          
        mt.subtype = kSubtypeArgb32;                             
        if (sg->SetMediaType(&mt) < 0) {                         
            MessageBoxA(hwnd, english ? "Failed insert a SampleGrabber"
                                      : JP_SG_INSERT, "DirectShow", 0);
            TeardownDShowGraph(rec);
            sg->Release();
            return false;
        }
    }
    if (graph->AddFilter(grabber, L"Sample Grabber") < 0) {
        MessageBoxA(hwnd, english
                    ? "Could not add Sample Grabber filter to the graph."
                    : JP_SG_ADD, "DirectShow", 0);
        TeardownDShowGraph(rec);
        if (sg != nullptr)
            sg->Release();
        return false;
    }
    if (sg != nullptr)
        sg->Release();

    ICaptureGraphBuilder2* builder = nullptr;
    if (CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr,
                         CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
                         reinterpret_cast<void**>(&builder)) < 0 ||
        builder == nullptr) {
        MessageBoxA(hwnd, english ? "failed create a CaptureGraphBuilder"
                                  : JP_CGB, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }
    if (builder->SetFiltergraph(graph) < 0) {
        MessageBoxA(hwnd, english ? "failed set a filter graph"
                                  : JP_SET_GRAPH, "DirectShow", 0);
        builder->Release();
        TeardownDShowGraph(rec);
        return false;
    }
    if (builder->SetOutputFileName(
            &MEDIASUBTYPE_Avi, static_cast<LPCOLESTR>(outPath),
            &rec->aviMux, &rec->fileWriter) < 0) {
        MessageBoxA(hwnd, english ? "Cannot set up MUX and File Writer"
                                  : JP_MUX, "DirectShow", 0);
        builder->Release();
        TeardownDShowGraph(rec);
        return false;
    }
    builder->Release();
    IBaseFilter* mux = rec->aviMux;
    IBaseFilter* comp = rec->compressor;

    if (comp != nullptr && graph->AddFilter(comp, L"Compressor") < 0) {
        MessageBoxA(hwnd, english
                    ? "Could not add selected compressor to the graph."
                    : JP_DRIVER, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }
    rec->sourceOutput = FindOutputPin(rec, source, 0);
    rec->grabberInput = FindInputPin(rec, grabber, 0);
    rec->grabberOutput = FindOutputPin(rec, grabber, 0);
    if (comp != nullptr) {
        rec->compressorInput = FindInputPin(rec, comp, 0);
        rec->compressorOutput = FindOutputPin(rec, comp, 0);
    }
    rec->muxVideoInput = FindInputPin(rec, mux, 0);
    IPin* srcOut = rec->sourceOutput;
    IPin* grabIn = rec->grabberInput;
    IPin* grabOut = rec->grabberOutput;
    IPin* compIn = rec->compressorInput;
    IPin* compOut = rec->compressorOutput;
    IPin* muxIn = rec->muxVideoInput;

    if (graph->Connect(srcOut, grabIn) < 0) {
        MessageBoxA(hwnd, english
                    ? "Could not connect SourcePin to pVideoIn."
                    : JP_SRC_PIN, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }
    if (comp != nullptr) {
        if (graph->Connect(grabOut, compIn) < 0) {
            MessageBoxA(hwnd, english
                        ? "Could not connect pVideoOut to CompressorIn."
                        : JP_COMP_IN, "DirectShow", 0);
            TeardownDShowGraph(rec);
            return false;
        }
        if (graph->Connect(compOut, muxIn) < 0) {
            MessageBoxA(hwnd, english
                        ? "Could not use selected compressor"
                        : JP_CANNOT_USE, "DirectShow", 0);
            TeardownDShowGraph(rec);
            return false;
        }
        IAMVfwCompressDialogs* vfw = rec->compressorDialogs;
        if (rec->compressorStateSize > 0 &&
            vfw != nullptr) {
            vfw->SendDriverMessage(
                0x5001,
                static_cast<long>(
                    reinterpret_cast<UINT_PTR>(rec->compressorState)),
                rec->compressorStateSize);
        }
    } else if (graph->Connect(grabOut, muxIn) < 0) {
        MessageBoxA(hwnd, english
                    ? "Could not connect pVideoOut to pMuxIn1."
                    : JP_MUX_IN1, "DirectShow", 0);
        TeardownDShowGraph(rec);
        return false;
    }

    if (wavPath != nullptr) {
        IBaseFilter* waveSrc = nullptr;
        graph->AddSourceFilter(wavPath, L"Source", &waveSrc);     
        rec->waveSource = waveSrc;
        if (waveSrc == nullptr) {
            MessageBoxA(hwnd, english
                        ? "Could not read WAVE file.\n\nTrying output "
                          "only movie."
                        : JP_WAVE_READ, "DirectShow", 0);
        } else {
            rec->waveOutput = FindOutputPin(rec, waveSrc, 0);
            rec->muxAudioInput = FindInputPin(rec, mux, 1);
            CoCreateInstance(kClsidSampleGrabber, nullptr,
                             CLSCTX_INPROC_SERVER, IID_IBaseFilter,
                             reinterpret_cast<void**>(&rec->audioGrabber));
            IBaseFilter* grabber2 = rec->audioGrabber;
            ISampleGrabber* sg2 = nullptr;
            if (grabber2 != nullptr)
                grabber2->QueryInterface(kIidISampleGrabber,
                                         reinterpret_cast<void**>(&sg2));
            if (sg2 != nullptr) {
                AM_MEDIA_TYPE mt = {};
                mt.bFixedSizeSamples = TRUE;
                mt.lSampleSize = 1;
                mt.majortype = MEDIATYPE_Audio;                   
                sg2->SetMediaType(&mt);                          
                sg2->Release();
            }
            if (grabber2 != nullptr)
                graph->AddFilter(grabber2, L"AudioGrabber");
            rec->audioGrabberInput = FindInputPin(rec, grabber2, 0);
            rec->audioGrabberOutput = FindOutputPin(rec, grabber2, 0);
            graph->Connect(
                rec->waveOutput, rec->audioGrabberInput);
            if (graph->Connect(
                    rec->audioGrabberOutput, rec->muxAudioInput) >= 0) {
                IAMStreamControl* stream = nullptr;
                IPin* muxAudioIn = rec->muxAudioInput;
                if (muxAudioIn != nullptr)
                    muxAudioIn->QueryInterface(
                        IID_IAMStreamControl,
                        reinterpret_cast<void**>(&stream));
                if (stream != nullptr) {
                    stream->StartAt(nullptr, 0);                  
                    REFERENCE_TIME stop =
                        static_cast<REFERENCE_TIME>(seconds * 1e7f);
                    stream->StopAt(&stop, 0, 0);                  
                    stream->Release();
                }
                IConfigAviMux* aviMux = nullptr;
                if (mux != nullptr)
                    mux->QueryInterface(IID_IConfigAviMux,
                                        reinterpret_cast<void**>(&aviMux));
                if (aviMux != nullptr) {
                    aviMux->SetMasterStream(1);                   
                    aviMux->Release();
                }
            } else {
                MessageBoxA(hwnd, english
                            ? "Could not out Wave file.\n\nTrying "
                              "output only movie."
                            : JP_WAVE_OUT, "DirectShow", 0);
            }
        }
    }

    IMediaFilter* mediaFilter = nullptr;
    graph->QueryInterface(IID_IMediaFilter,
                          reinterpret_cast<void**>(&mediaFilter));
    if (mediaFilter != nullptr) {
        if (mediaFilter->SetSyncSource(nullptr) < 0)
            MessageBoxA(hwnd, english
                        ? "Could not SetSyncSource NULL on the graph"
                        : JP_SYNC, "DirectShow", 0);
        mediaFilter->Release();
    }
    IConfigInterleaving* interleave = nullptr;
    bool interleaveOk = false;
    if (mux != nullptr)
        mux->QueryInterface(IID_IConfigInterleaving,
                            reinterpret_cast<void**>(&interleave));
    if (interleave != nullptr) {
        if (interleave->put_Mode(
                static_cast<InterleavingMode>(2)) < 0) {
            MessageBoxA(hwnd, english ? "Could not set interleaving mode"
                                      : JP_ILV_MODE, "DirectShow", 0);
        } else {
            REFERENCE_TIME full = 10000000, cap = 7500000;
            if (interleave->put_Interleaving(&full, &cap) < 0) {
                MessageBoxA(hwnd, english
                            ? "Could not put interleave time"
                            : JP_ILV_TIME, "DirectShow", 0);
            } else {
                interleaveOk = true;
            }
        }
        interleave->Release();
    }
    if (!interleaveOk) {
        TeardownDShowGraph(rec);                                  
        return false;
    }

    graph->QueryInterface(IID_IMediaEvent,
                          reinterpret_cast<void**>(&rec->mediaEvent));
    graph->QueryInterface(IID_IMediaControl,
                          reinterpret_cast<void**>(&rec->mediaControl));
    IMediaControl* control = rec->mediaControl;
    if (control != nullptr)
        control->Run();                                           
    return true;
}

}  
