#ifndef RINMOTIONDANCE_WINDOW_DIALOG_SCAFFOLD_HPP
#define RINMOTIONDANCE_WINDOW_DIALOG_SCAFFOLD_HPP

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdlib>

#include "rinmotiondance/mmd_app.hpp"

namespace rinmotiondance {

inline void PrefillEdit(HWND hDlg, int controlId, const char* text) {
    SendMessageA(GetDlgItem(hDlg, controlId), EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(text));
}

inline void SelectAllEdit(HWND hDlg, int controlId) {
    SetFocus(GetDlgItem(hDlg, controlId));
    SendMessageA(GetDlgItem(hDlg, controlId), EM_SETSEL, 0,
                 GetWindowTextLengthA(GetDlgItem(hDlg, controlId)));
}

inline int ReadIntFromEdit(HWND hDlg, int controlId, int maxChars = 20) {
    char text[0x100];
    GetWindowTextA(GetDlgItem(hDlg, controlId), text, maxChars);
    return static_cast<int>(atol(text));
}

inline float ReadFloatFromEdit(HWND hDlg, int controlId, int maxChars = 20) {
    char text[0x100];
    GetWindowTextA(GetDlgItem(hDlg, controlId), text, maxChars);
    return static_cast<float>(atof(text));
}

inline void MakeDialogTopmostIfRequested(MMDApp* app, HWND hDlg) {
    if (app->FloatingWindow() != nullptr) {
        SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, 3u);
    }
}

}  

#endif  
