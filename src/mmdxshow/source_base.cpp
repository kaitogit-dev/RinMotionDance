
#include "mmdxshow.hpp"
#include <new>
#include <stdlib.h>   
#include <amvideo.h>  


extern "C" {
extern const GUID MMDXSHOW_GUID_NULL;            
extern const GUID MMDXSHOW_CLSID_FilterMapper;   
extern const GUID MMDXSHOW_IID_IFilterMapper;    
}

static const GUID MMDXSHOW_CLSID_MemoryAllocator =
    {0x1e651cc0, 0xb199, 0x11d0, {0x82, 0x12, 0x0, 0xc0, 0x4f, 0xc3, 0x2c, 0x45}};
static const GUID MMDXSHOW_IID_IMemAllocator =
    {0x56a8689c, 0xad4, 0x11ce, {0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70}};
static const GUID MMDXSHOW_IID_IMemInputPin =
    {0x56a8689d, 0xad4, 0x11ce, {0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70}};
static const GUID MMDXSHOW_IID_56A868A2 =
    {0x56a868a2, 0xad4, 0x11ce, {0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70}};

static inline char* Slot(void* p, intptr_t off) { return (char*)p + off; }
static inline char* Slot(const void* p, intptr_t off) { return (char*)p + off; }

#if defined(_M_X64)

constexpr intptr_t kF_State     = 0x28;
constexpr intptr_t kF_Clsid     = 0x40;
constexpr intptr_t kF_PLock     = 0x50;
constexpr intptr_t kF_PGraph    = 0x60;
constexpr intptr_t kF_PEventSink= 0x68;
constexpr intptr_t kF_CPins     = 0x78;
constexpr intptr_t kF_PPins     = 0x80;

constexpr intptr_t kP_Connected = 0x30;
constexpr intptr_t kP_NameW     = 0x28;
constexpr intptr_t kP_Dir       = 0x38;
constexpr intptr_t kP_PLock     = 0x40;
constexpr intptr_t kP_Flag24    = 0x48;
constexpr intptr_t kP_Flag25    = 0x49;
constexpr intptr_t kP_Flag26    = 0x4A;
constexpr intptr_t kP_Owner     = 0x50;
constexpr intptr_t kP_QSink     = 0x58;
constexpr intptr_t kP_Mt        = 0x68;   
constexpr intptr_t kP_TStart    = 0xC0;
constexpr intptr_t kP_TStop     = 0xC8;
constexpr intptr_t kP_DRate     = 0xD0;
constexpr intptr_t kP_Alloc     = 0xD8;
constexpr intptr_t kP_Input     = 0xE0;
constexpr intptr_t kP_Filter    = 0xE8;

constexpr intptr_t kT_EventSend = 0x08;
constexpr intptr_t kT_EventReply= 0x10;
constexpr intptr_t kT_Param     = 0x18;
constexpr intptr_t kT_Reply     = 0x1C;
constexpr intptr_t kT_Thread    = 0x20;
constexpr intptr_t kT_CritSec   = 0x28;

constexpr intptr_t kPin_CBasePin    = 0x78;
constexpr intptr_t kPin_CAMThread   = 0x00;

constexpr intptr_t kObj_IBaseFilter    = 0x18;
constexpr intptr_t kObj_IAMovieSetup   = 0x20;
constexpr intptr_t kPinSub_IPin        = 0x18;
constexpr intptr_t kPinSub_IQualCtl    = 0x20;

#else

constexpr intptr_t kF_State     = 0x14;
constexpr intptr_t kF_Clsid     = 0x28;
constexpr intptr_t kF_PLock     = 0x38;
constexpr intptr_t kF_PGraph    = 0x40;
constexpr intptr_t kF_PEventSink= 0x44;
constexpr intptr_t kF_CPins     = 0x50;
constexpr intptr_t kF_PPins     = 0x54;

constexpr intptr_t kP_Connected = 0x18;
constexpr intptr_t kP_NameW     = 0x14;
constexpr intptr_t kP_Dir       = 0x1C;
constexpr intptr_t kP_PLock     = 0x20;
constexpr intptr_t kP_Flag24    = 0x24;
constexpr intptr_t kP_Flag25    = 0x25;
constexpr intptr_t kP_Flag26    = 0x26;
constexpr intptr_t kP_Owner     = 0x28;
constexpr intptr_t kP_QSink     = 0x2C;
constexpr intptr_t kP_Mt        = 0x34;   
constexpr intptr_t kP_TStart    = 0x80;
constexpr intptr_t kP_TStop     = 0x88;
constexpr intptr_t kP_DRate     = 0x90;
constexpr intptr_t kP_Alloc     = 0x98;
constexpr intptr_t kP_Input     = 0x9C;
constexpr intptr_t kP_Filter    = 0xA0;

constexpr intptr_t kT_EventSend = 0x04;
constexpr intptr_t kT_EventReply= 0x08;
constexpr intptr_t kT_Param     = 0x0C;
constexpr intptr_t kT_Reply     = 0x10;
constexpr intptr_t kT_Thread    = 0x14;
constexpr intptr_t kT_CritSec   = 0x18;

constexpr intptr_t kPin_CBasePin    = 0x48;
constexpr intptr_t kPin_CAMThread   = 0x00;

constexpr intptr_t kObj_IBaseFilter    = 0x0C;
constexpr intptr_t kObj_IAMovieSetup   = 0x10;
constexpr intptr_t kPinSub_IPin        = 0x0C;
constexpr intptr_t kPinSub_IQualCtl    = 0x10;

#endif

static inline CLSID&          F_clsid(CBaseFilter* f)  { return *(CLSID*)Slot(f, kF_Clsid); }   
static inline CRITICAL_SECTION*& F_pLock(CBaseFilter* f){ return *(CRITICAL_SECTION**)Slot(f, kF_PLock); } 
static inline FILTER_STATE&   F_state(CBaseFilter* f)  { return *(FILTER_STATE*)Slot(f, kF_State); }        
static inline IFilterGraph*&  F_pGraph(CBaseFilter* f) { return *(IFilterGraph**)Slot(f, kF_PGraph); }       
static inline IUnknown*&      F_pEventSink(CBaseFilter* f) { return *(IUnknown**)Slot(f, kF_PEventSink); }      

static inline WCHAR*&             P_pNameW(CBasePin* p)   { return *(WCHAR**)Slot(p, kP_NameW); }           
static inline PIN_DIRECTION&      P_dir(CBasePin* p)      { return *(PIN_DIRECTION*)Slot(p, kP_Dir); }    
static inline CRITICAL_SECTION*&  P_pLock(CBasePin* p)    { return *(CRITICAL_SECTION**)Slot(p, kP_PLock); } 
static inline unsigned char&      P_flag24(CBasePin* p)   { return *(unsigned char*)Slot(p, kP_Flag24); }    
static inline unsigned char&      P_flag25(CBasePin* p)   { return *(unsigned char*)Slot(p, kP_Flag25); }    
static inline unsigned char&      P_flag26(CBasePin* p)   { return *(unsigned char*)Slot(p, kP_Flag26); }    
static inline CBaseFilter*&       P_pOwner(CBasePin* p)   { return *(CBaseFilter**)Slot(p, kP_Owner); }     
static inline IPin*&              P_connected(CBasePin* p){ return *(IPin**)Slot(p, kP_Connected); }            
static inline IMemAllocator*&     P_alloc(CBasePin* p)    { return *(IMemAllocator**)Slot(p, kP_Alloc); }   
static inline IMemInputPin*&      P_input(CBasePin* p)    { return *(IMemInputPin**)Slot(p, kP_Input); }    
static inline void*&              P_pNotify(CBasePin* p)  { return *(void**)Slot(p, kP_QSink); }            
static inline AM_MEDIA_TYPE&      P_mt(CBasePin* p)       { return *(AM_MEDIA_TYPE*)Slot(p, kP_Mt); }    
static inline REFERENCE_TIME&     P_tStart(CBasePin* p)   { return *(REFERENCE_TIME*)Slot(p, kP_TStart); }   
static inline REFERENCE_TIME&     P_tStop(CBasePin* p)    { return *(REFERENCE_TIME*)Slot(p, kP_TStop); }   
static inline double&             P_dRate(CBasePin* p)    { return *(double*)Slot(p, kP_DRate); }           

