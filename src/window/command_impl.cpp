#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

HWND MainHwnd(MMDApp* app) {
    return static_cast<HWND>(app->Hwnd());
}

template <typename Key>
bool AnyAllocatedKey(const Key* keys, std::size_t count) {
    if (keys == nullptr)
        return false;
    for (std::size_t i = 0; i < count; ++i) {
        if (keys[i].allocated != 0)
            return true;
    }
    return false;
}

bool RememberFileDialogDirs(MMDApp* app) {
    return (GetMenuState(GetMenu(MainHwnd(app)), 0x12D, 0) & 8) != 0;
}

const char kCaptionSavePoseJp[] =
    "\x83\x7C\x81\x5B\x83\x59\x83\x66\x81\x5B\x83\x5E"  
    "\x95\xDB\x91\xB6";                                  
const char kMsgSavePoseJp[] =
    "\x83\x7C\x81\x5B\x83\x59\x83\x66\x81\x5B\x83\x5E\x82\xCD\x91\x49\x91\xF0"
    "\x82\xB3\x82\xEA\x82\xBD\x83\x7B\x81\x5B\x83\x93\x82\xCC\x82\xDD"
    "\x82\xAA\x83\x5A\x81\x5B\x83\x75\x82\xB3\x82\xEA\x82\xDC\x82\xB7\n"
    "\x83\x7B\x81\x5B\x83\x93\x82\xCC\x88\xEA\x95\x94\x81\x41\x82\xE0"
    "\x82\xB5\x82\xAD\x82\xCD\x91\x53\x82\xC4\x82\xF0\x91\x49\x91\xF0"
    "\x82\xB5\x82\xC4\x82\xAD\x82\xBE\x82\xB3\x82\xA2";
const char kCaptionSaveMotionJp[] =
    "\x83\x82\x81\x5B\x83\x56\x83\x87\x83\x93\x83\x66\x81\x5B\x83\x5E"  
    "\x95\xDB\x91\xB6";                                                 
const char kMsgSaveMotionJp[] =
    "\x83\x82\x81\x5B\x83\x56\x83\x87\x83\x93\x83\x66\x81\x5B\x83\x5E"
    "\x82\xCD\x91\x49\x91\xF0\x82\xB3\x82\xEA\x82\xBD"
    "\x83\x74\x83\x8C\x81\x5B\x83\x80\x83\x7C\x83\x43\x83\x93\x83\x67"
    "\x82\xAA\x83\x5A\x81\x5B\x83\x75\x82\xB3\x82\xEA\x82\xDC\x82\xB7\n"
    "\x83\x74\x83\x8C\x81\x5B\x83\x80\x82\xCC\x88\xEA\x95\x94\x81\x41"
    "\x82\xE0\x82\xB5\x82\xAD\x82\xCD\x91\x53\x82\xC4\x82\xF0\x91\x49"
    "\x91\xF0\x82\xB5\x82\xC4\x82\xAD\x82\xBE\x82\xB3\x82\xA2";
const char kCaptionNewJp[] =
    "\x90\x56\x8B\x4B\x8D\xEC\x90\xAC";
const char kMsgNewJp[] =
    "\x8C\xBB\x8D\xDD\x82\xCC\x8F\xF3\x91\xD4\x82\xCD\x94\x6A\x8A\xFC"
    "\x82\xB3\x82\xEA\x82\xDC\x82\xB7\n\n"
    "\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5\x82\xB7\x82\xA9\x81\x48";
const char kCaptionOpenFileJp[] =
    "\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xF0\x8A\x4A\x82\xAD";
const char kMsgOpenDirtyJp[] =
    "\x95\xDB\x91\xB6\x82\xB5\x82\xC4\x82\xA2\x82\xC8\x82\xA2\x95\xCF\x8D\x58"
    "\x93\x5F\x82\xAA\x82\xA0\x82\xE8\x82\xDC\x82\xB7\n\n"
    "\x82\xB1\x82\xCC\x82\xDC\x82\xDC\x83\x8D\x81\x5B\x83\x68\x82\xB5"
    "\x82\xC4\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5\x82\xB7\x82\xA9"
    "\x81\x48";

const wchar_t kTitleLoadPoseJp[] =
    L"\x30DD\x30FC\x30BA\x30C7\x30FC\x30BF\x8AAD\x8FBC";
