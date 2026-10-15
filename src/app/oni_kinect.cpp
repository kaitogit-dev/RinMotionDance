#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#include <io.h>

#include <cstdint>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

static IDirect3DDevice9* OniRenderDevice(MMDApp* app) {
    D3DRenderer* r = app->Renderer();
    if (r == nullptr)
        return nullptr;
    return r->device;  
}

namespace {

const char* const kOniExports[7] = {
    "?OpenNIInit@@YG_NPAUHWND__@@_NPAUIDirect3DDevice9@@PA_WPAD@Z",
    "?OpenNIClean@@YGXXZ",
    "?OpenNIDrawDepthMap@@YGX_N@Z",
    "?OpenNIDepthTexture@@YGXPAPAUIDirect3DTexture9@@@Z",
    "?OpenNIGetSkeltonJointPosition@@YGXHPAUD3DXVECTOR3@@@Z",
    "?OpenNIIsTracking@@YGXPA_N@Z",
    "?OpenNIGetVersion@@YGXPAM@Z",
};

void KinMessageBox(MMDApp* app, const char* en, const char* jp) {
    MessageBoxA(static_cast<HWND>(app->Hwnd()),
                app->EnglishUI() != 0 ? en : jp, "Kinect", MB_OK);  
}

using OpenNiInitFn = unsigned char(__stdcall*)(HWND, unsigned char,
                                               IDirect3DDevice9*,
                                               const wchar_t*, const char*);

const char kJpOniNotFound[] =
    "\x44\x78\x4f\x70\x65\x6e\x4e\x49\x2e\x64\x6c\x6c\x82\xaa\x44\x61"
    "\x74\x61\x83\x74\x83\x48\x83\x8b\x83\x5f\x93\xe0\x82\xc9\x82\xa0"
    "\x82\xe8\x82\xdc\x82\xb9\x82\xf1\x81\x42\x0a\x0a\x68\x74\x74\x70"
    "\x3a\x2f\x2f\x77\x77\x77\x2e\x67\x65\x6f\x63\x69\x74\x69\x65\x73"
    "\x2e\x6a\x70\x2f\x68\x69\x67\x75\x63\x68\x75\x75\x34\x2f\x20\x82"
    "\xe6\x82\xe8\x44\x78\x4f\x70\x65\x6e\x4e\x49\x2e\x64\x6c\x6c\x82"
    "\xf0\x83\x5f\x83\x45\x83\x93\x83\x8d\x81\x5b\x83\x68\x82\xb5\x82"
    "\xc4\x0a\x44\x61\x74\x61\x83\x74\x83\x48\x83\x8b\x83\x5f\x93\xe0"
    "\x82\xc9\x92\x75\x82\xa2\x82\xc4\x89\xba\x82\xb3\x82\xa2\x81\x42";
const char kJpOniCannotMove[] =
    "\x4f\x70\x65\x6e\x4e\x49\x82\xaa\x93\xae\x8d\xec\x82\xb5\x82\xdc"
    "\x82\xb9\x82\xf1\x81\x42\x82\xa8\x82\xbb\x82\xe7\x82\xad\x4f\x70"
    "\x65\x6e\x4e\x49\x82\xcc\x83\x43\x83\x93\x83\x58\x83\x67\x81\x5b"
    "\x83\x8b\x82\xc9\x8e\xb8\x94\x73\x82\xb5\x82\xc4\x82\xa2\x82\xdc"
    "\x82\xb7\x81\x42\x0a\x4f\x70\x65\x6e\x4e\x49\x82\xcc\x83\x54\x83"
    "\x93\x83\x76\x83\x8b\x81\x41\x4e\x69\x55\x73\x65\x72\x54\x72\x61"
    "\x63\x6b\x65\x72\x82\xaa\x90\xb3\x82\xb5\x82\xad\x93\xae\x8d\xec"
    "\x82\xb7\x82\xe9\x82\xe6\x82\xa4\x82\xc9\x4f\x70\x65\x6e\x4e\x49"
    "\x82\xf0\x83\x43\x83\x93\x83\x58\x83\x67\x81\x5b\x83\x8b\x82\xb5"
    "\x92\xbc\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2\x81\x42";
const char kJpOniVersion130[] =
    "\x44\x78\x4f\x70\x65\x6e\x4e\x49\x2e\x64\x6c\x6c\x82\xcc\x83\x6f"
    "\x81\x5b\x83\x57\x83\x87\x83\x93\x82\xaa\x88\xd9\x82\xc8\x82\xe8"
    "\x82\xdc\x82\xb7\x0a\x28\x83\x6f\x81\x5b\x83\x57\x83\x87\x83\x93"
    "\x31\x2e\x33\x30\x82\xaa\x95\x4b\x97\x76\x82\xc5\x82\xb7\x29";
const char kJpOniVersionRange[] =
    "\x44\x78\x4f\x70\x65\x6e\x4e\x49\x2e\x64\x6c\x6c\x82\xcc\x83\x6f"
    "\x81\x5b\x83\x57\x83\x87\x83\x93\x82\xaa\x88\xd9\x82\xc8\x82\xe8"
    "\x82\xdc\x82\xb7\x0a\x28\x83\x6f\x81\x5b\x83\x57\x83\x87\x83\x93"
    "\x31\x2e\x33\x30\x81\x60\x31\x2e\x35\x30\x82\xdc\x82\xc5\x82\xcc"
    "\x82\xc7\x82\xea\x82\xa9\x29";

}  

