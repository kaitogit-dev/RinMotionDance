
#include <initguid.h>       

#include "mmdxshow.hpp"


DEFINE_GUID(MMDXSHOW_MEDIATYPE_Video,
    0x73646976, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(MMDXSHOW_GUID_NULL,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
DEFINE_GUID(MMDXSHOW_CLSID_FilterMapper,
    0xe436ebb2, 0x524f, 0x11ce, 0x9f, 0x53, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70);
DEFINE_GUID(MMDXSHOW_IID_IFilterMapper,
    0x56a868a3, 0xad4, 0x11ce, 0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70);
DEFINE_GUID(MMDXSHOW_CLSID_FilterMapper2,
    0xcda42200, 0xbd88, 0x11d0, 0xbd, 0x4e, 0x0, 0xa0, 0xc9, 0x11, 0xce, 0x86);
DEFINE_GUID(MMDXSHOW_IID_IFilterMapper2,
    0xb79bb0b0, 0x33c1, 0x11d1, 0xab, 0xe1, 0x0, 0xa0, 0xc9, 0x5, 0xf3, 0x75);

static HRESULT AMovieDllRegisterServer2(BOOL bRegister);   
static void    AMovieInitServer(BOOL bLoading);            


LONG           g_cModuleRef   = 0;   
LONG           g_cServerLocks = 0;   
HINSTANCE      g_hInst        = NULL;
DWORD          g_amPlatform   = 0;   
OSVERSIONINFOA g_osVer        = {};  
HMODULE        g_hOle32Lib    = NULL;

void MMDxShow_LockModule(void)
{
    InterlockedIncrement(&g_cModuleRef);
}

void MMDxShow_UnlockModule(void)
{
    if (InterlockedDecrement(&g_cModuleRef) == 0 && g_hOle32Lib != NULL) {
        FreeLibrary(g_hOle32Lib);
        g_hOle32Lib = NULL;
    }
}

bool MMDxShow_IsEqualGUID16(const GUID* a, const GUID* b)
{
    const DWORD* p = reinterpret_cast<const DWORD*>(a);
    const DWORD* q = reinterpret_cast<const DWORD*>(b);
    for (unsigned i = 4; i; --i) {
        if (*p != *q) {
            const BYTE* x = reinterpret_cast<const BYTE*>(a);
            const BYTE* y = reinterpret_cast<const BYTE*>(b);
            for (unsigned j = 0; j < 16; ++j)
                if (x[j] != y[j]) return false;
            return true;
        }
        ++p; ++q;
    }
    return true;
}

HRESULT MMDxShow_ReturnSelf(void* pSelf, void** ppv)
{
    if (ppv == NULL)
        return E_POINTER;
    *ppv = pSelf;
    reinterpret_cast<IUnknown*>(pSelf)->AddRef();
    return S_OK;
}


void MMDxShow_InitMediaType(AM_MEDIA_TYPE* pmt)
{
    ZeroMemory(pmt, sizeof(AM_MEDIA_TYPE));
    pmt->bFixedSizeSamples = TRUE;
    pmt->lSampleSize       = 1;
}

void MMDxShow_FreeMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt->cbFormat) {
        CoTaskMemFree(pmt->pbFormat);
        pmt->cbFormat = 0;
        pmt->pbFormat = NULL;
    }
    if (pmt->pUnk != NULL) {
        pmt->pUnk->Release();
        pmt->pUnk = NULL;
    }
}

HRESULT MMDxShow_CopyMediaType(AM_MEDIA_TYPE* pmtDst, const AM_MEDIA_TYPE* pmtSrc)
{
    memcpy(pmtDst, pmtSrc, sizeof(AM_MEDIA_TYPE));
    if (pmtSrc->cbFormat) {
        pmtDst->pbFormat = (BYTE*)CoTaskMemAlloc(pmtSrc->cbFormat);
        if (pmtDst->pbFormat == NULL) {
            pmtDst->cbFormat = 0;
            return E_OUTOFMEMORY;
        }
        memcpy(pmtDst->pbFormat, pmtSrc->pbFormat, pmtSrc->cbFormat);
    }
    if (pmtDst->pUnk != NULL)
        pmtDst->pUnk->AddRef();
    return S_OK;
}