const wchar_t kTitleSavePoseJp[] =
    L"\x30DD\x30FC\x30BA\x30C7\x30FC\x30BF\x4FDD\x5B58";
const wchar_t kTitleLoadMotionJp[] =
    L"\x30E2\x30FC\x30B7\x30E7\x30F3\x30C7\x30FC\x30BF\x8AAD\x8FBC";
const wchar_t kTitleSaveMotionJp[] =
    L"\x30E2\x30FC\x30B7\x30E7\x30F3\x30C7\x30FC\x30BF\x4FDD\x5B58";
const wchar_t kTitleOpenFileJp[] =
    L"\x30D5\x30A1\x30A4\x30EB\x3092\x8AAD\x8FBC";
const wchar_t kTitleSaveFileJp[] =
    L"\x30D5\x30A1\x30A4\x30EB\x3092\x4FDD\x5B58\x3059\x308B";
const wchar_t kTitleOpenWaveJp[] = L"WAVE File";
const wchar_t kTitleLoadAviJp[] = L"AVI";

const wchar_t kFmtEmptyPath[] = L"\x0\x0%s%s";

constexpr DWORD kDialogPathLen = 0x100;

}  

void CmdLoadPose(MMDApp* app) {
    app->state.dialogFlags[12] = 1;             
    app->state.enterKeyState = 1;               
    if (app->state.optflag[0] != 0) {           
        return;
    }
    SetCurrentDirectoryW(app->ExeDir());        
    wchar_t path[kDialogPathLen];
    swprintf_s(path, kDialogPathLen, kFmtEmptyPath, L"", L"");
    wchar_t fileTitle[kDialogPathLen];
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app->FloatingWindow() != nullptr  
                        ? app->FloatingWindow()
                        : MainHwnd(app);
    ofn.lpstrFilter = L"Vocaloid Pose Data files(*.vpd)\0*.vpd\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = kDialogPathLen;
    ofn.Flags = OFN_FILEMUSTEXIST;              
    ofn.lpstrInitialDir = RememberFileDialogDirs(app)
                              ? app->DirPose()  
                              : L"UserFile\\Pose";
    ofn.lpstrDefExt = L"vpd";
    ofn.nMaxFileTitle = kDialogPathLen;
    ofn.lpstrFileTitle = fileTitle;
    ofn.lpstrTitle = app->EnglishUI() != 0
                         ? L"load pose data"
                         : kTitleLoadPoseJp;
    if (!GetOpenFileNameW(&ofn)) {
        return;
    }
    if (RememberFileDialogDirs(app)) {          
        wchar_t* dir = ExtractDirFromPath(
            app->PathWorkspace().projectDirectory, path);
        CopyDirPathW(app->DirPose(), dir);      
    }
    LoadVpdFile(path);                          
    app->PhysicsResetPending() = 1;             
}

void CmdSavePose(MMDApp* app) {
    if (app->state.optflag[0] != 0) {           
        return;
    }
    bool anySelected = false;
    unsigned char* const model = app->SelectedModel();
    if (model != nullptr) {
        const int count = static_cast<int>(mdl::Mdl(model)->boneCount);
        unsigned char* const selBits = mdl::Mdl(model)->boneSelection;
        if (selBits != nullptr) {
            for (int b = 0; b < count; ++b) {
                if (selBits[b] != 0) {
                    anySelected = true;
                    break;
                }
            }
        }
    }
    app->state.enterKeyState = 1;               
    if (!anySelected) {
        MessageBoxA(MainHwnd(app),
                    app->EnglishUI() != 0
                        ? "Pose data will be saved only selected bone.\n"
                          "Please select bone."
                        : kMsgSavePoseJp,
                    app->EnglishUI() != 0 ? "save pose data" : kCaptionSavePoseJp,
                    MB_TOPMOST);                
        return;
    }
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[kDialogPathLen];
    swprintf_s(path, kDialogPathLen, kFmtEmptyPath, L"", L"");
    wchar_t fileTitle[kDialogPathLen];
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = MainHwnd(app);              
    ofn.lpstrFilter = L"Vocaloid Pose Data files(*.vpd)\0*.vpd\0";
    ofn.lpstrFile = path;
    ofn.nFilterIndex = 1;
    ofn.nMaxFile = kDialogPathLen;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;  
    ofn.lpstrInitialDir = RememberFileDialogDirs(app)
                              ? app->DirPose()
                              : L"UserFile\\Pose";
    ofn.lpstrDefExt = L"vpd";
    ofn.nMaxFileTitle = kDialogPathLen;
    ofn.lpstrFileTitle = fileTitle;
    ofn.lpstrTitle = app->EnglishUI() != 0
                         ? L"save pose data"
                         : kTitleSavePoseJp;
    if (!GetSaveFileNameW(&ofn)) {
        return;
    }
    if (RememberFileDialogDirs(app)) {
        wchar_t* dir = ExtractDirFromPath(
            app->PathWorkspace().projectDirectory, path);
        CopyDirPathW(app->DirPose(), dir);
    }
    SaveVpdFile(path);                          
}

