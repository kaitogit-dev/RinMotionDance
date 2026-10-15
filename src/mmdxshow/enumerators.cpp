
#include "mmdxshow.hpp"
#include <new>



void CEnumPins::ListInit(PinList* pList)
{
    pList->pHead     = NULL;
    pList->pTail     = NULL;
    pList->nCount    = 0;
    pList->nCapacity = 10;
    pList->nFree     = 0;
    pList->pFree     = NULL;
}

CEnumPins::PinNode* CEnumPins::ListPushBack(PinList* pList, CBasePin* pPin)
{
    PinNode* pNode = pList->pFree;
    if (pNode) {
        pList->pFree = pNode->pNext;
        --pList->nFree;
    } else {
        pNode = new (std::nothrow) PinNode();
        if (pNode == NULL)
            return NULL;                       
    }
    pNode->pPin  = pPin;
    pNode->pNext = NULL;
    pNode->pPrev = pList->pTail;
    if (pList->pTail) {
        pList->pTail->pNext = pNode;
        ++pList->nCount;
    } else {
        ++pList->nCount;
        pList->pHead = pNode;
    }
    pList->pTail = pNode;
    return pNode;
}

void CEnumPins::ListFreeActive(PinList* pList)
{
    PinNode* pNode = pList->pHead;
    while (pNode) {
        PinNode* pNext = pNode->pNext;
        delete pNode;
        pNode = pNext;
    }
    pList->nCount = 0;
    pList->pTail  = NULL;
    pList->pHead  = NULL;
}

void CEnumPins::ListDestroy(PinList* pList)
{
    ListFreeActive(pList);
    PinNode* pNode = pList->pFree;
    while (pNode) {
        PinNode* pNext = pNode->pNext;
        delete pNode;
        pNode = pNext;
    }
}

void CEnumPins::ListCopy(PinList* pList, const PinList* pSrc)
{
    PinNode* pNode = pSrc->pHead;              
    while (pNode) {
        CBasePin* pPin = pNode->pPin;          
        pNode = pNode->pNext;
        if (ListPushBack(pList, pPin) == NULL)
            break;                             
    }
}

CEnumPins::PinNode* CEnumPins::ListFind(const PinList* pList, CBasePin* pPin)
{
    for (PinNode* pNode = pList->pHead; pNode; pNode = pNode->pNext)
        if (pNode->pPin == pPin)
            return pNode;
    return NULL;
}


CEnumPins::CEnumPins(CBaseFilter* pFilter, CEnumPins* pEnum)
{
    m_Position   = 0;
    m_PinCount   = 0;
    m_pFilter    = pFilter;
    m_cRef       = 1;
    ListInit(&m_Pins);

    static_cast<IBaseFilter*>(m_pFilter)->AddRef();

    if (pEnum) {
        m_Position   = pEnum->m_Position;
        m_PinCount   = pEnum->m_PinCount;
        m_PinVersion = pEnum->m_PinVersion;
        ListCopy(&m_Pins, &pEnum->m_Pins);
    } else {
        m_PinVersion = m_pFilter->GetPinVersion();   
        m_PinCount   = m_pFilter->GetPinCount();     
    }
}

CEnumPins::~CEnumPins()
{
    static_cast<IBaseFilter*>(m_pFilter)->Release();   
    ListDestroy(&m_Pins);                            
}

STDMETHODIMP CEnumPins::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == NULL)
        return E_POINTER;
    if (MMDxShow_IsEqualGUID16(&riid, &IID_IEnumPins) ||
        MMDxShow_IsEqualGUID16(&riid, &IID_IUnknown))
        return MMDxShow_ReturnSelf(this, ppv);          
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumPins::AddRef()
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumPins::Release()
{
    const LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
        return 0;
    }
    return (ULONG)cRef;
}

STDMETHODIMP CEnumPins::Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched)
{
    if (ppPins == NULL)
        return E_POINTER;
    if (pcFetched)
        *pcFetched = 0;
    else if (cPins > 1)
        return E_INVALIDARG;

    ULONG nFetched = 0;
    if (m_pFilter->GetPinVersion() != m_PinVersion)     
        RefreshPinState();                              

    LONG cWanted = m_PinCount - m_Position;
    if (cWanted >= (LONG)cPins)
        cWanted = (LONG)cPins;
    if (cWanted == 0)
        return S_FALSE;

    while (cWanted) {
        const LONG nPin = m_Position;
        if (m_PinCount == nPin)
            break;
        ++m_Position;
        CBasePin* pPin = m_pFilter->GetPin(nPin);       
        if (pPin == NULL)
            return VFW_E_ENUM_OUT_OF_SYNC;              
        if (ListFind(&m_Pins, pPin) == NULL) {       
            *ppPins = static_cast<IPin*>(pPin);
            static_cast<IPin*>(pPin)->AddRef();
            ++nFetched;
            ++ppPins;
            ListPushBack(&m_Pins, pPin);             
            --cWanted;
        }
    }

    if (pcFetched)
        *pcFetched = nFetched;
    return (cPins != nFetched) ? S_FALSE : S_OK;
}

