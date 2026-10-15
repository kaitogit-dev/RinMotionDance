#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

#include "dialog_scaffold.hpp"

namespace rinmotiondance {


static const char kCaptionDelUnusedJp[] =
    "\x95\x73\x97\x76"                     
    "\x83\x7C\x83\x43\x83\x93\x83\x67"     
    "\x82\xCC\x8D\xED\x8F\x9C";            
static const char kCaptionDelLipJp[] =
    "\x83\x8A\x83\x62\x83\x76\x83\x74\x83\x8C\x81\x5B\x83\x80"  
    "\x8D\xED\x8F\x9C";                                        
static const char kMsgDelLipJp[] =
    "\x83\x8A\x83\x62\x83\x76\x82\xCC"     
    "\x83\x74\x83\x8C\x81\x5B\x83\x80\x82\xF0"  
    "\x91\x53\x82\xC4\x8D\xED\x8F\x9C\x82\xB5\x82\xDC\x82\xB7\n"  
    "\x95\x5C\x8F\xEE\x83\x74\x83\x8C\x81\x5B\x83\x80"  
    "\x82\xCC\x95\xCF\x8D\x58\x82\xCD"    
    "\x8C\xB3\x82\xC9\x96\xDF\x82\xB9\x82\xDC\x82\xB9\x82\xF1\n\n"  
    "\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5\x82\xB7\x82\xA9\x81\x48";  
static const char kMsgDelEyesJp[] =
    "\x96\xDA\x82\xCC\x83\x74\x83\x8C\x81\x5B\x83\x80\x82\xF0"  
    "\x91\x53\x82\xC4\x8D\xED\x8F\x9C\x82\xB5\x82\xDC\x82\xB7\n"  
    "\x95\x5C\x8F\xEE\x83\x74\x83\x8C\x81\x5B\x83\x80"  
    "\x82\xCC\x95\xCF\x8D\x58\x82\xCD"    
    "\x8C\xB3\x82\xC9\x96\xDF\x82\xB9\x82\xDC\x82\xB9\x82\xF1\n\n"  
    "\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5\x82\xB7\x82\xA9\x81\x48";  
static const char kMsgDelEyebrowJp[] =
    "\x82\xDC\x82\xE4\x82\xCC\x83\x74\x83\x8C\x81\x5B\x83\x80\x82\xF0"  
    "\x91\x53\x82\xC4\x8D\xED\x8F\x9C\x82\xB5\x82\xDC\x82\xB7\n"  
    "\x95\x5C\x8F\xEE\x83\x74\x83\x8C\x81\x5B\x83\x80"  
    "\x82\xCC\x95\xCF\x8D\x58\x82\xCD"    
    "\x8C\xB3\x82\xC9\x96\xDF\x82\xB9\x82\xDC\x82\xB9\x82\xF1\n\n"  
    "\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5\x82\xB7\x82\xA9\x81\x48";  
static const char kCaptionNewJp[] =
    "\x90\x56\x8B\x4B\x8D\xEC\x90\xAC";  
static const char kMsgNoBlinkMorphJp[] =
    "\x82\xB1\x82\xCC\x83\x82\x83\x66\x83\x8B\x82\xCC\x95\x5C\x8F\xEE\x82\xC9"
    "\x81\x68\x82\xDC\x82\xCE\x82\xBD\x82\xAB\x81\x68\x82\xAA\x82\xC8\x82\xA2"
    "\x88\xD7\n\x8E\xA9\x93\xAE\x83\x8A\x83\x62\x83\x76\x83\x56\x83\x93\x83\x4E"
    "\x82\xCD\x82\xC5\x82\xAB\x82\xDC\x82\xB9\x82\xF1";
static const char kCaptionRegBlinkJp[] =
    "\x82\xDC\x82\xCE\x82\xBD\x82\xAB\x83\x89\x83\x93\x83\x5F\x83\x80\x93\x6F\x98\x5E";
static const char kCaptionBlinkCntJp[] =
    "\x82\xDC\x82\xCE\x82\xBD\x82\xAB\x92\xC7\x89\xC1";
static const char kFmtPointsDeletedJp[] =
    "%d"
    "\x8C\xC2"                                
    "\x82\xCC"                                
    "\x83\x7C\x83\x43\x83\x93\x83\x67"        
    "\x82\xF0"                                
    "\x8D\xED\x8F\x9C"                        
    "\x82\xB5\x82\xDC\x82\xB5\x82\xBD";       
static const char kFmtBlinksAddedJp[] =
    "%d"
    "\x89\xF1"                                
    "\x82\xCC"                                
    "\x82\xDC\x82\xCE\x82\xBD\x82\xAB"        
    "\x82\xF0"                                
    "\x92\xC7\x89\xC1"                        
    "\x82\xB5\x82\xDC\x82\xB5\x82\xBD";       
static const char kNameMabataki[] =
    "\x82\xDC\x82\xCE\x82\xBD\x82\xAB\x00\x00\x00\x00";
static const char kCaptionPasteJp[] =
    "\x95\xCA\xCC\xDA\xB0\xD1\x82\xD6\xCD\xDF\xB0\xBD\xC4";
static const char kMsgPasteAccJp[] =
    "\x8C\xBB\x8D\xDD\x83\x52\x83\x73\x81\x5B\x97\xCC\x88\xE6\x82\xC9"
    "\x8A\x69\x94\x5B\x82\xB3\x82\xEA\x82\xC4\x82\xA2\x82\xE9"
    "\x83\x41\x83\x4E\x83\x5A\x83\x54\x83\x8A\x82\xCC\x83\x74\x83\x8C"
    "\x81\x5B\x83\x80\x83\x66\x81\x5B\x83\x5E\x82\xF0\n"
    "\x83\x41\x83\x4E\x83\x5A\x83\x54\x83\x8A\x96\xBC\x82\xF0\x96\xB3"
    "\x8E\x8B\x82\xB5\x82\xC4\x91\x80\x8D\xEC\x91\xCE\x8F\xDB"
    "\x83\x41\x83\x4E\x83\x5A\x83\x54\x83\x8A\x82\xCC\x83\x74\x83\x8C"
    "\x81\x5B\x83\x80\x82\xF0\x8D\xED\x8F\x9C\x82\xB5\x82\xDC\x82\xB7\n\n"
    "\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5\x82\xB7\x82\xA9\x81\x48";
static const char kMsgPasteBoneJp[] =
    "\x8C\xBB\x8D\xDD\x83\x52\x83\x73\x81\x5B\x97\xCC\x88\xE6\x82\xC9"
    "\x8A\x69\x94\x5B\x82\xB3\x82\xEA\x82\xC4\x82\xA2\x82\xE9"
    "\x83\x74\x83\x8C\x81\x5B\x83\x80\x83\x66\x81\x5B\x83\x5E\x82\xF0"
    "\x83\x7B\x81\x5B\x83\x93\x96\xBC\x82\xF0\x96\xB3\x8E\x8B\x82\xB5"
    "\x82\xC4\n\x91\x80\x8D\xEC\x91\xCE\x8F\xDB\x83\x7B\x81\x5B\x83\x93"
    "\x28\x89\xE6\x96\xCA\x8D\xB6\x8F\xE3\x82\xC9\x95\x5C\x8E\xA6\x29"
    "\x82\xF0\x91\x49\x91\xF0\x82\xB5\x82\xC4\n\x8D\xED\x8F\x9C\x82\xB5"
    "\x82\xDC\x82\xB7\n\n\x82\xE6\x82\xEB\x82\xB5\x82\xA2\x82\xC5\x82\xB7"
    "\x82\xA9\x81\x48";
static const char kMsgSelectAccJp[] =
    "\x91\x80\x8D\xEC\x91\xCE\x8F\xDB\x83\x41\x83\x4E\x83\x5A\x83\x54"
    "\x83\x8A\x28\x89\xE6\x96\xCA\x8D\xB6\x8F\xE3\x82\xC9\x95\x5C\x8E\xA6"
    "\x29\x82\xF0\x91\x49\x91\xF0\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2";
static const char kMsgSelectBoneJp[] =
    "\x91\x80\x8D\xEC\x91\xCE\x8F\xDB\x83\x7B\x81\x5B\x83\x93\x28\x89\xE6"
    "\x96\xCA\x8D\xB6\x8F\xE3\x82\xC9\x95\x5C\x8E\xA6\x29\x82\xF0\x91\x49"
    "\x91\xF0\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2";

static const wchar_t kEmptyPathFormat[] = L"\x0\x0%s%s";

static const char kMsgCanvasTooBigJp[] =
    "\x82\xB1\x82\xCC\x50\x43\x82\xC5"   
    "\x8F\x88\x97\x9D\x82\xC5\x82\xAB"   
    "\x82\xE9\x91\xE5\x82\xAB\x82\xB3"   
    "\x82\xF0\x92\xB4\x82\xA6\x82\xC4"   
    "\x82\xA2\x82\xDC\x82\xB7";          
static const char kMsgEndLessJp[] =
    "\x98\x5E\x89\xE6"        
    "\x8F\x49\x97\xB9"        
    "\xCC\xDA\xB0\xD1"        
    "\x82\xAA"                
    "\x98\x5E\x89\xE6"        
    "\x8A\x4A\x8E\x6E"        
    "\xCC\xDA\xB0\xD1"        
    "\x82\xE6\x82\xE8\x82\xE0"  
    "\x8F\xAC\x82\xB3\x82\xA2\x82\xC5\x82\xB7";  
static const wchar_t kAccTypeCameraJp[] = L"\x30AB\x30E1\x30E9";       
static const wchar_t kAccTypeLightJp[] = L"\x7167\x660E";              
static const wchar_t kAccTypeSelfShadowJp[] = L"\x30BB\x30EB\x30D5\x5F71";  
static const wchar_t kAccTypeGravityJp[] = L"\x91CD\x529B";            

static const char kEmptyCaption[] = "";

void PanelPaint(MMDApp* app);                       
void SelectionReeval(MMDApp* app);                  

void RefreshMainWindowViewport(MMDApp* app);  
void AviBgOverlayRefresh(MMDApp* app);             
void PicBgOverlayRefresh(MMDApp* app);             
void PostDeviceReset(MMDApp* app);             
void LayoutViewportPanels(MMDApp* app);             
void SnapshotSelectedKeysForUndo(unsigned char* model, int frame);  
int SeekModelFrame(unsigned char* model, int frame, int physicsMode);  
void DeleteMarkedKeyframes(MMDApp* app);                       
void LoadVsqFile(MMDApp* app, const wchar_t* path);  
void CopyDirPathW(wchar_t* dest, const wchar_t* src); 
void CopyPathW(wchar_t* dest, const wchar_t* src);
void StartAviRecordFullscreen(MMDApp* app);        
void StartAviRecordWindow(MMDApp* app);            
void RegisterMorphKeyCurrent(unsigned char* model, int morph,
               int frame);                         
void RefreshLightPanel(MMDApp* app);                       
void RefreshSelfShadowPanel(MMDApp* app);                       
void ApplyGravityTrack(MMDApp* app);                       
void ApplyAccessoryTrack(MMDApp* app, int slot);             
void SyncAccessoryEditPanel(MMDApp* app);                       
void LoadBackgroundPicture(MMDApp* app);
int PasteAccessoryKeyRecord(MMDApp* app, void* rec, int useSelectedSlot);  
void ResetBoneKeyCursor(unsigned char* model);              
bool RegisterBoneKey(unsigned char* model, unsigned char* rec, int frameOffset,
                   unsigned char useSelected);  
void IdentityCtor(void* obj);                         
void* ConstructArrayElements(void* block, std::uint32_t elementSize,
                             std::uint32_t count,
                             void* ctor);          

std::uint32_t MulOrMax(std::uint32_t a, std::uint32_t b) {
    const std::uint64_t r = static_cast<std::uint64_t>(a) * b;
    return r > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<std::uint32_t>(r);
}

static unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

HWND MainHwnd(MMDApp* app) {
    return static_cast<HWND>(app->Hwnd());  
}

void FramePurgeTail(MMDApp* app) {
    unsigned char* model = ActiveModel(app);
    SeekModelFrame(model, app->state.currentFrame,
              app->PlaybackPhysicsMode());
    PanelPaint(app);      
    SelectionReeval(app); 
    app->SceneModified() = 1;
}

void PurgeMorphFrames(MMDApp* app, std::uint8_t type) {
    unsigned char* model = ActiveModel(app);
    const std::int32_t morphCount =
        static_cast<std::int32_t>(mdl::Mdl(model)->morphCount);
    for (std::uint16_t m = 0; static_cast<std::int32_t>(m) < morphCount; ++m) {
        model = ActiveModel(app);
        const mdl::MorphRecord* const morphs = mdl::Morphs(model);
        if (morphs[m].type != type) {
            continue;
        }
        mdl::MorphKey* frames = mdl::MorphKeys(model);
        std::int32_t cur = m;  
        if (frames[m].next != 0) {
            for (;;) {
                frames[cur].frame = 0;
                frames[cur].previous = 0;
                frames[cur].allocated = 0;
                frames[cur].value = 0.0f;
                const std::int32_t next =
                    static_cast<std::int32_t>(frames[cur].next);
                frames[m].next = 0;
                cur = next;
                model = ActiveModel(app);  
                frames = mdl::MorphKeys(model);
                if (frames[cur].next == 0) {
                    break;
                }
            }
        }
        frames[m].frame = 0;
        frames[m].previous = 0;
        frames[m].allocated = 0;
        frames[m].value = 0.0f;
    }
}


int FillCodecCombo(DShowRecorder* recorder, HWND hCombo, char english);  
void RebindCodecOnComboChange(DShowRecorder* recorder, int sel, HWND hButton, char english);  
void ShowCodecConfigDialog(DShowRecorder* recorder, HWND hDlg);  
void ApplyCameraFrameScaleAdd(MMDApp* app, HWND hDlg);  
void BuildAccessoryOrderArray(MMDApp* app, int count);        
void ApplyAccessorySettingsDialog(MMDApp* app, int count, HWND hDlg);  
LRESULT __stdcall GroundShadowColorEditSubclassProc(HWND, UINT, WPARAM, LPARAM);  

INT_PTR __stdcall ScreenSizeDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                    LPARAM lParam) {  
    MMDApp* app = g_Block;
    (void)lParam;
    char text[0x100];
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        sprintf_s(text, 0x100, "%3d",
                  app->RenderWidth());
        PrefillEdit(hDlg, panel::kScreenWidthEdit, text);
        sprintf_s(text, 0x100, "%3d",
                  app->RenderHeight());
        PrefillEdit(hDlg, panel::kScreenHeightEdit, text);
        SelectAllEdit(hDlg, panel::kScreenWidthEdit);
    } else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == 1) {  
            const int w = ReadIntFromEdit(hDlg, panel::kScreenWidthEdit);
            const int h = ReadIntFromEdit(hDlg, panel::kScreenHeightEdit);
            const D3DRenderer* sub = app->Renderer();
            if (w > sub->maxTextureWidth ||  
                h > sub->maxTextureHeight) {  
                MessageBoxA(hDlg,
                            app->EnglishUI() != 0 ? "too big for your PC!"
                                                  : kMsgCanvasTooBigJp,
                            kEmptyCaption, 0);
            } else {
                app->RenderWidth() = w;
                app->RenderHeight() = h;
                EndDialog(hDlg, 1);
            }
        } else if (LOWORD(wParam) == 2) {  
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

INT_PTR __stdcall ModelOffsetDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                     LPARAM lParam) {  
    MMDApp* app = g_Block;
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        PrefillEdit(hDlg, panel::kBiasXEdit, "0.0");
        PrefillEdit(hDlg, panel::kBiasYEdit, "0.0");
        PrefillEdit(hDlg, panel::kBiasZEdit, "0.0");
        SelectAllEdit(hDlg, panel::kBiasXEdit);
    } else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == 1) {  
            app->state.modelOffsetX = ReadFloatFromEdit(hDlg, panel::kBiasXEdit);
            app->state.modelOffsetY = ReadFloatFromEdit(hDlg, panel::kBiasYEdit);
            app->state.modelOffsetZ = ReadFloatFromEdit(hDlg, panel::kBiasZEdit);
            EndDialog(hDlg, 1);
        } else if (LOWORD(wParam) == 2) {  
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

INT_PTR __stdcall FrameShiftDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                    LPARAM lParam) {  
    MMDApp* app = g_Block;
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        PrefillEdit(hDlg, panel::kShiftFramesEdit, "0");
        SelectAllEdit(hDlg, panel::kShiftFramesEdit);
    } else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == 1) {  
            app->MorphFrameShift() = ReadIntFromEdit(hDlg, panel::kShiftFramesEdit);
            EndDialog(hDlg, 1);
        } else if (LOWORD(wParam) == 2) {  
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

INT_PTR __stdcall BlinkRegisterDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                       LPARAM lParam) {  
    MMDApp* app = g_Block;
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
    } else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == 1) {  
            app->BlinkStartFrame() = ReadIntFromEdit(hDlg, panel::kBlinkStartEdit);
            app->BlinkEndFrame() = ReadIntFromEdit(hDlg, panel::kBlinkEndEdit);
            EndDialog(hDlg, 1);
        } else if (LOWORD(wParam) == 2) {  
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

INT_PTR __stdcall AviOutDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                LPARAM lParam) {  
    MMDApp* app = g_Block;
    char text[0x100];
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        const HWND main = MainHwnd(app);
        GetWindowTextA(GetDlgItem(main, panel::kPlayStartFrameEdit), text, 8);
        PrefillEdit(hDlg, panel::kAviStartFrameEdit, text);
        GetWindowTextA(GetDlgItem(main, panel::kPlayStopFrameEdit), text, 8);
        PrefillEdit(hDlg, panel::kAviEndFrameEdit, text);
        PrefillEdit(hDlg, panel::kAviFpsEdit, "30");
        sprintf_s(text, 0x100, "%5d",
                  app->RenderWidth());
        PrefillEdit(hDlg, panel::kAviWidthEdit, text);
        sprintf_s(text, 0x100, "%5d",
                  app->RenderHeight());
        PrefillEdit(hDlg, panel::kAviHeightEdit, text);
        EnableWindow(GetDlgItem(hDlg, panel::kAviWidthEdit), FALSE);
        EnableWindow(GetDlgItem(hDlg, panel::kAviHeightEdit), FALSE);
        if (app->state.waveEnabled != 0) {
            SendMessageA(GetDlgItem(hDlg, panel::kAviWaveCheckbox), BM_SETCHECK, 1, 0);
        } else {
            EnableWindow(GetDlgItem(hDlg, panel::kAviWaveCheckbox), FALSE);
        }
        const D3DRenderer* sub = app->Renderer();
        if (sub->stereoEnabled == 0) {  
            EnableWindow(GetDlgItem(hDlg, panel::kAvi3dVisionCheckbox), FALSE);
            EnableWindow(GetDlgItem(hDlg, panel::kAviLeftHalfCheckbox), FALSE);
        }
        app->AviCodecSelection() = FillCodecCombo(
            app->Recorder(), GetDlgItem(hDlg, panel::kAviCodecCombo),
            app->EnglishUI());
        EnableWindow(GetDlgItem(hDlg, panel::kAviSettingsButton), FALSE);
    } else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == 1) {  
            app->AviRecordStartFrame() =
                ReadIntFromEdit(hDlg, panel::kAviStartFrameEdit);
            app->AviRecordEndFrame() =
                ReadIntFromEdit(hDlg, panel::kAviEndFrameEdit);
            app->AviRecordFps() =
                ReadFloatFromEdit(hDlg, panel::kAviFpsEdit);
            app->AviIncludeWave() =
                IsDlgButtonChecked(hDlg, panel::kAviWaveCheckbox) == 1 ? 1 : 0;
            app->AviStereoOutput() =
                IsDlgButtonChecked(hDlg, panel::kAvi3dVisionCheckbox) == 1 ? 1 : 0;
            app->AviStereoWidthMultiplier() =
                (IsDlgButtonChecked(hDlg, panel::kAviLeftHalfCheckbox) != 1) + 1;
            if (app->AviRecordEndFrame() >= app->AviRecordStartFrame()) {
                EndDialog(hDlg, 1);
            } else if (app->EnglishUI() != 0) {
                MessageBoxA(hDlg, "End frame is less than start frame!",
                            "WARNING", 0);
            } else {
                MessageBoxA(hDlg, kMsgEndLessJp, "WARNING", 0);
            }
        } else if (LOWORD(wParam) == 2) {  
            EndDialog(hDlg, 2);
        } else if (HIWORD(wParam) == 1  ) {
            if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hDlg, panel::kAviCodecCombo)) {
                const LRESULT sel = SendMessageA(GetDlgItem(hDlg, panel::kAviCodecCombo),
                                                 CB_GETCURSEL, 0, 0);
                if (app->AviCodecSelection() != sel) {
                    RebindCodecOnComboChange(app->Recorder(),
                              static_cast<int>(sel), GetDlgItem(hDlg, panel::kAviSettingsButton),
                              app->EnglishUI());
                    app->AviCodecSelection() =
                        static_cast<std::int32_t>(sel);
                }
            }
        } else if (LOWORD(wParam) == 0x2D4) {
            ShowCodecConfigDialog(app->Recorder(), hDlg);
        }
    }
    return 0;
}