static inline CAMThread* PinToThread(CBasePin* p)
{
    return reinterpret_cast<CAMThread*>(Slot(p, -kPin_CBasePin));
}


static int MMDxShow_wcslen(LPCWSTR p)
{
    int n = -1;
    do { ++n; } while (p[n]);
    return n;
}

static WCHAR* MMDxShow_wcsncpyClamp(WCHAR* dst, const WCHAR* src, int cchMax)
{
    if (cchMax == 0)
        return dst;
    for (int i = 1; i < cchMax; ++i) {
        WCHAR c = *src;
        *dst++ = *src++;
        if (c == 0)
            return dst;                     
    }
    *dst = 0;                               
    return dst;
}

static int MMDxShow_IntToWide(int v, LPWSTR dst)
{
    CHAR buf[32];
    wsprintfA(buf, "%d", v);
    return MultiByteToWideChar(0, 0, buf, -1, dst, 32);
}

static HRESULT MMDxShow_NotifyEvent(CBaseFilter* pFilter, LONG lCode, LONG_PTR lParam1, LONG_PTR lParam2)
{
    IUnknown* pSink = F_pEventSink(pFilter);
    if (pSink == NULL)
        return E_POINTER;
    LONG_PTR p2 = (lCode == 1)
        ? (LONG_PTR)static_cast<IBaseFilter*>(pFilter)
        : lParam2;
    return reinterpret_cast<IMediaEventSink*>(pSink)->Notify(
        lCode, lParam1, p2);
}


static BOOL MMDxShow_MediaTypePartialEqual(const AM_MEDIA_TYPE* pmt1,
                                           const AM_MEDIA_TYPE* pmt2)
{
    if (!MMDxShow_IsEqualGUID16(&pmt1->majortype, &pmt2->majortype))
        return FALSE;
    if (!MMDxShow_IsEqualGUID16(&pmt1->subtype, &pmt2->subtype))
        return FALSE;
    if (!MMDxShow_IsEqualGUID16(&pmt1->formattype, &pmt2->formattype))
        return FALSE;
    if (pmt1->cbFormat != pmt2->cbFormat)
        return FALSE;
    if (pmt1->cbFormat == 0)
        return TRUE;
    return memcmp(pmt1->pbFormat, pmt2->pbFormat, pmt1->cbFormat) == 0;
}

static BOOL MMDxShow_MediaTypeIsWildcard(const AM_MEDIA_TYPE* pmt)
{
    if (MMDxShow_IsEqualGUID16(&pmt->majortype, &MMDXSHOW_GUID_NULL))
        return TRUE;
    if (MMDxShow_IsEqualGUID16(&pmt->formattype, &MMDXSHOW_GUID_NULL))
        return TRUE;
    return FALSE;
}

static BOOL MMDxShow_MediaTypePartialMatch(const AM_MEDIA_TYPE* pmt,
                                           const AM_MEDIA_TYPE* pmtMatch)
{
    if (MMDxShow_IsEqualGUID16(&pmtMatch->majortype, &MMDXSHOW_GUID_NULL) ||
        MMDxShow_IsEqualGUID16(&pmt->majortype, &pmtMatch->majortype))
    {
        if (!MMDxShow_IsEqualGUID16(&pmtMatch->subtype, &MMDXSHOW_GUID_NULL) &&
            !MMDxShow_IsEqualGUID16(&pmt->subtype, &pmtMatch->subtype))
            return FALSE;
        if (MMDxShow_IsEqualGUID16(&pmtMatch->formattype, &MMDXSHOW_GUID_NULL))
            return TRUE;
        if (!MMDxShow_IsEqualGUID16(&pmt->formattype, &pmtMatch->formattype))
            return FALSE;
        if (pmt->cbFormat != pmtMatch->cbFormat)
            return FALSE;
        if (pmt->cbFormat == 0)
            return TRUE;
        return memcmp(pmt->pbFormat, pmtMatch->pbFormat, pmt->cbFormat) == 0;
    }
    return FALSE;              
}

static void MMDxShow_DeleteMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt != NULL) {
        MMDxShow_FreeMediaType(pmt);      
        CoTaskMemFree(pmt);
    }
}