STDMETHODIMP CEnumPins::Skip(ULONG cPins)
{
    if (m_pFilter->GetPinVersion() != m_PinVersion)
        return VFW_E_ENUM_OUT_OF_SYNC;
    const LONG nCurrent = m_Position;
    if (cPins > (ULONG)(m_PinCount - nCurrent))
        return S_FALSE;
    m_Position = (LONG)cPins + nCurrent;
    return S_OK;
}

STDMETHODIMP CEnumPins::Reset()
{
    m_PinVersion = m_pFilter->GetPinVersion();          
    m_PinCount   = m_pFilter->GetPinCount();            
    m_Position   = 0;
    ListFreeActive(&m_Pins);                         
    return S_OK;
}

void CEnumPins::RefreshPinState()
{
    m_PinVersion = m_pFilter->GetPinVersion();
    m_PinCount   = m_pFilter->GetPinCount();
    m_Position   = 0;
}

STDMETHODIMP CEnumPins::Clone(IEnumPins** ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;
    if (m_pFilter->GetPinVersion() != m_PinVersion) {
        *ppEnum = NULL;
        return VFW_E_ENUM_OUT_OF_SYNC;
    }
    CEnumPins* pNew = new (std::nothrow) CEnumPins(m_pFilter, this);   
    *ppEnum = pNew;
    if (pNew == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}


CEnumMediaTypes::CEnumMediaTypes(CBasePin* pPin, CEnumMediaTypes* pEnum)
{
    m_pPin     = pPin;
    m_Position = 0;
    m_cRef     = 1;
    static_cast<IPin*>(m_pPin)->AddRef();
    if (pEnum) {
        m_Position = pEnum->m_Position;
        m_Version  = pEnum->m_Version;
    } else {
        m_Version = m_pPin->GetMediaTypeVersion();      
    }
}

CEnumMediaTypes::~CEnumMediaTypes()
{
    static_cast<IPin*>(m_pPin)->Release();              
}

STDMETHODIMP CEnumMediaTypes::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == NULL)
        return E_POINTER;
    if (MMDxShow_IsEqualGUID16(&riid, &MMDXSHOW_IID_IEnumMediaTypes) ||
        MMDxShow_IsEqualGUID16(&riid, &IID_IUnknown))
        return MMDxShow_ReturnSelf(this, ppv);
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumMediaTypes::AddRef()
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumMediaTypes::Release()
{
    const LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
        return 0;
    }
    return (ULONG)cRef;
}

STDMETHODIMP CEnumMediaTypes::Next(ULONG cMediaTypes, AM_MEDIA_TYPE** ppMediaTypes,
                                   ULONG* pcFetched)
{
    if (ppMediaTypes == NULL)
        return E_POINTER;
    if (m_pPin->GetMediaTypeVersion() != m_Version)     
        return VFW_E_ENUM_OUT_OF_SYNC;
    if (pcFetched)
        *pcFetched = 0;
    else if (cMediaTypes > 1)
        return E_INVALIDARG;

    ULONG nFetched = 0;
    while (cMediaTypes) {
        AM_MEDIA_TYPE mt;                               
        MMDxShow_InitMediaType(&mt);
        const LONG nType = m_Position;
        ++m_Position;
        if (m_pPin->GetMediaType(nType, &mt) != 0) {    
            MMDxShow_FreeMediaType(&mt);                
            break;
        }
        AM_MEDIA_TYPE* pmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        *ppMediaTypes = pmt;
        if (pmt == NULL) {
            MMDxShow_FreeMediaType(&mt);                
            break;
        }
        ++ppMediaTypes;
        ++nFetched;
        memcpy(pmt, &mt, sizeof(AM_MEDIA_TYPE));
        mt.pUnk = NULL;
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
        --cMediaTypes;
        MMDxShow_FreeMediaType(&mt);                    
    }

    if (pcFetched)
        *pcFetched = nFetched;
    return cMediaTypes ? S_FALSE : S_OK;
}

STDMETHODIMP CEnumMediaTypes::Skip(ULONG cMediaTypes)
{
    if (cMediaTypes == 0)
        return S_OK;
    if (m_pPin->GetMediaTypeVersion() != m_Version)
        return VFW_E_ENUM_OUT_OF_SYNC;

    m_Position += (LONG)cMediaTypes;

    AM_MEDIA_TYPE mt;
    MMDxShow_InitMediaType(&mt);
    const BOOL bPastEnd = (m_pPin->GetMediaType(m_Position - 1, &mt) != 0);
    MMDxShow_FreeMediaType(&mt);
    return bPastEnd ? S_FALSE : S_OK;
}

STDMETHODIMP CEnumMediaTypes::Reset()
{
    m_Position = 0;
    m_Version  = m_pPin->GetMediaTypeVersion();
    return S_OK;
}

STDMETHODIMP CEnumMediaTypes::Clone(IEnumMediaTypes** ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;
    if (m_pPin->GetMediaTypeVersion() != m_Version) {
        *ppEnum = NULL;
        return VFW_E_ENUM_OUT_OF_SYNC;
    }
    CEnumMediaTypes* pNew = new (std::nothrow) CEnumMediaTypes(m_pPin, this); 
    *ppEnum = pNew;
    if (pNew == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}