void CmdResetState(MMDApp* app) {
    app->state.enterKeyState = 1;               
    const int r = MessageBoxA(
        MainHwnd(app),
        app->EnglishUI() != 0
            ? "The existing state will be annulled.\n\nAre you OK?"
            : kMsgNewJp,
        app->EnglishUI() != 0 ? "new" : kCaptionNewJp,
        MB_OKCANCEL | MB_TOPMOST);              
    if (r == IDOK) {
        ResetAppState(app);                     
    }
}

void CmdOpenScene(MMDApp* app) {
    app->state.enterKeyState = 1;               
    if (app->SceneModified() != 0) {            
        const int r = MessageBoxA(
            MainHwnd(app),
            app->EnglishUI() != 0
                ? "There is a change point not preserved.\n\nAre you OK?"
                : kMsgOpenDirtyJp,
            app->EnglishUI() != 0 ? "open file data" : kCaptionOpenFileJp,
            MB_OKCANCEL | MB_TOPMOST);          
        if (r != IDOK) {
            return;
        }
    }
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[kDialogPathLen];
    swprintf_s(path, kDialogPathLen, kFmtEmptyPath, L"", L"");
    wchar_t fileTitle[kDialogPathLen];
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app->FloatingWindow() != nullptr
                        ? app->FloatingWindow()
                        : MainHwnd(app);
    ofn.lpstrFilter = L"PolygonMovieMaker files(*.pmm)\0*.pmm\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = kDialogPathLen;
    ofn.Flags = OFN_FILEMUSTEXIST;              
    ofn.lpstrInitialDir = RememberFileDialogDirs(app)
                              ? app->DirUser()  
                              : L"UserFile";
    ofn.lpstrDefExt = L"pmm";
    ofn.nMaxFileTitle = kDialogPathLen;
    ofn.lpstrFileTitle = fileTitle;
    ofn.lpstrTitle = app->EnglishUI() != 0
                         ? L"open file data"
                         : kTitleOpenFileJp;
    if (!GetOpenFileNameW(&ofn)) {
        return;
    }
    if (RememberFileDialogDirs(app)) {
        wchar_t* dir = ExtractDirFromPath(
            app->PathWorkspace().projectDirectory, path);
        CopyDirPathW(app->DirUser(), dir);
    }
    wcscpy_s(app->EnvFileName(), kDialogPathLen, path);  
    LoadSceneFile();                            
}

void CmdOpenWave(MMDApp* app) {
    app->state.enterKeyState = 1;               
    if (app->DirectSoundAvailable() == 0) {     
        MessageBoxA(
            MainHwnd(app),
            app->EnglishUI() != 0
                ? "You cannot play WAVE because failed initialization of "
                  "DirectSound!"
                : "DirectSound",
            app->EnglishUI() != 0 ? "DirectSound error" : "DirectSound",
            0);
        return;
    }
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[kDialogPathLen];
    swprintf_s(path, kDialogPathLen, kFmtEmptyPath, L"", L"");
    wchar_t fileTitle[kDialogPathLen];
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app->FloatingWindow() != nullptr
                        ? app->FloatingWindow()
                        : MainHwnd(app);
    ofn.lpstrFilter = L"Wave files(*.wav)\0*.wav\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = kDialogPathLen;
    ofn.Flags = OFN_FILEMUSTEXIST;              
    ofn.lpstrInitialDir = RememberFileDialogDirs(app)
                              ? app->DirWave()  
                              : L"UserFile\\Wave";
    ofn.lpstrDefExt = L"wav";
    ofn.nMaxFileTitle = kDialogPathLen;
    ofn.lpstrFileTitle = fileTitle;
    ofn.lpstrTitle = app->EnglishUI() != 0
                         ? L"open WAVE File"
                         : kTitleOpenWaveJp;
    if (!GetOpenFileNameW(&ofn)) {
        return;
    }
    wcscpy_s(app->WavePath(), kDialogPathLen, path);  
    if (RememberFileDialogDirs(app)) {
        wchar_t* dir = ExtractDirFromPath(
            app->PathWorkspace().projectDirectory, path);
        CopyDirPathW(app->DirWave(), dir);
    }
    LoadWaveFile(app);                          
    app->SceneModified() = 1;                   
}