INT_PTR __stdcall CameraFrameMultiplyDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                             LPARAM lParam) {  
    MMDApp* app = g_Block;
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        for (int i = 0; i < 16; i += 2) {
            PrefillEdit(hDlg, panel::kCamMulPosXScaleEdit + i, "1.0");
        }
        for (int j = 0; j < 16; j += 2) {
            PrefillEdit(hDlg, panel::kCamMulPosXOffsetEdit + j, "0.0");
        }
        SelectAllEdit(hDlg, panel::kCamMulPosXScaleEdit);
    } else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == 1) {  
            ApplyCameraFrameScaleAdd(app, hDlg);  
            EndDialog(hDlg, 1);
            return 0;
        }
        if (LOWORD(wParam) == 2) {  
            EndDialog(hDlg, 2);
            return 0;
        }
    }
    return 0;
}

INT_PTR __stdcall GroundShadowColorDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                           LPARAM lParam) {  
    MMDApp* app = g_Block;
    (void)lParam;
    char text[0x100];
    switch (msg) {
    case WM_INITDIALOG:
        MakeDialogTopmostIfRequested(app, hDlg);
        reinterpret_cast<void*&>(app->GroundShadowColorEditProc()) = reinterpret_cast<void*>(
            GetWindowLongPtrA(GetDlgItem(hDlg, panel::kGroundShadowEdit), GWLP_WNDPROC));
        SetWindowLongPtrA(GetDlgItem(hDlg, panel::kGroundShadowEdit), GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(GroundShadowColorEditSubclassProc));
        sprintf_s(text, 0x100, "%3.2f", app->GroundShadowColor()[0]);
        PrefillEdit(hDlg, panel::kGroundShadowEdit, text);
        SendMessageA(GetDlgItem(hDlg, panel::kGroundShadowSlider), TBM_SETRANGEMIN, 0, 0);
        SendMessageA(GetDlgItem(hDlg, panel::kGroundShadowSlider), TBM_SETRANGEMAX, 0, 200);
        SendMessageA(GetDlgItem(hDlg, panel::kGroundShadowSlider), TBM_SETTICFREQ, 0x64, 0);
        SendMessageA(GetDlgItem(hDlg, panel::kGroundShadowSlider), TBM_SETPOS, 1,
                     static_cast<LPARAM>(
                         static_cast<int>(app->GroundShadowColor()[0] * 100.0)));
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 2) {  
            DestroyWindow(hDlg);
            app->GroundShadowColorDialog() = 0;
            return 1;
        }
        break;
    case WM_HSCROLL: {
        app->state.messageSeen = 1;  
        const float v = static_cast<float>(
            static_cast<double>(SendMessageA(GetDlgItem(hDlg, panel::kGroundShadowSlider),
                                             TBM_GETPOS, 0, 0)) /
            100.0);
        for (int c = 0; c < 4; ++c) {
            app->GroundShadowColor()[c] = v;
        }
        SendMessageA(GetDlgItem(hDlg, panel::kGroundShadowEdit), EM_SETSEL, 0,
                     GetWindowTextLengthA(GetDlgItem(hDlg, panel::kGroundShadowEdit)));
        sprintf_s(text, 0x100, "%3.2f", v);
        SendMessageA(GetDlgItem(hDlg, panel::kGroundShadowEdit), EM_REPLACESEL, 0,
                     (LPARAM)text);
        return 1;
    }
    }
    return 0;
}

