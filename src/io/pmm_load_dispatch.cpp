#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <io.h>
#include <cstdlib>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

namespace {

const char kJpOpenFailFmt[] =         
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xaa\x93\xc7\x82\xdd\x8d\x9e"
    "\x82\xdf\x82\xdc\x82\xb9\x82\xf1:%d";
const char kJpOpenCaption[] =         
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x93\xc7\x8d\x9e";
const char kJpNotPmmText[] =          
    "\x82\xb1\x82\xcc\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xcd"
    "Polygon Movie Maker"
    "\x97\x70\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xc5\x82\xcd\x82\xa0"
    "\x82\xe8\x82\xdc\x82\xb9\x82\xf1";
const char kJpBadVersionText[] =      
    "\x82\xb1\x82\xcc\x83\x6f\x81\x5b\x83\x57\x83\x87\x83\x93\x82\xcc"
    "\x70\x6d\x6d\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xcd\x93\xc7\x82\xdf"
    "\x82\xdc\x82\xb9\x82\xf1";

}  

void PathToProjectDir(wchar_t* destination, const wchar_t* source) {
    wchar_t local[0x100];
    wcscpy_s(local, 0x100, source);
    if (local[0] == L'\0') {
        destination[0] = L'\0';
        return;
    }
    wchar_t* hit = wcsstr(local, L"UserFile");
    if (hit != nullptr) {
        *hit = L'\0';
        wcscpy_s(destination, 0x100, local);
        return;
    }
    const size_t len = wcslen(local);
    if (local[len - 1] == L'\\') {
        wcscpy_s(destination, 0x100, local);
        return;
    }
    destination[0] = L'\0';
}

void FontSubInit(MMDApp* app, const wchar_t* directory) {
    if (app == nullptr || directory == nullptr)
        return;
    PathResolutionWorkspace& paths = app->PathWorkspace();
    wcscpy_s(paths.executableDirectory, 0x100, directory);
    PathToProjectDir(paths.projectDirectory, paths.executableDirectory);
}

void LoadSceneFile() {

    MMDApp* app = g_Block;
    char box[0x100];

    app->SceneModified() = 0;                                  
    int fd = -1;
    const errno_t err =
        _wsopen_s(&fd, app->EnvFileName(), 0x8000, 0x40, 0x80);
    if (err != 0) {
        if (app->EnglishUI() == 0)
            sprintf_s(box, 0x100, kJpOpenFailFmt, err);
        else
            sprintf_s(box, 0x100, "Cannot open file:%d", err);
        MessageBoxA(reinterpret_cast<HWND>(app->Hwnd()), box,
                    app->EnglishUI() == 0 ? kJpOpenCaption : "open file", 0);
        return;
    }

    app->state.windowLayoutReady = 0;              
    PathToProjectDir(app->PathWorkspace().projectDirectory,     
                      app->EnvFileName());

    char hdr[0x1E];
    _read(fd, hdr, 0x1E);                                       
    char* const signature = strstr(hdr, "Polygon Movie maker");
    if (signature == nullptr) {
        MessageBoxA(reinterpret_cast<HWND>(app->Hwnd()),
                    app->EnglishUI() != 0
                        ? "This isn't the data for Polygon Movie Maker!"
                        : kJpNotPmmText,
                    app->EnglishUI() == 0 ? kJpOpenCaption : "open file", 0);
        _close(fd);                                             
        return;
    }

    if (strncmp(signature + 0x14, "0001", 5) == 0) {
        LoadSceneV1(app, fd);
        return;
    }
    if (strncmp(signature + 0x14, "0002", 5) == 0) {
        LoadSceneV2(app, fd);                                     
        return;                                                 
    }
    MessageBoxA(reinterpret_cast<HWND>(app->Hwnd()),
                app->EnglishUI() != 0
                    ? "Cannot read this version of pmm!"
                    : kJpBadVersionText,
                app->EnglishUI() == 0 ? kJpOpenCaption : "open file", 0);
    _close(fd);                                                 
}

}  