STDMETHODIMP CBaseFilter::QueryInterface(REFIID riid, void** ppv)
{
    return m_pOuterUnknown->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CBaseFilter::AddRef()
{
    return m_pOuterUnknown->AddRef();
}

STDMETHODIMP_(ULONG) CBaseFilter::Release()
{
    return m_pOuterUnknown->Release();
}

STDMETHODIMP CBasePin::QueryInterface(REFIID riid, void** ppv)
{
    return m_pOuterUnknown->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CBasePin::AddRef()
{
    return m_pOuterUnknown->AddRef();
}

STDMETHODIMP_(ULONG) CBasePin::Release()
{
    return m_pOuterUnknown->Release();
}


STDMETHODIMP_(ULONG) CUnknown::NonDelegatingAddRef()
{
    InterlockedIncrement(&m_cRef);
    const ULONG c = static_cast<ULONG>(m_cRef);
    return c <= 1 ? 1 : c;                                  
}

STDMETHODIMP_(ULONG) CUnknown::NonDelegatingRelease()
{
    if (InterlockedDecrement(&m_cRef) == 0) {
        ++m_cRef;                                           
        delete this;                                        
        return 0;
    }
    const ULONG c = static_cast<ULONG>(m_cRef);
    return c <= 1 ? 1 : c;
}

STDMETHODIMP CUnknown::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    static const GUID kIID_IUnknown_ =
        {0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
    if (MMDxShow_IsEqualGUID16(&riid, &kIID_IUnknown_))
        return MMDxShow_ReturnSelf(static_cast<INonDelegatingUnknown*>(this), ppv);
    return E_NOINTERFACE;
}

CUnknown::~CUnknown()
{
}


CAMThread::CAMThread()
{
    m_hEventSend  = CreateEventA(NULL, TRUE,  FALSE, NULL);   
    m_hEventReply = CreateEventA(NULL, FALSE, FALSE, NULL);   
    InitializeCriticalSection(&m_CritSecSend);
    InitializeCriticalSection(&m_CritSecReply);
    m_hThread = NULL;                                         
}

CAMThread::~CAMThread()
{
    HANDLE hThread = (HANDLE)InterlockedExchangePointer((volatile PVOID*)&m_hThread, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, 0xFFFFFFFF);
        CloseHandle(hThread);
    }
    DeleteCriticalSection(&m_CritSecReply);                   
    DeleteCriticalSection(&m_CritSecSend);                    
    CloseHandle(m_hEventReply);                               
    CloseHandle(m_hEventSend);                                
}

static HRESULT MMDxShow_ThreadCoInit(void)
{
    HMODULE h = GetModuleHandleA("ole32.dll");
    if (h != NULL) {
        typedef HRESULT (__stdcall *PFN_CoInitEx)(LPVOID, DWORD);
        PFN_CoInitEx pfn = (PFN_CoInitEx)GetProcAddress(h, "CoInitializeEx");
        if (pfn != NULL)
            return pfn(NULL, 4);
    }
    return E_FAIL;                                            
}

static DWORD WINAPI MMDxShow_InitialThreadProc(LPVOID lpParameter)
{
    const HRESULT hrCo = MMDxShow_ThreadCoInit();             
    const DWORD dw = reinterpret_cast<CAMThread*>(lpParameter)->ThreadProc();
    if (hrCo >= 0)
        CoUninitialize();
    return dw;
}

static BOOL MMDxShow_ThreadCreate(CAMThread* pRoot)
{
    CRITICAL_SECTION* pcs = (CRITICAL_SECTION*)Slot(pRoot, kT_CritSec);
    EnterCriticalSection(pcs);
    HANDLE* phThread = (HANDLE*)Slot(pRoot, kT_Thread);
    if (*phThread != NULL) {
        LeaveCriticalSection(pcs);
        return 0;                                             
    }
    DWORD dwTid;
    HANDLE h = CreateThread(NULL, 0, MMDxShow_InitialThreadProc, pRoot, 0, &dwTid);
    *phThread = h;
    LeaveCriticalSection(pcs);
    return h != NULL ? 1 : 0;
}

static DWORD MMDxShow_ThreadCallWorker(CAMThread* pRoot, DWORD dwCmd)
{
    CRITICAL_SECTION* pcs = (CRITICAL_SECTION*)Slot(pRoot, kT_CritSec);
    EnterCriticalSection(pcs);
    if (*(HANDLE*)Slot(pRoot, kT_Thread) == NULL) {
        LeaveCriticalSection(pcs);
        return (DWORD)E_FAIL;                                 
    }
    *(DWORD*)Slot(pRoot, kT_Param) = dwCmd;                   
    SetEvent(*(HANDLE*)Slot(pRoot, kT_EventSend));            
    WaitForSingleObject(*(HANDLE*)Slot(pRoot, kT_EventReply), 0xFFFFFFFF); 
    const DWORD dwReply = *(DWORD*)Slot(pRoot, kT_Reply);     
    LeaveCriticalSection(pcs);
    return dwReply;
}

static DWORD MMDxShow_ThreadGetRequest(CAMThread* pRoot)
{
    WaitForSingleObject(*(HANDLE*)Slot(pRoot, kT_EventSend), 0xFFFFFFFF);
    return *(DWORD*)Slot(pRoot, kT_Param);
}

static BOOL MMDxShow_ThreadCheckRequest(CAMThread* pRoot, DWORD* pParam)
{
    if (WaitForSingleObject(*(HANDLE*)Slot(pRoot, kT_EventSend), 0) != WAIT_OBJECT_0)
        return 0;
    if (pParam != NULL)
        *pParam = *(DWORD*)Slot(pRoot, kT_Param);
    return 1;
}

static BOOL MMDxShow_ThreadReply(CAMThread* pRoot, DWORD dw)
{
    const HANDLE hSend = *(HANDLE*)Slot(pRoot, kT_EventSend);
    *(DWORD*)Slot(pRoot, kT_Reply) = dw;                      
    ResetEvent(hSend);
    return SetEvent(*(HANDLE*)Slot(pRoot, kT_EventReply));    
}

DWORD CAMThread::ThreadProc()
{
    while (MMDxShow_ThreadGetRequest(this) != 0)
        MMDxShow_ThreadReply(this, (DWORD)E_NOTIMPL);         

    const HRESULT hrInit = (HRESULT)OnThreadInit();           
    if (hrInit >= 0) {
        MMDxShow_ThreadReply(this, 0);
        DWORD cmd;
        do {
            cmd = MMDxShow_ThreadGetRequest(this);            
            switch (cmd) {
            case 1:                                           
            case 2:                                           
                MMDxShow_ThreadReply(this, 0);
                DoBufferProcessingLoop();                     
                break;
            case 3:                                           
            case 4:                                           
                MMDxShow_ThreadReply(this, 0);
                break;
            default:
                MMDxShow_ThreadReply(this, (DWORD)E_NOINTERFACE); 
                break;
            }
        } while (cmd != 4);
        return (DWORD)(OnThreadExit() < 0);                   
    }
    OnThreadExit();                                           
    MMDxShow_ThreadReply(this, (DWORD)hrInit);
    return 1;
}

DWORD CSourceStream::ThreadProc()
{
    return CAMThread::ThreadProc();                           
}


DWORD CAMThread::OnThreadInit()     
{
    return 0;
}

DWORD CAMThread::OnThreadExit()     
{
    return 0;
}

DWORD CAMThread::OnLoopEnter()      
{
    return 0;
}

DWORD CAMThread::DoBufferProcessingLoop()
{
    MMDXTrace("DoBufferProcessingLoop enter\n");
    OnLoopEnter();                                             
    CSourceStream* pStream = static_cast<CSourceStream*>(this);
    CBasePin& pin = *static_cast<CBaseOutputPin*>(pStream);
    DWORD cmd;
    for (;;) {
        while (MMDxShow_ThreadCheckRequest(this, &cmd)) {
            if (cmd == 2 || cmd == 1) {
                MMDxShow_ThreadReply(this, 0);
            } else {
                if (cmd == 3)
                    return 1;
                MMDxShow_ThreadReply(this, (DWORD)E_NOTIMPL); 
            }
            if (cmd == 3)
                return 1;
        }
        IMediaSample* pSample;
        bool requestPending = false;
        while (pin.GetDeliveryBuffer(&pSample, NULL, NULL, 0) < 0) { 
            MMDXTrace("GetDeliveryBuffer retry hr pending\n");
            Sleep(1);
            if (MMDxShow_ThreadCheckRequest(this, &cmd)) {
                requestPending = true;
                break;
            }
        }
        if (!requestPending) {
            const HRESULT hr = FillBuffer(pSample);           
            if (hr != 0) {
                pSample->Release();
                pin.DeliverEndOfStream();                     
                if (hr == 1)
                    return 0;
                CBaseFilter* pFilter = *(CBaseFilter**)Slot(&pin, kP_Filter);   
                MMDxShow_NotifyEvent(pFilter, 3, hr, 0);
                return (DWORD)hr;
            }
            const HRESULT hrDeliver = pin.Deliver(pSample);   
            pSample->Release();
            if (hrDeliver != 0)
                return 0;                                     
        }
    }
}


HRESULT CBaseFilter::StreamTime(REFERENCE_TIME* prtStream)
{
    if (m_pClock == NULL)
        return (HRESULT)0x80040213;                           
    const HRESULT hr = m_pClock->GetTime(prtStream);
    if (hr < 0)
        return hr;
    *prtStream = (REFERENCE_TIME)((ULONGLONG)*prtStream - (ULONGLONG)m_tStart);
    return S_OK;
}

LONG CBaseFilter::GetPinVersion()
{
    return m_PinVersion;                                      
}

const MMDXSHOW_FILTER_SETUP* CBaseFilter::GetSetupData()
{
    return NULL;
}


STDMETHODIMP CBaseFilter::GetClassID(CLSID* pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    memcpy(pClassID, Slot(this, kF_Clsid), sizeof(CLSID));     
    return S_OK;
}

STDMETHODIMP CBaseFilter::GetState(DWORD  , FILTER_STATE* pState)
{
    if (pState == NULL)
        return E_POINTER;
    *pState = m_State;                                        
    return S_OK;
}

STDMETHODIMP CBaseFilter::Stop()
{
    EnterCriticalSection(F_pLock(this));                      
    HRESULT hr = S_OK;
    if (m_State != State_Stopped) {
        const int cPins = GetPinCount();                      
        for (int i = 0; i < cPins; ++i) {
            CBasePin* pPin = GetPin(i);                       
            if (P_connected(pPin) != NULL) {
                const HRESULT hrPin = pPin->Inactive();       
                if (hrPin < 0 && hr >= 0)
                    hr = hrPin;
            }
        }
    }
    m_State = State_Stopped;                                  
    LeaveCriticalSection(F_pLock(this));
    return hr;
}

STDMETHODIMP CBaseFilter::Pause()
{
    EnterCriticalSection(F_pLock(this));
    if (m_State != State_Stopped) {
        m_State = State_Paused;                               
        LeaveCriticalSection(F_pLock(this));
        return S_OK;
    }
    const int cPins = GetPinCount();
    if (cPins > 0) {
        for (int i = 0; i < cPins; ++i) {
            CBasePin* pPin = GetPin(i);
            if (P_connected(pPin) != NULL) {
                const HRESULT hr = pPin->Active();                
                if (hr < 0) {
                    LeaveCriticalSection(F_pLock(this));
                    return hr;
                }
            }
        }
    }
    m_State = State_Paused;
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

STDMETHODIMP CBaseFilter::Run(REFERENCE_TIME tStart)
{
    EnterCriticalSection(F_pLock(this));
    m_tStart = tStart;                                        
    if (m_State == State_Stopped) {
        const HRESULT hr = Pause();                           
        if (hr < 0) {
            LeaveCriticalSection(F_pLock(this));
            return hr;
        }
    }
    if (m_State != State_Running) {
        const int cPins = GetPinCount();
        for (int i = 0; i < cPins; ++i) {
            CBasePin* pPin = GetPin(i);
            if (P_connected(pPin) != NULL) {
                const HRESULT hr = pPin->Run(tStart);
                if (hr < 0) {
                    LeaveCriticalSection(F_pLock(this));
                    return hr;
                }
            }
        }
    }
    m_State = State_Running;                                  
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

STDMETHODIMP CBaseFilter::SetSyncSource(IReferenceClock* pClock)
{
    EnterCriticalSection(F_pLock(this));
    if (pClock != NULL)
        pClock->AddRef();
    if (m_pClock != NULL)
        m_pClock->Release();
    m_pClock = pClock;                                        
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

STDMETHODIMP CBaseFilter::GetSyncSource(IReferenceClock** ppClock)
{
    if (ppClock == NULL)
        return E_POINTER;
    EnterCriticalSection(F_pLock(this));
    if (m_pClock != NULL)
        m_pClock->AddRef();
    *ppClock = m_pClock;
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

STDMETHODIMP CBaseFilter::EnumPins(IEnumPins** ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;
    CEnumPins* pEnum = new CEnumPins(this, NULL);             
    *ppEnum = pEnum;
    return pEnum != NULL ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CBaseFilter::FindPin(LPCWSTR Id, IPin** ppPin)
{
    if (ppPin == NULL)
        return E_POINTER;
    CBasePin* pPin = GetPin(_wtoi(Id) - 1);                   
    *ppPin = pPin ? static_cast<IPin*>(pPin) : NULL;
    if (pPin == NULL)
        return (HRESULT)0x80040216;                           
    static_cast<IPin*>(pPin)->AddRef();
    return S_OK;
}

STDMETHODIMP CBaseFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
    if (pInfo == NULL)
        return E_POINTER;
    if (m_pName != NULL)
        MMDxShow_wcsncpyClamp(pInfo->achName, m_pName, 128);  
    else
        pInfo->achName[0] = 0;
    pInfo->pGraph = F_pGraph(this);                           
    if (pInfo->pGraph != NULL)
        pInfo->pGraph->AddRef();
    return S_OK;
}

STDMETHODIMP CBaseFilter::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
    EnterCriticalSection(F_pLock(this));
    F_pGraph(this) = pGraph;                                  
    if (pGraph != NULL) {
        IUnknown* pUnk = NULL;
        if (pGraph->QueryInterface(MMDXSHOW_IID_56A868A2, (void**)&pUnk) >= 0)
            pUnk->Release();                                  
    } else {
        F_pEventSink(this) = NULL;                            
    }
    if (m_pName != NULL) {                                    
        ::operator delete(m_pName);
        m_pName = NULL;
    }
    if (pName != NULL) {
        const int cch = MMDxShow_wcslen(pName) + 1;           
        m_pName = (WCHAR*)operator new(2 * cch);
        if (m_pName != NULL)
            memcpy(m_pName, pName, 2 * cch);
    }
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

STDMETHODIMP CBaseFilter::QueryVendorInfo(LPWSTR*  )
{
    return E_NOTIMPL;
}


static HRESULT MMDxShow_RegisterFilterMapper1(const MMDXSHOW_FILTER_SETUP* pSetup,
                                              IFilterMapper* pMapper, BOOL bRegister)
{
    if (pSetup == NULL)
        return S_FALSE;

    const CLSID clsid = *pSetup->clsID;
    HRESULT hr = pMapper->UnregisterFilter(clsid);                    
    if (bRegister) {
        hr = pMapper->RegisterFilter(clsid, pSetup->szName, pSetup->dwMerit); 
        if (SUCCEEDED(hr)) {
            for (UINT p = 0; p < pSetup->nPins; ++p) {
                const MMDXSHOW_PIN_SETUP* pPin = &pSetup->lpPin[p];
                hr = pMapper->RegisterPin(clsid, pPin->strName,       
                                          pPin->bRendered, pPin->bOutput,
                                          pPin->bZero, pPin->bMany,
                                          *pPin->clsConnectsToFilter,
                                          pPin->strConnectsToPin);
                if (FAILED(hr))
                    break;
                for (UINT t = 0; t < pPin->nMediaTypes; ++t) {
                    const MMDXSHOW_MEDIATYPE_SETUP* pMt = &pPin->lpMediaType[t];
                    hr = pMapper->RegisterPinType(clsid, pPin->strName, 
                                                  *pMt->clsMajorType,
                                                  *pMt->clsSubType);
                    if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                        return hr;
                }
            }
        }
    }
    return (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ? S_OK : hr;
}

STDMETHODIMP CBaseFilter::Register()
{
    const MMDXSHOW_FILTER_SETUP* pSetup = GetSetupData();     
    if (pSetup == NULL)
        return 1;                                             
    CoInitialize(NULL);
    IFilterMapper* pMapper = NULL;
    if (CoCreateInstance(MMDXSHOW_CLSID_FilterMapper, NULL, CLSCTX_INPROC_SERVER,
                         MMDXSHOW_IID_IFilterMapper, (LPVOID*)&pMapper) >= 0) {
        MMDxShow_RegisterFilterMapper1(pSetup, pMapper, TRUE);        
        pMapper->Release();
    }
    CoFreeUnusedLibraries();
    CoUninitialize();
    return S_OK;
}

STDMETHODIMP CBaseFilter::Unregister()
{
    const MMDXSHOW_FILTER_SETUP* pSetup = GetSetupData();     
    if (pSetup == NULL)
        return 1;                                             
    CoInitialize(NULL);
    IFilterMapper* pMapper = NULL;
    HRESULT hr;
    if ((hr = CoCreateInstance(MMDXSHOW_CLSID_FilterMapper, NULL, CLSCTX_INPROC_SERVER,
                               MMDXSHOW_IID_IFilterMapper, (LPVOID*)&pMapper)) >= 0) {
        hr = MMDxShow_RegisterFilterMapper1(pSetup, pMapper, FALSE);  
        pMapper->Release();
    }
    CoFreeUnusedLibraries();
    CoUninitialize();
    return (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ? S_OK : hr;
}

STDMETHODIMP CBaseFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    static const GUID kIID_IAMovieSetup_ =
        {0xA3D8CEC0, 0x7E5A, 0x11CF, {0xBB, 0xC5, 0x00, 0x80, 0x5F, 0x6C, 0xEF, 0x20}};
    if (MMDxShow_IsEqualGUID16(&riid, &IID_IBaseFilter) ||
        MMDxShow_IsEqualGUID16(&riid, &IID_IMediaFilter) ||
        MMDxShow_IsEqualGUID16(&riid, &IID_IPersist))
        return MMDxShow_ReturnSelf(reinterpret_cast<char*>(this) + kObj_IBaseFilter, ppv);
    if (MMDxShow_IsEqualGUID16(&riid, &kIID_IAMovieSetup_))
        return MMDxShow_ReturnSelf(reinterpret_cast<char*>(this) + kObj_IAMovieSetup, ppv);
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}


CSource::CSource(const char* pName, LPUNKNOWN pUnkOuter, const CLSID* pClsID)
    : CBaseFilter(pName, pUnkOuter)
{
    MMDxShow_LockModule();                 
    m_State      = State_Stopped;          
    m_pClock     = NULL;                   
    m_tStart     = 0;                      
    F_clsid(this) = *pClsID;                                  
    F_pLock(this) = &m_CritSec;            
    m_pName      = NULL;                   
    F_pGraph(this) = NULL;                 
    F_pEventSink(this) = NULL;             
    m_PinVersion = 1;                      
    m_cPins      = 0;                      
    m_ppPins     = NULL;                   
    InitializeCriticalSection(&m_CritSec);
}

CSource::~CSource()
{
    while (m_cPins > 0) {                                     
        CSourceStream* pPin = m_ppPins[m_cPins - 1];
        if (pPin != NULL)
            delete pPin;                                      
    }
    DeleteCriticalSection(&m_CritSec);                        
    ::operator delete(m_pName);                                 
    if (m_pClock != NULL) {                                   
        m_pClock->Release();
        m_pClock = NULL;
    }
    MMDxShow_UnlockModule();                                  
}

int CSource::GetPinCount()
{
    EnterCriticalSection(&m_CritSec);                         
    const int cPins = m_cPins;                                
    LeaveCriticalSection(&m_CritSec);
    return cPins;
}

CBasePin* CSource::GetPin(int n)
{
    EnterCriticalSection(&m_CritSec);
    if (n >= 0 && n < m_cPins && m_ppPins != NULL && m_ppPins[n] != NULL) {
        CBasePin* pPin = m_ppPins[n];                         
        LeaveCriticalSection(&m_CritSec);
        return pPin;
    }
    LeaveCriticalSection(&m_CritSec);
    return NULL;
}

void CSource::AddPin(CSourceStream* pPin)
{
    EnterCriticalSection(&m_CritSec);
    CSourceStream** ppNew = static_cast<CSourceStream**>(
        operator new(sizeof(*ppNew) * (m_cPins + 1)));
    if (ppNew != NULL) {
        if (m_ppPins != NULL) {
            memcpy(ppNew, m_ppPins, sizeof(*ppNew) * m_cPins);
            ppNew[m_cPins] = pPin;                            
            ::operator delete(m_ppPins);
        }
        ppNew[m_cPins] = pPin;                                
        m_ppPins = ppNew;
        ++m_cPins;
        LeaveCriticalSection(&m_CritSec);
    } else {
        LeaveCriticalSection(&m_CritSec);
    }
}

void CSource::RemovePin(CSourceStream* pPin)
{
    if (m_cPins <= 0)
        return;
    int i = 0;
    for (; i < m_cPins; ++i)
        if (m_ppPins[i] == pPin)
            break;
    if (i >= m_cPins)
        return;                                               
    if (m_cPins == 1) {
        ::operator delete(m_ppPins);
        --m_cPins;
        m_ppPins = NULL;
        return;
    }
    for (int j = i + 1; j < m_cPins; ++j)
        m_ppPins[j - 1] = m_ppPins[j];
    --m_cPins;
}


CBasePin::CBasePin(const char*  , CBaseFilter* pFilter, HRESULT*  )
    : CUnknown("", NULL)
{
#if defined(_M_X64)
    static_assert(offsetof(CPushPinDIBSq, m_pFilter) == 0x160, "x64 CBasePin::m_pFilter back-pointer");
    static_assert(offsetof(CPushPinDIBSq, m_pAllocator) == 0x150, "x64 m_pAllocator");
    static_assert(offsetof(CPushPinDIBSq, m_pInputPin) == 0x158, "x64 m_pInputPin");
#elif defined(_M_IX86)
    static_assert(offsetof(CPushPinDIBSq, m_pFilter) == 0xE8, "CBasePin::m_pFilter back-pointer");
    static_assert(offsetof(CPushPinDIBSq, m_pAllocator) == 0xE0, "m_pAllocator");
    static_assert(offsetof(CPushPinDIBSq, m_pInputPin) == 0xE4, "m_pInputPin");
#endif
    MMDxShow_LockModule();                                    
    P_pNameW(this)   = NULL;                                  
    m_Connected      = NULL;                                  
    P_dir(this)      = PINDIR_OUTPUT;                         
    P_pLock(this)    = &pFilter->m_CritSec;                   
    P_flag24(this)   = 0;                                     
    P_flag25(this)   = 0;                                     
    P_flag26(this)   = 0;                                     
    P_pOwner(this)   = pFilter;                               
    P_pNotify(this)  = NULL;                                  
    m_TypeVersion    = 1;                                     
    MMDxShow_InitMediaType(&P_mt(this));                      
    P_tStart(this)   = 0;                                     
    P_tStop(this)    = (REFERENCE_TIME)((ULONGLONG)0x7FFFFFFF << 32 | 0xFFFFFFFF); 
    P_dRate(this)    = 1.0;                                   
    m_pAllocator     = NULL;                                  
    m_pInputPin      = NULL;                                  
}

STDMETHODIMP CBasePin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    if (MMDxShow_IsEqualGUID16(&riid, &IID_IPin))
        return MMDxShow_ReturnSelf(reinterpret_cast<char*>(this) + kPinSub_IPin, ppv);
    if (MMDxShow_IsEqualGUID16(&riid, &IID_IQualityControl))
        return MMDxShow_ReturnSelf(reinterpret_cast<char*>(this) + kPinSub_IQualCtl, ppv);
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CBasePin::NonDelegatingAddRef()
{
    return static_cast<IBaseFilter*>(P_pOwner(this))->AddRef();   
}

STDMETHODIMP_(ULONG) CBasePin::NonDelegatingRelease()
{
    return static_cast<IBaseFilter*>(P_pOwner(this))->Release();  
}

LONG CBasePin::GetMediaTypeVersion()
{
    return m_TypeVersion;
}

HRESULT CBasePin::Run(REFERENCE_TIME  )
{
    return S_OK;
}

static HRESULT MMDxPin_CommitAllocator(CBasePin* pPin)
{
    if (P_alloc(pPin) != NULL)
        return P_alloc(pPin)->Commit();
    return (HRESULT)0x8004020A;                               
}

static HRESULT MMDxPin_DecommitAllocator(CBasePin* pPin)
{
    P_flag24(pPin) = 0;                                       
    if (P_alloc(pPin) != NULL)
        return P_alloc(pPin)->Decommit();
    return (HRESULT)0x8004020A;                               
}

HRESULT CBasePin::Active()
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      
    EnterCriticalSection(pcsFilter);
    EnterCriticalSection(F_pLock(m_pFilter));                 
    const FILTER_STATE state = F_state(m_pFilter);            
    const BOOL bActive = (state == State_Paused || state == State_Running);
    LeaveCriticalSection(F_pLock(m_pFilter));
    if (bActive) {
        LeaveCriticalSection(pcsFilter);
        return 1;                                             
    }
    if (m_Connected == NULL) {
        LeaveCriticalSection(pcsFilter);
        return 0;                                             
    }
    HRESULT hr = MMDxPin_CommitAllocator(this);               
    if (hr < 0) {
        LeaveCriticalSection(pcsFilter);
        return hr;
    }
    if (MMDxShow_ThreadCreate(PinToThread(this))) {           
        hr = (HRESULT)MMDxShow_ThreadCallWorker(PinToThread(this), 0);
        if (hr < 0) {
            LeaveCriticalSection(pcsFilter);
            return hr;
        }
        const HRESULT hr2 = (HRESULT)MMDxShow_ThreadCallWorker(PinToThread(this), 1);
        LeaveCriticalSection(pcsFilter);
        return hr2;
    }
    LeaveCriticalSection(pcsFilter);
    return E_FAIL;                                            
}

HRESULT CBasePin::Inactive()
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      
    EnterCriticalSection(pcsFilter);
    if (m_Connected != NULL) {
        HRESULT hr = MMDxPin_DecommitAllocator(this);         
        if (hr < 0) {
            LeaveCriticalSection(pcsFilter);
            return hr;
        }
        CAMThread* pThread = PinToThread(this);
        if (*(HANDLE*)Slot(pThread, kT_Thread) != NULL) {     
            hr = (HRESULT)MMDxShow_ThreadCallWorker(pThread, 3);
            if (hr < 0) {
                LeaveCriticalSection(pcsFilter);
                return hr;
            }
            hr = (HRESULT)MMDxShow_ThreadCallWorker(pThread, 4);
            if (hr < 0) {
                LeaveCriticalSection(pcsFilter);
                return hr;
            }
            HANDLE hThread = (HANDLE)InterlockedExchangePointer(
                (volatile PVOID*)Slot(pThread, kT_Thread), NULL);
            if (hThread != NULL) {
                WaitForSingleObject(hThread, 0xFFFFFFFF);
                CloseHandle(hThread);
            }
        }
    }
    LeaveCriticalSection(pcsFilter);
    return 0;                                                 
}

HRESULT CBasePin::GetMediaType(int iPosition, AM_MEDIA_TYPE* pmt)
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      
    EnterCriticalSection(pcsFilter);
    if (iPosition < 0) {
        LeaveCriticalSection(pcsFilter);
        return E_INVALIDARG;                                  
    }
    if (iPosition > 0) {
        LeaveCriticalSection(pcsFilter);
        return (HRESULT)0x40103;                              
    }
    CSourceStream* pStream = static_cast<CSourceStream*>(this);
    const HRESULT hr = pStream->GetMediaType1(pmt);           
    LeaveCriticalSection(pcsFilter);
    return hr;
}

HRESULT CBasePin::SetMediaType(const AM_MEDIA_TYPE* pmt)
{
    if (pmt == &P_mt(this))
        return S_OK;
    MMDxShow_FreeMediaType(&P_mt(this));                      
    if (MMDxShow_CopyMediaType(&P_mt(this), pmt) >= 0)        
        return S_OK;
    return E_OUTOFMEMORY;
}

HRESULT CBasePin::CheckConnect(IPin* pPeer)
{
    PIN_INFO info;
    pPeer->QueryPinInfo(&info);                               
    IBaseFilter* pPeerFilter = info.pFilter;
    if (reinterpret_cast<IBaseFilter*>((INT_PTR)P_dir(this)) == pPeerFilter)
        return (HRESULT)0x80040208;
    const HRESULT hr = pPeer->QueryInterface(MMDXSHOW_IID_IMemInputPin,
                                             (void**)&P_input(this));     
    return hr >= 0 ? S_OK : hr;                               
}

HRESULT CBasePin::BreakConnect()
{
    if (m_pAllocator != NULL) {
        const HRESULT hr = m_pAllocator->Decommit();          
        if (hr < 0)
            return hr;
        m_pAllocator->Release();
        m_pAllocator = NULL;
    }
    if (m_pInputPin != NULL) {
        m_pInputPin->Release();
        m_pInputPin = NULL;
    }
    return S_OK;
}

HRESULT CBasePin::CompleteConnect(IPin*  )
{
    return DecideAllocator(m_pInputPin, &m_pAllocator);
}

HRESULT CBasePin::DecideAllocator(IMemInputPin* pPin, IMemAllocator** ppAlloc)
{
    *ppAlloc = NULL;                                          
    ALLOCATOR_PROPERTIES props = { 0, 0, 0, 0 };              
    pPin->GetAllocatorRequirements(&props);
    if (props.cbAlign == 0)
        props.cbAlign = 1;
    if (pPin->GetAllocator(ppAlloc) >= 0 &&                   
        DecideBufferSize(*ppAlloc, &props) >= 0 &&            
        pPin->NotifyAllocator(*ppAlloc, 0) >= 0)              
        return S_OK;
    if (*ppAlloc != NULL) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }
    HRESULT hr = InitAllocator(ppAlloc);                      
    if (hr >= 0) {
        hr = DecideBufferSize(*ppAlloc, &props);              
        if (hr >= 0) {
            hr = pPin->NotifyAllocator(*ppAlloc, 0);          
            if (hr >= 0)
                return S_OK;
        }
    }
    if (*ppAlloc != NULL) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }
    return hr;
}

HRESULT CBasePin::GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime,
                                    REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
    if (m_pAllocator != NULL)
        return m_pAllocator->GetBuffer(ppSample, pStartTime, pEndTime, dwFlags);
    return E_NOINTERFACE;                                     
}