static int g_accOrderCount = 0;

INT_PTR __stdcall AccessorySettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                           LPARAM lParam) {  
    MMDApp* app = g_Block;
    char text[0x100];
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        const HWND main = MainHwnd(app);
        g_accOrderCount = static_cast<int>(
            SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_GETCOUNT, 0, 0));
        app->DialogOrders().accessoryIndices.reset(
            new std::int32_t[static_cast<std::size_t>(g_accOrderCount)]);
        for (int i = 0; i < g_accOrderCount; ++i) {
            SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_GETLBTEXT, i,
                         (LPARAM)text);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_ADDSTRING, 0,
                         (LPARAM)text);
        }
        BuildAccessoryOrderArray(app, g_accOrderCount);
        sprintf_s(text, 0x64, "%d",
                  app->state.accessoryRenderSplitOrder);
        SetWindowTextA(GetDlgItem(hDlg, panel::kAccessoryOrderEdit), text);
        SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL,
                     0xFFFFFFFF, 0);
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    if (LOWORD(wParam) != 633) {
        switch (LOWORD(wParam)) {
        case 0x276: {  
            const LRESULT sel = SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox),
                                             LB_GETCURSEL, 0, 0);
            if (sel >= 1) {
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, sel,
                             (LPARAM)text);
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_DELETESTRING,
                             sel, 0);
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_INSERTSTRING,
                             sel - 1, (LPARAM)text);
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL,
                             sel - 1, 0);
                app->DialogOrders().SwapAccessories(sel - 1, sel);
            }
            return 0;
        }
        case 0x277: {  
            const LRESULT sel = SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox),
                                             LB_GETCURSEL, 0, 0);
            if (sel != -1 && sel < g_accOrderCount - 1) {
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, sel,
                             (LPARAM)text);
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_DELETESTRING,
                             sel, 0);
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_INSERTSTRING,
                             sel + 1, (LPARAM)text);
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL,
                             sel + 1, 0);
                app->DialogOrders().SwapAccessories(sel + 1, sel);
            }
            return 0;
        }
        case 0x278: {  
            app->state.accessoryRenderSplitOrder =
                ReadIntFromEdit(hDlg, panel::kAccessoryOrderEdit, 256);
            ApplyAccessorySettingsDialog(app, g_accOrderCount, hDlg);
            const HWND main = MainHwnd(app);
            SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_RESETCONTENT, 0, 0);
            for (int j = 0; j < g_accOrderCount; ++j) {
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, j,
                             (LPARAM)text);
                SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_ADDSTRING, 0,
                             (LPARAM)text);
            }
            SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_SETCURSEL, 0, 0);
            SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_RESETCONTENT, 0, 0);
            if (app->EnglishUI() != 0) {
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)"camera");
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)"light");
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)"s shadow");
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)"grav");
            } else {
                SendMessageW(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)kAccTypeCameraJp);
                SendMessageW(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)kAccTypeLightJp);
                SendMessageW(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)kAccTypeSelfShadowJp);
                SendMessageW(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)kAccTypeGravityJp);
            }
            char buf[100];
            const LRESULT n = SendMessageA(GetDlgItem(main, panel::kAccessoryCombo),
                                           CB_GETCOUNT, 0, 0);
            for (LRESULT k = 0; k < n; ++k) {
                SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_GETLBTEXT, k,
                             (LPARAM)buf);
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             (LPARAM)buf);
            }
            SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_SETCURSEL, 0, 0);
            EndDialog(hDlg, 1);
            app->DialogOrders().accessoryIndices.reset();
            return 0;
        }
        case 2:  
            EndDialog(hDlg, 2);
            app->DialogOrders().accessoryIndices.reset();
            return 0;
        }
        if (HIWORD(wParam) == 1  ) {
            const LRESULT sel = SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox),
                                             LB_GETCURSEL, 0, 0);
            if (sel >= 0) {
                SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, sel,
                             (LPARAM)text);
                SendMessageA(GetDlgItem(hDlg, panel::kAccessoryNameEdit), EM_SETSEL, 0,
                             GetWindowTextLengthA(GetDlgItem(hDlg, panel::kAccessoryNameEdit)));
                SendMessageA(GetDlgItem(hDlg, panel::kAccessoryNameEdit), EM_REPLACESEL, 0,
                             (LPARAM)text);
            }
            return 0;
        }
        if (reinterpret_cast<HWND>(lParam) != GetDlgItem(hDlg, panel::kAccessoryOrderEdit) ||
            HIWORD(wParam) != 0x300  ) {
            return 0;
        }
        {
            GetWindowTextA(GetDlgItem(hDlg, panel::kAccessoryOrderEdit), text, 256);
            int orderIdx = atol(text);
            if (orderIdx > 0) {
                if (orderIdx > g_accOrderCount) {
                    orderIdx = g_accOrderCount;
                    sprintf_s(text, 0x64, "%d", g_accOrderCount);
                    SetWindowTextA(GetDlgItem(hDlg, panel::kAccessoryOrderEdit), text);
                    SendMessageA(GetDlgItem(hDlg, panel::kAccessoryOrderEdit), EM_SETSEL,
                                 0, GetWindowTextLengthA(GetDlgItem(hDlg, panel::kAccessoryOrderEdit)));
                }
            } else {
                orderIdx = 0;
                sprintf_s(text, 0x64, "%d", 0);
                SetWindowTextA(GetDlgItem(hDlg, panel::kAccessoryOrderEdit), text);
                SendMessageA(GetDlgItem(hDlg, panel::kAccessoryOrderEdit), EM_SETSEL, 0,
                             GetWindowTextLengthA(GetDlgItem(hDlg, panel::kAccessoryOrderEdit)));
            }
            const bool below = orderIdx < g_accOrderCount;
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL,
                         below ? static_cast<WPARAM>(orderIdx)
                               : static_cast<WPARAM>(0xFFFFFFFF),
                         0);
        }
        return 0;
    }
    if (GetFocus() != GetDlgItem(hDlg, panel::kAccessoryOrderEdit)) {
        const LRESULT sel = SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox),
                                         LB_GETCURSEL, 0, 0);
        GetWindowTextA(GetDlgItem(hDlg, panel::kAccessoryNameEdit), text, 256);
        if (text[0] != 0 && sel < g_accOrderCount && sel >= 0) {
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_DELETESTRING,
                         sel, 0);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_INSERTSTRING,
                         sel, (LPARAM)text);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL,
                         sel, 0);
        }
    }
    return 0;
}

