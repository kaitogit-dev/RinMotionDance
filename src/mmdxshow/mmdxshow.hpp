
#pragma once

#ifndef _MMDXSHOW_H_
#define _MMDXSHOW_H_

#include <windows.h>
#include <objbase.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <strmif.h>     

#ifndef VFW_E_ENUM_OUT_OF_SYNC
#define VFW_E_ENUM_OUT_OF_SYNC ((HRESULT)0x80040203L)
#endif

inline void MMDXTrace(const char* fmt, ...) {
    static const bool enabled = !!std::getenv("MMDX_TRACE");
    if (!enabled)
        return;
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
}

#ifndef __IAMovieSetup_INTERFACE_DEFINED__
#define __IAMovieSetup_INTERFACE_DEFINED__
#undef  INTERFACE
#define INTERFACE IAMovieSetup
DECLARE_INTERFACE_(IAMovieSetup, IUnknown)
{
    STDMETHOD(Register)(THIS) PURE;
    STDMETHOD(Unregister)(THIS) PURE;
};
#endif 


DEFINE_GUID(MMDXSHOW_CLSID_PushSourceDIBSq,
    0x2f1713b8, 0xdd1f, 0x4186, 0x93, 0xbe, 0xfd, 0xa5, 0xb, 0xf6, 0x87, 0xc7);

DEFINE_GUID(MMDXSHOW_IID_IPushSource,
    0xecfab031, 0x72ba, 0x4120, 0xb9, 0xf7, 0x8a, 0x3d, 0x5f, 0xd3, 0x8d, 0xec);

DEFINE_GUID(MMDXSHOW_IID_IEnumMediaTypes,
    0x89c31040, 0x846b, 0x11ce, 0x97, 0xd3, 0x0, 0xaa, 0x0, 0x55, 0x59, 0x5a);



extern LONG  g_cModuleRef;

extern LONG  g_cServerLocks;

extern HINSTANCE g_hInst;

extern DWORD g_amPlatform;

extern OSVERSIONINFOA g_osVer;

extern HMODULE g_hOle32Lib;

void MMDxShow_LockModule(void);     
void MMDxShow_UnlockModule(void);   

bool MMDxShow_IsEqualGUID16(const GUID* a, const GUID* b);

HRESULT MMDxShow_ReturnSelf(void* pSelf, void** ppv);

void    MMDxShow_InitMediaType(AM_MEDIA_TYPE* pmt);
void    MMDxShow_FreeMediaType(AM_MEDIA_TYPE* pmt);
HRESULT MMDxShow_CopyMediaType(AM_MEDIA_TYPE* pmtDst, const AM_MEDIA_TYPE* pmtSrc);


struct MMDXSHOW_MEDIATYPE_SETUP
{
    const CLSID* clsMajorType;  
    const CLSID* clsSubType;    
};

struct MMDXSHOW_PIN_SETUP
{
    const WCHAR* strName;               
    BOOL         bRendered;             
    BOOL         bOutput;               
    BOOL         bZero;                 
    BOOL         bMany;                 
    const CLSID* clsConnectsToFilter;   
    const WCHAR* strConnectsToPin;      
    UINT         nMediaTypes;           
    const MMDXSHOW_MEDIATYPE_SETUP* lpMediaType; 
};

struct MMDXSHOW_FILTER_SETUP
{
    const CLSID*      clsID;    
    const WCHAR*      szName;   
    DWORD             dwMerit;  
    UINT              nPins;    
    const MMDXSHOW_PIN_SETUP* lpPin; 
};

class CUnknown;
typedef CUnknown* (*MMDXSHOW_LPNEWCOMOBJECT)(LPUNKNOWN pUnkOuter, HRESULT* phr);

CUnknown* MMDxShow_NewPushSourceDIBSq(LPUNKNOWN pUnkOuter, HRESULT* phr);

struct CFactoryTemplate
{
    const WCHAR*              m_Name;                 
    const CLSID*              m_ClsID;                
    MMDXSHOW_LPNEWCOMOBJECT   m_lpfnNew;              
    void (*m_lpfnInit)(BOOL bLoading, const CLSID* rclsid); 
    const MMDXSHOW_FILTER_SETUP* m_pAMovieSetup_Filter;     
};
extern const CFactoryTemplate g_Templates[1];
extern const int              g_cTemplates;



class INonDelegatingUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) = 0;  
    virtual ULONG   STDMETHODCALLTYPE NonDelegatingAddRef()  = 0;                               
    virtual ULONG   STDMETHODCALLTYPE NonDelegatingRelease() = 0;                               
};

class CUnknown : public INonDelegatingUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv);  
    virtual ULONG   STDMETHODCALLTYPE NonDelegatingAddRef();   
    virtual ULONG   STDMETHODCALLTYPE NonDelegatingRelease();  
    virtual ~CUnknown();                                       
protected:
    CUnknown(const char*  , LPUNKNOWN pUnkOuter)
        : m_pOuterUnknown(pUnkOuter ? pUnkOuter : (LPUNKNOWN)(INonDelegatingUnknown*)this)
        , m_cRef(0) {}
    LPUNKNOWN m_pOuterUnknown;   
    LONG      m_cRef;            
};

class CBaseFilter : public CUnknown, public IBaseFilter, public IAMovieSetup
{
public:
    virtual HRESULT StreamTime(REFERENCE_TIME* prtStream);   
    virtual LONG    GetPinVersion();                         
    virtual int     GetPinCount() = 0;                       
    virtual class CBasePin* GetPin(int n) = 0;               
    virtual const MMDXSHOW_FILTER_SETUP* GetSetupData();     

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    ULONG   STDMETHODCALLTYPE AddRef();
    ULONG   STDMETHODCALLTYPE Release();
    STDMETHOD(GetClassID)(CLSID* pClassID);                  
    STDMETHOD(Stop)();                                       
    STDMETHOD(Pause)();                                      
    STDMETHOD(Run)(REFERENCE_TIME tStart);                   
    STDMETHOD(GetState)(DWORD dwMillis, FILTER_STATE* pState); 
    STDMETHOD(SetSyncSource)(IReferenceClock* pClock);       
    STDMETHOD(GetSyncSource)(IReferenceClock** ppClock);     
    STDMETHOD(EnumPins)(IEnumPins** ppEnum);                 
    STDMETHOD(FindPin)(LPCWSTR Id, IPin** ppPin);            
    STDMETHOD(QueryFilterInfo)(FILTER_INFO* pInfo);          
    STDMETHOD(JoinFilterGraph)(IFilterGraph* pGraph, LPCWSTR pName); 
    STDMETHOD(QueryVendorInfo)(LPWSTR* pVendorInfo);         

    STDMETHOD(Register)();                                   
    STDMETHOD(Unregister)();                                 

    HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override; 

protected:
    CBaseFilter(const char* pName, LPUNKNOWN pUnkOuter)
        : CUnknown(pName, pUnkOuter) {}
public:
    FILTER_STATE     m_State;        
    IReferenceClock* m_pClock;       
    REFERENCE_TIME   m_tStart;       
    CLSID            m_clsid;        
    void*            m_pLockSlot;    
    WCHAR*           m_pName;        
    IFilterGraph*    m_pGraph;       
    IUnknown*        m_pEventSink;   
    LONG             m_PinVersion;   
    int              m_pad4C;        
    int              m_cPins;        
    class CSourceStream** m_ppPins;  
    CRITICAL_SECTION m_CritSec;      
};

class CSource : public CBaseFilter
{
public:
    CSource(const char* pName, LPUNKNOWN pUnkOuter, const CLSID* pClsID); 
    virtual ~CSource();                                      
    int  GetPinCount() override;                             
    CBasePin* GetPin(int n) override;                        
    void AddPin(CSourceStream* pPin);                        
    void RemovePin(CSourceStream* pPin);
};