HRESULT CBasePin::Deliver(IMediaSample* pSample)
{
    if (m_pInputPin != NULL)
        return m_pInputPin->Receive(pSample);
    return (HRESULT)0x80040209;                               
}

HRESULT CBasePin::InitAllocator(IMemAllocator** ppAlloc)
{
    return CoCreateInstance(MMDXSHOW_CLSID_MemoryAllocator, NULL,
                            CLSCTX_INPROC_SERVER, MMDXSHOW_IID_IMemAllocator,
                            (LPVOID*)ppAlloc);
}

static HRESULT MMDxPin_ForwardToConnected(CBasePin* pPin, DWORD vtSlot)
{
    IPin* pConnected = P_connected(pPin);
    if (pConnected != NULL)
        return ((HRESULT (__stdcall *)(IPin*))(*(void***)pConnected)[vtSlot / 4])(
            pConnected);
    return (HRESULT)0x80040209;                               
}

HRESULT CBasePin::DeliverEndOfStream() { return MMDxPin_ForwardToConnected(this, 0x38); }
HRESULT CBasePin::DeliverBeginFlush()  { return MMDxPin_ForwardToConnected(this, 0x3C); }
HRESULT CBasePin::DeliverEndFlush()    { return MMDxPin_ForwardToConnected(this, 0x40); }