void BuildAccessoryOrderArray(MMDApp* app, int count) {  
    BuildAccessoryOrderIndices(app->DialogOrders().accessoryIndices.get(), count,
        [app](int slot) { return app->AccessorySlot(slot); });
    PostLanguageSweep(app);                                     
    PostLanguageSweep2(app);                                    
}
void ApplyAccessorySettingsDialog(MMDApp* app, int count, HWND hDlg) {  
    auto& s = *app;
    const std::int32_t* order = app->DialogOrders().accessoryIndices.get();
    char text[100];
    const HWND list = GetDlgItem(hDlg, panel::kOrderListBox);
    s.SelectedObjectSlot() = ApplyAccessoryOrderIndices(order, count,
        [app](int slot) { return app->AccessorySlot(slot); },
        [&](int index, mdl::AccessoryRecord& accessory) {
            SendMessageA(list, LB_GETTEXT, index, reinterpret_cast<LPARAM>(text));
            strcpy_s(accessory.name, sizeof(accessory.name), text);
        });
    SyncAccessoryEditPanel(app);                                             
    for (int j = 0; j < 255; ++j) {
        mdl::AccessoryRecord* acc = app->AccessorySlot(j);
        if (acc != nullptr)
            acc->rowSelected = 0;                               
    }
    {
        mdl::AccessoryRecord* selected =
            app->AccessorySlot(s.SelectedObjectSlot());
        if (selected != nullptr)
            selected->rowSelected = 1;                          
    }
    PostLanguageSweep(app);                                     
    PostLanguageSweep2(app);                                    
}
LRESULT __stdcall GroundShadowColorEditSubclassProc(HWND hWnd, UINT uMsg,
                                                    WPARAM wParam,
                                                    LPARAM lParam) {  
    MMDApp* app = g_Block;
    if (uMsg != WM_KEYDOWN || wParam != 13 ||  
        hWnd != GetDlgItem(app->GroundShadowColorDialog(),
                           panel::kGroundShadowEdit)) {
        return CallWindowProcA(app->GroundShadowColorEditProc(), hWnd, uMsg,
                               wParam, lParam);
    }
    char text[16];  
    GetWindowTextA(hWnd, text, 8);
    const float v = static_cast<float>(atof(text));
    for (int c = 0; c < 4; ++c) {  
        app->GroundShadowColor()[c] = v;
    }
    SendMessageA(GetDlgItem(app->GroundShadowColorDialog(), 0x1CA), TBM_SETPOS,
                 1, static_cast<LPARAM>(static_cast<int>(v * 100.0f)));
    return 0;
}