#if defined(_M_X64)
static_assert(sizeof(CSource) == 0xB0, "x64 CSource must end at the IPushSource vptr slot (+0xB0)");
static_assert(offsetof(CSource, m_State) == 0x28, "m_State");
static_assert(offsetof(CSource, m_pClock) == 0x30, "m_pClock");
static_assert(offsetof(CSource, m_tStart) == 0x38, "m_tStart");
static_assert(offsetof(CSource, m_clsid) == 0x40, "inline CLSID");
static_assert(offsetof(CSource, m_pLockSlot) == 0x50, "pLock slot");
static_assert(offsetof(CSource, m_pName) == 0x58, "m_pName");
static_assert(offsetof(CSource, m_pGraph) == 0x60, "m_pGraph");
static_assert(offsetof(CSource, m_pEventSink) == 0x68, "m_pEventSink");
static_assert(offsetof(CSource, m_PinVersion) == 0x70, "m_PinVersion");
static_assert(offsetof(CSource, m_cPins) == 0x78, "m_cPins");
static_assert(offsetof(CSource, m_ppPins) == 0x80, "m_ppPins");
static_assert(offsetof(CSource, m_CritSec) == 0x88, "m_CritSec");
#elif defined(_M_IX86)
static_assert(sizeof(CSource) == 0x70, "CSource layout must match operator new(0x70) at 0x10001B70");
static_assert(offsetof(CSource, m_State) == 0x14, "m_State");
static_assert(offsetof(CSource, m_pClock) == 0x18, "m_pClock");
static_assert(offsetof(CSource, m_tStart) == 0x20, "m_tStart");
static_assert(offsetof(CSource, m_clsid) == 0x28, "inline CLSID");
static_assert(offsetof(CSource, m_pLockSlot) == 0x38, "pLock slot");
static_assert(offsetof(CSource, m_pName) == 0x3C, "m_pName");
static_assert(offsetof(CSource, m_pGraph) == 0x40, "m_pGraph");
static_assert(offsetof(CSource, m_pEventSink) == 0x44, "m_pEventSink");
static_assert(offsetof(CSource, m_PinVersion) == 0x48, "m_PinVersion");
static_assert(offsetof(CSource, m_cPins) == 0x50, "m_cPins");
static_assert(offsetof(CSource, m_ppPins) == 0x54, "m_ppPins");
static_assert(offsetof(CSource, m_CritSec) == 0x58, "m_CritSec");
#endif

interface IPushSource : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetBitmapInfo(const void* pBitmapInfo, int n, float fps) = 0;  
    virtual HRESULT STDMETHODCALLTYPE GetStreamingState(void* pState) = 0;                           
    virtual HRESULT STDMETHODCALLTYPE StartStreaming(DWORD_PTR dwBits) = 0;                          
    virtual HRESULT STDMETHODCALLTYPE BeginStreaming(void) = 0;                                      
    virtual HRESULT STDMETHODCALLTYPE GetRate(float* pRate) = 0;                                     
};

class CPushPinDIBSq;   

class CPushSourceDIBSq : public CSource, public IPushSource
{
public:
    CPushSourceDIBSq(LPUNKNOWN pUnkOuter, HRESULT* phr);     
    virtual ~CPushSourceDIBSq();                             
    HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    ULONG   STDMETHODCALLTYPE AddRef();
    ULONG   STDMETHODCALLTYPE Release();
    STDMETHODIMP SetBitmapInfo(const void* pBitmapInfo, int n, float fps) override;
    STDMETHODIMP GetStreamingState(void* pState) override;
    STDMETHODIMP StartStreaming(DWORD_PTR dwBits) override;
    STDMETHODIMP BeginStreaming(void) override;
    STDMETHODIMP GetRate(float* pRate) override;
    CPushPinDIBSq* m_pPin;                                   
};
#if defined(_M_X64)
static_assert(sizeof(CPushSourceDIBSq) == 0xC0, "x64 filter layout must match operator new(0xC0)");
static_assert(offsetof(CPushSourceDIBSq, m_pPin) == 0xB8, "x64 m_pPin");
#elif defined(_M_IX86)
static_assert(sizeof(CPushSourceDIBSq) == 0x78, "filter layout must match operator new(0x78)");
#endif

class CAMThread
{
public:
    virtual DWORD ThreadProc();                              
    virtual ~CAMThread();                                    
    virtual HRESULT FillBuffer(IMediaSample* pSample) = 0;   
    virtual DWORD  OnThreadInit();                           
    virtual DWORD  OnThreadExit();                           
    virtual DWORD  OnLoopEnter();                            
    virtual DWORD  DoBufferProcessingLoop();                 
protected:
    CAMThread();
    HANDLE m_hEventSend;      
    HANDLE m_hEventReply;     
    DWORD  m_uParam;          
    DWORD  m_uReply;          
    HANDLE m_hThread;         
    CRITICAL_SECTION m_CritSecSend;   
    CRITICAL_SECTION m_CritSecReply;  
};
#if defined(_M_X64)
static_assert(sizeof(CAMThread) == 0x78, "x64 CAMThread layout must match the VC10 rebuild");
#elif defined(_M_IX86)
static_assert(sizeof(CAMThread) == 0x48, "CAMThread layout must match the binary");
#endif