HRESULT CBasePin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop,
                                    double dRate)
{
    if (m_Connected != NULL)
        return m_Connected->NewSegment(tStart, tStop, dRate);
    return (HRESULT)0x80040209;                               
}

HRESULT CBasePin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProps)
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      
    EnterCriticalSection(pcsFilter);
    if (pAlloc == NULL || pProps == NULL) {
        LeaveCriticalSection(pcsFilter);
        return E_POINTER;                                     
    }
    CPushPinDIBSq* pPinObj = static_cast<CPushPinDIBSq*>(this);
    if (pPinObj->m_bStreamEnded != 0 ||
        pPinObj->m_bBitmapSet == 0) {
        LeaveCriticalSection(pcsFilter);
        return E_FAIL;                                        
    }
    const VIDEOINFOHEADER* pVIH =
        (const VIDEOINFOHEADER*)P_mt(this).pbFormat;
    const DWORD cbWanted = (DWORD)pVIH->bmiHeader.biSizeImage; 
    pProps->cBuffers = 1;                                     
    if (cbWanted > (DWORD)pProps->cbBuffer)
        pProps->cbBuffer = (LONG)cbWanted;                    
    ALLOCATOR_PROPERTIES actual;                              
    const HRESULT hr = pAlloc->SetProperties(pProps, &actual);
    if (hr < 0) {
        LeaveCriticalSection(pcsFilter);
        return hr;
    }
    if (actual.cbBuffer < pProps->cbBuffer) {
        LeaveCriticalSection(pcsFilter);
        return E_FAIL;                                        
    }
    LeaveCriticalSection(pcsFilter);
    return S_OK;
}