static const MMDXSHOW_MEDIATYPE_SETUP s_MediaTypeSetup =
{
    &MMDXSHOW_MEDIATYPE_Video,
    &MMDXSHOW_GUID_NULL,
};

static const MMDXSHOW_PIN_SETUP s_PinSetup =
{
    L"Output",      
    FALSE,          
    TRUE,           
    FALSE,          
    FALSE,          
    &MMDXSHOW_GUID_NULL,   
    NULL,           
    1,              
    &s_MediaTypeSetup,
};

static const MMDXSHOW_FILTER_SETUP s_FilterSetup =
{
    &MMDXSHOW_CLSID_PushSourceDIBSq,                                
    L"PushSource DIBBitmap Sequence Filter",                        
    0x00200000,                                                     
    1,                                                              
    &s_PinSetup,
};

const CFactoryTemplate g_Templates[1] =
{
    {
        L"PushSource DIBBitmap Sequence Filter",                    
        &MMDXSHOW_CLSID_PushSourceDIBSq,                            
        &MMDxShow_NewPushSourceDIBSq,                               
        NULL,                                                       
        &s_FilterSetup,                                             
    },
};
const int g_cTemplates = 1;                                         


CClassFactory::CClassFactory(const CFactoryTemplate* pTemplate)
{
    MMDxShow_LockModule();     
    m_pTemplate = pTemplate;   
    m_cRef      = 0;           
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    if (MMDxShow_IsEqualGUID16(&riid, &IID_IUnknown) ||
        MMDxShow_IsEqualGUID16(&riid, &IID_IClassFactory))
        return MMDxShow_ReturnSelf(this, ppv);
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return (ULONG)++m_cRef;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    const bool bLast = (m_cRef-- == 1);
    const ULONG cRef = (ULONG)m_cRef;
    if (bLast) {
        MMDxShow_UnlockModule();   
        delete this;
        return 0;
    }
    return cRef;
}

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void** ppv)
{
    if (ppv == NULL)
        return E_POINTER;
    if (pUnkOuter != NULL && !MMDxShow_IsEqualGUID16(&riid, &IID_IUnknown))
        return E_NOINTERFACE;
    *ppv = NULL;

    HRESULT hr;
    CUnknown* pUnknown = m_pTemplate->m_lpfnNew(pUnkOuter, &hr);
    if (pUnknown == NULL) {
        if (hr >= 0)
            hr = E_OUTOFMEMORY;
        return hr;
    }
    if (hr >= 0) {
        pUnknown->NonDelegatingAddRef();                    
        hr = pUnknown->NonDelegatingQueryInterface(riid, ppv); 
        pUnknown->NonDelegatingRelease();                   
    } else {
        delete pUnknown;    
    }
    return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        ++g_cServerLocks;
    else
        --g_cServerLocks;
    return S_OK;
}


extern "C" HRESULT WINAPI DllCanUnloadNow(void)
{
    if (g_cServerLocks > 0 || g_cModuleRef != 0)
        return S_FALSE;
    return S_OK;
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!MMDxShow_IsEqualGUID16(&riid, &IID_IUnknown) &&
        !MMDxShow_IsEqualGUID16(&riid, &IID_IClassFactory))
        return E_NOINTERFACE;

    if (g_cTemplates <= 0)
        return CLASS_E_CLASSNOTAVAILABLE;              

    const CFactoryTemplate* pTemplate = NULL;
    int i = 0;
    for (;; ++i) {
        if (i >= g_cTemplates)
            return CLASS_E_CLASSNOTAVAILABLE;
        if (MMDxShow_IsEqualGUID16(g_Templates[i].m_ClsID, &rclsid)) {
            pTemplate = &g_Templates[i];
            break;
        }
    }

    CClassFactory* pFactory = new CClassFactory(pTemplate);   
    *ppv = pFactory;
    if (pFactory == NULL)
        return E_OUTOFMEMORY;
    pFactory->AddRef();                                       
    return S_OK;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    (void)lpvReserved;
    if (fdwReason == DLL_PROCESS_DETACH) {
        AMovieInitServer(FALSE);      
        return TRUE;
    }
    if (fdwReason != DLL_PROCESS_ATTACH)
        return TRUE;

    DisableThreadLibraryCalls(hinstDLL);

    g_amPlatform = 1;                 
    g_osVer.dwOSVersionInfoSize = sizeof(g_osVer);
    if (GetVersionExA(&g_osVer))
        g_amPlatform = g_osVer.dwPlatformId;

    g_hInst = hinstDLL;

    AMovieInitServer(TRUE);           
    return TRUE;
}

