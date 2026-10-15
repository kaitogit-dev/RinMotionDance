#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <new>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {


namespace {

const char kJpDirtyText[] =
    "\x95\xdb\x91\xb6\x82\xb5\x82\xc4\x82\xa2\x82\xc8\x82\xa2"
    "\x95\xcf\x8d\x58\x93\x5f\x82\xaa\x82\xa0\x82\xe8\x82\xdc\x82\xb7"
    "\x0a\x0a"
    "\x82\xb1\x82\xcc\x82\xdc\x82\xdc\x83\x8d\x81\x5b\x83\x68"
    "\x82\xb5\x82\xc4\x82\xe6\x82\xeb\x82\xb5\x82\xa2\x82\xc5\x82\xb7\x82\xa9\x81\x48";
const char kJpDirtyCaption[] =
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xf0\x8a\x4a\x82\xad";
const char kJpPoseText[] =
    "\x83\x4a\x83\x81\x83\x89\x81\x45\x8f\xc6\x96\xbe"
    "\x83\x82\x81\x5b\x83\x68\x82\xc5\x82\xcd"
    "\x93\xc7\x82\xdf\x82\xdc\x82\xb9\x82\xf1";
const char kJpPoseCaption[] = "\x83\x7c\x81\x5b\x83\x59\x93\xc7\x8d\x9e";
const char kJpVsqText[] =
    "\x83\x4a\x83\x81\x83\x89\x81\x45\x8f\xc6\x96\xbe"
    "\x83\x82\x81\x5b\x83\x68\x82\xc5\x82\xcd" "vsq"
    "\x83\x66\x81\x5b\x83\x5e\x82\xcd"
    "\x93\xc7\x82\xdf\x82\xdc\x82\xb9\x82\xf1";
const char kJpOniText[] =
    "\x83\x4a\x83\x81\x83\x89\x81\x45\x8f\xc6\x96\xbe"
    "\x83\x82\x81\x5b\x83\x68\x82\xc5\x82\xcd" "oni"
    "\x83\x66\x81\x5b\x83\x5e\x82\xcd"
    "\x93\xc7\x82\xdf\x82\xdc\x82\xb9\x82\xf1";

HWND MainHwnd(MMDApp* app) {
    return static_cast<HWND>(app->Hwnd());
}

bool DirMenuChecked(MMDApp* app) {
    return (GetMenuState(GetMenu(MainHwnd(app)), 0x12D, 0) & MF_CHECKED) != 0;
}

wchar_t* DirOf(MMDApp* app, const wchar_t* szFile) {
    return ExtractDirFromPath(app->PathWorkspace().executableDirectory,
                              szFile);
}

}  