void CmdSaveScene(MMDApp* app) {
    app->state.dialogFlags[5] = 1;              
    app->state.enterKeyState = 1;               
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[kDialogPathLen];
    swprintf_s(path, kDialogPathLen, kFmtEmptyPath, L"", L"");
    wchar_t fileTitle[kDialogPathLen];
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = MainHwnd(app);              
    ofn.lpstrFilter = L"PolygonMovieMaker files(*.pmm)\0*.pmm\0";
    ofn.lpstrFile = path;
    ofn.nFilterIndex = 1;
    ofn.nMaxFile = kDialogPathLen;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;  
    ofn.lpstrInitialDir = RememberFileDialogDirs(app)
                              ? app->DirUser()
                              : L"UserFile";
    ofn.lpstrDefExt = L"pmm";
    ofn.nMaxFileTitle = kDialogPathLen;
    ofn.lpstrFileTitle = fileTitle;
    ofn.lpstrTitle = app->EnglishUI() != 0
                         ? L"save file"
                         : kTitleSaveFileJp;
    if (!GetSaveFileNameW(&ofn)) {
        return;
    }
    if (RememberFileDialogDirs(app)) {
        wchar_t* dir = ExtractDirFromPath(
            app->PathWorkspace().projectDirectory, path);
        CopyDirPathW(app->DirUser(), dir);
    }
    wcscpy_s(app->EnvFileName(), kDialogPathLen, path);  
    SaveSceneFile(app);                         
}

void CmdLoadMotion(MMDApp* app) {
    app->state.enterKeyState = 1;               
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[kDialogPathLen];
    swprintf_s(path, kDialogPathLen, kFmtEmptyPath, L"", L"");
    wchar_t fileTitle[kDialogPathLen];
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app->FloatingWindow() != nullptr
                        ? app->FloatingWindow()
                        : MainHwnd(app);
    ofn.lpstrFilter = L"Vocaloid Motion Data files(*.vmd)\0*.vmd\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = kDialogPathLen;
    ofn.Flags = OFN_FILEMUSTEXIST;              
    ofn.lpstrInitialDir = RememberFileDialogDirs(app)
                              ? app->DirMotion()  
                              : L"UserFile\\Motion";
    ofn.lpstrDefExt = L"vmd";
    ofn.nMaxFileTitle = kDialogPathLen;
    ofn.lpstrFileTitle = fileTitle;
    ofn.lpstrTitle = app->EnglishUI() != 0
                         ? L"load motion data"
                         : kTitleLoadMotionJp;
    if (!GetOpenFileNameW(&ofn)) {
        return;
    }
    if (RememberFileDialogDirs(app)) {
        wchar_t* dir = ExtractDirFromPath(
            app->PathWorkspace().projectDirectory, path);
        CopyDirPathW(app->DirMotion(), dir);
    }
    LoadVmdFile(path);                          
    app->SceneModified() = 1;                   
    app->PhysicsResetPending() = 1;             
}