extern "C" HRESULT WINAPI DllRegisterServer(void)
{
    return AMovieDllRegisterServer2(TRUE);    
}

extern "C" HRESULT WINAPI DllUnregisterServer(void)
{
    return AMovieDllRegisterServer2(FALSE);   
}


static LSTATUS RecursiveDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
{
    HKEY hk;
    CHAR szName[260];
    DWORD cchName;
    FILETIME ftLastWriteTime;

    if (lstrlenA(lpSubKey) == 0)
        return (LSTATUS)0x80004005L;      
    if (RegOpenKeyExA(hKey, lpSubKey, 0, KEY_ALL_ACCESS  , &hk) == ERROR_SUCCESS) {
        for (cchName = 260;
             RegEnumKeyExA(hk, 0, szName, &cchName, NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS;
             cchName = 260)
        {
            RecursiveDeleteKeyA(hk, szName);
        }
        RegCloseKey(hk);
        RegDeleteKeyA(hKey, lpSubKey);
    }
    return ERROR_SUCCESS;
}

static HRESULT RegSetClassIdKey(REFCLSID clsid, LPCWSTR wszName, LPCWSTR wszModule,
                                LPCWSTR wszThreadingModel  ,
                                LPCWSTR wszServerKey  )
{
    HKEY hkClsid, hkServer;
    CHAR szKey[260];
    WCHAR wszGuid[40];
    LSTATUS lr;

    StringFromGUID2(clsid, wszGuid, 39);
    wsprintfA(szKey, "CLSID\\%ls", wszGuid);

    lr = RegCreateKeyA(HKEY_CLASSES_ROOT, szKey, &hkClsid);
    if (lr)
        return HRESULT_FROM_WIN32(lr);

    wsprintfA(szKey, "%ls", wszName);
    lr = RegSetValueA(hkClsid, NULL, REG_SZ, szKey, 260);
    if (lr == ERROR_SUCCESS) {
        wsprintfA(szKey, "%ls", wszServerKey);
        lr = RegCreateKeyA(hkClsid, szKey, &hkServer);
        if (lr == ERROR_SUCCESS) {
            wsprintfA(szKey, "%ls", wszModule);
            lr = RegSetValueA(hkServer, NULL, REG_SZ, szKey, lstrlenA(szKey) + 1);
            if (lr == ERROR_SUCCESS) {
                wsprintfA(szKey, "%ls", wszThreadingModel);
                lr = RegSetValueExA(hkServer, "ThreadingModel", 0, REG_SZ,
                                    (const BYTE*)szKey, lstrlenA(szKey) + 1);
            }
            RegCloseKey(hkServer);
        }
    }
    RegCloseKey(hkClsid);
    if (lr > 0)
        return (HRESULT)((WORD)lr | 0x80070000);   
    return (HRESULT)lr;
}

static HRESULT RegRemoveClassIdKey(REFCLSID clsid)
{
    CHAR szKey[260];
    WCHAR wszGuid[40];

    StringFromGUID2(clsid, wszGuid, 39);
    wsprintfA(szKey, "CLSID\\%ls", wszGuid);
    RecursiveDeleteKeyA(HKEY_CLASSES_ROOT, szKey);
    return S_OK;
}

static HRESULT RegisterServerClassIds(LPCWSTR wszModule, BOOL bRegister)
{
    HRESULT hr = S_OK;
    for (int i = 0; i < g_cTemplates; ++i) {
        const CFactoryTemplate* pT = &g_Templates[i];
        if (bRegister)
            hr = RegSetClassIdKey(*pT->m_ClsID, pT->m_Name, wszModule, L"Both", L"InprocServer32");
        else
            hr = RegRemoveClassIdKey(*pT->m_ClsID);
        if (FAILED(hr))
            break;
    }
    return hr;
}

static HRESULT RegisterFilterMapper2(const MMDXSHOW_FILTER_SETUP* pSetup,
                                     IFilterMapper2* pMapper, BOOL bRegister)
{
    if (pSetup == NULL)
        return S_FALSE;                       

    HRESULT hr = pMapper->UnregisterFilter(NULL, NULL, *pSetup->clsID);
    if (bRegister) {
        REGFILTER2 rf2;
        rf2.dwVersion = 1;
        rf2.dwMerit   = pSetup->dwMerit;
        rf2.cPins     = pSetup->nPins;
        rf2.rgPins    = (REGFILTERPINS*)pSetup->lpPin;
        hr = pMapper->RegisterFilter(*pSetup->clsID, pSetup->szName,
                                     NULL, NULL, NULL, &rf2);
    }
    return (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ? S_OK : hr;
}

HRESULT RegisterFilterMapper1(const MMDXSHOW_FILTER_SETUP* pSetup,
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

static HRESULT AMovieDllRegisterServer2(BOOL bRegister)
{
    CHAR  szFile[MAX_PATH];
    WCHAR wszFile[MAX_PATH];
    HRESULT hr;

    if (!GetModuleFileNameA(g_hInst, szFile, MAX_PATH))
        return HRESULT_FROM_WIN32(GetLastError());
    MultiByteToWideChar(CP_ACP, 0, szFile, lstrlenA(szFile) + 1, wszFile, MAX_PATH);

    if (bRegister) {
        hr = RegisterServerClassIds(wszFile, TRUE);
        if (FAILED(hr))
            return hr;
    }

    CoInitialize(NULL);

    IFilterMapper2* pFM2 = NULL;
    IFilterMapper*  pFM1 = NULL;
    hr = CoCreateInstance(MMDXSHOW_CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
                          MMDXSHOW_IID_IFilterMapper2, (void**)&pFM2);
    if (FAILED(hr))
        hr = CoCreateInstance(MMDXSHOW_CLSID_FilterMapper, NULL, CLSCTX_INPROC_SERVER,
                              MMDXSHOW_IID_IFilterMapper, (void**)&pFM1);

    if (SUCCEEDED(hr)) {
        for (int i = 0; i < g_cTemplates; ++i) {
            const MMDXSHOW_FILTER_SETUP* pSetup = g_Templates[i].m_pAMovieSetup_Filter;
            if (pSetup != NULL) {
                if (pFM2 != NULL)
                    hr = RegisterFilterMapper2(pSetup, pFM2, bRegister);
                else
                    hr = RegisterFilterMapper1(pSetup, pFM1, bRegister);
            }
            if (FAILED(hr))
                break;
        }
        IUnknown* pUnk = pFM2 ? (IUnknown*)pFM2 : (IUnknown*)pFM1;
        pUnk->Release();      
    }

    CoFreeUnusedLibraries();
    CoUninitialize();

    if (SUCCEEDED(hr) && !bRegister)
        return RegisterServerClassIds(wszFile, FALSE);
    return hr;
}

static void AMovieInitServer(BOOL bLoading)
{
    for (int i = 0; i < g_cTemplates; ++i) {
        if (g_Templates[i].m_lpfnInit)
            g_Templates[i].m_lpfnInit(bLoading, g_Templates[i].m_ClsID);
    }
}