void CmdFileMenu(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify) {
    (void)notify;
    switch (id) {
    case 212: {
        app->state.enterKeyState = 1;  
        const std::intptr_t result = DialogBoxParamA(
            static_cast<HINSTANCE>(app->HInstance()),  
            MAKEINTRESOURCEA(app->EnglishUI() != 0 ? 0x28D : 0x25F),
            MainHwnd(app), ScreenSizeDlgProc, 0);
        if (result == 2) {  
            break;
        }
        app->SceneModified() = 1;
        RefreshMainWindowViewport(app);                                 
        if (app->state.aviBackgroundEnabled == 1) {
            AviBgOverlayRefresh(app);                     
        }
        if (app->state.pictureBackgroundEnabled != 0) {
            PicBgOverlayRefresh(app);                     
        }
        LayoutViewportPanels(app);                                 
        InvalidateRect(MainHwnd(app), nullptr, FALSE);
        break;
    }

    case 219: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        app->state.dialogFlags[6] = 1;
        app->state.enterKeyState = 1;
        const std::intptr_t result = DialogBoxParamA(
            static_cast<HINSTANCE>(app->HInstance()),
            MAKEINTRESOURCEA(app->EnglishUI() != 0 ? 0x28B : 0x258),
            MainHwnd(app), ModelOffsetDlgProc, 0);
        if (result == 2) {  
            break;
        }
        unsigned char* model = ActiveModel(app);
        SnapshotSelectedKeysForUndo(model, app->state.currentFrame);
        std::int32_t frameIdx = 0;  
        for (std::size_t frameIdx = 0;
             frameIdx < mdl::kBoneKeyCapacity; ++frameIdx) {
            model = ActiveModel(app);
            mdl::BoneKey* const keys = mdl::BoneKeys(model);
            mdl::BoneKey& key = keys[frameIdx];
            if (key.allocated != 0) {
                const std::int32_t boneCount =
                    static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);
                std::int32_t bone = static_cast<std::int32_t>(frameIdx);
                if (bone >= boneCount) {
                    do {
                        bone = static_cast<std::int32_t>(keys[bone].previous);
                    } while (bone >= boneCount);
                }
                const mdl::BoneRecord* const bones = mdl::Bones(model);
                if (bones[bone].parent == -1) {
                    key.position[0] += app->state.modelOffsetX;
                    key.position[1] += app->state.modelOffsetY;
                    key.position[2] += app->state.modelOffsetZ;
                }
            }
        }
        PanelPaint(app);      
        SelectionReeval(app); 
        model = ActiveModel(app);
        SeekModelFrame(model, app->state.currentFrame,
                  app->PlaybackPhysicsMode());
        app->SceneModified() = 1;
        app->PhysicsResetPending() = 1;
        break;
    }

    case 221: {
        app->state.dialogFlags[8] = 1;
        if (app->state.groundShadowEnabled != 0) {
            app->state.groundShadowEnabled = 0;
            CheckMenuItem(GetMenu(MainHwnd(app)), 0xDD, MF_UNCHECKED);
        } else {
            app->state.groundShadowEnabled = 1;
            CheckMenuItem(GetMenu(MainHwnd(app)), 0xDD, MF_CHECKED);
        }
        break;
    }

    case 222: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        app->state.dialogFlags[4] = 1;
        {
            unsigned char* m = ActiveModel(app);
            mdl::BoneKey* f = mdl::BoneKeys(m);
            for (std::size_t i = 0; i < mdl::kBoneKeyCapacity; ++i)
                f[i].allocated = 0;
        }
        {
            unsigned char* m = ActiveModel(app);
            mdl::MorphKey* f = mdl::MorphKeys(m);
            for (std::size_t i = 0; i < mdl::kMorphKeyCapacity; ++i)
                f[i].allocated = 0;
        }
        {
            unsigned char* m = ActiveModel(app);
            mdl::DisplayKey* f = mdl::DisplayKeys(m);
            for (std::size_t i = 0; i < mdl::kDisplayKeyCapacity; ++i)
                f[i].allocated = 0;
        }

        std::int32_t deleted = 0;  

        unsigned char* model = ActiveModel(app);
        const std::int32_t boneCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);
        for (std::uint16_t b = 0;
             static_cast<std::int32_t>(b) < boneCount; ++b) {
            model = ActiveModel(app);
            rinmotiondance::mdl::BoneRecord* boneFrames =
                rinmotiondance::mdl::Bones(model);
            const mdl::BoneType type = boneFrames[b].type;
            if (type == mdl::BoneType::InertTip ||
                type == mdl::BoneType::Effector) {
                continue;
            }
            unsigned char* frames = reinterpret_cast<unsigned char*>(
                rinmotiondance::mdl::BoneKeys(model));
            std::int32_t cur = *reinterpret_cast<std::int32_t*>(
                frames + 0x3C * b + 8);           
            if (cur == 0) {
                continue;
            }
            std::int32_t next = *reinterpret_cast<std::int32_t*>(
                frames + 0x3C * cur + 8);
            if (next == 0) {
                continue;
            }
            for (;;) {
                const std::int32_t prev = *reinterpret_cast<std::int32_t*>(
                    frames + 0x3C * cur + 4);
                const float curX = *reinterpret_cast<float*>(frames + 0x3C * cur + 0x1C);
                const float curY = *reinterpret_cast<float*>(frames + 0x3C * cur + 0x20);
                const float curZ = *reinterpret_cast<float*>(frames + 0x3C * cur + 0x24);
                const float curQx = *reinterpret_cast<float*>(frames + 0x3C * cur + 0x28);
                const float curQy = *reinterpret_cast<float*>(frames + 0x3C * cur + 0x2C);
                const float curQz = *reinterpret_cast<float*>(frames + 0x3C * cur + 0x30);
                const float curQw = *reinterpret_cast<float*>(frames + 0x3C * cur + 0x34);
                const float pX = *reinterpret_cast<float*>(frames + 0x3C * prev + 0x1C);
                const float pY = *reinterpret_cast<float*>(frames + 0x3C * prev + 0x20);
                const float pZ = *reinterpret_cast<float*>(frames + 0x3C * prev + 0x24);
                const float pQx = *reinterpret_cast<float*>(frames + 0x3C * prev + 0x28);
                const float pQy = *reinterpret_cast<float*>(frames + 0x3C * prev + 0x2C);
                const float pQz = *reinterpret_cast<float*>(frames + 0x3C * prev + 0x30);
                const float pQw = *reinterpret_cast<float*>(frames + 0x3C * prev + 0x34);
                const float nX = *reinterpret_cast<float*>(frames + 0x3C * next + 0x1C);
                const float nY = *reinterpret_cast<float*>(frames + 0x3C * next + 0x20);
                const float nZ = *reinterpret_cast<float*>(frames + 0x3C * next + 0x24);
                const float nQx = *reinterpret_cast<float*>(frames + 0x3C * next + 0x28);
                const float nQy = *reinterpret_cast<float*>(frames + 0x3C * next + 0x2C);
                const float nQz = *reinterpret_cast<float*>(frames + 0x3C * next + 0x30);
                const float nQw = *reinterpret_cast<float*>(frames + 0x3C * next + 0x34);
                const bool all = (curX == pX) && (curY == pY) && (curZ == pZ) &&
                                 (curQx == pQx) && (curQy == pQy) &&
                                 (curQz == pQz) && (curQw == pQw) &&
                                 (curX == nX) && (curY == nY) && (curZ == nZ) &&
                                 (curQx == nQx) && (curQy == nQy) &&
                                 (curQz == nQz) && (curQw == nQw);
                if (all) {
                    ++deleted;
                    frames[0x3C * cur + 0x38] = 1;
                }
                model = ActiveModel(app);
                frames = reinterpret_cast<unsigned char*>(
                    rinmotiondance::mdl::BoneKeys(model));
                cur = *reinterpret_cast<std::int32_t*>(frames + 0x3C * cur + 8);
                next = *reinterpret_cast<std::int32_t*>(frames + 0x3C * cur + 8);
                if (next == 0) {
                    break;
                }
            }
        }

        model = ActiveModel(app);
        const std::int32_t morphCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->morphCount);
        for (std::uint16_t m = 0;
             static_cast<std::int32_t>(m) < morphCount; ++m) {
            model = ActiveModel(app);
            unsigned char* frames = reinterpret_cast<unsigned char*>(
                mdl::MorphKeys(model));
            std::int32_t cur = *reinterpret_cast<std::int32_t*>(
                frames + 0x14 * m + 8);
            if (cur == 0) {
                continue;
            }
            std::int32_t next = *reinterpret_cast<std::int32_t*>(
                frames + 0x14 * cur + 8);
            if (next == 0) {
                continue;
            }
            for (;;) {
                const std::int32_t prev = *reinterpret_cast<std::int32_t*>(
                    frames + 0x14 * cur + 4);
                const bool hit =
                    (*reinterpret_cast<float*>(frames + 0x14 * cur + 0xC) ==
                     *reinterpret_cast<float*>(frames + 0x14 * prev + 0xC)) &&
                    (*reinterpret_cast<float*>(frames + 0x14 * cur + 0xC) ==
                     *reinterpret_cast<float*>(frames + 0x14 * next + 0xC));
                if (hit) {
                    ++deleted;
                    frames[0x14 * cur + 0x10] = 1;
                }
                model = ActiveModel(app);
                frames = reinterpret_cast<unsigned char*>(
                    mdl::MorphKeys(model));
                cur = *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 8);
                next = *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 8);
                if (next == 0) {
                    break;
                }
            }
        }

        model = ActiveModel(app);
        mdl::DisplayKey* frames = mdl::DisplayKeys(model);
        std::int32_t cur = static_cast<std::int32_t>(frames->next);
        if (cur != 0) {
            std::int32_t next = static_cast<std::int32_t>(frames[cur].next);
            while (next != 0) {
                const std::int32_t prev =
                    static_cast<std::int32_t>(frames[cur].previous);
                const std::uint8_t typeByte = frames[cur].visible;
                const mdl::DisplayKey& fPrev = frames[prev];
                const mdl::DisplayKey& fNext = frames[next];
                if (fPrev.visible == typeByte && fNext.visible == typeByte) {
                    std::uint8_t nameOk = 1;
                    const std::int32_t ikCount = static_cast<std::int32_t>(
                        mdl::Mdl(model)->ikChainCount);
                    if (ikCount > 0) {
                        unsigned char* m2 = ActiveModel(app);  
                        mdl::DisplayKey* f2 = mdl::DisplayKeys(m2);
                        const std::int32_t p1 =
                            static_cast<std::int32_t>(f2[cur].next);
                        const char* x1 = reinterpret_cast<const char*>(
                            mdl::IkStates(f2[p1]));
                        const std::int32_t p2 =
                            static_cast<std::int32_t>(f2[cur].previous);
                        const char* x2 = reinterpret_cast<const char*>(
                            mdl::IkStates(f2[p2]));
                        const char* y = reinterpret_cast<const char*>(
                            mdl::IkStates(f2[cur]));
                        for (std::int32_t k = 0; k < ikCount; ++k) {
                            if (x2[k] != y[k] || x1[k] != y[k]) {
                                nameOk = 0;
                                break;
                            }
                        }
                    }
                    if (nameOk != 0) {
                        const std::int32_t camCount = static_cast<std::int32_t>(
                            mdl::Mdl(model)->boneOrderCount);
                        const char* A = reinterpret_cast<const char*>(
                            mdl::SelectorStates(frames[cur]));
                        const char* E = reinterpret_cast<const char*>(
                            mdl::SelectorStates(fPrev));
                        const char* B = reinterpret_cast<const char*>(
                            mdl::SelectorStates(fNext));
                        bool camOk = true;
                        for (std::int32_t j = 0; j < camCount; ++j) {
                            const char* pA = A + 8 * j;
                            if (*reinterpret_cast<const std::int32_t*>(B + 8 * j) !=
                                    *reinterpret_cast<const std::int32_t*>(pA) ||
                                *reinterpret_cast<const std::int32_t*>(B + 8 * j + 4) !=
                                    *reinterpret_cast<const std::int32_t*>(pA + 4) ||
                                *reinterpret_cast<const std::int32_t*>(E + 8 * j + 4) !=
                                    *reinterpret_cast<const std::int32_t*>(pA + 4) ||
                                *reinterpret_cast<const std::int32_t*>(E + 8 * j) !=
                                    *reinterpret_cast<const std::int32_t*>(pA)) {
                                camOk = false;
                                break;
                            }
                        }
                        if (camOk) {
                            ++deleted;
                            frames[cur].allocated = 1;
                        }
                    }
                }
                model = ActiveModel(app);
                frames = mdl::DisplayKeys(model);
                cur = static_cast<std::int32_t>(frames[cur].next);
                next = static_cast<std::int32_t>(frames[cur].next);
            }
        }

        DeleteMarkedKeyframes(app);                              
        app->state.enterKeyState = 1;
        char text[0x100];
        if (app->EnglishUI() != 0) {
            sprintf_s(text, 0x100, "%d point was deleted.", deleted);
            MessageBoxA(MainHwnd(app), text, "delete unused frame", MB_TOPMOST);
        } else {
            sprintf_s(text, 0x100, kFmtPointsDeletedJp, deleted);
            MessageBoxA(MainHwnd(app), text, kCaptionDelUnusedJp, MB_TOPMOST);
        }
        app->SceneModified() = 1;
        break;
    }

    case 224: {
        SetCurrentDirectoryW(app->ExeDir());
        app->state.enterKeyState = 1;
        wchar_t path[0x100];
        swprintf_s(path, 0x100, kEmptyPathFormat, L"", L"");
        OPENFILENAMEW ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner =
            app->state.floatingWindow != 0
                ? reinterpret_cast<HWND>(app->state.floatingWindow)
                : MainHwnd(app);
        ofn.lpstrFilter = L"vsq files(*.vsq)\0*.vsq\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = 0x100;
        ofn.Flags = OFN_FILEMUSTEXIST;
        ofn.lpstrInitialDir = L"UserFile\\Vsq";
        ofn.lpstrDefExt = L"vsq";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = app->EnglishUI() != 0
                             ? L"load vsq data"          
                             : L"vsq\x30C7\x30FC\x30BF\x8AAD\x8FBC";  
        if (GetOpenFileNameW(&ofn)) {
            LoadVsqFile(app, path);                     
            app->SceneModified() = 1;
        }
        break;
    }

    case 223: {
        app->state.dialogFlags[3] = 1;
        app->state.enterKeyState = 1;
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t path[0x100];
        swprintf_s(path, 0x100, kEmptyPathFormat, L"", L"");
        OPENFILENAMEW ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = MainHwnd(app);
        ofn.lpstrFilter = L"AVI files(*.avi)\0*.avi\0";   
        ofn.lpstrFile = path;
        if ((GetMenuState(GetMenu(MainHwnd(app)), 0x12D, 0) & 8) != 0) {
            ofn.lpstrInitialDir = app->DirUser();          
        } else {
            ofn.lpstrInitialDir = L"UserFile";             
        }
        ofn.nFilterIndex = 1;
        ofn.nMaxFile = 0x100;
        ofn.Flags = (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY);
        ofn.lpstrDefExt = L"avi";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = app->EnglishUI() != 0
                             ? L"output AVI file"   
                             : L"AVI\x51FA\x529B";  
        if (!GetSaveFileNameW(&ofn)) {
            break;
        }
        if ((GetMenuState(GetMenu(MainHwnd(app)), 0x12D, 0) & 8) != 0) {
            wchar_t* dir = ExtractDirFromPath(
                app->PathWorkspace().projectDirectory, path);
            CopyDirPathW(app->DirUser(), dir);   
        }
        CopyPathW(app->AviOutputPath(), path);  
        app->state.enterKeyState = 1;
        const std::intptr_t r = DialogBoxParamA(
            static_cast<HINSTANCE>(app->HInstance()),
            MAKEINTRESOURCEA(app->EnglishUI() != 0 ? 0x28E : 0x260),
            MainHwnd(app), AviOutDlgProc, 0);
        if (r == 2) {  
            break;
        }
        if (app->AviStereoOutput() != 0) {
            StartAviRecordFullscreen(app);  
        } else {
            StartAviRecordWindow(app);  
        }
        break;
    }

    case 225: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        app->state.enterKeyState = 1;
        const std::intptr_t r = DialogBoxParamA(
            static_cast<HINSTANCE>(app->HInstance()),
            MAKEINTRESOURCEA(app->EnglishUI() != 0 ? 0x28F : 0x267),
            MainHwnd(app), FrameShiftDlgProc, 0);
        if (r == 2) {  
            break;
        }
        unsigned char* model = ActiveModel(app);
        const std::int32_t morphCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->morphCount);
        for (std::uint16_t m = 0; static_cast<std::int32_t>(m) < morphCount;
             ++m) {
            model = ActiveModel(app);
            const mdl::MorphRecord* const morphs = mdl::Morphs(model);
            if (morphs[m].type != 3) {
                continue;
            }
            unsigned char* frames = reinterpret_cast<unsigned char*>(
                mdl::MorphKeys(model));
            std::int32_t cur = *reinterpret_cast<std::int32_t*>(
                frames + 0x14 * m + 8);  
            if (cur == 0) {
                continue;
            }
            for (;;) {
                model = ActiveModel(app);
                frames = reinterpret_cast<unsigned char*>(
                    mdl::MorphKeys(model));
                const std::int32_t next = *reinterpret_cast<std::int32_t*>(
                    frames + 0x14 * cur + 8);
                if (next != 0) {
                    if (cur == m) {  
                        cur = next;
                        continue;
                    }
                    std::int32_t v = *reinterpret_cast<std::int32_t*>(
                                         frames + 0x14 * cur + 0) +
                                     app->MorphFrameShift();
                    *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 0) =
                        v;
                    if (v > 0) {
                        cur = *reinterpret_cast<std::int32_t*>(
                            frames + 0x14 * cur + 8);
                        continue;  
                    }
                    const std::int32_t oldNext =
                        *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 8);
                    *reinterpret_cast<std::int32_t*>(frames + 0x14 * m + 8) =
                        oldNext;
                    *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 0) = 0;
                    *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 8) = 0;
                    *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 4) = 0;
                    frames[0x14 * cur + 0x10] = 0;
                    *reinterpret_cast<float*>(frames + 0x14 * cur + 0xC) = 0.0f;
                    cur = oldNext;
                    continue;
                }
                if (cur == m) {
                    break;
                }
                std::int32_t v = *reinterpret_cast<std::int32_t*>(
                                     frames + 0x14 * cur + 0) +
                                 app->MorphFrameShift();
                *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 0) = v;
                if (v > 0) {
                    break;
                }
                *reinterpret_cast<std::int32_t*>(frames + 0x14 * m + 8) =
                    *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 8);
                *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 0) = 0;
                *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 8) = 0;
                *reinterpret_cast<std::int32_t*>(frames + 0x14 * cur + 4) = 0;
                frames[0x14 * cur + 0x10] = 0;
                *reinterpret_cast<float*>(frames + 0x14 * cur + 0xC) = 0.0f;
                break;
            }
        }
        FramePurgeTail(app);
        break;
    }

    case 226: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        app->state.enterKeyState = 1;
        const int r = MessageBoxA(
            MainHwnd(app),
            app->EnglishUI() != 0
                ? "Trying to delete all lip frame.\n"
                  "You cannot undo this oparation.\n\nAre you OK?"
                : kMsgDelLipJp,
            app->EnglishUI() != 0 ? "delete lip frame" : kCaptionDelLipJp,
            (MB_OKCANCEL | MB_TOPMOST));
        if (r != IDOK) {
            break;
        }
        PurgeMorphFrames(app, 3);
        FramePurgeTail(app);
        break;
    }

    case 227: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        unsigned char* model = ActiveModel(app);
        const std::int32_t morphCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->morphCount);
        std::int32_t blinkMorph = -1;  
        if (morphCount > 0) {
            const mdl::MorphRecord* const morphs = mdl::Morphs(model);
            for (std::uint16_t m = 0;
                 static_cast<std::int32_t>(m) < morphCount; ++m) {
                if (memcmp(morphs[m].name, kNameMabataki, 9) == 0) {
                    blinkMorph = m;
                    break;
                }
            }
        }
        if (blinkMorph < 0) {
            app->state.enterKeyState = 1;
            MessageBoxA(
                MainHwnd(app),
                app->EnglishUI() != 0
                    ? "You cannot register blinking because this model has "
                      "not facial motion of 'blink'."
                    : kMsgNoBlinkMorphJp,
                app->EnglishUI() != 0 ? "randomly register blinking"
                                      : kCaptionRegBlinkJp,
                MB_TOPMOST);
            break;
        }
        app->state.enterKeyState = 1;
        const std::intptr_t r = DialogBoxParamA(
            static_cast<HINSTANCE>(app->HInstance()),
            MAKEINTRESOURCEA(app->EnglishUI() != 0 ? 0x290 : 0x269),
            MainHwnd(app), BlinkRegisterDlgProc, 0);
        if (r == 2) {  
            break;
        }
        std::int32_t frame = app->BlinkStartFrame();
        std::int32_t count = 0;
        while (frame < app->BlinkEndFrame()) {
            const std::int32_t q = (rand() * 230) / 32768;  
            const std::int8_t phase = static_cast<std::int8_t>(app->state.blinkPhase);
            bool registerKeys = false;
            if (q < 90 && phase <= 0) {
                frame += 7;
                app->state.blinkPhase = 3;
                registerKeys = true;
            } else if (q > 50) {
                frame += q;
                app->state.blinkPhase =
                    static_cast<std::uint8_t>(phase - 1);
                registerKeys = true;
            }
            if (registerKeys) {
                model = ActiveModel(app);
                mdl::Morphs(model)[blinkMorph].value = 0.0f;
                RegisterMorphKeyCurrent(model, blinkMorph, frame);
                model = ActiveModel(app);
                mdl::Morphs(model)[blinkMorph].value = 1.0f;
                RegisterMorphKeyCurrent(model, blinkMorph, frame + 2);
                model = ActiveModel(app);
                mdl::Morphs(model)[blinkMorph].value = 1.0f;
                RegisterMorphKeyCurrent(model, blinkMorph, frame + 3);
                model = ActiveModel(app);
                mdl::Morphs(model)[blinkMorph].value = 0.0f;
                RegisterMorphKeyCurrent(model, blinkMorph, frame + 6);
                ++count;
            }
        }
        model = ActiveModel(app);
        const std::int32_t frameCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->maxFrame);
        if (app->state.lastRegisteredFrame < frameCount) {
            app->state.lastRegisteredFrame = frameCount;
        }
        app->state.enterKeyState = 1;
        char text[0x100];
        if (app->EnglishUI() != 0) {
            sprintf_s(text, 0x100, "%d blinking is registerd", count);
            MessageBoxA(MainHwnd(app), text, "register blinking", MB_TOPMOST);
        } else {
            sprintf_s(text, 0x100, kFmtBlinksAddedJp, count);
            MessageBoxA(MainHwnd(app), text, kCaptionBlinkCntJp, MB_TOPMOST);
        }
        app->SceneModified() = 1;
        break;
    }

    case 228: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        app->state.enterKeyState = 1;
        const int r = MessageBoxA(
            MainHwnd(app),
            app->EnglishUI() != 0
                ? "Trying to delete all eyes frame.\n"
                  "You cannot undo this oparation.\n\nAre you OK?"
                : kMsgDelEyesJp,
            app->EnglishUI() != 0 ? "delete lip frame" : kCaptionNewJp,
            (MB_OKCANCEL | MB_TOPMOST));
        if (r != IDOK) {
            break;
        }
        PurgeMorphFrames(app, 2);
        FramePurgeTail(app);
        break;
    }

    case 250: {
        app->state.dialogFlags[15] = 1;
        const std::uint8_t opt = app->state.optflag[0];
        const bool isAcc = opt != 0;
        const auto& clipboardCounts = app->ClipboardCounts();
        if (isAcc ? clipboardCounts.accessories == 0
                  : clipboardCounts.bones == 0) {
            break;
        }
        const char* msg = app->EnglishUI() != 0
                              ? (isAcc
                                     ? "Trying to paste accessary flame data "
                                       "to difference accessary\nwhich is "
                                       "selected for manipulate.\n\nAre you ok?"
                                     : "Trying to paste flame data to "
                                       "difference bone\nwhich is selected "
                                       "for manipulate.\n\nAre you ok?")
                              : (isAcc ? kMsgPasteAccJp : kMsgPasteBoneJp);
        const char* cap = app->EnglishUI() != 0 ? "paste to difference flame"
                                                : kCaptionPasteJp;
        const std::uint32_t flags =
            app->state.floatingWindow != 0 ? (MB_OKCANCEL | MB_TOPMOST) : MB_OKCANCEL;
        if (MessageBoxA(MainHwnd(app), msg, cap, flags) != 1) {
            break;
        }
        if (!isAcc) {
            unsigned char* model = ActiveModel(app);
            if (mdl::Mdl(model)->selectedBone == -1) {
                MessageBoxA(
                    MainHwnd(app),
                    app->EnglishUI() != 0 ? "Please select bone."
                                          : kMsgSelectBoneJp,
                    cap,
                    app->state.floatingWindow != 0
                        ? MB_TOPMOST
                        : MB_OK);
                break;
            }
            app->SceneModified() = 1;
            {
                model = ActiveModel(app);
                mdl::BoneKey* f = mdl::BoneKeys(model);
                for (std::size_t i = 0; i < mdl::kBoneKeyCapacity; ++i)
                    f[i].allocated = 0;
            }
            {
                model = ActiveModel(app);
                mdl::MorphKey* f = mdl::MorphKeys(model);
                for (std::size_t i = 0; i < mdl::kMorphKeyCapacity; ++i)
                    f[i].allocated = 0;
            }
            {
                model = ActiveModel(app);
                mdl::DisplayKey* f = mdl::DisplayKeys(model);
                for (std::size_t i = 0; i < mdl::kDisplayKeyCapacity; ++i)
                    f[i].allocated = 0;
            }
            EnableWindow(GetDlgItem(MainHwnd(app), panel::kUndoButton), TRUE);
            EnableWindow(GetDlgItem(MainHwnd(app), panel::kRedoButton), FALSE);
            model = ActiveModel(app);
            mdl::Mdl(model)->undoDirty = 1;
            mdl::Mdl(model)->redoDirty = 0;
            std::int32_t idx =
                static_cast<std::int32_t>(mdl::Mdl(model)->undoState[0]);
            ++idx;
            if (idx >= 0x1E) {
                idx = 0;
            }
            mdl::Mdl(model)->undoState[0] = static_cast<std::uint32_t>(idx);
            mdl::Mdl(model)->undoState[1] = static_cast<std::uint32_t>(idx);
            mdl::UndoRecord& undo =
                mdl::Mdl(model)->undoRings[0].slots[idx];
            undo.operation = 2;
            undo.frame = app->state.currentFrame;
            if (undo.bonePose != nullptr) {
                ::operator delete(undo.bonePose);
                undo.bonePose = nullptr;
            }
            const std::int32_t boneCount =
                static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);
            auto* blob = static_cast<mdl::BonePoseSnapshot*>(
                ::operator new(MulOrMax(static_cast<std::uint32_t>(boneCount),
                                        sizeof(mdl::BonePoseSnapshot))));
            if (blob != nullptr) {
                ConstructArrayElements(blob, sizeof(mdl::BonePoseSnapshot),
                                       static_cast<std::uint32_t>(boneCount),
                                       &IdentityCtor);
            }
            undo.bonePose = blob;
            memset(blob, 0, static_cast<std::size_t>(boneCount) *
                                sizeof(mdl::BonePoseSnapshot));
            model = ActiveModel(app);
            const mdl::ModelRecord* const record = mdl::Mdl(model);
            const unsigned char* const physicsState = record->bonePhysicsState;
            const mdl::BoneRecord* const bones = mdl::Bones(model);
            for (std::int32_t i = 0; i < boneCount; ++i) {
                mdl::BonePoseSnapshot& rec = blob[i];
                rec.boneIndex = i;
                memcpy(rec.position, bones[i].trans, sizeof rec.position);
                memcpy(rec.rotation, bones[i].rotQuat, sizeof rec.rotation);
                rec.physicsDisabled = physicsState[i];
            }
            model = ActiveModel(app);
            undo.dirty = 0;
            if (undo.auxiliaryPose != nullptr) {
                ::operator delete(undo.auxiliaryPose);
                undo.auxiliaryPose = nullptr;
            }
            const std::int32_t recCount = static_cast<std::int32_t>(
                app->ClipboardCounts().bones);
            unsigned char* blob2 = static_cast<unsigned char*>(
                ::operator new(MulOrMax(static_cast<std::uint32_t>(3 * recCount),
                                        0x40u)));
            if (blob2 != nullptr) {
                ConstructArrayElements(blob2, 0x40,
                                       static_cast<std::uint32_t>(3 * recCount),
                                       &IdentityCtor);
            }
            model = ActiveModel(app);
            undo.auxiliaryPose = blob2;
            memset(blob2, 0, static_cast<std::size_t>(3 * recCount) * 0x40u);
            memset(mdl::Mdl(model)->keyVisitMap, 0,
                    sizeof(mdl::Mdl(model)->keyVisitMap));
            ResetBoneKeyCursor(model);
            if (recCount > 0) {
                for (std::int32_t j = 0; j < recCount; ++j) {
                    const mdl::BoneClipboardRecord* src =
                        app->BoneClipboard() + j;
                    unsigned char stackRec[0x54];
                    memcpy(stackRec, src, 0x54);
                    model = ActiveModel(app);
                    if (!RegisterBoneKey(model, stackRec,
                                   app->state.currentFrame,
                                   1)) {
                        break;
                    }
                }
            }
            PanelPaint(app);
            SelectionReeval(app);
            model = ActiveModel(app);
            SeekModelFrame(model, app->state.currentFrame,
                      app->PlaybackPhysicsMode());
            app->PhysicsResetPending() = 1;
            break;
        }
        const std::uint8_t slotIdx = app->SelectedObjectSlot();
        if (app->AccessorySlot(slotIdx) == nullptr) {
            MessageBoxA(
                MainHwnd(app),
                app->EnglishUI() != 0 ? "Please select accessary."
                                      : kMsgSelectAccJp,
                cap,
                app->state.floatingWindow != 0 ? MB_TOPMOST
                                                                  : MB_OK);
            break;
        }
        app->SceneModified() = 1;
        for (std::size_t key = 0; key < 10000; ++key) {
            app->CameraKeys()[key].selected = 0;
            app->LightKeys()[key].selected = 0;
            app->ShadowKeys()[key].selected = 0;
            app->GravityKeys()[key].selected = 0;
        }
        for (int tbl = 0; tbl < 0xFF; ++tbl) {
            unsigned char* t = reinterpret_cast<unsigned char*>(
                app->AccessoryKeys(tbl));
            for (std::size_t off = 0; off < 0x927C0u; off += 0x3C) {
                t[off + 0x18] = 0;
            }
        }
        const std::int32_t accCount = static_cast<std::int32_t>(
            app->ClipboardCounts().accessories);
        for (std::int32_t i = 0; i < accCount; ++i) {
            const mdl::AccessoryClipboardKey* src =
                app->AccessoryClipboard() + i;
            unsigned char stackRec[0x34];
            memcpy(stackRec, src, 0x34);
            if (!PasteAccessoryKeyRecord(app, stackRec, 1)) {
                break;
            }
        }
        PanelPaint(app);
        SelectionReeval(app);
        ReloadModels(app);
        RefreshLightPanel(app);
        RefreshSelfShadowPanel(app);
        ApplyGravityTrack(app);
        for (std::int32_t i = 0; i < 0xFF; ++i) {
            if (app->AccessorySlot(i) != nullptr) {
                ApplyAccessoryTrack(app, i);
            }
        }
        SyncAccessoryEditPanel(app);
        break;
    }

    case 229: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        app->SceneModified() = 1;
        {
            unsigned char* m = ActiveModel(app);
            mdl::BoneKey* f = mdl::BoneKeys(m);
            for (std::size_t i = 0; i < mdl::kBoneKeyCapacity; ++i)
                f[i].allocated = 0;
        }
        {
            unsigned char* m = ActiveModel(app);
            mdl::MorphKey* f = mdl::MorphKeys(m);
            for (std::size_t i = 0; i < mdl::kMorphKeyCapacity; ++i)
                f[i].allocated = 0;
        }
        {
            unsigned char* m = ActiveModel(app);
            mdl::DisplayKey* f = mdl::DisplayKeys(m);
            for (std::size_t i = 0; i < mdl::kDisplayKeyCapacity; ++i)
                f[i].allocated = 0;
        }
        unsigned char* model = ActiveModel(app);
        const std::int32_t morphCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->morphCount);
        if (morphCount > 0) {
            for (std::uint16_t m = 0; static_cast<std::int32_t>(m) < morphCount;
                 ++m) {
                model = ActiveModel(app);
                RegisterMorphKeyCurrent(model, m, app->state.currentFrame);
                model = ActiveModel(app);
                const std::int32_t frameCount =
                    static_cast<std::int32_t>(mdl::Mdl(model)->maxFrame);
                if (app->state.lastRegisteredFrame < frameCount) {
                    app->state.lastRegisteredFrame = frameCount;
                }
            }
        }
        model = ActiveModel(app);
        SeekModelFrame(model, app->state.currentFrame,
                  app->PlaybackPhysicsMode());
        PanelPaint(app);      
        SelectionReeval(app); 
        app->SceneModified() = 1;
        break;
    }

    case 230: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        unsigned char* model = ActiveModel(app);
        const std::int32_t morphCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->morphCount);
        if (mdl::Morphs(model) != nullptr && morphCount > 0) {
            for (std::int32_t i = 0; i < morphCount; ++i) {
                model = ActiveModel(app);
                mdl::Morphs(model)[i].value = 0.0f;
            }
        }
        const HWND main = MainHwnd(app);
        SetWindowTextA(GetDlgItem(main, panel::kMorphEdit0), "0.00000");
        SetWindowTextA(GetDlgItem(main, panel::kMorphEdit1), "0.00000");
        SetWindowTextA(GetDlgItem(main, panel::kMorphEdit2), "0.00000");
        SetWindowTextA(GetDlgItem(main, panel::kMorphEdit3), "0.00000");
        SendMessageA(GetDlgItem(main, panel::kMorphSlider0), TBM_SETPOS, 1, 0);
        SendMessageA(GetDlgItem(main, panel::kMorphSlider1), TBM_SETPOS, 1, 0);
        SendMessageA(GetDlgItem(main, panel::kMorphSlider2), TBM_SETPOS, 1, 0);
        SendMessageA(GetDlgItem(main, panel::kMorphSlider3), TBM_SETPOS, 1, 0);
        break;
    }

    case 231: {
        if (app->state.optflag[0] != 0) {  
            break;
        }
        app->state.enterKeyState = 1;
        const int r = MessageBoxA(
            MainHwnd(app),
            app->EnglishUI() != 0
                ? "Trying to delete all eyebrow frame.\n"
                  "You cannot undo this oparation.\n\nAre you OK?"
                : kMsgDelEyebrowJp,
            app->EnglishUI() != 0 ? "delete lip frame" : kCaptionNewJp,
            (MB_OKCANCEL | MB_TOPMOST));
        if (r != IDOK) {
            break;
        }
        PurgeMorphFrames(app, 1);
        FramePurgeTail(app);
        break;
    }

    case 247: {
        app->state.dialogFlags[2] = 1;
        if (app->state.followCameraEnabled == 0) {
            app->state.followCameraEnabled = 1;
            app->ViewOffsetX() = 0.0f;
            app->ViewOffsetY() = 0.0f;
            CheckMenuItem(GetMenu(MainHwnd(app)), 0xF7, MF_CHECKED);
            SendMessageA(GetDlgItem(MainHwnd(app), panel::kFollowCameraCheckbox), BM_SETCHECK,
                         1, 0);
            ReloadModels(app);                             
            RefreshLightPanel(app);                                
            RefreshSelfShadowPanel(app);                                
            ApplyGravityTrack(app);                                
            for (std::int32_t i = 0; i < 0xFF; ++i) {
                if (app->AccessorySlot(i) != nullptr) {
                    ApplyAccessoryTrack(app, i);                     
                }
            }
            SyncAccessoryEditPanel(app);                                
            if (app->state.optflag[0] == 0) {  
                app->CameraAttachmentTransformSuppressed() = 0;
                PostModelReload(app);                      
            }
            PostViewRefresh(app);                          
            PostLanguageSweep2(app);                       
        } else {
            app->state.followCameraEnabled = 0;
            CheckMenuItem(GetMenu(MainHwnd(app)), 0xF7, MF_UNCHECKED);
            SendMessageA(GetDlgItem(MainHwnd(app), panel::kFollowCameraCheckbox), BM_SETCHECK,
                         0, 0);
            if (app->state.optflag[0] != 0) {  
                PostViewRefresh(app);                      
                PostLanguageSweep2(app);                   
                break;
            }
            const std::int32_t cam = app->CameraParentModel();
            if (cam != app->SelectedModelSlot() ||
                cam < 0) {
                PostViewRefresh(app);
                PostLanguageSweep2(app);
                break;
            }
            app->CameraAttachmentTransformSuppressed() = 0;
            PostModelReload(app);                          
            PostViewRefresh(app);                          
            PostLanguageSweep2(app);                       
        }
        break;
    }

    case 232: {
        app->state.enterKeyState = 1;
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t path[0x100];
        swprintf_s(path, 0x100, kEmptyPathFormat, L"", L"");
        OPENFILENAMEW ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner =
            app->state.floatingWindow != 0
                ? reinterpret_cast<HWND>(app->state.floatingWindow)
                : MainHwnd(app);
        if (app->EnglishUI() != 0) {
            ofn.lpstrFilter =
                L"All Picture files\0*.bmp;*.jpg;*.png;*.tga;*.dds;*.dib;"
                L"*.pfm;*.hdr\0"
                L"Bmp files(*.bmp)\0*.bmp\0"
                L"Jpeg files(*.jpg)\0*.jpg\0"
                L"Png files(*.png)\0*.png\0"
                L"Tga files(*.tga)\0*.tga\0"
                L"Dds files(*.dds)\0*.dds\0"
                L"DIB files(*.dib)\0*.dib\0"
                L"pfm files(*.pfm)\0*.pfm\0"
                L"HDR files(*.hdr)\0*.hdr\0\0";
        } else {
            ofn.lpstrFilter =
                L"\x5168\x3066\x306E\x5BFE\x5FDC\x30D5\x30A9\x30FC\x30DE"
                L"\x30C3\x30C8\0*.bmp;*.jpg;*.png;*.tga;*.dds;*.dib;*.pfm;"
                L"*.hdr\0"
                L"Bmp files(*.bmp)\0*.bmp\0"
                L"Jpeg files(*.jpg)\0*.jpg\0"
                L"Png files(*.png)\0*.png\0"
                L"Tga files(*.tga)\0*.tga\0"
                L"Dds files(*.dds)\0*.dds\0"
                L"DIB files(*.dib)\0*.dib\0"
                L"pfm files(*.pfm)\0*.pfm\0"
                L"HDR files(*.hdr)\0*.hdr\0\0";
        }
        ofn.lpstrFile = path;
        ofn.nMaxFile = 0x100;
        ofn.Flags = OFN_FILEMUSTEXIST;
        if ((GetMenuState(GetMenu(MainHwnd(app)), 0x12D, 0) & 8) != 0) {
            ofn.lpstrInitialDir = app->DirBg();           
        } else {
            ofn.lpstrInitialDir = L"UserFile\\BackGround";
        }
        ofn.lpstrDefExt = L"bmp";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = app->EnglishUI() != 0
                             ? L"load picture data"       
                             : L"\x80CC\x666F\x753B\x50CF"  
                               L"\x30C7\x30FC\x30BF\x8AAD\x8FBC";  
        if (!GetOpenFileNameW(&ofn)) {
            break;
        }
        if ((GetMenuState(GetMenu(MainHwnd(app)), 0x12D, 0) & 8) != 0) {
            wchar_t* dir = ExtractDirFromPath(
                app->PathWorkspace().projectDirectory, path);
            CopyDirPathW(app->DirBg(), dir);                 
        }
        CopyPathW(app->state.pictureBackgroundPath,
                  path);                                  
        LoadBackgroundPicture(app);                       
        app->SceneModified() = 1;
        break;
    }

    case 233: {
        app->state.dialogFlags[12] = 1;
        if (app->state.pictureBackgroundEnabled != 0) {
            app->state.pictureBackgroundEnabled = 0;
            CheckMenuItem(GetMenu(MainHwnd(app)), 0xE9, MF_UNCHECKED);
            break;
        }
        if (app->PictureBackgroundTexture() == 0) {
            break;
        }
        app->state.pictureBackgroundEnabled = 1;
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xE9, MF_CHECKED);
        break;
    }

    case 234:  
        app->state.fpsLimit = 1000.0f;
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEB, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEC, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEA, MF_CHECKED);
        break;

    case 235:  
        app->state.fpsLimit = 30.0f;
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEA, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEC, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEB, MF_CHECKED);
        break;

    case 236:  
        app->state.fpsLimit = 60.0f;
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEB, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEA, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xEC, MF_CHECKED);
        break;

    case 242: {
        app->state.dialogFlags[7] = 1;
        if (app->state.optflag[0] == 0) {  
            break;
        }
        app->state.enterKeyState = 1;
        DialogBoxParamA(
            static_cast<HINSTANCE>(app->HInstance()),
            MAKEINTRESOURCEA(app->EnglishUI() != 0 ? 0x291 : 0x26F),
            MainHwnd(app), CameraFrameMultiplyDlgProc, 0);
        break;
    }

    case 237: {  
        app->state.dialogFlags[2] = 1;
        std::int32_t esi = 0;
        auto* frames = app->CameraKeys();
        for (std::size_t key = 0; key < 10000; ++key) {
            frames[key].selected = 0;
            const bool used = frames[key].frame != 0;
            if (used || esi == 0) {
                frames[key].selected = 1;
            }
            ++esi;
        }
        PanelPaint(app);      
        SelectionReeval(app); 
        break;
    }

    case 238: {  
        app->state.dialogFlags[17] = 1;
        std::int32_t esi = 0;
        auto* frames = app->LightKeys();
        for (std::size_t key = 0; key < 10000; ++key) {
            frames[key].selected = 0;
            const bool used = frames[key].frame != 0;
            if (used || esi == 0) {
                frames[key].selected = 1;
            }
            ++esi;
        }
        PanelPaint(app);
        break;
    }

    case 239: {  
        app->state.dialogFlags[8] = 1;
        std::int32_t esi = 0;
        auto* frames = app->ShadowKeys();
        for (std::size_t key = 0; key < 10000; ++key) {
            frames[key].selected = 0;
            const bool used = frames[key].frame != 0;
            if (used || esi == 0) {
                frames[key].selected = 1;
            }
            ++esi;
        }
        PanelPaint(app);
        break;
    }

    case 240: {  
        app->state.dialogFlags[3] = 1;
        std::int32_t esi = 0;
        auto* frames = app->GravityKeys();
        for (std::size_t key = 0; key < 10000; ++key) {
            frames[key].selected = 0;
            const bool used = frames[key].frame != 0;
            if (used || esi == 0) {
                frames[key].selected = 1;
            }
            ++esi;
        }
        PanelPaint(app);
        break;
    }

    case 241: {  
        app->state.dialogFlags[5] = 1;
        for (int tbl = 0; tbl < 0xFF; ++tbl) {
            unsigned char* frames = reinterpret_cast<unsigned char*>(
                app->AccessoryKeys(tbl));
            std::int32_t esi = 0;  
            for (std::size_t off = 0; off < 0x927C0u; off += 0x3C) {
                frames[off + 0x18] = 0;
                const bool used =
                    *reinterpret_cast<std::int32_t*>(frames + off) != 0;
                if (used || esi == 0) {
                    frames[off + 0x18] = 1;
                }
                ++esi;
            }
        }
        PanelPaint(app);
        break;
    }

    case 243: {  
        app->state.dialogFlags[3] = 1;
        app->CaptureMode() = ScreenCaptureMode::Disabled;
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF3, MF_CHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF4, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF5, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF6, MF_UNCHECKED);
        break;
    }

    case 244: {  
        app->CaptureMode() = ScreenCaptureMode::FullFrame;
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF3, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF4, MF_CHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF5, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF6, MF_UNCHECKED);
        break;
    }

    case 245: {  
        app->CaptureMode() = ScreenCaptureMode::CropFourByThree;
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF3, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF4, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF5, MF_CHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF6, MF_UNCHECKED);
        break;
    }

    case 246: {  
        app->state.dialogFlags[6] = 1;
        app->CaptureMode() = ScreenCaptureMode::BackgroundRefresh;
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF3, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF4, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF5, MF_UNCHECKED);
        CheckMenuItem(GetMenu(MainHwnd(app)), 0xF6, MF_CHECKED);
        break;
    }

    case 248: {
        app->state.dialogFlags[0] = 1;
        if (app->GroundShadowColorDialog() != 0) {
            break;  
        }
        HWND dlg = CreateDialogParamA(
            static_cast<HINSTANCE>(app->HInstance()),
            MAKEINTRESOURCEA(app->EnglishUI() != 0 ? 0x292 : 0x270),
            MainHwnd(app), GroundShadowColorDlgProc, 0);
        app->GroundShadowColorDialog() = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        break;
    }

    case 249: {
        app->state.dialogFlags[8] = 1;
        app->state.enterKeyState = 1;
        const std::intptr_t r = DialogBoxParamA(
            static_cast<HINSTANCE>(app->HInstance()),
            MAKEINTRESOURCEA(app->EnglishUI() != 0 ? 0x293 : 0x273),
            MainHwnd(app), AccessorySettingsDlgProc, 0);
        if (r == 2) {  
            break;
        }
        break;  
    }

    default:
        break;
    }
}

}  