void CmdSaveMotion(MMDApp* app) {
    app->state.dialogFlags[17] = 1;             
    app->state.enterKeyState = 1;               
    auto anySelectedByte = [](unsigned char* table, std::size_t off,
                              std::size_t stride, int cap) -> bool {
        if (table == nullptr) {
            return false;
        }
        for (int i = 0; i < cap; ++i) {
            if (table[off + stride * i] != 0) {
                return true;
            }
        }
        return false;
    };
    auto anyAllocatedKey = [](const auto* keys, std::size_t count) {
        return AnyAllocatedKey(keys, count);
    };
    bool any;
    if (app->state.optflag[0] != 0) {
        any = anySelectedByte(reinterpret_cast<unsigned char*>(
                                  app->state.cameraKeyTrack), 0x48, 0x54, 10000) ||
              anySelectedByte(reinterpret_cast<unsigned char*>(
                                  app->state.lightKeyTrack), 0x24, 0x28, 10000) ||
              anySelectedByte(reinterpret_cast<unsigned char*>(
                                  app->state.selfShadowKeyTrack), 0x14, 0x18, 10000);
    } else {
        any = false;
        unsigned char* const model = app->SelectedModel();
        if (model != nullptr) {
            any = anyAllocatedKey(mdl::BoneKeys(model), mdl::kBoneKeyCapacity) ||
                  anyAllocatedKey(mdl::MorphKeys(model), mdl::kMorphKeyCapacity) ||
                  anyAllocatedKey(mdl::DisplayKeys(model), mdl::kDisplayKeyCapacity);
        }
    }
    if (!any) {
        MessageBoxA(MainHwnd(app),
                    app->EnglishUI() != 0
                        ? "Motion data will be saved only selected frame point.\n"
                          "Please select frame point."
                        : kMsgSaveMotionJp,
                    app->EnglishUI() != 0 ? "save motion data"
                                          : kCaptionSaveMotionJp,
                    MB_TOPMOST);                
        return;
    }
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[kDialogPathLen];
    swprintf_s(path, kDialogPathLen, kFmtEmptyPath, L"", L"");
    wchar_t fileTitle[kDialogPathLen];
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = MainHwnd(app);              
    ofn.lpstrFilter = L"Vocaloid Motion Data files(*.vmd)\0*.vmd\0";
    ofn.lpstrFile = path;
    ofn.nFilterIndex = 1;
    ofn.nMaxFile = kDialogPathLen;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;  
    ofn.lpstrInitialDir = RememberFileDialogDirs(app)
                              ? app->DirMotion()
                              : L"UserFile\\Motion";
    ofn.lpstrDefExt = L"vmd";
    ofn.nMaxFileTitle = kDialogPathLen;
    ofn.lpstrFileTitle = fileTitle;
    ofn.lpstrTitle = app->EnglishUI() != 0
                         ? L"save motion data"
                         : kTitleSaveMotionJp;
    if (!GetSaveFileNameW(&ofn)) {
        return;
    }
    if (RememberFileDialogDirs(app)) {
        wchar_t* dir = ExtractDirFromPath(
            app->PathWorkspace().projectDirectory, path);
        CopyDirPathW(app->DirMotion(), dir);
    }
    SaveVmdFile(path);                          
}

void CmdLoadAvi(MMDApp* app) {
    app->state.dialogFlags[17] = 1;             
    app->state.enterKeyState = 1;               
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[kDialogPathLen];
    swprintf_s(path, kDialogPathLen, kFmtEmptyPath, L"", L"");
    wchar_t fileTitle[kDialogPathLen];
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app->FloatingWindow() != nullptr
                        ? app->FloatingWindow()
                        : MainHwnd(app);
    ofn.lpstrFilter = L"avi files(*.avi)\0*.avi\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = kDialogPathLen;
    ofn.Flags = OFN_FILEMUSTEXIST;              
    ofn.lpstrInitialDir = RememberFileDialogDirs(app)
                              ? app->DirBg()    
                              : L"UserFile\\BackGround";
    ofn.lpstrDefExt = L"avi";
    ofn.nMaxFileTitle = kDialogPathLen;
    ofn.lpstrFileTitle = fileTitle;
    ofn.lpstrTitle = app->EnglishUI() != 0
                         ? L"load AVI file"
                         : kTitleLoadAviJp;
    if (!GetOpenFileNameW(&ofn)) {
        return;
    }
    if (RememberFileDialogDirs(app)) {
        wchar_t* dir = ExtractDirFromPath(
            app->PathWorkspace().projectDirectory, path);
        CopyDirPathW(app->DirBg(), dir);
    }
    wcscpy_s(app->AviBackgroundPath(), kDialogPathLen, path);  
    LoadAviFile(app);                           
    app->SceneModified() = 1;                   
}

}  
