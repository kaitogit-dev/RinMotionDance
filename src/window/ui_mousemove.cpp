#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/timeline_selection_grid.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"

namespace rinmotiondance {

void PanelPaint(MMDApp* app);

void SyncAccessoryEditPanel(MMDApp* a);              
void RefreshLightPanel(MMDApp* a);              
void ApplyGravityTrack(MMDApp* a);              
void RefreshSelfShadowPanel(MMDApp* a);              
void DragInterpolationControlPoint(MMDApp* a);  
void ReloadModels(MMDApp* a);              
void RelayoutSidebarControls(MMDApp* a);              
int SeekModelFrame(unsigned char* model, int frame, int physicsMode);  
void ApplyAccessoryTrack(MMDApp* a, int idx);     

namespace {


constexpr int kRowPitch = 13;     
constexpr int kColPitch = 14;     
constexpr int kGridX0 = 94;       
constexpr int kColY0 = 209;       
constexpr int kColY0Bone = 153;   
constexpr int kSidebarMin = 250;  



unsigned char* CurrentModel(MMDApp* app) {
    return app->SelectedModel();
}

template <typename Key>
void RemapList(MMDApp* app, const int* recs, int count, Key* keys,
               int row, int limit, std::uint32_t* maxModel) {
    auto valid = [limit](int idx) {
        return limit >= 0 ? idx >= limit : idx > 0;
    };
    if (row == 0) {
        for (int i = 0; i < count; ++i) {
            const int* rec = recs + 4 * i;
            if (valid(rec[0]))
                keys[rec[0]].frame = rec[3];
        }
    } else if (row < 0) {
        for (int i = 0; i < count; ++i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[0];
            if (valid(idx)) {
                Key& key = keys[idx];
                const std::int32_t pf = static_cast<std::int32_t>(
                    keys[key.previous].frame);
                if (pf + 1 < row + rec[3]) {
                    const std::int32_t v = row + rec[3];
                    if (v > 0)
                        key.frame = v;
                } else {
                    key.frame = pf + 1;
                }
            }
        }
    } else {
        for (int i = count - 1; i >= 0; --i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[0];
            if (valid(idx)) {
                Key& key = keys[idx];
                if (key.next == 0) {
                    key.frame = row + rec[3];
                } else {
                    const std::int32_t nextFrame =
                        static_cast<std::int32_t>(keys[key.next].frame);
                    if (nextFrame - 1 > row + rec[3])
                        key.frame = row + rec[3];
                    else
                        key.frame = nextFrame - 1;
                }
                const std::uint32_t frame = key.frame;
                if (maxModel != nullptr) {
                    if (*maxModel < frame)
                        *maxModel = frame;
                    if (app->LastRegisteredFrame() < *maxModel)
                        app->LastRegisteredFrame() = *maxModel;
                } else if (app->LastRegisteredFrame() < frame) {
                    app->LastRegisteredFrame() = frame;
                }
            }
        }
    }
}

void RemapAccList(MMDApp* app, const int* recs, int count, int row) {
    auto listOf = [app](int slot) {
        return app->AccessoryKeys(slot);
    };
    if (row == 0) {
        for (int i = 0; i < count; ++i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[1];
            if (idx > 0)
                listOf(rec[2])[idx].frame = rec[3];
        }
    } else if (row < 0) {
        for (int i = 0; i < count; ++i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[1];
            if (idx > 0) {
                mdl::AccessoryKey* list = listOf(rec[2]);
                mdl::AccessoryKey& key = list[idx];
                const std::int32_t pf = static_cast<std::int32_t>(
                    list[key.previous].frame);
                if (pf + 1 < row + rec[3]) {
                    const std::int32_t v = row + rec[3];
                    if (v > 0)
                        key.frame = v;
                } else {
                    key.frame = pf + 1;
                }
            }
        }
    } else {
        for (int i = count - 1; i >= 0; --i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[1];
            if (idx > 0) {
                mdl::AccessoryKey* list = listOf(rec[2]);
                mdl::AccessoryKey& key = list[idx];
                if (key.next == 0) {
                    key.frame = row + rec[3];
                } else {
                    const std::int32_t nextFrame =
                        static_cast<std::int32_t>(list[key.next].frame);
                    if (nextFrame - 1 > row + rec[3])
                        key.frame = row + rec[3];
                    else
                        key.frame = nextFrame - 1;
                }
                const std::uint32_t frame = key.frame;
                if (app->LastRegisteredFrame() < frame)
                    app->LastRegisteredFrame() = frame;
            }
        }
    }
}

inline unsigned char HoverFlag(MMDApp* app) {
    return static_cast<unsigned char>(!app->CtrlModifierActive());
}

void SetAccessorySelected(MMDApp* app, int slot, int keyIndex,
                          std::uint8_t selected) {
    auto* keys = app->AccessoryKeys(slot);
    keys[keyIndex].selected = selected;
}

}  

void HandleMouseMove(std::uint32_t lParam, int mouseY) {
    MMDApp* app = g_Block;
    const int X = static_cast<int>(lParam & 0xFFFFu);  
    const int Y = mouseY;                              

    if (app->state.a06B5 != 0) {
        D3DRenderer* sub = app->Renderer();
        const int ratio = static_cast<int>(
            sub->viewScale * 200.0);  
        if (X > static_cast<int>(app->state.hideLeft) - ratio)
            return;
        app->state.a06B5 = 0;
    }
    if (app->state.a06B4 != 0) {
        if (Y > app->state.hideBottom)
            return;
        app->state.a06B4 = 0;
    }
    if (app->state.a06B6 != 0) {
        if (X > app->state.hideLeft ||
            X < app->state.hideRight)
            return;
        app->state.a06B6 = 0;
    }

    if (app->MouseY() != Y || app->MouseX() != X) {
        app->MouseX() = X;
        app->MouseY() = Y;
        if (X > 0xEA60)
            app->MouseX() = X - 0x10000;
        if (Y > 0xEA60)
            app->MouseY() = Y - 0x10000;
        if (app->state.separateWindowMouseSeen != 0) {
            const int x0 = app->MouseX();
            const int y0 = app->MouseY();
            if (std::abs(app->PreviousMouseX() - x0) > 50 ||
                std::abs(app->PreviousMouseY() - y0) > 50)
                app->state.mouseJumped = 1;
            app->PreviousMouseX() = x0;
            app->PreviousMouseY() = y0;
            app->state.separateWindowMouseSeen = 0;
        }
        RECT rc;
        GetClientRect(static_cast<HWND>(app->Hwnd()), &rc);
        if ((app->SidebarWidth() <=
                 app->MouseX() && app->MouseY() <= rc.bottom - 158 &&
             app->MouseX() <=
                 app->SidebarWidth() + 6) &
            (app->FloatingWindow() == nullptr)) {
            SetCursor(LoadCursorA(nullptr,
                                  reinterpret_cast<LPCSTR>(
                                      static_cast<INT_PTR>(0x7F84))));  
        }

        if (app->SidebarResizeDragging() != 0) {
            const int x = app->MouseX();
            app->SidebarWidth() = x;
            if (x < kSidebarMin)
                app->SidebarWidth() = kSidebarMin;
            if (rc.right - 5 < app->SidebarWidth())
                app->SidebarWidth() = rc.right - 5;
            app->SidebarRatio() = static_cast<float>(
                static_cast<double>(app->SidebarWidth()) /
                static_cast<double>(rc.right));
            RelayoutSidebarControls(app);
            PanelPaint(app);
            const int sidebar = app->SidebarWidth();
            HWND hwnd = static_cast<HWND>(app->Hwnd());
            rc.bottom -= 158;
            rc.left = sidebar - 19;
            InvalidateRect(hwnd, &rc, FALSE);
            rc.right = sidebar;
            rc.left = 0;
            rc.top = rc.bottom - 90;
            InvalidateRect(hwnd, &rc, FALSE);
            rc.left = 0;
            rc.top = 0;
            rc.right = sidebar;
            rc.bottom = 145;
            InvalidateRect(hwnd, &rc, FALSE);
        }
        if (app->ViewportToolHovered() == 1) {
            SetCursor(LoadCursorA(nullptr,
                                  reinterpret_cast<LPCSTR>(
                                      static_cast<INT_PTR>(0x7F89))));  
        }

        if (app->TimelineSelectionChanged() != 0) {
            const int row =
                (app->MouseX() -
                 app->SelectionBoxAnchorX() - 6) /
                kRowPitch;  
            app->SceneModified() = 1;
            if (app->state.optflag[0] == 0) {
                unsigned char* model = CurrentModel(app);
                mdl::ModelRecord& record = *mdl::Mdl(model);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::ModelBone)),
                          app->TimelineSelectionCount(TimelineSelectionBand::ModelBone),
                          mdl::DisplayKeys(model), row, -1,
                          &record.maxFrame);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::ModelMorph)),
                          app->TimelineSelectionCount(
                              TimelineSelectionBand::ModelMorph),
                          mdl::MorphKeys(model), row,
                          static_cast<std::int32_t>(record.morphCount),
                          &record.maxFrame);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::ModelIk)),
                          app->TimelineSelectionCount(TimelineSelectionBand::ModelIk),
                          mdl::BoneKeys(model), row,
                          static_cast<std::int32_t>(record.boneCount),
                          &record.maxFrame);
                PanelPaint(app);  
                SeekModelFrame(model,
                          app->state.currentFrame,
                          app->state.playbackPhysicsMode);
            } else {
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::Camera)),
                          app->TimelineSelectionCount(TimelineSelectionBand::Camera),
                          app->CameraKeys(),
                          row, -1, nullptr);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::Light)),
                          app->TimelineSelectionCount(TimelineSelectionBand::Light),
                          app->LightKeys(),
                          row, -1, nullptr);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::SelfShadow)),
                          app->TimelineSelectionCount(TimelineSelectionBand::SelfShadow),
                          app->ShadowKeys(),
                          row, -1, nullptr);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::Gravity)),
                          app->TimelineSelectionCount(TimelineSelectionBand::Gravity),
                          app->GravityKeys(),
                          row, -1, nullptr);
                RemapAccList(app,
                             reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                                 TimelineSelectionBand::Accessory)),
                             app->TimelineSelectionCount(
                                 TimelineSelectionBand::Accessory), row);
                PanelPaint(app);   
                ReloadModels(app);    
                RefreshLightPanel(app);    
                RefreshSelfShadowPanel(app);    
                ApplyGravityTrack(app);    
                for (int i = 0; i < 255; ++i) {
                    if (app->ObjectSlot(i) != nullptr)
                        ApplyAccessoryTrack(app, i);
                }
                SyncAccessoryEditPanel(app);    
            }
        } else if (app->SelectionBoxDragging() != 0) {
            int xEnd = app->MouseX();
            int xStart;
            if (xEnd <= app->SelectionBoxAnchorX() + 6) {
                xStart = xEnd;
                xEnd = app->SelectionBoxAnchorX() + 6;
            } else {
                xStart = app->SelectionBoxAnchorX() + 6;
            }
            int yTop, yBottom;
            if (app->MouseY() <=
                app->SelectionBoxAnchorY() + 145) {
                yTop = app->MouseY();
                yBottom = app->SelectionBoxAnchorY() + 145;
            } else {
                yTop = app->SelectionBoxAnchorY() + 145;
                yBottom = app->MouseY();
            }
            int rowStart = (xStart - kGridX0) / kRowPitch;  
            if (xStart - kRowPitch * rowStart - kGridX0 <= 6)
                rowStart -= 1;
            if (rowStart < 0)
                rowStart = 0;
            int rowEnd = (xEnd - kGridX0) / kRowPitch;      
            if (xEnd - kRowPitch * rowEnd - kGridX0 > 6)
                rowEnd += 1;
            if (rowEnd > app->SidebarWidth() - 22)
                rowEnd = app->SidebarWidth() - 22;
            int colStart = (yTop - kColY0) / kColPitch;     
            if (yTop - kColPitch * colStart - kColY0 <= 7)
                colStart -= 1;
            if (colStart < 0)
                colStart = 0;
            int colEnd = (yBottom - kColY0) / kColPitch;    
            if (yBottom - kColPitch * colEnd - kColY0 > 7)
                colEnd += 1;

            const unsigned char flag = HoverFlag(app);
            if (app->state.optflag[0] != 0) {
                if (!app->ShiftModifierActive()) {
                    auto* cameraKeys = app->CameraKeys();
                    auto* lightKeys = app->LightKeys();
                    auto* shadowKeys = app->ShadowKeys();
                    auto* gravityKeys = app->GravityKeys();
                    int* bandMaps = app->state.rowHitBand1;
                    int* accWalk =
                        app->state.rowHitAcc + 1;
                    for (int row_i = 0; row_i < 200; ++row_i) {
                        const int b0 = bandMaps[-200];  
                        if (b0 >= 0)
                            cameraKeys[b0].selected = 0;
                        const int b1 = bandMaps[0];     
                        if (b1 >= 0)
                            lightKeys[b1].selected = 0;
                        const int b2 = bandMaps[200];   
                        if (b2 >= 0)
                            shadowKeys[b2].selected = 0;
                        const int b3 = bandMaps[400];   
                        if (b3 >= 0)
                            gravityKeys[b3].selected = 0;
                        int* slotWalk = app->state.jointLineMap + 4;
                        for (int g = 0; g < 40; ++g, slotWalk += 5, accWalk += 5) {
                            const int a0 = accWalk[-1];
                            if (a0 >= 0) {
                                const int s0 = slotWalk[-1];
                                if (s0 >= 0)
                                    SetAccessorySelected(app, s0, a0, 0);
                            }
                            const int accSel0 = accWalk[0];
                            if (accSel0 >= 0 && slotWalk[0] >= 0)
                                SetAccessorySelected(app, slotWalk[0], accSel0, 0);
                            const int accSel1 = accWalk[1];
                            if (accSel1 >= 0) {
                                const int s2 = slotWalk[1];
                                if (s2 >= 0)
                                    SetAccessorySelected(app, s2, accSel1, 0);
                            }
                            const int accSel2 = accWalk[2];
                            if (accSel2 >= 0) {
                                const int s3 = slotWalk[2];
                                if (s3 >= 0)
                                    SetAccessorySelected(app, s3, accSel2, 0);
                            }
                            const int accSel3 = accWalk[3];
                            if (accSel3 >= 0) {
                                const int s4 = slotWalk[3];
                                if (s4 >= 0)
                                    SetAccessorySelected(app, s4, accSel3, 0);
                            }
                        }
                        ++bandMaps;
                    }
                }
                if (yBottom >= 160 && yTop <= 174 && rowStart < rowEnd) {
                    auto* keys = app->CameraKeys();
                    int* m = app->state.rowHitBand0 + rowStart;
                    int n = rowEnd - rowStart;
                    do {
                        if (*m >= 0)
                            keys[*m].selected = flag;
                        ++m;
                    } while (--n);
                }
                if (yBottom >= 174 && yTop <= 188 && rowStart < rowEnd) {
                    auto* keys = app->LightKeys();
                    int* m = app->state.rowHitBand1 + rowStart;
                    int n = rowEnd - rowStart;
                    do {
                        if (*m >= 0)
                            keys[*m].selected = flag;
                        ++m;
                    } while (--n);
                }
                if (yBottom >= 188 && yTop <= 202 && rowStart < rowEnd) {
                    auto* keys = app->ShadowKeys();
                    int* m = app->state.rowHitBand2 + rowStart;
                    int n = rowEnd - rowStart;
                    do {
                        if (*m >= 0)
                            keys[*m].selected = flag;
                        ++m;
                    } while (--n);
                }
                if (yBottom >= 202 && yTop <= 216 && rowStart < rowEnd) {
                    auto* keys = app->GravityKeys();
                    int* m = app->state.rowHitBand3 + rowStart;
                    int n = rowEnd - rowStart;
                    do {
                        if (*m >= 0)
                            keys[*m].selected = flag;
                        ++m;
                    } while (--n);
                }
                VisitTimelineSelectionCells(rowStart, rowEnd, colStart, colEnd,
                    [&](std::size_t cell, int column) {
                        const int key = app->state.rowHitAcc[cell];
                        const int slot = app->state.jointLineMap[column];
                        if (key >= 0 && slot >= 0)
                            SetAccessorySelected(app, slot, key, flag);
                    });
            } else {
                unsigned char* model = CurrentModel(app);
                mdl::DisplayKey* displayKeys = mdl::DisplayKeys(model);
                mdl::MorphKey* morphKeys = mdl::MorphKeys(model);
                mdl::BoneKey* boneKeys = mdl::BoneKeys(model);
                if (!app->ShiftModifierActive()) {
                    displayKeys[0].allocated = 0;
                    boneKeys[0].allocated = 0;
                    for (std::size_t cell = 0; cell < 200 * 200; ++cell) {
                        const int display = app->state.rowHitIk[cell];
                        if (display > 0)
                            displayKeys[display].allocated = 0;
                        const int morph = app->state.rowHitMorph[cell];
                        if (morph > 0)
                            morphKeys[morph].allocated = 0;
                        const int bone = app->state.rowHitBone[cell];
                        if (bone > 0)
                            boneKeys[bone].allocated = 0;
                    }
                }
                int colStartB = (yTop - kColY0Bone) / kColPitch;
                if (yTop - kColPitch * colStartB - kColY0Bone <= 7)
                    colStartB -= 1;
                if (colStartB < 0)
                    colStartB = 0;
                int colEndB = (yBottom - kColY0Bone) / kColPitch;
                if (yBottom - kColPitch * colEndB - kColY0Bone > 7)
                    colEndB += 1;
                VisitTimelineSelectionCells(rowStart, rowEnd, colStartB, colEndB,
                    [&](std::size_t cell, int) {
                        const int display = app->state.rowHitIk[cell];
                        if (display > 0)
                            displayKeys[display].allocated = flag;
                        else if (display == -10)
                            displayKeys[0].allocated = flag;
                        const int morph = app->state.rowHitMorph[cell];
                        if (morph > 0)
                            morphKeys[morph].allocated = flag;
                        else if (morph == -10)
                            morphKeys[0].allocated = flag;
                        const int bone = app->state.rowHitBone[cell];
                        if (bone > 0)
                            boneKeys[bone].allocated = flag;
                        else if (bone == -10)
                            boneKeys[0].allocated = flag;
                    });
                app->TimelineRangeFirstOffset() = rowStart;
                app->TimelineRangeLastOffset() = rowEnd;
                app->TimelineRangeFirstBase() = colStartB;
                app->TimelineRangeLastBase() = colEndB;
            }
            PanelPaint(app);  
            app->TimelineRangeFirstOffset() = 0;
            app->TimelineRangeLastOffset() = 0;
            app->TimelineRangeFirstBase() = 0;
            app->TimelineRangeLastBase() = 0;
        }
    }

    if (app->PendingTimelineSelectionRow() != TimelineSelectionRow::None)
        DragInterpolationControlPoint(app);
}

}  
