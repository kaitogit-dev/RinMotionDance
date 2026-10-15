#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void InitSelectNavDialog(MMDApp* app, HWND hDlg);    
void SyncSelectAttachCombo(MMDApp* app, HWND hDlg);  
void ResetModelSelection(MMDApp* app);               
void ApplyBoneAttach(MMDApp* app, HWND hDlg);        
void RebuildTargetBoneList(MMDApp* app, HWND hDlg, int keepSelection);  
void CommitTargetBonePick(MMDApp* app, HWND hDlg);   
void ApplyFrameRangeScale(HWND hDlg);                

INT_PTR CALLBACK FrameRangeDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    char text[256];

    if (Msg == WM_INITDIALOG) {                     
        MMDApp* app = g_Block;
        if (app->state.floatingWindow != 0)   
            SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOSIZE | SWP_NOMOVE);  

        const HWND main = reinterpret_cast<HWND>(
            app->state.hwnd);
        GetWindowTextA(GetDlgItem(main, panel::kRangeStartEdit), text, 8);           
        sprintf_s(text, 0x100, "%d", atol(text));
        SendMessageA(GetDlgItem(hDlg, panel::kScaleFromEdit), EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(text));
        GetWindowTextA(GetDlgItem(main, panel::kRangeStopEdit), text, 8);           
        sprintf_s(text, 0x100, "%d", atol(text));
        SendMessageA(GetDlgItem(hDlg, panel::kScaleToEdit), EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(text));

        SendMessageA(GetDlgItem(hDlg, panel::kScaleRateEdit), EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>("1.0"));  
        SetFocus(GetDlgItem(hDlg, panel::kScaleRateEdit));                           
        SendMessageA(GetDlgItem(hDlg, panel::kScaleRateEdit), EM_SETSEL, 0, 3);
        SendMessageA(GetDlgItem(hDlg, panel::kScaleBoneCheckbox), BM_SETCHECK, 1, 0);
        SendMessageA(GetDlgItem(hDlg, panel::kScaleMorphCheckbox), BM_SETCHECK, 1, 0);
        SendMessageA(GetDlgItem(hDlg, panel::kScaleDispIkCheckbox), BM_SETCHECK, 1, 0);

        SetFocus(GetDlgItem(hDlg, panel::kScaleFromEdit));                           
        const HWND edit = GetDlgItem(hDlg, panel::kScaleFromEdit);
        SendMessageA(edit, EM_SETSEL, 0, GetWindowTextLengthA(edit));
        return 0;
    }
    if (Msg == WM_COMMAND) {                        
        if (LOWORD(wParam) == 1) {                  
            ApplyFrameRangeScale(hDlg);              
            EndDialog(hDlg, 1);
        } else if (LOWORD(wParam) == 2) {           
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

INT_PTR CALLBACK SelectNavDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam) {
    if (Msg == WM_INITDIALOG) {                     
        InitSelectNavDialog(g_Block, hDlg);                   
        return 0;
    }
    if (Msg != WM_COMMAND)
        return 0;

    if (LOWORD(wParam) == 2) {                      
        EndDialog(hDlg, 1);
        return 0;
    }
    if (LOWORD(wParam) == 671) {                    
        HWND combo = GetDlgItem(hDlg, panel::kBoneNameCombo);
        const LRESULT cur = SendMessageA(combo, CB_GETCURSEL, 0, 0);
        if (cur > 0) {
            SendMessageA(combo, CB_SETCURSEL, cur - 1, 0);
            SyncSelectAttachCombo(g_Block, hDlg);
        }
        return 0;
    }
    if (LOWORD(wParam) == 670) {                    
        HWND combo = GetDlgItem(hDlg, panel::kBoneNameCombo);
        const LRESULT cur = SendMessageA(combo, CB_GETCURSEL, 0, 0);
        if (cur < SendMessageA(combo, CB_GETCOUNT, 0, 0) - 1) {
            SendMessageA(combo, CB_SETCURSEL, cur + 1, 0);
            SyncSelectAttachCombo(g_Block, hDlg);
        }
        return 0;
    }
    if (LOWORD(wParam) == 632) {                    
        ResetModelSelection(g_Block);
        return 0;
    }
    if (LOWORD(wParam) == 630) {                    
        ApplyBoneAttach(g_Block, hDlg);
        return 0;
    }
    if (HIWORD(wParam) == 1) {                      
        const HWND ctrl = reinterpret_cast<HWND>(lParam);
        if (ctrl == GetDlgItem(hDlg, panel::kBoneNameCombo)) {
            SyncSelectAttachCombo(g_Block, hDlg);
            return 0;
        }
        if (ctrl == GetDlgItem(hDlg, panel::kMorphNameCombo)) {
            RebuildTargetBoneList(g_Block, hDlg, 0);
            return 0;
        }
        if (ctrl == GetDlgItem(hDlg, panel::kGroupNameCombo)) {
            CommitTargetBonePick(g_Block, hDlg);
            return 0;
        }
    }
    return 0;
}

}  