void OpenNiInit(MMDApp* app, const char* sjisPath) {
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());                     

    SetCurrentDirectoryW(s.state.exeDir);   

    int fd = -1;
    if (_wsopen_s(&fd, L"Data\\DxOpenNI.dll",
                  _O_BINARY  , _SH_DENYNO  ,
                  _S_IWRITE  ) != 0) {
        KinMessageBox(app,
            "DxOpenNI.dll cannot find in Data folder.\n\n"
            "Please download it from http://www.geocities.jp/higuchuu4/"
            "index_e.htm \nand set it into data folder.",           
            kJpOniNotFound);                                       
        return;
    }
    _close(fd);                                                    

    HMODULE module = LoadLibraryA("Data\\DxOpenNI.dll");           
    s.state.oniModule = module;
    if (module == nullptr) {
        ShowWin32ErrorMessage(app, "GetProcAddress", GetLastError());
        KinMessageBox(app,
            "Cannot move OpenNI.\nOpenNI might be failed install.", 
            kJpOniCannotMove);                                      
        return;
    }

    for (int i = 0; i < 7; ++i)
        s.OniExportSlot(i) =
            reinterpret_cast<void*>(GetProcAddress(module, kOniExports[i]));
    bool anyNull = false;
    for (int i = 0; i < 7; ++i)
        anyNull = anyNull || s.OniExportSlot(i) == nullptr;
    if (anyNull) {
        KinMessageBox(app,
            "\"DxOpenNI.dll\" version is wrong.\n"
            "(Version1.30 is necessary)",                           
            kJpOniVersion130);                                      
        FreeLibrary(module);                                        
        s.state.oniModule = nullptr;
        return;
    }

    auto getVersion = reinterpret_cast<void(__stdcall*)(float*)>(
        s.state.oniExportSlot6);
    float version = 0.0f;
    getVersion(&version);
    std::uint8_t versionCode;
    if (version == 1.3f) {                                          
        versionCode = 0x0D;
    } else if (version == 1.4f) {                                  
        versionCode = 0x0E;
    } else if (version == 1.5f) {                                  
        versionCode = 0x0F;
    } else {
        KinMessageBox(app,
            "\"DxOpenNI.dll\" version is wrong.\n"
            "(Version1.30 or 1.40 or 1.50 is necessary)",           
            kJpOniVersionRange);                                    
        FreeLibrary(module);
        s.state.oniModule = nullptr;
        return;
    }
    s.state.openniVersion = versionCode;

    auto init = reinterpret_cast<OpenNiInitFn>(
        s.state.oniExportSlot0);
    const unsigned char ok = init(hwnd, s.EnglishUI(), OniRenderDevice(app),
                                  s.state.exeDir,
                                  sjisPath);
    if (ok == 0) {
        FreeLibrary(module);                                        
        s.state.oniModule = nullptr;
        return;
    }

    CheckMenuItem(GetMenu(hwnd), 0x123, MF_CHECKED);                
    s.state.depthDeviceEnabled = 1;                   

    if (s.state.optflag[0] == 0) {         
        unsigned char* model = s.SelectedModel();
        ModelInitMorphSlots(model);                                 
    }

    s.state.fpsLimitSaved =
        s.state.fpsLimit;
    if (sjisPath != nullptr) {
        s.state.kinectCaptureActive = 1;
        s.state.fpsLimit = 30.0f;              
    }
    s.state.autoRepeat = 0;                   
}

void DisableKinect(MMDApp* app) {
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());

    CheckMenuItem(GetMenu(hwnd), 0x123, MF_UNCHECKED);              
    EnableMenuItem(GetMenu(hwnd), 0x124, 1);                        
    CheckMenuItem(GetMenu(hwnd), 0x124, MF_UNCHECKED);              

    auto clean = reinterpret_cast<void(__stdcall*)()>(
        s.state.oniExportSlot1);
    clean();                                                        

    s.state.depthDeviceEnabled = 0;                   
    s.state.autoRepeat = 0;                   

    unsigned char* model = s.SelectedModel();
    if (model != nullptr)                                           
        SeekModelFrame(model, s.state.currentFrame,
                  s.PlaybackPhysicsMode());

    s.state.fpsLimit =                         
        s.state.fpsLimitSaved;
    s.PhysicsResetPending() = 1;                                   
    s.state.kinectCaptureActive = 0;                   
}

}  