void HandleDropFiles(HDROP hDrop) {
    MMDApp* app = g_Block;

    const int fileCount = DragQueryFileA(hDrop, 0xFFFFFFFF, nullptr, 0);
    for (int iFile = 0; iFile < fileCount; ++iFile) {
        wchar_t szFile[256];
        DragQueryFileW(hDrop, iFile, szFile, static_cast<UINT>(sizeof(szFile) / sizeof(szFile[0])));

        if (wcsstr(szFile, L".pmm") || wcsstr(szFile, L".PMM") ||
            wcsstr(szFile, L".Pmm")) {
            if (app->SceneModified() != 0) {
                int r;
                if (app->EnglishUI()) {
                    r = MessageBoxA(MainHwnd(app),
                                    "There is a change point not preserved.\n\nAre you OK?",
                                    "open file data", MB_OKCANCEL);
                } else {
                    r = MessageBoxA(MainHwnd(app), kJpDirtyText,
                                    kJpDirtyCaption, MB_OKCANCEL);
                }
                if (r != IDOK)
                    return;  
            }
            if (DirMenuChecked(app)) {  
                wcscpy_s(app->DirUser(), 0x3E8, DirOf(app, szFile));  
            }
            wcscpy_s(app->EnvFileName(), 0x100, szFile);              
            LoadSceneFile();                                          
            continue;
        }

        if ((wcsstr(szFile, L".pmd") || wcsstr(szFile, L".PMD") ||
             wcsstr(szFile, L".Pmd")) ||
            (wcsstr(szFile, L".pmx") || wcsstr(szFile, L".PMX") ||
             wcsstr(szFile, L".Pmx"))) {
            if (DirMenuChecked(app)) {
                wcscpy_s(app->DirModel(), 0x3E8, DirOf(app, szFile));  
            }
            LoadModelFile(app, szFile);                               
            continue;
        }

        if (wcsstr(szFile, L".vpd") || wcsstr(szFile, L".VPD") ||
            wcsstr(szFile, L".Vpd")) {
            if (app->state.optflag[0] != 0) {  
                if (app->EnglishUI())
                    MessageBoxA(MainHwnd(app), "Not selected model!",
                                "open pose data", MB_OK);
                else
                    MessageBoxA(MainHwnd(app), kJpPoseText, kJpPoseCaption,
                                MB_OK);
            } else {
                if (DirMenuChecked(app)) {
                    wcscpy_s(app->DirPose(), 0x3E8, DirOf(app, szFile));  
                }
                LoadVpdFile(szFile);                                  
            }
            continue;
        }

        if (wcsstr(szFile, L".vmd") || wcsstr(szFile, L".VMD") ||
            wcsstr(szFile, L".Vmd")) {
            if (DirMenuChecked(app)) {
                wcscpy_s(app->DirMotion(), 0x3E8, DirOf(app, szFile));  
            }
            LoadVmdFile(szFile);                                      
            app->SceneModified() = 1;
            continue;
        }

        if (wcsstr(szFile, L".x") || wcsstr(szFile, L".X")) {
            if (DirMenuChecked(app)) {
                wcscpy_s(app->DirAccs(), 0x3E8, DirOf(app, szFile));   
            }
            LoadAccessoryFile(szFile);  
            continue;
        }

        if (wcsstr(szFile, L".vac") || wcsstr(szFile, L".VAC") ||
            wcsstr(szFile, L"Vac")) {
            if (DirMenuChecked(app)) {
                wcscpy_s(app->DirAccs(), 0x3E8, DirOf(app, szFile));
            }
            LoadAccessoryFile(szFile);  
            continue;
        }

        if (wcsstr(szFile, L".avi") || wcsstr(szFile, L".AVI") ||
            wcsstr(szFile, L".Avi")) {
            if (DirMenuChecked(app)) {
                wcscpy_s(app->DirBg(), 0x3E8, DirOf(app, szFile));     
            }
            wcscpy_s(app->state.aviBackgroundPath, 0x100, szFile);              
            LoadAviFile(app);                                        
            app->SceneModified() = 1;
            continue;
        }

        if (wcsstr(szFile, L".bmp") || wcsstr(szFile, L".BMP") ||
            wcsstr(szFile, L".Bmp")) {
            if (DirMenuChecked(app)) {
                wcscpy_s(app->DirBg(), 0x3E8, DirOf(app, szFile));
            }
        } else {
            if (!(wcsstr(szFile, L".jpg") || wcsstr(szFile, L".JPG") ||
                  wcsstr(szFile, L".Jpg"))) {
                if (wcsstr(szFile, L".wav") || wcsstr(szFile, L".WAV") ||
                    wcsstr(szFile, L".Wav")) {
                    wcscpy_s(app->state.wavPath, 0x100, szFile);  
                    if (DirMenuChecked(app)) {
                        wcscpy_s(app->DirWave(), 0x3E8, DirOf(app, szFile));  
                    }
                    LoadWaveFile(app);                                
                    app->SceneModified() = 1;
                } else {
                    if (wcsstr(szFile, L".vsq") || wcsstr(szFile, L".VSQ") ||
                        wcsstr(szFile, L".Vsq")) {
                        if (app->state.optflag[0] != 0) {
                            if (app->EnglishUI())
                                MessageBoxA(MainHwnd(app), "Not selected model!",
                                            "open vsq data", MB_OK);
                            else
                                MessageBoxA(MainHwnd(app), kJpVsqText, "vsq",
                                            MB_OK);
                        } else {
                            LoadVsqFile(app, szFile);                   
                            app->SceneModified() = 1;
                        }
                    } else {
                        if (wcsstr(szFile, L".oni") || wcsstr(szFile, L".ONI") ||
                            wcsstr(szFile, L".Oni")) {
                            if (app->state.optflag[0] != 0) {
                                if (app->EnglishUI())
                                    MessageBoxA(MainHwnd(app), "Please select model!",
                                                "open oni data", MB_OK);
                                else
                                    MessageBoxA(MainHwnd(app), kJpOniText, "oni",
                                                MB_OK);
                            } else {
                                if (app->state.depthDeviceEnabled != 0)
                                    DisableKinect(app);                
                                char sjisPath[256];
                                WideToSjis(app->Renderer(), sjisPath, szFile, 256);  
                                OpenNiInit(app, sjisPath);             
                            }
                        }
                    }
                }
                continue;  
            }
        }
        wcscpy_s(app->state.pictureBackgroundPath, 0x100, szFile);    
        LoadBackgroundPicture(app);                                   
        app->SceneModified() = 1;
    }
    DragFinish(hDrop);  
}

}  