static HRESULT MMDxPin_AgreeMediaType(CBasePin* pPin, IPin* pReceivePin,
                                      const AM_MEDIA_TYPE* pmt)
{
    HRESULT hr = pPin->CheckConnect(pReceivePin);  
    if (hr < 0) {
        pPin->BreakConnect();                                 
        return hr;
    }
    hr = pPin->CheckMediaType(pmt);                           
    if (hr != 0) {
        if (hr >= 0 || hr == E_FAIL || hr == E_INVALIDARG)    
            hr = (HRESULT)0x8004022A;                         
    } else {
        P_connected(pPin) = pReceivePin;                      
        pReceivePin->AddRef();
        hr = pPin->SetMediaType(pmt);             
        if (hr >= 0) {
            hr = pReceivePin->ReceiveConnection(
                    static_cast<IPin*>(pPin), pmt);           
            if (hr >= 0) {
                hr = pPin->CompleteConnect(pReceivePin);      
                if (hr >= 0)
                    return hr;
                pReceivePin->Disconnect();                    
            }
        }
    }
    pPin->BreakConnect();                                     
    if (P_connected(pPin) != NULL) {
        P_connected(pPin)->Release();
        P_connected(pPin) = NULL;
    }
    return hr;
}

static HRESULT MMDxPin_TryMediaTypes(CBasePin* pPin, IPin* pReceivePin,
                                     const AM_MEDIA_TYPE* pmt,
                                     IEnumMediaTypes* pEnum)
{
    const HRESULT hrReset = pEnum->Reset();                   
    if (hrReset < 0)
        return hrReset;
    HRESULT hrFail = S_OK;
    for (;;) {
        AM_MEDIA_TYPE* pmtCur = NULL;
        if (pEnum->Next(1, &pmtCur, NULL) != 0)               
            break;                                            
        HRESULT hrThis;
        if (pmt == NULL || MMDxShow_MediaTypePartialMatch(pmtCur, pmt)) {
            const HRESULT hrTry = MMDxPin_AgreeMediaType(pPin, pReceivePin, pmtCur);
            hrThis = hrTry;
            if (hrTry < 0 && hrFail >= 0 &&
                hrTry != E_FAIL && hrTry != E_INVALIDARG && hrTry != (HRESULT)0x8004022A)
                hrFail = hrTry;                               
        } else {
            hrThis = (HRESULT)0x80040207;                     
        }
        MMDxShow_DeleteMediaType(pmtCur);                     
        if (hrThis == 0)
            return S_OK;
    }
    return hrFail != 0 ? hrFail : (HRESULT)0x80040207;
}