class CBasePin : public CUnknown, public IPin, public IQualityControl
{
public:
    virtual LONG    GetMediaTypeVersion();                                  
    virtual HRESULT Active();                                               
    virtual HRESULT Inactive();                                             
    virtual HRESULT Run(REFERENCE_TIME tStart);                             
    virtual HRESULT CheckMediaType(const AM_MEDIA_TYPE* pmt) = 0;           
    virtual HRESULT SetMediaType(const AM_MEDIA_TYPE* pmt);                 
    virtual HRESULT CheckConnect(IPin* pPeer);                              
    virtual HRESULT BreakConnect();                                         
    virtual HRESULT CompleteConnect(IPin* pReceivePin);                     
    virtual HRESULT GetMediaType(int iPosition, AM_MEDIA_TYPE* pmt);        
    virtual HRESULT DecideAllocator(IMemInputPin* pPin, IMemAllocator** ppAlloc); 
    virtual HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProps); 
    virtual HRESULT GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime,
                                      REFERENCE_TIME* pEndTime, DWORD dwFlags); 
    virtual HRESULT Deliver(IMediaSample* pSample);                         
    virtual HRESULT InitAllocator(IMemAllocator** ppAlloc);                 
    virtual HRESULT DeliverEndOfStream();                                   
    virtual HRESULT DeliverBeginFlush();                                    
    virtual HRESULT DeliverEndFlush();                                      
    virtual HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop,
                                      double dRate);                        

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    ULONG   STDMETHODCALLTYPE AddRef();
    ULONG   STDMETHODCALLTYPE Release();
    STDMETHOD(Connect)(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt);        
    STDMETHOD(ReceiveConnection)(IPin* pConnector, const AM_MEDIA_TYPE* pmt); 
    STDMETHOD(Disconnect)();                                                
    STDMETHOD(ConnectedTo)(IPin** ppPin);                                   
    STDMETHOD(ConnectionMediaType)(AM_MEDIA_TYPE* pmt);                     
    STDMETHOD(QueryPinInfo)(PIN_INFO* pInfo);                               
    STDMETHOD(QueryDirection)(PIN_DIRECTION* pPinDir);                      
    STDMETHOD(QueryId)(LPWSTR* Id);                                         
    STDMETHOD(QueryAccept)(const AM_MEDIA_TYPE* pmt);                       
    STDMETHOD(EnumMediaTypes)(IEnumMediaTypes** ppEnum);                    
    STDMETHOD(QueryInternalConnections)(IPin** apPin, ULONG* nPin);         
    STDMETHOD(EndOfStream)();                                               
    STDMETHOD(BeginFlush)();                                                
    STDMETHOD(EndFlush)();                                                  
    STDMETHOD(NewSegment)(REFERENCE_TIME tStart, REFERENCE_TIME tEnd,
                          double dRate);                                    

    STDMETHOD(SetSink)(IQualityControl* piqc);                              
    STDMETHOD(Notify)(IBaseFilter* pSelf, Quality q);                       

    HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override; 
    ULONG   STDMETHODCALLTYPE NonDelegatingAddRef() override;    
    ULONG   STDMETHODCALLTYPE NonDelegatingRelease() override;   

protected:
    CBasePin(const char* pName, CBaseFilter* pFilter, HRESULT* phr);
    WCHAR*            m_pName;         
    IPin*             m_Connected;     
    PIN_DIRECTION     m_dir;           
    void*             m_pinPad20[2];   
    CBaseFilter*      m_pFilterOwner;  
    void*             m_pQSink;        
    LONG              m_TypeVersion;   
#if defined(_M_X64)
    void*             m_pinPad34[14];  
#else
    void*             m_pinPad34[25];  
#endif
    IMemAllocator*    m_pAllocator;    
    IMemInputPin*     m_pInputPin;     
    CBaseFilter*      m_pFilter;       
