
#include "mmdxshow.hpp"

#include <amvideo.h>  
#include <string.h>   


namespace {

const GUID kMediaVideo =
    { 0x73646976, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

const GUID kFormatVideoInfo =
    { 0x05589f80, 0xc356, 0x11ce, { 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a } };

const GUID kSubtypeARGB32 =
    { 0x773c9ac0, 0x3274, 0x11d0, { 0xb7, 0x24, 0x00, 0xaa, 0x00, 0x6c, 0x1a, 0x01 } };

} 


namespace {

inline BITMAPINFOHEADER*& PushBIH(CPushPinDIBSq* pPin)
{
    return reinterpret_cast<BITMAPINFOHEADER*&>(pPin->m_pPushCfg);
}

} 


namespace {

void MtSetType_10004AB0(AM_MEDIA_TYPE* pmt, const GUID* pGuid)
{
    memcpy(&pmt->majortype, pGuid, sizeof(GUID));
}

void MtSetFormatType_10004B00(AM_MEDIA_TYPE* pmt, const GUID* pGuid)
{
    memcpy(&pmt->formattype, pGuid, sizeof(GUID));
}

void MtSetSubtype_10004AD0(AM_MEDIA_TYPE* pmt, const GUID* pGuid)
{
    memcpy(&pmt->subtype, pGuid, sizeof(GUID));
}

BYTE* MtAllocFormatBuffer_10004B20(AM_MEDIA_TYPE* pmt, ULONG cb)
{
    if (pmt->cbFormat == cb)
        return pmt->pbFormat;

    BYTE* pNew = (BYTE*)CoTaskMemAlloc(cb);
    if (pNew)
    {
        if (pmt->cbFormat)
            CoTaskMemFree(pmt->pbFormat);
        pmt->pbFormat = pNew;
        pmt->cbFormat = cb;
        return pNew;
    }
    if (cb > pmt->cbFormat)
        return NULL;
    return pmt->pbFormat;   
}

int MtSetFormat_10004F10(AM_MEDIA_TYPE* pmt, const void* pFormat, size_t cb)
{
    BYTE* pDst = MtAllocFormatBuffer_10004B20(pmt, (ULONG)cb);
    if (pDst == NULL)
        return 0;
    memcpy(pDst, pFormat, cb);
    return 1;
}

void MtSetVariableSize_10004AF0(AM_MEDIA_TYPE* pmt)
{
    pmt->bFixedSizeSamples = 0;
}

int MtSetSampleSize_10004EF0(AM_MEDIA_TYPE* pmt, LONG lSize)
{
    if (lSize == 0)
    {
        MtSetVariableSize_10004AF0(pmt);
        return 0;
    }
    pmt->bFixedSizeSamples = 1;
    pmt->lSampleSize = (ULONG)lSize;
    return lSize;
}

BOOL MtIsEqual_10004DD0(const AM_MEDIA_TYPE* pmt1, const AM_MEDIA_TYPE* pmt2)
{
    if (!MMDxShow_IsEqualGUID16(&pmt1->majortype,  &pmt2->majortype))  return FALSE;
    if (!MMDxShow_IsEqualGUID16(&pmt1->subtype,    &pmt2->subtype))    return FALSE;
    if (!MMDxShow_IsEqualGUID16(&pmt1->formattype, &pmt2->formattype)) return FALSE;
    if (pmt1->cbFormat != pmt2->cbFormat)                              return FALSE;
    if (pmt1->cbFormat == 0)                                           return TRUE;
    return memcmp(pmt1->pbFormat, pmt2->pbFormat, pmt1->cbFormat) == 0 ? TRUE : FALSE;
}

} 


namespace {

LONG GetBitmapSize_10006130(const BITMAPINFOHEADER* pbih)
{
    DWORD t = (DWORD)(pbih->biBitCount) * (DWORD)(pbih->biWidth); 
    t += 0x1F;
    t >>= 3;
    t &= 0x1FFFFFFC;
    DWORD d = t * (DWORD)pbih->biHeight;                          
    if (pbih->biHeight < 0)
        d = (DWORD)-(LONG)d;                                      
    return (LONG)d;
}

int FixupDisplayVIH_100050E0(VIDEOINFOHEADER* pVIH)
{
    ::SetRectEmpty(&pVIH->rcSource);        
    ::SetRectEmpty(&pVIH->rcTarget);        

    const WORD wBpp = pVIH->bmiHeader.biBitCount;         
    if (wBpp <= 8 && pVIH->bmiHeader.biClrUsed == 0)      
        pVIH->bmiHeader.biClrUsed = 1u << wBpp;
    if ((DWORD)pVIH->bmiHeader.biClrImportant > pVIH->bmiHeader.biClrUsed)
        pVIH->bmiHeader.biClrImportant = 1u << wBpp;      
    if (pVIH->bmiHeader.biSizeImage == 0)                 
        pVIH->bmiHeader.biSizeImage =
            (DWORD)GetBitmapSize_10006130(&pVIH->bmiHeader);
    return 0;
}

HRESULT ProbeDisplayFormat_10005140(CPushPinDIBSq* pPin, LPCSTR pszDevice)
{
    VIDEOINFOHEADER* pVIH =
        reinterpret_cast<VIDEOINFOHEADER*>(pPin->m_displayBlock);

    ::EnterCriticalSection(&pPin->m_csDisplay);
#if defined(_M_X64)
    memset(pPin->m_displayBlock, 0, 0x464);
#else
    memset(pPin->m_displayBlock, 0, sizeof(pPin->m_displayBlock));
#endif
    pVIH->bmiHeader.biSize = 40;                    

    HDC hdc;
    if (pszDevice && ::lstrcmpiA(pszDevice, "DISPLAY") != 0)
        hdc = ::CreateDCA(NULL, pszDevice, NULL, NULL);
    else
        hdc = ::CreateDCA("DISPLAY", NULL, NULL, NULL);

    if (hdc)
    {
        HBITMAP hbm = ::CreateCompatibleBitmap(hdc, 1, 1);
        if (hbm)
        {
            ::GetDIBits(hdc, hbm, 0, 1, NULL, (BITMAPINFO*)&pVIH->bmiHeader, 0);
            ::GetDIBits(hdc, hbm, 0, 1, NULL, (BITMAPINFO*)&pVIH->bmiHeader, 0);
            ::DeleteObject(hbm);
        }
        ::DeleteDC(hdc);
        FixupDisplayVIH_100050E0(pVIH);
        ::LeaveCriticalSection(&pPin->m_csDisplay);
        return S_OK;
    }
    ::LeaveCriticalSection(&pPin->m_csDisplay);
    return E_FAIL;
}

void InitDisplayState_10005220(CPushPinDIBSq* pPin)
{
    ::InitializeCriticalSection(&pPin->m_csDisplay);
    ProbeDisplayFormat_10005140(pPin, NULL);
}

} 


CPushPinDIBSq::CPushPinDIBSq(HRESULT* phr, CSource* pParent)
    : CSourceStream(NULL, phr, pParent, L"Out")
{
    for (size_t i = 0; i < sizeof(m_padF0) / sizeof(m_padF0[0]); ++i)
        m_padF0[i] = NULL;
    m_frameCount = 0;                      
    ::InitializeCriticalSection(&m_csRender);   
    InitDisplayState_10005220(this);            
    PushBIH(this)   = NULL;                
    m_nPushExtra = 0;                          
    m_rtAvgPerFrame = 0;                      
    m_bBitmapSet   = 0;                    
    m_bFrameAck    = 0;                    
    m_bFrameReady  = 0;                    
    m_bStreamEnded = 0;                    
}

CPushPinDIBSq::~CPushPinDIBSq()
{
    m_bStreamEnded = 1;                    
    m_bFrameReady  = 1;                    
    if (PushBIH(this))
    {
        ::operator delete((void*)PushBIH(this)); 
        PushBIH(this) = NULL;
    }
    ::DeleteCriticalSection(&m_csDisplay); 
    ::DeleteCriticalSection(&m_csRender);  
}


HRESULT CPushPinDIBSq::FillBuffer(IMediaSample* pSample)
{
    if (pSample == NULL)
        return E_POINTER;                          

    if (m_bStreamEnded)                            
        return 1;                                  

    if (!m_bBitmapSet)                             
        return E_FAIL;                             

    ::EnterCriticalSection(&m_csRender);           

    BYTE* pDst = NULL;
    pSample->GetPointer(&pDst);                    
    LONG cbSample = pSample->GetSize();            

    MMDXTrace("FillBuffer enter ended=%d ready=%d\n",
              (int)m_bStreamEnded, (int)m_bFrameReady);
    while (!m_bFrameReady && !m_bStreamEnded)
        ::Sleep(1);
    MMDXTrace("FillBuffer wait done ended=%d ready=%d\n",
              (int)m_bStreamEnded, (int)m_bFrameReady);

    if (m_bStreamEnded)                            
    {
        ::LeaveCriticalSection(&m_csRender);
        return 1;                                  
    }

    m_bFrameReady = 0;                             

    LONG cbCopy = (LONG)PushBIH(this)->biSizeImage; 
    if ((ULONG)cbCopy >= (ULONG)cbSample)           
        cbCopy = cbSample;

    memcpy(pDst, m_pFrameBits, (size_t)cbCopy);     

    m_bFrameAck = (m_bStreamEnded == 0);            

    REFERENCE_TIME rtStart =
        (REFERENCE_TIME)(LONG)m_frameCount * (REFERENCE_TIME)m_rtAvgPerFrame;
    REFERENCE_TIME rtEnd = rtStart + (REFERENCE_TIME)m_rtAvgPerFrame;

    pSample->SetTime(&rtStart, &rtEnd);            

    m_frameCount += 1;                             

    pSample->SetSyncPoint(TRUE);                   
    MMDXTrace("FillBuffer delivered frame %d\n", m_frameCount);

    ::LeaveCriticalSection(&m_csRender);
    return S_OK;
}

HRESULT CPushPinDIBSq::GetMediaType1(AM_MEDIA_TYPE* pmt)
{
    if (pmt == NULL)
        return E_POINTER;

    ::EnterCriticalSection(&m_pFilter->m_CritSec);

    if (m_bStreamEnded || !m_bBitmapSet)
    {
        ::LeaveCriticalSection(&m_pFilter->m_CritSec);
        return E_FAIL;
    }

    struct {
        RECT           rcSource;              
        RECT           rcDest;                
        DWORD          pad20[2];              
        LONGLONG       AvgTimePerFrame;       
        BITMAPINFOHEADER bmiHeader;           
    } vih;
    memset(&vih, 0, sizeof(vih));             
    vih.AvgTimePerFrame = (LONGLONG)m_rtAvgPerFrame;
    memcpy(&vih.bmiHeader, PushBIH(this), sizeof(BITMAPINFOHEADER));

    MtSetType_10004AB0(pmt, &kMediaVideo);
    MtSetFormatType_10004B00(pmt, &kFormatVideoInfo);
    MtSetFormat_10004F10(pmt, &vih, sizeof(vih));
    MtSetSubtype_10004AD0(pmt, &kSubtypeARGB32);
    MtSetSampleSize_10004EF0(pmt, (LONG)vih.bmiHeader.biSizeImage);

    ::LeaveCriticalSection(&m_pFilter->m_CritSec);
    return S_OK;
}

HRESULT CPushPinDIBSq::CheckMediaType(const AM_MEDIA_TYPE* pmt)
{
    if (pmt == NULL)
        return E_POINTER;

    AM_MEDIA_TYPE mt;
    MMDxShow_InitMediaType(&mt);

    GetMediaType1(&mt);                            

    BOOL bSame = MtIsEqual_10004DD0(&mt, pmt);

    MMDxShow_FreeMediaType(&mt);

    return bSame ? S_OK : E_FAIL;
}

HRESULT CPushPinDIBSq::DecideBufferSize(IMemAllocator* pAlloc,
                                        ALLOCATOR_PROPERTIES* pProps)
{
    ::EnterCriticalSection(&m_pFilter->m_CritSec);   

    if (pAlloc == NULL || pProps == NULL)
    {
        ::LeaveCriticalSection(&m_pFilter->m_CritSec);
        return E_POINTER;
    }
    if (m_bStreamEnded || !m_bBitmapSet)
    {
        ::LeaveCriticalSection(&m_pFilter->m_CritSec);
        return E_FAIL;
    }

    pProps->cBuffers = 1;

    LONG cbFrame = (LONG)PushBIH(this)->biSizeImage;  
    if ((ULONG)cbFrame > (ULONG)pProps->cbBuffer)
        pProps->cbBuffer = cbFrame;

    ALLOCATOR_PROPERTIES actual;
    HRESULT hr = pAlloc->SetProperties(pProps, &actual);
    if (hr < 0)
    {
        ::LeaveCriticalSection(&m_pFilter->m_CritSec);
        return hr;
    }
    if (actual.cbBuffer < pProps->cbBuffer)
    {
        ::LeaveCriticalSection(&m_pFilter->m_CritSec);
        return E_FAIL;
    }

    ::LeaveCriticalSection(&m_pFilter->m_CritSec);
    return S_OK;
}
