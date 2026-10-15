
#include <new>
#include <string.h>

#include "mmdxshow.hpp"

DEFINE_GUID(MMDXSHOW_CLSID_FilterMapper,
    0xe436ebb2, 0x524f, 0x11ce, 0x9f, 0x53, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70);
DEFINE_GUID(MMDXSHOW_IID_IFilterMapper,
    0x56a868a3, 0xad4, 0x11ce, 0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70);

HRESULT RegisterFilterMapper1(const MMDXSHOW_FILTER_SETUP* pSetup,
                              IFilterMapper* pMapper, BOOL bRegister);

#if defined(_M_X64)
enum PinSlot {
    kPinOff_PushCfg     = 0x630,
    kPinOff_PushExtra   = 0x640,
    kPinOff_AvgPerFrame = 0x648,
    kPinOff_BitmapSet   = 0x650,
    kPinOff_FrameAck    = 0x651,
    kPinOff_FrameReady  = 0x652,
    kPinOff_StreamEnded = 0x653,
    kPinOff_FrameBits   = 0x658,
    kFilterOff_PushSrc  = 0xB0,   
    kFilterAllocSize    = 0xC0,   
    kPinAllocSize       = 0x660   
};
#else
enum PinSlot {
    kPinOff_PushCfg     = 0x5A0,
    kPinOff_PushExtra   = 0x5A4,
    kPinOff_AvgPerFrame = 0x5A8,
    kPinOff_BitmapSet   = 0x5B0,
    kPinOff_FrameAck    = 0x5B1,
    kPinOff_FrameReady  = 0x5B2,
    kPinOff_StreamEnded = 0x5B3,
    kPinOff_FrameBits   = 0x5B4,
    kFilterOff_PushSrc  = 0x70,   
    kFilterAllocSize    = 0x78,   
    kPinAllocSize       = 0x5B8   
};
#endif

HRESULT STDMETHODCALLTYPE CPushSourceDIBSq::NonDelegatingQueryInterface(
    REFIID riid, void** ppv)
{
    if (!MMDxShow_IsEqualGUID16(&riid, &MMDXSHOW_IID_IPushSource))   
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);  
    return MMDxShow_ReturnSelf(this ? reinterpret_cast<BYTE*>(this) + kFilterOff_PushSrc
                                    : NULL,    
                             ppv);
}

HRESULT STDMETHODCALLTYPE CPushSourceDIBSq::QueryInterface(REFIID riid, void** ppv)
{
    return m_pOuterUnknown->QueryInterface(riid, ppv);
}

ULONG STDMETHODCALLTYPE CPushSourceDIBSq::AddRef()
{
    return m_pOuterUnknown->AddRef();
}

ULONG STDMETHODCALLTYPE CPushSourceDIBSq::Release()
{
    return m_pOuterUnknown->Release();
}


HRESULT CPushSourceDIBSq::SetBitmapInfo(const void* pBitmapInfo,
                                        int n, float fps)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[kPinOff_StreamEnded] != 0)      
        return E_FAIL;                     

    void* p = operator new(0x2C, std::nothrow);
    *reinterpret_cast<void**>(pb + kPinOff_PushCfg) = p;
    memcpy(p, pBitmapInfo, 0x28);          
    *reinterpret_cast<int*>(pb + kPinOff_PushExtra) = n;

    *reinterpret_cast<LONGLONG*>(pb + kPinOff_AvgPerFrame) =
        10000000 / (LONGLONG)(unsigned __int64)fps;

    pb[kPinOff_BitmapSet] = 1;
    pb[kPinOff_FrameAck]  = 1;             
    return S_OK;                           
}

HRESULT CPushSourceDIBSq::GetStreamingState(void* pState)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[kPinOff_StreamEnded] != 0)      
        return E_FAIL;
    *reinterpret_cast<BYTE*>(pState) = pb[kPinOff_FrameAck];   
    return S_OK;
}

HRESULT CPushSourceDIBSq::StartStreaming(DWORD_PTR dwBits)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[kPinOff_StreamEnded] != 0)      
        return E_FAIL;
    pb[kPinOff_FrameAck] = 0;
    *reinterpret_cast<DWORD_PTR*>(pb + kPinOff_FrameBits) = dwBits;
    pb[kPinOff_FrameReady] = 1;
    MMDXTrace("StartStreaming bits=%x ready->1\n", (unsigned)dwBits);
    return S_OK;
}

HRESULT CPushSourceDIBSq::BeginStreaming(void)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[kPinOff_StreamEnded] != 0)      
        return E_FAIL;
    pb[kPinOff_StreamEnded] = 1;
    pb[kPinOff_FrameAck]   = 0;
    pb[kPinOff_FrameReady] = 0;
    return S_OK;
}

HRESULT CPushSourceDIBSq::GetRate(float* pRate)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[kPinOff_StreamEnded] != 0)      
        return E_FAIL;
    *pRate = 1.02f;
    return S_OK;
}




CPushSourceDIBSq::CPushSourceDIBSq(LPUNKNOWN pUnkOuter, HRESULT* phr)
    : CSource(NULL, pUnkOuter, &MMDXSHOW_CLSID_PushSourceDIBSq)
{
    void* pMem = operator new(kPinAllocSize, std::nothrow);   
    CPushPinDIBSq* pPin = (CPushPinDIBSq*)pMem;
    if (pPin != NULL)
        pPin = new (pPin) CPushPinDIBSq(phr, this);
    m_pPin = pPin;
    if (phr != NULL)
        *phr = (pPin != NULL) ? S_OK : E_OUTOFMEMORY;   
}

CPushSourceDIBSq::~CPushSourceDIBSq()
{
    CPushPinDIBSq* pPin = m_pPin;
    BYTE* pb = reinterpret_cast<BYTE*>(pPin);

    pb[kPinOff_StreamEnded] = 1;     
    pb[kPinOff_FrameReady] = 1;      
    if (pPin != NULL) {
        void** vtbl = *reinterpret_cast<void***>(pPin);   
        ((void (__thiscall *)(void*, unsigned))(vtbl[1]))(pPin, 1);
    }
}

CUnknown* MMDxShow_NewPushSourceDIBSq(LPUNKNOWN pUnkOuter, HRESULT* phr)
{
    CPushSourceDIBSq* pObject =
        (CPushSourceDIBSq*)operator new(kFilterAllocSize, std::nothrow);
    if (pObject != NULL)
        pObject = new (pObject) CPushSourceDIBSq(pUnkOuter, phr);
    if (phr != NULL)
        *phr = (pObject != NULL) ? S_OK : E_OUTOFMEMORY;
    return pObject;
}