#if !defined(_M_X64)
    void*             m_pinPadA4;      
#endif
};

class CBaseOutputPin : public CBasePin
{
protected:
    CBaseOutputPin(const char* pName, CBaseFilter* pFilter, HRESULT* phr)
        : CBasePin(pName, pFilter, phr) {}
};

class CSourceStream : public CAMThread, public CBaseOutputPin
{
public:
    CSourceStream(const char* pName, HRESULT* phr, CSource* pParent, LPCWSTR pName2);
    virtual DWORD  ThreadProc() override;                      
    virtual ~CSourceStream();                                  
    virtual HRESULT CheckMediaType(const AM_MEDIA_TYPE* pmt) override; 
    virtual HRESULT GetMediaType1(AM_MEDIA_TYPE* pmt);         
};

class CPushPinDIBSq : public CSourceStream
{
public:
    CPushPinDIBSq(HRESULT* phr, CSource* pParent);              
    virtual ~CPushPinDIBSq();                                  
    virtual HRESULT FillBuffer(IMediaSample* pSample) override; 
    virtual HRESULT GetMediaType1(AM_MEDIA_TYPE* pmt) override; 
    virtual HRESULT CheckMediaType(const AM_MEDIA_TYPE* pmt) override; 
    virtual HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProps) override; 

#if defined(_M_X64)
    void*            m_padF0[2];       
#else
    void*            m_padF0[4];       
#endif
    int              m_frameCount;     
    CRITICAL_SECTION m_csRender;       
#if !defined(_M_X64)
    void*            m_pad11C;         
#endif
    CRITICAL_SECTION m_csDisplay;      
#if defined(_M_X64)
    unsigned char    m_displayBlock[0x460]; 
#else
    unsigned char    m_displayBlock[0x464]; 
    void*            m_pad59C;         
#endif
    void*            m_pPushCfg;       
#if defined(_M_X64)
    void*            m_pinPad638;      
#endif
    int              m_nPushExtra;     
    LONGLONG         m_rtAvgPerFrame;  
    unsigned char    m_bBitmapSet;     
    unsigned char    m_bFrameAck;      
    unsigned char    m_bFrameReady;    
    unsigned char    m_bStreamEnded;   
    void*            m_pFrameBits;     
};
#if defined(_M_X64)
static_assert(sizeof(CPushPinDIBSq) == 0x660, "x64 pin layout must match operator new(0x660)");
static_assert(offsetof(CPushPinDIBSq, m_frameCount) == 0x178, "x64 m_frameCount");
static_assert(offsetof(CPushPinDIBSq, m_csRender) == 0x180, "x64 m_csRender");
static_assert(offsetof(CPushPinDIBSq, m_csDisplay) == 0x1A8, "x64 m_csDisplay");
static_assert(offsetof(CPushPinDIBSq, m_displayBlock) == 0x1D0, "x64 m_displayBlock");
static_assert(offsetof(CPushPinDIBSq, m_pPushCfg) == 0x630, "x64 m_pPushCfg");
static_assert(offsetof(CPushPinDIBSq, m_nPushExtra) == 0x640, "x64 m_nPushExtra");
static_assert(offsetof(CPushPinDIBSq, m_rtAvgPerFrame) == 0x648, "x64 m_rtAvgPerFrame");
static_assert(offsetof(CPushPinDIBSq, m_bStreamEnded) == 0x653, "x64 m_bStreamEnded");
static_assert(offsetof(CPushPinDIBSq, m_pFrameBits) == 0x658, "x64 m_pFrameBits");
static_assert(offsetof(CPushPinDIBSq, m_padF0) == 0x168, "x64 CBasePin subobject must end at 0x168");
#elif defined(_M_IX86)
static_assert(sizeof(CPushPinDIBSq) == 0x5B8, "pin layout must match operator new size in the binary");
static_assert(offsetof(CPushPinDIBSq, m_padF0) == 0xF0, "CBasePin subobject must end at 0xF0");
static_assert(offsetof(CPushPinDIBSq, m_frameCount) == 0x100, "m_frameCount");
static_assert(offsetof(CPushPinDIBSq, m_csRender) == 0x104, "m_csRender");
static_assert(offsetof(CPushPinDIBSq, m_csDisplay) == 0x120, "m_csDisplay");
static_assert(offsetof(CPushPinDIBSq, m_displayBlock) == 0x138, "m_displayBlock");
static_assert(offsetof(CPushPinDIBSq, m_pPushCfg) == 0x5A0, "m_pPushCfg");
static_assert(offsetof(CPushPinDIBSq, m_nPushExtra) == 0x5A4, "m_nPushExtra");
static_assert(offsetof(CPushPinDIBSq, m_rtAvgPerFrame) == 0x5A8, "m_rtAvgPerFrame");
static_assert(offsetof(CPushPinDIBSq, m_bStreamEnded) == 0x5B3, "m_bStreamEnded");
static_assert(offsetof(CPushPinDIBSq, m_pFrameBits) == 0x5B4, "m_pFrameBits");
#endif