static HRESULT MMDxPin_AgreeMediaTypeOrEnum(CBasePin* pPin, IPin* pReceivePin,
                                            const AM_MEDIA_TYPE* pmt)
{
    if (pmt != NULL && !MMDxShow_MediaTypeIsWildcard(pmt))    
        return MMDxPin_AgreeMediaType(pPin, pReceivePin, pmt);
    HRESULT hrFail = (HRESULT)0x80040207;                     
    for (int i = 0; i < 2; ++i) {
        IEnumMediaTypes* pEnum = NULL;
        IPin* pEnumSrc = ((DWORD)P_flag26(pPin) == (DWORD)i)  
            ? pReceivePin
            : static_cast<IPin*>(pPin);                       
        const HRESULT hrEnum = pEnumSrc->EnumMediaTypes(&pEnum); 
        if (hrEnum >= 0) {
            const HRESULT hr = MMDxPin_TryMediaTypes(pPin, pReceivePin, pmt, pEnum);
            pEnum->Release();
            if (hr >= 0)
                return S_OK;
            if (hr != E_FAIL && hr != E_INVALIDARG && hr != (HRESULT)0x8004022A)
                hrFail = hr;
        }
    }
    return hrFail;
}

STDMETHODIMP CBasePin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
    if (pReceivePin == NULL)
        return E_POINTER;
    EnterCriticalSection(P_pLock(this));                      
    if (m_Connected != NULL) {
        LeaveCriticalSection(P_pLock(this));
        return (HRESULT)0x80040204;                           
    }
    if (F_state(P_pOwner(this)) == State_Stopped ||           
        P_flag25(this) != 0) {
        const HRESULT hr = MMDxPin_AgreeMediaTypeOrEnum(this, pReceivePin, pmt); 
        if (hr >= 0) {
            LeaveCriticalSection(P_pLock(this));
            return S_OK;
        }
        BreakConnect();                                       
        LeaveCriticalSection(P_pLock(this));
        return hr;
    }
    LeaveCriticalSection(P_pLock(this));
    return (HRESULT)0x80040224;                               
}

STDMETHODIMP CBasePin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
    if (pConnector == NULL || pmt == NULL)
        return E_POINTER;
    EnterCriticalSection(P_pLock(this));                      
    if (m_Connected != NULL) {
        LeaveCriticalSection(P_pLock(this));
        return (HRESULT)0x80040204;                           
    }
    if (F_state(P_pOwner(this)) != State_Stopped && P_flag25(this) == 0) {
        LeaveCriticalSection(P_pLock(this));
        return (HRESULT)0x80040224;                           
    }
    HRESULT hr = CheckConnect(pConnector);      
    if (hr >= 0) {
        hr = CheckMediaType(pmt);                             
        if (hr != 0) {
            BreakConnect();                                   
            if (hr >= 0 || hr == E_FAIL || hr == E_INVALIDARG)
                hr = (HRESULT)0x8004022A;                     
        } else {
            m_Connected = pConnector;                         
            pConnector->AddRef();
            hr = SetMediaType(pmt);           
            if (hr >= 0) {
                hr = CompleteConnect(pConnector);             
                if (hr >= 0) {
                    LeaveCriticalSection(P_pLock(this));
                    return S_OK;
                }
            }
            m_Connected->Release();
            m_Connected = NULL;
            BreakConnect();                                   
        }
    } else {
        BreakConnect();                                       
    }
    LeaveCriticalSection(P_pLock(this));
    return hr;
}

