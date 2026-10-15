#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>

#include "rinmotiondance/bottom_panel_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"

namespace rinmotiondance {
namespace {

constexpr char kJpPlaying[] = "\x8d\xc4\x90\xb6\x92\x86";  
constexpr char kJpAngle[] = "\x8a\x70\x93\x78";          
constexpr char kJpBonePosition[] = "\x83\x7b\x81\x5b\x83\x93\x88\xca\x92\x75";  
constexpr char kJpCameraFollowMode[] = "\x83\x4a\x83\x81\x83\x89\x83\x7b\x81\x5b\x83\x93\x92\xc7\x8f\x5d\x83\x82\x81\x5b\x83\x68\x92\x86";  
constexpr char kJpCameraFollowModeHint[] = "\x83\x4a\x83\x81\x83\x89\x83\x7b\x81\x5b\x83\x93\x92\xc7\x8f\x5d\x83\x82\x81\x5b\x83\x68\x92\x86\x20\x28\x92\xc7\x8f\x5d\x83\x7b\x83\x5e\x83\x93\x82\xf0\x89\xf0\x8f\x9c\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2\x29";  
constexpr char kJpCameraCenter[] = "\x83\x4a\x83\x81\x83\x89\x92\x86\x90\x53";  
constexpr char kJpCamLightAccessoryPanel[] = "\xb6\xd2\xd7\xa5\x8f\xc6\x96\xbe\xa5\xb1\xb8\xbe\xbb\xd8";  
constexpr char kJpCamLightAccessoryPanelFmt[] = "\xb6\xd2\xd7\xa5\x8f\xc6\x96\xbe\xa5\xb1\xb8\xbe\xbb\xd8\x20\x2f\x20\x25\x73";  

}  

void HandlePaletteChanged(HDC hdc) {
    MMDApp* app = g_Block;  

    RECT rc;
    GetClientRect(reinterpret_cast<HWND>(app->state.hwnd),
                  &rc);                                     
    const std::int32_t bottom = rc.bottom;
    const std::int32_t side = app->SidebarWidth();
    const COLORREF colA = app->ThemeColor(UiThemeColor::ControlLight);
    const COLORREF colB = app->ThemeColor(UiThemeColor::ControlDark);

    float t = static_cast<float>(-45.0 / ((50.0 - static_cast<double>(bottom)) - 161.0));
    std::uint32_t lerp = ColorLerp(colA, colB, t);          
    FillPanelBottom(hdc, 3, 50, side - 3, 95, colA, lerp, 1);      

    t = static_cast<float>(((50.0 - static_cast<double>(bottom)) - 228.0) /
                           ((50.0 - static_cast<double>(bottom)) - 161.0));
    const std::uint32_t lerp2 = ColorLerp(colA, colB, t);   
    FillPanelBottom(hdc, 3, 95, 6, bottom - 228, lerp, lerp2, 1);  

    const double row3Span = (50.0 - static_cast<double>(bottom)) - 161.0;
    const std::uint32_t lerpNeg111 = ColorLerp(colA, colB, static_cast<float>(-111.0 / row3Span));  
    const std::uint32_t lerpNeg94 = ColorLerp(colA, colB, static_cast<float>(-94.0 / row3Span));    
    FillPanelBottom(hdc, side - 19, 144, side - 3, 161, lerpNeg94, lerpNeg111, 1);  

    t = static_cast<float>(((50.0 - static_cast<double>(bottom)) - 248.0) /
                           ((50.0 - static_cast<double>(bottom)) - 161.0));
    lerp = ColorLerp(colA, colB, t);                        
    FillPanelBottom(hdc, 3, bottom - 248, side - 3, bottom - 161, lerp, colB, 1);  

    FillPanelBottom(hdc, 3, bottom - 157, 215, bottom - 3,
                    app->ThemeColorAt(6), app->ThemeColorAt(7),
                    1);                                          
    FillPanelBottom(hdc, 3, 3, side - 3, 47,
                    app->ThemeColorAt(4), app->ThemeColorAt(5),
                    1);                                          
    FillPanelBottom(hdc, 218, bottom - 157, 358, bottom - 3,
                    app->ThemeColorAt(8), app->ThemeColorAt(9),
                    1);                                          

    const COLORREF colC = app->ThemeColorAt(10);
    const COLORREF colD = app->ThemeColorAt(11);
    const BottomPanelLayout panelLayout = ComputeBottomPanelLayout(app);
    std::int32_t xOff = panelLayout.leading;
    if (app->state.optflag[0] != 0) {   
        if (app->state.optflag[1] != 0) {  
            FillPanelBottom(hdc, 0x169, bottom - 157, 0x202, bottom - 3, colC, colD, 1);  
        } else {
            FillPanelBottom(hdc, 0x169, bottom - 157, 0x17B, bottom - 3, colC, colD, 1);  
        }
    } else if (app->state.optflag[4] != 0) {  
        FillPanelBottom(hdc, 0x169, bottom - 157, 0x22E, bottom - 3, colC, colD, 1);  
    } else {
        FillPanelBottom(hdc, 0x169, bottom - 157, 0x17B, bottom - 3, colC, colD, 1);  
    }

    const COLORREF colE = app->ThemeColorAt(12);
    const COLORREF colF = app->ThemeColorAt(13);
    if (app->state.optflag[0] != 0) {   
        if (app->state.optflag[2] != 0) {  
            FillPanelBottom(hdc, 0x1F3 - xOff, bottom - 157, 0x2A7 - xOff,
                            bottom - 3, colE, colF, 1);        
        } else {
            FillPanelBottom(hdc, 0x1F3 - xOff, bottom - 157, 0x205 - xOff,
                            bottom - 3, colE, colF, 1);        
        }
    } else if (app->state.optflag[5] != 0) {  
        FillPanelBottom(hdc, 0x254 - xOff, bottom - 157, 0x361 - xOff,
                        bottom - 3, colE, colF, 1);            
    } else {
        FillPanelBottom(hdc, 0x254 - xOff, bottom - 157, 0x266 - xOff,
                        bottom - 3, colE, colF, 1);            
    }
    xOff = panelLayout.afterLightOrFace;

    if (app->state.optflag[0] != 0) {   
        const COLORREF colG = app->ThemeColorAt(20);
        if (app->state.optflag[6] != 0) {  
            FillPanelBottom(hdc, 0x2AA - xOff, bottom - 157, 0x35B - xOff,
                            bottom - 3, colG, colD, 1);        
        } else {
            FillPanelBottom(hdc, 0x2AA - xOff, bottom - 157, 0x2BC - xOff,
                            bottom - 3, colG, colD, 1);        
        }
    }
    xOff = panelLayout.afterSelfShadow;

    if (app->state.optflag[0] != 0) {   
        const COLORREF colH = app->ThemeColorAt(14);
        const COLORREF colI = app->ThemeColorAt(15);
        if (app->state.optflag[3] != 0) {  
            FillPanelBottom(hdc, 0x35E - xOff, bottom - 157, 0x422 - xOff,
                            bottom - 3, colH, colI, 1);        
        } else {
            FillPanelBottom(hdc, 0x35E - xOff, bottom - 157, 0x370 - xOff,
                            bottom - 3, colH, colI, 1);        
        }
    }

    const std::int32_t xFinal = panelLayout.final;

    FillPanelBottom(hdc, 0x371 - xFinal, bottom - 157, 0x412 - xFinal,
                    bottom - 0x52,
                    app->ThemeColorAt(16), app->ThemeColorAt(17),
                    1);                                          
    FillPanelBottom(hdc, 0x371 - xFinal, bottom - 0x4F, 0x3F3 - xFinal,
                    bottom - 3,
                    app->ThemeColorAt(18), app->ThemeColorAt(19),
                    1);                                          
    FillPanelBottom(hdc, 0x3F6 - xFinal, bottom - 0x4F, 0x412 - xFinal,
                    bottom - 3,
                    app->ThemeColorAt(18), app->ThemeColorAt(19),
                    1);                                          
}

void HandlePaletteChanged2(HDC hdc) {
    MMDApp* app = g_Block;  

    RECT rc;
    std::int32_t xBase;
    HWND hwnd = app->FloatingWindow();
    if (hwnd != nullptr) {
        GetClientRect(hwnd, &rc);
        xBase = 0;
    } else {
        hwnd = reinterpret_cast<HWND>(app->state.hwnd);  
        GetClientRect(hwnd, &rc);
        xBase = app->SidebarWidth() + 9;
    }
    const LONG right = rc.right;   
    const LONG blitX = rc.right;   

    const std::int32_t hideTop = app->state.hideTop;      
    const std::int32_t hideBottom = app->state.hideBottom;  

    FillPanelBottom(hdc, xBase, hideTop - 25, right, hideTop - 1,
                    0x969696u, 0x646464u, 1);                    
    FillPanelBottom(hdc, xBase, hideBottom, right, hideBottom + 32,
                    0x787878u, 0xC8C8C8u, 1);                    

    HDC compat = CreateCompatibleDC(nullptr);                    
    SelectObject(compat,
                 reinterpret_cast<HGDIOBJ>(app->state.bmpRes119));  
    BitBlt(hdc, blitX - 58, hideTop - 24, 49, 24, compat, 0, 0,
           0xCC0020u);                                           
    DeleteDC(compat);                                            

    const bool playing =
        (app->FrameStepPlayback() == 0) & (app->PlaybackActive() != 0);

    char buffer[256];  

    if (app->state.optflag[0] != 0) {   
        if (app->EnglishUI() != 0) {                             
            if (playing) {
                DrawGlyph(app, "Playing", hdc, 20, xBase + 10, hideTop - 22,
                          0xFF, 0xFF, 0xFF, 1);                  
            } else {
                mdl::AccessoryRecord* acc =
                    app->AccessorySlot(app->SelectedAccessorySlot());
                if (acc == nullptr) {
                    DrawGlyph(app, "camera light accessary", hdc, 20,
                              xBase + 10, hideTop - 22, 0xFF, 0xFF, 0xFF, 1);  
                } else {
                    sprintf_s(buffer, 0x100, "camera light accessary / %s",
                              acc->name);                        
                    DrawGlyph(app, buffer, hdc, 20, xBase + 10, hideTop - 22,
                              0xFF, 0xFF, 0xFF, 1);              
                }
            }
            DrawGlyph(app, "camera", hdc, 14, xBase + 90, hideBottom + 10,
                      0, 0, 0, 1);                               
            DrawGlyph(app, "angle", hdc, 14, xBase + 365, hideBottom + 10,
                      0, 0, 0, 1);                               
            return;
        }
        if (playing) {
            DrawGlyph(app, kJpPlaying, hdc, 16, xBase + 10, hideTop - 20,
                      0xFF, 0xFF, 0xFF, 1);                      
        } else {
            mdl::AccessoryRecord* acc =
                app->AccessorySlot(app->SelectedAccessorySlot());
            if (acc == nullptr) {
                DrawGlyph(app, kJpCamLightAccessoryPanel, hdc, 16, xBase + 10, hideTop - 20,
                          0xFF, 0xFF, 0xFF, 1);                  
            } else {
                sprintf_s(buffer, 0x100, kJpCamLightAccessoryPanelFmt, acc->name);    
                DrawGlyph(app, buffer, hdc, 16, xBase + 10, hideTop - 20,
                          0xFF, 0xFF, 0xFF, 1);                  
            }
        }
        DrawGlyph(app, kJpCameraCenter, hdc, 14, xBase + 72, hideBottom + 11,
                  0, 0, 0, 1);                                   
    } else {                                                     
        if (app->EnglishUI() != 0) {
            if (playing) {
                DrawGlyph(app, "Playing", hdc, 20, xBase + 10, hideTop - 22,
                          0xFF, 0xFF, 0xFF, 1);                  
            } else {
                unsigned char* model = app->SelectedModel();
                if (model != nullptr) {                          
                    const rinmotiondance::mdl::ModelRecord& record =
                        *rinmotiondance::mdl::Mdl(model);
                    const std::int32_t boneIdx = record.selectedBone;
                    if (boneIdx < 0) {
                        sprintf_s(buffer, 0x100, "%s", record.nameEn);
                    } else {
                        sprintf_s(buffer, 0x100, "%s : %s",
                                  record.nameEn,
                                  record.boneTable[boneIdx].nameEn);
                    }
                    const int width = DrawGlyph(app, buffer, hdc, 20,
                                                xBase + 10, hideTop - 22,
                                                0xFF, 0xFF, 0xFF, 1);  
                    const std::int32_t traceTarget =
                        app->CameraParentModel();
                    const bool traceActive =
                        (app->state.followCameraEnabled != 0) &
                        (app->SelectedModelSlot() == traceTarget) &
                        (traceTarget >= 0);                      
                    if (traceActive) {
                        if (app->state.viewDirty != 0) {  
                            DrawGlyph(app, "camera bone trace mode (release trace button)",
                                      hdc, 16, xBase + width + 30, hideTop - 22,
                                      0xFF, 0xFF, 0xFF, 1);      
                            DrawGlyph(app, "camera bone trace mode (release trace button)",
                                      hdc, 16, xBase + width + 29, hideTop - 23,
                                      0xFF, 0x32, 0x32, 1);      
                        } else {
                            DrawGlyph(app, "camera bone trace mode",
                                      hdc, 16, xBase + width + 30, hideTop - 22,
                                      0xFF, 0xFF, 0xFF, 1);      
                        }
                    }
                }
            }
            DrawGlyph(app, "bone place", hdc, 14, xBase + 66, hideBottom + 10,
                      0, 0, 0, 1);                               
            DrawGlyph(app, "angle", hdc, 14, xBase + 365, hideBottom + 10,
                      0, 0, 0, 1);                               
            return;
        }
        if (playing) {
            DrawGlyph(app, kJpPlaying, hdc, 20, xBase + 10, hideTop - 22,
                      0xFF, 0xFF, 0xFF, 1);                      
        } else {
            unsigned char* model = app->SelectedModel();
            if (model != nullptr) {                              
                const rinmotiondance::mdl::ModelRecord& record =
                    *rinmotiondance::mdl::Mdl(model);
                sprintf_s(buffer, 0x100, "%s", record.name);
                const int width = DrawGlyph(app, buffer, hdc, 16,
                                            xBase + 10, hideTop - 20,
                                            0xFF, 0xFF, 0xFF, 1);  
                const std::int32_t traceTarget = app->CameraParentModel();
                const bool traceActive =
                    (app->state.followCameraEnabled != 0) &
                    (app->SelectedModelSlot() == traceTarget) &
                    (traceTarget >= 0);                          
                if (traceActive) {
                    if (app->state.viewDirty != 0) {  
                        DrawGlyph(app, kJpCameraFollowModeHint, hdc, 16, xBase + width + 30,
                                  hideTop - 20, 0xFF, 0xFF, 0xFF, 1);  
                        DrawGlyph(app, kJpCameraFollowModeHint, hdc, 16, xBase + width + 29,
                                  hideTop - 21, 0xFF, 0x32, 0x32, 1);  
                    } else {
                        DrawGlyph(app, kJpCameraFollowMode, hdc, 16, xBase + width + 30,
                                  hideTop - 20, 0xFF, 0xFF, 0xFF, 1);  
                    }
                }
            }
        }
        DrawGlyph(app, kJpBonePosition, hdc, 14, xBase + 68, hideBottom + 11,
                  0, 0, 0, 1);                                   
    }

    DrawGlyph(app, kJpAngle, hdc, 14, xBase + 369, hideBottom + 11,
              0, 0, 0, 1);                                       
}

}  