class CEnumPins : public IEnumPins
{
public:
    CEnumPins(CBaseFilter* pFilter, CEnumPins* pEnum);
    virtual ~CEnumPins();

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(Next)(ULONG cPins, IPin** ppPins, ULONG* pcFetched);
    STDMETHOD(Skip)(ULONG cPins);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumPins** ppEnum);

private:
    struct PinNode { PinNode* pPrev; PinNode* pNext; CBasePin* pPin; };
    struct PinList
    {
        PinNode* pHead;    
        PinNode* pTail;    
        LONG     nCount;   
        LONG     nCapacity;
        LONG     nFree;    
        PinNode* pFree;    
    };

    static void     ListInit(PinList* pList);                       
    static PinNode* ListPushBack(PinList* pList, CBasePin* pPin);   
    static void     ListFreeActive(PinList* pList);                 
    static void     ListDestroy(PinList* pList);                    
    static void     ListCopy(PinList* pList, const PinList* pSrc);  
    static PinNode* ListFind(const PinList* pList, CBasePin* pPin); 
    void            RefreshPinState();                              

    LONG         m_Position;    
    LONG         m_PinCount;    
    CBaseFilter* m_pFilter;     
    LONG         m_PinVersion;  
    LONG         m_cRef;        
    PinList      m_Pins;        
};
#if defined(_M_X64)
static_assert(sizeof(CEnumPins) == 0x48, "x64 CEnumPins layout must match the 0x180004B70 operator new(0x48)");
#elif defined(_M_IX86)
static_assert(sizeof(CEnumPins) == 0x30, "CEnumPins layout must match 0x100044F0's operator new(0x30)");
#endif

class CEnumMediaTypes : public IEnumMediaTypes
{
public:
    CEnumMediaTypes(CBasePin* pPin, CEnumMediaTypes* pEnum);
    virtual ~CEnumMediaTypes();

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(Next)(ULONG cMediaTypes, AM_MEDIA_TYPE** ppMediaTypes, ULONG* pcFetched);
    STDMETHOD(Skip)(ULONG cMediaTypes);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumMediaTypes** ppEnum);

private:
    LONG      m_Position;   
    CBasePin* m_pPin;       
    LONG      m_Version;    
    LONG      m_cRef;       
};
#if defined(_M_X64)
static_assert(sizeof(CEnumMediaTypes) == 0x20, "x64 CEnumMediaTypes layout must match the 0x180004A70 operator new(0x20)");
#elif defined(_M_IX86)
static_assert(sizeof(CEnumMediaTypes) == 0x14, "CEnumMediaTypes layout must match 0x10004600's operator new(0x14)");
#endif

class CClassFactory : public IClassFactory
{
public:
    CClassFactory(const CFactoryTemplate* pTemplate);
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppv);
    STDMETHOD(LockServer)(BOOL fLock);
private:
    const CFactoryTemplate* m_pTemplate;  
    LONG                    m_cRef;       
};
#if defined(_M_X64)
static_assert(sizeof(CClassFactory) == 0x18, "x64 class factory must match the operator new(0x18) in DllGetClassObject");
#elif defined(_M_IX86)
static_assert(sizeof(CClassFactory) == 0x0C, "class factory must match the operator new(0x0C) in DllGetClassObject");
#endif

extern "C" {
HRESULT WINAPI DllCanUnloadNow(void);
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);
HRESULT WINAPI DllRegisterServer(void);
HRESULT WINAPI DllUnregisterServer(void);
BOOL    WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
}

#endif 