static HRESULT MMDxPin_DisconnectPeer(CBasePin* pPin)
{
    if (P_connected(pPin) == NULL)
        return 1;                                             
    const HRESULT hr = pPin->BreakConnect();                  
    if (hr >= 0) {
        P_connected(pPin)->Release();
        P_connected(pPin) = NULL;
        return S_OK;
    }
    return hr;
}

STDMETHODIMP CBasePin::Disconnect()
{
    EnterCriticalSection(P_pLock(this));                      
    if (F_state(P_pOwner(this)) != State_Stopped &&           
        P_flag25(this) == 0) {
        LeaveCriticalSection(P_pLock(this));
        return (HRESULT)0x80040224;                           
    }
    const HRESULT hr = MMDxPin_DisconnectPeer(this);          
    LeaveCriticalSection(P_pLock(this));
    return hr;
}

STDMETHODIMP CBasePin::ConnectedTo(IPin** ppPin)
{
    if (ppPin == NULL)
        return E_POINTER;
    *ppPin = m_Connected;                                     
    if (m_Connected == NULL)
        return (HRESULT)0x80040209;                           
    m_Connected->AddRef();
    return S_OK;
}

STDMETHODIMP CBasePin::ConnectionMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt == NULL)
        return E_POINTER;
    EnterCriticalSection(P_pLock(this));                      
    if (m_Connected != NULL) {                                
        MMDxShow_CopyMediaType(pmt, &P_mt(this));             
        LeaveCriticalSection(P_pLock(this));
        return S_OK;
    }
    MMDxShow_InitMediaType(pmt);                              
    LeaveCriticalSection(P_pLock(this));
    return (HRESULT)0x80040209;                               
}

STDMETHODIMP CBasePin::QueryPinInfo(PIN_INFO* pInfo)
{
    if (pInfo == NULL)
        return E_POINTER;
    CBaseFilter* pOwner = P_pOwner(this);                     
    pInfo->pFilter = pOwner ? static_cast<IBaseFilter*>(pOwner) : NULL;
    if (pOwner != NULL)
        pInfo->pFilter->AddRef();                             
    if (P_pNameW(this) != NULL)                               
        MMDxShow_wcsncpyClamp(pInfo->achName, P_pNameW(this), 128);
    else
        pInfo->achName[0] = 0;
    pInfo->dir = P_dir(this);                                 
    return S_OK;
}

STDMETHODIMP CBasePin::QueryDirection(PIN_DIRECTION* pPinDir)
{
    if (pPinDir == NULL)
        return E_POINTER;
    *pPinDir = P_dir(this);                                   
    return S_OK;
}

static int MMDxFilter_FindPinIndex(CBaseFilter* pFilter, IPin* pIPin)
{
    const int cPins = *(int*)Slot(pFilter, kF_CPins);         
    if (cPins <= 0)
        return -1;
    CSourceStream* const* ppPins = *(CSourceStream***)Slot(pFilter, kF_PPins);
    for (int i = 0; i < cPins; ++i)
        if (ppPins[i] != NULL && static_cast<IPin*>(ppPins[i]) == pIPin)
            return i;
    return -1;
}

STDMETHODIMP CBasePin::QueryId(LPWSTR* Id)
{
    if (Id == NULL)
        return E_POINTER;
    const int idx = MMDxFilter_FindPinIndex(P_pOwner(this), static_cast<IPin*>(this));
    const int n = idx + 1;
    if (n < 1)
        return (HRESULT)0x80040216;                           
    WCHAR* p = (WCHAR*)CoTaskMemAlloc(8);
    *Id = p;
    if (p == NULL)
        return E_OUTOFMEMORY;
    MMDxShow_IntToWide(n, p);                                 
    return S_OK;
}

STDMETHODIMP CBasePin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
    if (pmt == NULL)
        return E_POINTER;
    const HRESULT hr = CheckMediaType(pmt);                   
    if (hr < 0)
        return 1;                                             
    return hr;
}

STDMETHODIMP CBasePin::EnumMediaTypes(IEnumMediaTypes** ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;
    CEnumMediaTypes* pEnum = new CEnumMediaTypes(this, NULL); 
    *ppEnum = pEnum;
    return pEnum != NULL ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CBasePin::QueryInternalConnections(IPin**  , ULONG*  )
{
    return E_NOTIMPL;                                         
}

STDMETHODIMP CBasePin::EndOfStream()  { return E_UNEXPECTED; }
STDMETHODIMP CBasePin::BeginFlush()   { return E_UNEXPECTED; }
STDMETHODIMP CBasePin::EndFlush()     { return E_UNEXPECTED; }

STDMETHODIMP CBasePin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop,
                                  double dRate)
{
    P_tStart(this) = tStart;                                  
    P_tStop(this)  = tStop;                                   
    P_dRate(this)  = dRate;                                   
    return S_OK;
}


STDMETHODIMP CBasePin::SetSink(IQualityControl*  )
{
    return E_FAIL;                                            
}

STDMETHODIMP CBasePin::Notify(IBaseFilter* pSelf, Quality  )
{
    EnterCriticalSection(P_pLock(this));                      
    P_pNotify(this) = pSelf;                                  
    LeaveCriticalSection(P_pLock(this));
    return S_OK;
}


CSourceStream::CSourceStream(const char* pName, HRESULT* phr,
                             CSource* pParent, LPCWSTR pName2)
    : CAMThread(), CBaseOutputPin(pName, pParent, phr)
{

    if (pName2 != NULL) {
        const int cch = MMDxShow_wcslen(pName2) + 1;          
        WCHAR* p = (WCHAR*)operator new(2 * cch);
        P_pNameW(this) = p;                                   
        if (p != NULL)
            memcpy(p, pName2, 2 * cch);
    }
    m_pFilter = pParent;                                      
    *phr = S_OK;                                              
    pParent->AddPin(this);                                    
}

CSourceStream::~CSourceStream()
{
    CBasePin* pPin = static_cast<CBasePin*>(this);
    static_cast<CSource*>(*(CBaseFilter**)Slot(pPin, kP_Filter))->RemovePin(this);   
    ::operator delete(P_pNameW(pPin));                         
    MMDxShow_FreeMediaType(&P_mt(pPin));                       
    MMDxShow_UnlockModule();                                  
}

HRESULT CSourceStream::CheckMediaType(const AM_MEDIA_TYPE* pmt)
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      
    EnterCriticalSection(pcsFilter);
    AM_MEDIA_TYPE mt;
    MMDxShow_InitMediaType(&mt);                              
    this->GetMediaType1(&mt);
    const BOOL bEqual = MMDxShow_MediaTypePartialEqual(&mt, pmt);  
    MMDxShow_FreeMediaType(&mt);                              
    LeaveCriticalSection(pcsFilter);
    if (bEqual)
        return S_OK;                                          
    return E_FAIL;                                            
}

HRESULT CSourceStream::GetMediaType1(AM_MEDIA_TYPE* pmt)
{
    (void)pmt;                 
    return E_UNEXPECTED;       
}
