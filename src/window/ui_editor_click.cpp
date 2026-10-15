#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void PanelPaint(MMDApp* app);                                   
void PostLanguageSweep(MMDApp* app);                            
void TimelineDrawTicks(int frameOffset, int width);             
void SetFrameNormalized(int frame);                             

void RefreshLightPanel(MMDApp* app);                                    
void RefreshSelfShadowPanel(MMDApp* app);                                    
void ApplyGravityTrack(MMDApp* app);                                    
void SyncAccessoryEditPanel(MMDApp* app);                                    
void ApplyAccessoryTrack(MMDApp* app, int idx);                           
void SnapshotPoseBeforeFrameChange(unsigned char* model, int frame);  
void SyncModelEditControls(unsigned char* model);                     
void SnapshotSelectedKeysForUndo(unsigned char* model, int frame);    
int SeekModelFrame(unsigned char* model, int frame, int physicsMode);  
void WaveRestartAt(void* sub, double v);                       
void AviBgOverlayRefresh(MMDApp* app);                                    
void AppendBoneKeyToUndo(unsigned char* model, int index);           
void PostLanguageSweep2(MMDApp* app);  
void SelectionReeval(MMDApp* app);                              

int CompareFunction(const void* a, const void* b) {
    const std::int32_t keyA = static_cast<const std::int32_t*>(a)[3];
    const std::int32_t keyB = static_cast<const std::int32_t*>(b)[3];
    if (keyA >= keyB)
        return keyA > keyB;
    return -1;
}


static int Div13(int v) {
    return v / 13;
}

static int Div14(int v) {
    return v / 14;
}

static unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

static unsigned char* ModelAtSlot(MMDApp* app, std::size_t slot) {
    return app->ModelSlot(static_cast<int>(slot));
}

static bool MatchText(const char* a, const char* b) {
    while (*a != '\0') {
        if (*a != *b)
            return false;
        ++a;
        ++b;
    }
    return *b == '\0';
}

static void ClearSelectionBitmaps(MMDApp* app) {
    for (int i = 0; i < 0x2710; ++i) {
        app->CameraKeys()[i].selected = 0;
        app->LightKeys()[i].selected = 0;
        app->ShadowKeys()[i].selected = 0;
        app->GravityKeys()[i].selected = 0;
    }
    for (int s = 0; s < 0xFF; ++s) {
        auto* rec = reinterpret_cast<unsigned char*>(app->AccessoryKeys(s));
        if (rec == nullptr)
            continue;
        for (int i = 0; i < 0x2710; ++i)
            rec[i * 0x3C + 0x18] = 0;
    }
}

static void ClearActiveModelFlags(MMDApp* app) {
    if (ActiveModel(app) == nullptr)
        return;
    unsigned char* model = ActiveModel(app);
    rinmotiondance::mdl::BoneKey* boneKeys = rinmotiondance::mdl::BoneKeys(model);
    for (std::size_t i = 0; i < rinmotiondance::mdl::kBoneKeyCapacity; ++i)
        boneKeys[i].allocated = 0;

    rinmotiondance::mdl::MorphKey* morphKeys = rinmotiondance::mdl::MorphKeys(model);
    for (std::size_t i = 0; i < rinmotiondance::mdl::kMorphKeyCapacity; ++i)
        morphKeys[i].allocated = 0;
    rinmotiondance::mdl::DisplayKey* displayKeys = rinmotiondance::mdl::DisplayKeys(model);
    for (std::size_t i = 0; i < rinmotiondance::mdl::kDisplayKeyCapacity; ++i)
        displayKeys[i].allocated = 0;
}

static std::int32_t TimelineAddress(const void* address) {
    return static_cast<std::int32_t>(
        reinterpret_cast<std::uintptr_t>(address));
}

static void SetTimelineSelectionRange(MMDApp* app, std::int32_t firstOffset,
                                      const void* firstBase,
                                      std::int32_t lastOffset,
                                      const void* lastBase) {
    app->TimelineRangeFirstOffset() = firstOffset;
    app->TimelineRangeFirstBase() = TimelineAddress(firstBase);
    app->TimelineRangeLastOffset() = lastOffset;
    app->TimelineRangeLastBase() = TimelineAddress(lastBase);
}

static void FillSelectionRecords(MMDApp* app, unsigned char* arr, int recStride,
                                 int flagOff, int flagStride,
                                 TimelineSelectionBand band) {
    const std::int32_t cnt = app->TimelineSelectionCount(band);
    if (cnt <= 0)
        return;
    std::int32_t* out = static_cast<std::int32_t*>(
        ::operator new(static_cast<std::size_t>(cnt) * 0x10));
    app->TimelineSelectionRecords(band) =
        reinterpret_cast<TimelineSelectionRecord*>(out);
    int recIdx = 2;
    for (int i = 0; i < 0x2710 / 5; ++i) {
        for (int k = 0; k < 5; ++k) {
            if (arr[i * recStride + flagOff + k * flagStride] != 0) {
                out[0] = recIdx - 2 + k;
                out[3] = *reinterpret_cast<std::int32_t*>(
                    arr + i * recStride + k * flagStride);
                out += 4;
            }
        }
        recIdx += 5;
    }
    qsort(app->TimelineSelectionRecords(band), static_cast<std::size_t>(cnt), 0x10,
          CompareFunction);
}

static void SelectionStats(MMDApp* app, int x, int y, HWND hwnd) {
    app->SelectionBoxDragging() = 1;
    app->SelectionBoxAnchorX() = x - 6;
    app->SelectionBoxAnchorY() = y - 0x91;
    if (app->TimelineSelectionChanged() != 0) {
        memset(app->TimelineSelectionRegion(), 0, 0x40);        
        if (app->state.optflag[0] != 0) {   
            unsigned char* rarr = reinterpret_cast<unsigned char*>(app->CameraKeys());
            unsigned char* jarr = reinterpret_cast<unsigned char*>(app->LightKeys());
            unsigned char* iarr = reinterpret_cast<unsigned char*>(app->ShadowKeys());
            unsigned char* marr = reinterpret_cast<unsigned char*>(app->GravityKeys());
            for (int off = 0; off < 0x2710; ++off) {
                if (rarr[off * 0x54 + 0x48] != 0)
                    ++app->TimelineSelectionCount(TimelineSelectionBand::Camera);
                if (jarr[off * 0x28 + 0x24] != 0)
                    ++app->TimelineSelectionCount(TimelineSelectionBand::Light);
                if (iarr[off * 0x18 + 0x14] != 0)
                    ++app->TimelineSelectionCount(TimelineSelectionBand::SelfShadow);
                if (marr[off * 0x24 + 0x21] != 0)
                    ++app->TimelineSelectionCount(TimelineSelectionBand::Gravity);
                for (int g = 0; g < 0x33; ++g) {
                    for (int j = 0; j < 5; ++j) {
                        unsigned char* slot = reinterpret_cast<unsigned char*>(
                            app->AccessoryKeys(g * 5 + j));
                        if (slot[off * 0x3C + 0x18] != 0)
                            ++app->TimelineSelectionCount(TimelineSelectionBand::Accessory);
                    }
                }
            }
            FillSelectionRecords(app, rarr, 0x1A4, 0x48, 0x54,
                                 TimelineSelectionBand::Camera);
            FillSelectionRecords(app, jarr, 0xC8, 0x24, 0x28,
                                 TimelineSelectionBand::Light);
            FillSelectionRecords(app, iarr, 0x78, 0x14, 0x18,
                                 TimelineSelectionBand::SelfShadow);
            FillSelectionRecords(app, marr, 0xB4, 0x21, 0x24,
                                 TimelineSelectionBand::Gravity);
            if (app->TimelineSelectionCount(TimelineSelectionBand::Accessory) > 0) {
                std::int32_t* out = static_cast<std::int32_t*>(::operator new(
                    static_cast<std::size_t>(app->TimelineSelectionCount(
                        TimelineSelectionBand::Accessory)) * 0x10));
                app->TimelineSelectionRecords(TimelineSelectionBand::Accessory) =
                    reinterpret_cast<TimelineSelectionRecord*>(out);
                for (int off = 0; off < 0x2710; ++off) {
                    for (int g = 0; g < 0x33; ++g) {
                        for (int j = 0; j < 5; ++j) {
                            unsigned char* slot = reinterpret_cast<unsigned char*>(
                                app->AccessoryKeys(g * 5 + j));
                            if (slot[off * 0x3C + 0x18] != 0) {
                                out[1] = off;
                                out[2] = g * 5 + j;
                                out[3] = *reinterpret_cast<std::int32_t*>(slot + off * 0x3C);
                                out += 4;
                            }
                        }
                    }
                }
                qsort(app->TimelineSelectionRecords(TimelineSelectionBand::Accessory),
                      static_cast<std::size_t>(app->TimelineSelectionCount(
                          TimelineSelectionBand::Accessory)),
                      0x10, CompareFunction);
            }
        } else {
            std::int32_t& boneCnt =
                app->TimelineSelectionCount(TimelineSelectionBand::ModelIk);
            boneCnt = 0;
            unsigned char* m = ActiveModel(app);
            if (m != nullptr) {
                rinmotiondance::mdl::BoneKey* bones = rinmotiondance::mdl::BoneKeys(m);
                for (std::size_t i = 0; i < rinmotiondance::mdl::kBoneKeyCapacity; ++i)
                    if (bones[i].allocated != 0) ++boneCnt;
                if (boneCnt > 0) {
                    EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);      
                    EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);     
                    auto* record = rinmotiondance::mdl::Mdl(m);
                    record->undoDirty = 1;
                    record->redoDirty = 0;
                    std::uint32_t& kf = record->undoState[0];
                    kf = kf + 1;
                    if (kf >= 0x1E)
                        kf = 0;
                    record->undoState[1] = kf;
                    auto& undo = record->undoRings[0].slots[kf];
                    undo.operation = 2;
                    undo.frame = app->state.currentFrame;
                    auto*& bufSlot = undo.bonePose;
                    if (bufSlot != nullptr) {
                        ::operator delete(bufSlot);
                        bufSlot = nullptr;
                    }
                    const std::int32_t boneN = rinmotiondance::mdl::Mdl(m)->boneCount;
                    bufSlot = static_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
                        ::operator new(static_cast<std::size_t>(boneN) *
                                       sizeof(rinmotiondance::mdl::BonePoseSnapshot)));
                    memset(bufSlot, 0, static_cast<std::size_t>(boneN) *
                                           sizeof(rinmotiondance::mdl::BonePoseSnapshot));
                    rinmotiondance::mdl::BoneRecord* srcBase = rinmotiondance::mdl::Bones(m);
                    unsigned char* flagMap = rinmotiondance::mdl::Mdl(m)->bonePhysicsState;
                    for (int i = 0; i < boneN; ++i) {
                        auto& out = bufSlot[i];
                        out.boneIndex = 0;
                        memcpy(out.position, srcBase[i].trans, sizeof out.position);
                        memcpy(out.rotation, srcBase[i].rotQuat, sizeof out.rotation);
                        out.physicsDisabled = flagMap[i];
                    }
                    undo.dirty = 0;
                    void*& dataSlot = undo.auxiliaryPose;
                    if (dataSlot != nullptr) {
                        ::operator delete(dataSlot);
                        dataSlot = nullptr;
                    }
                    dataSlot = ::operator new(static_cast<std::size_t>(boneCnt) * 0x40);
                    memset(dataSlot, 0, static_cast<std::size_t>(boneCnt) * 0x40);
                    memset(rinmotiondance::mdl::Mdl(m)->keyVisitMap, 0,
                            sizeof(rinmotiondance::mdl::Mdl(m)->keyVisitMap));
                    for (std::size_t i = 0; i < rinmotiondance::mdl::kBoneKeyCapacity; ++i)
                        if (bones[i].allocated != 0)
                            AppendBoneKeyToUndo(ActiveModel(app), static_cast<int>(i));
                    std::int32_t* bout = static_cast<std::int32_t*>(
                        ::operator new(static_cast<std::size_t>(boneCnt) * 0x10));
                    app->TimelineSelectionRecords(TimelineSelectionBand::ModelIk) =
                        reinterpret_cast<TimelineSelectionRecord*>(bout);
                    int idx = 2;
                    for (std::size_t i = 0; i < rinmotiondance::mdl::kBoneKeyCapacity / 6; ++i) {
                        for (int k = 0; k < 6; ++k) {
                            rinmotiondance::mdl::BoneKey& key = bones[i * 6 + k];
                            if (key.allocated != 0) {
                                bout[0] = idx - 2 + k;
                                bout[3] = static_cast<std::int32_t>(key.frame);
                                bout += 4;
                            }
                        }
                        idx += 6;
                    }
                    qsort(app->TimelineSelectionRecords(TimelineSelectionBand::ModelIk),
                          static_cast<std::size_t>(boneCnt),
                          0x10, CompareFunction);
                }
                std::int32_t& morphCnt =
                    app->TimelineSelectionCount(TimelineSelectionBand::ModelMorph);
                morphCnt = 0;
                {
                    unsigned char* mm = ActiveModel(app);
                    rinmotiondance::mdl::MorphKey* morphs = rinmotiondance::mdl::MorphKeys(mm);
                    for (std::size_t i = 0; i < rinmotiondance::mdl::kMorphKeyCapacity; ++i)
                        if (morphs[i].allocated != 0) ++morphCnt;
                    if (morphCnt > 0) {
                        std::int32_t* mout = static_cast<std::int32_t*>(
                            ::operator new(static_cast<std::size_t>(morphCnt) * 0x10));
                        app->TimelineSelectionRecords(TimelineSelectionBand::ModelMorph) =
                            reinterpret_cast<TimelineSelectionRecord*>(mout);
                        int idx = 2;
                        for (int i = 0; i < 0xFA0; ++i) {
                            for (int k = 0; k < 5; ++k) {
                                rinmotiondance::mdl::MorphKey& key = morphs[i * 5 + k];
                                if (key.allocated != 0) {
                                    mout[0] = idx - 2 + k;
                                    mout[3] = static_cast<std::int32_t>(key.frame);
                                    mout += 4;
                                }
                            }
                            idx += 5;
                        }
                        qsort(app->TimelineSelectionRecords(TimelineSelectionBand::ModelMorph),
                              static_cast<std::size_t>(morphCnt),
                              0x10, CompareFunction);
                    }
                }
                std::int32_t& ikCnt =
                    app->TimelineSelectionCount(TimelineSelectionBand::ModelBone);
                ikCnt = 0;
                {
                    unsigned char* mm = ActiveModel(app);
                    rinmotiondance::mdl::DisplayKey* displayKeys = rinmotiondance::mdl::DisplayKeys(mm);
                    for (std::size_t i = 0; i < rinmotiondance::mdl::kDisplayKeyCapacity; ++i)
                        if (displayKeys[i].allocated != 0) ++ikCnt;
                    if (ikCnt > 0) {
                        std::int32_t* iout = static_cast<std::int32_t*>(
                            ::operator new(static_cast<std::size_t>(ikCnt) * 0x10));
                        app->TimelineSelectionRecords(TimelineSelectionBand::ModelBone) =
                            reinterpret_cast<TimelineSelectionRecord*>(iout);
                        int idx = 2;
                        for (int i = 0; i < 0xC8; ++i) {       
                            for (int k = 0; k < 5; ++k) {
                                rinmotiondance::mdl::DisplayKey& key = displayKeys[i * 5 + k];
                                if (key.allocated != 0) {
                                    iout[0] = idx - 2 + k;
                                    iout[3] = static_cast<std::int32_t>(key.frame);
                                    iout += 4;
                                }
                            }
                            idx += 5;
                        }
                    }
                }
                qsort(app->TimelineSelectionRecords(TimelineSelectionBand::ModelBone),
                      static_cast<std::size_t>(
                          app->TimelineSelectionCount(TimelineSelectionBand::ModelBone)),
                      0x10, CompareFunction);
            }
        }
    }
    PanelPaint(app);                               
    app->ClearTimelineRange();
    SelectionReeval(app);                          
}

TimelineSelectionRecord* g_timelineSelectionRecords[8] = {};


static void ToggleTimelineCell(MMDApp* app, bool shiftActive,
                               std::uint8_t& selected) {
    if (selected != 0) {
        if (shiftActive)
            selected = 0;
        else
            app->TimelineSelectionChanged() = 1;
    } else if (!shiftActive) {
        ClearSelectionBitmaps(app);
        selected = 1;
    } else {
        selected = 1;
    }
}

struct TimelineBandHit {
    int yMin;
    int yMax;
    const std::int32_t* rowHits;              
    std::uint8_t& (*keySelected)(MMDApp*, int);
};

static std::uint8_t& CameraKeySelected(MMDApp* app, int i) {
    return app->CameraKeys()[i].selected;
}
static std::uint8_t& LightKeySelected(MMDApp* app, int i) {
    return app->LightKeys()[i].selected;
}
static std::uint8_t& ShadowKeySelected(MMDApp* app, int i) {
    return app->ShadowKeys()[i].selected;
}
static std::uint8_t& GravityKeySelected(MMDApp* app, int i) {
    return app->GravityKeys()[i].selected;
}

template <typename KeyT>
static void ToggleModelKeyAllocated(MMDApp* app, KeyT& key,
                                    bool shiftActive) {
    if (key.allocated != 0) {
        if (shiftActive)
            key.allocated = 0;
        else
            app->TimelineSelectionChanged() = 1;
    } else if (shiftActive) {
        key.allocated = 1;
    } else {
        ClearActiveModelFlags(app);
        key.allocated = 1;
    }
}

template <typename KeyT>
static void StageRangeRow(MMDApp* app, std::int32_t type, KeyT* keys,
                          bool shiftActive) {
    if (type == -1) {                          
        if (shiftActive) {
            SetTimelineSelectionRange(app, type * sizeof(*keys), keys,
                                      type * sizeof(*keys) + 1,
                                      reinterpret_cast<unsigned char*>(keys) + 1);
            app->TimelineRangeApplyEnabled() = 1;
        } else {
            app->TimelineSelectionChanged() = 1;
        }
    } else {                                   
        SetTimelineSelectionRange(app, type * sizeof(*keys), keys,
                                  type * sizeof(*keys) + 1,
                                  reinterpret_cast<unsigned char*>(keys) + 1);
        if (!shiftActive)
            ClearActiveModelFlags(app);
    }
}

static void HandleLButtonDown_MarginColumnToggle(MMDApp* app,
                                                 std::int32_t y) {
    if (app->state.optflag[0] == 0) {  
        unsigned char* model = app->SelectedModel();
        if (model == nullptr)
            return;
        const std::int32_t type =
            static_cast<std::int8_t>(
                mdl::Mdl(model)->boneListRowType[Div14(y - 0xA0)]);
        if (type > 0) {
            mdl::DisplayGroup& frame = mdl::DisplayGroups(model)[type];
            frame.flags = (frame.flags == 0) ? 1 : 0;   
            PostLanguageSweep(app);             
        }
    }
    return;
}

static void HandleLButtonDown_NameColumnHit(MMDApp* app, HWND hwnd,
                                            std::int32_t y) {
    if (app->state.optflag[0] != 0) {  
        if (!app->ShiftModifierActive()) {
            for (int i = 0; i < 255; ++i) {
                mdl::AccessoryRecord* accessory =
                    static_cast<mdl::AccessoryRecord*>(app->ObjectSlot(i));
                if (accessory != nullptr)
                    accessory->rowSelected = 0;
            }
            app->ClearGlobalTimelineTrackSelection();  
        }
        static const struct {
            int yMin, yMax;
            GlobalTimelineTrack track;
        } kColumnBands[4] = {
            {0xA0, 0xAE, GlobalTimelineTrack::Camera},      
            {0xAE, 0xBC, GlobalTimelineTrack::Light},       
            {0xBC, 0xCA, GlobalTimelineTrack::SelfShadow},  
            {0xCA, 0xD8, GlobalTimelineTrack::Gravity},     
        };
        for (const auto& band : kColumnBands) {
            if (y >= band.yMin && y < band.yMax) {
                app->GlobalTrackSelected(band.track) =
                    app->GlobalTrackSelected(band.track) == 0 ? 1 : 0;
                PostLanguageSweep(app);
                return;
            }
        }
        {                                                        
            const std::int32_t jrow = Div14(y - 0xD8);
            const std::int32_t jidx = app->state.jointLineMap[jrow];
            if (jidx >= 0) {
                mdl::AccessoryRecord* accessory = static_cast<
                    mdl::AccessoryRecord*>(app->ObjectSlot(jidx));
                if (accessory->rowSelected != 0) {
                    accessory->rowSelected = 0;
                    PostLanguageSweep(app);
                    PostLanguageSweep2(app);
                } else {
                    accessory->rowSelected = 1;
                    app->state.selectedObjectSlot =
                        static_cast<std::uint8_t>(jidx);
                    PostLanguageSweep(app);
                    SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo), CB_SETCURSEL,
                                 accessory->order, 0);
                    SyncAccessoryEditPanel(app);
                    PostLanguageSweep2(app);
                }
            } else {
                PostLanguageSweep2(app);
            }
            return;
        }
    }

    if (ActiveModel(app) == nullptr)
        return;
    if (!app->ShiftModifierActive()) {
        unsigned char* m0 = ActiveModel(app);
        const std::int32_t boneCount =
            rinmotiondance::mdl::Mdl(m0)->boneCount;
        for (int i = 0; i < boneCount; ++i) {
            unsigned char* mm = ActiveModel(app);
            rinmotiondance::mdl::Mdl(mm)->boneSelection[i] = 0;
        }
        for (int i = 0;
             i < static_cast<int>(mdl::Mdl(m0)->facialFrameCount); ++i) {
            unsigned char* mm = ActiveModel(app);
            mdl::DisplayFrames(mm)[i].selected = 0;
        }
        rinmotiondance::mdl::Mdl(ActiveModel(app))->displayKeyframesPresent = 0;
    }

    {
        unsigned char* m = ActiveModel(app);
        const std::int32_t row = Div14(y - 0xA0);
        const std::int32_t idx =
            rinmotiondance::mdl::Mdl(m)->boneListRowRecord[row];
        if (idx >= 0) {
            unsigned char* p = rinmotiondance::mdl::Mdl(m)->boneSelection + idx;
            if (*p != 0) {
                *p = 0;
            } else {
                *p = 1;
                rinmotiondance::mdl::Mdl(m)->selectedBone = idx;
            }
        } else if (idx == -999) {                       
            const std::int32_t want = -1 - idx;
            const std::int32_t morphCount =
                static_cast<std::int32_t>(mdl::Mdl(m)->facialFrameCount);
            int i = 0;
            for (; i < morphCount; ++i) {
                if (mdl::DisplayFrames(m)[i].targetIndex == want)
                    break;
            }
            if (i < morphCount) {
                mdl::FrameGroup& faceRec = mdl::DisplayFrames(m)[i];
                if (faceRec.selected != 0) {
                    faceRec.selected = 0;
                } else {
                    faceRec.selected = 1;
                    const std::int32_t morphIndex = faceRec.targetIndex;
                    auto& morph = rinmotiondance::mdl::Morphs(m)[morphIndex];
                    const auto panel = morph.panel;
                    HWND hCombo = nullptr, hSpin = nullptr, hText = nullptr;
                    switch (panel) {                         
                        case rinmotiondance::mdl::MorphPanel::eyebrow:
                            hCombo = GetDlgItem(hwnd, panel::kMorphCombo0);
                            hSpin = GetDlgItem(hwnd, panel::kMorphSlider0);
                            hText = GetDlgItem(hwnd, panel::kMorphEdit0);
                            break;
                        case rinmotiondance::mdl::MorphPanel::eye:
                            hCombo = GetDlgItem(hwnd, panel::kMorphCombo1);
                            hSpin = GetDlgItem(hwnd, panel::kMorphSlider1);
                            hText = GetDlgItem(hwnd, panel::kMorphEdit1);
                            break;
                        case rinmotiondance::mdl::MorphPanel::mouth:
                            hCombo = GetDlgItem(hwnd, panel::kMorphCombo2);
                            hSpin = GetDlgItem(hwnd, panel::kMorphSlider2);
                            hText = GetDlgItem(hwnd, panel::kMorphEdit2);
                            break;
                        case rinmotiondance::mdl::MorphPanel::other:
                            hCombo = GetDlgItem(hwnd, panel::kMorphCombo3);
                            hSpin = GetDlgItem(hwnd, panel::kMorphSlider3);
                            hText = GetDlgItem(hwnd, panel::kMorphEdit3);
                            break;
                    }
                    const auto panelIndex = static_cast<unsigned>(panel) - 1;
                    rinmotiondance::mdl::Mdl(m)->selectedMorphs[panelIndex] = morphIndex;
                    const LRESULT count = SendMessageA(hCombo, CB_GETCOUNT, 0, 0);
                    char nameBuf[0x64];
                    for (LONG i2 = 0; i2 < count; ++i2) {
                        SendMessageA(hCombo, CB_GETLBTEXT, i2,
                                     reinterpret_cast<LPARAM>(nameBuf));
                        if (MatchText(nameBuf, morph.name) ||
                            MatchText(nameBuf, morph.nameEn)) {
                            SendMessageA(hCombo, CB_SETCURSEL, i2, 0);
                        }
                    }
                    const float v = rinmotiondance::mdl::Morphs(m)[
                        rinmotiondance::mdl::Mdl(m)->selectedMorphs[panelIndex]].value;
                    SendMessageA(hSpin, TBM_SETPOS, 1,
                                 static_cast<LPARAM>(static_cast<std::int32_t>(v * 100.0)));
                    char valBuf[0x64];
                    sprintf_s(valBuf, 0x64u, "%5.4f", static_cast<double>(v));
                    SetWindowTextA(hText, valBuf);
                }
            }
        }
    }

    {
        unsigned char* m = ActiveModel(app);
        const std::int32_t type =
            static_cast<std::int8_t>(
                rinmotiondance::mdl::Mdl(m)->boneListRowType[Div14(y - 0xA0)]);
        if (type == 1) {
            mdl::Mdl(m)->displayKeyframesPresent =
                (mdl::Mdl(m)->displayKeyframesPresent == 0) ? 1 : 0;  
            PostLanguageSweep(app);
            PostLanguageSweep2(app);
            return;
        }
        if (type == 2) {
            for (int i = 0;
                 i < static_cast<int>(mdl::Mdl(m)->facialFrameCount); ++i) {
                mdl::FrameGroup& face = mdl::DisplayFrames(m)[i];
                face.selected = (face.selected == 0) ? 1 : 0;
            }
            PostLanguageSweep(app);
            PostLanguageSweep2(app);
            return;
        }
        if (type <= 0) {                                 
            PostLanguageSweep2(app);
            return;
        }
        for (int i = 0;
             i < static_cast<int>(mdl::Mdl(m)->rigidBodyCount); ++i) {
            mdl::FrameGroup& rigid = mdl::RigidGroups(m)[i];
            if (static_cast<std::int8_t>(rigid.groupIndex) == type) {
                const std::int32_t boneIdx = rigid.targetIndex;
                unsigned char* p =
                    rinmotiondance::mdl::Mdl(m)->boneSelection + boneIdx;
                if (*p != 0)
                    *p = 0;
                else
                    *p = 1;
            }
        }
        PostLanguageSweep(app);
        PostLanguageSweep2(app);
        return;
    }
}

static void HandleLButtonDown_TimelineAreaHit(MMDApp* app, HWND hwnd,
                                              std::int32_t x,
                                              std::int32_t y) {
    if (app->PlaybackActive() != 0)
        return;
    app->TimelineSelectionChanged() = 0;
    if (app->state.optflag[0] != 0) {  
        const std::int32_t rowD = Div13(x - 0x64);
        std::int32_t bandIdx = -1;  
        std::int32_t joint2dIdx = -1;                        
        const bool shiftActive = app->ShiftModifierActive();
        const TimelineBandHit bands[4] = {
            {0xA0, 0xAE, app->state.rowHitBand0, CameraKeySelected},  
            {0xAE, 0xBC, app->state.rowHitBand1, LightKeySelected},   
            {0xBC, 0xCA, app->state.rowHitBand2, ShadowKeySelected},  
            {0xCA, 0xD8, app->state.rowHitBand3, GravityKeySelected}, 
        };
        for (const TimelineBandHit& band : bands) {
            if (y < band.yMin || y >= band.yMax)
                continue;
            bandIdx = band.rowHits[rowD];
            if (bandIdx >= 0)
                ToggleTimelineCell(app, shiftActive,
                                   band.keySelected(app, bandIdx));
        }
        if (y >= 0xD8) {                                     
            const std::int32_t jrow = Div14(y - 0xD8);
            const std::int32_t slotIdx =
                app->state.jointLineMap[jrow];
            const std::int32_t col = Div13(x - 0x64);
            joint2dIdx =
                app->state.rowHitAcc[col * 0xC8 + jrow];
            if (joint2dIdx >= 0) {
                unsigned char* slot = reinterpret_cast<unsigned char*>(
                    app->AccessoryKeys(slotIdx));
                unsigned char* rec = slot + joint2dIdx * 0x3C + 0x18;
                if (*rec != 0) {
                    if (shiftActive)
                        *rec = 0;
                    else
                        app->TimelineSelectionChanged() = 1;
                } else if (!shiftActive) {
                    ClearSelectionBitmaps(app);
                    *rec = 1;
                } else {
                    *rec = 1;
                }
            }
        }
        if (bandIdx == -1 && joint2dIdx == -1 && !shiftActive) {
            ClearSelectionBitmaps(app);
        }
        SelectionStats(app, x, y, hwnd);       
        return;
    }
    else {
        if (ActiveModel(app) == nullptr)
            return;
        const bool shiftActive = app->ShiftModifierActive();
        const std::int32_t row2d = Div13(x - 0x64) * 0xC8 + Div14(y - 0xA0);
        const std::int32_t ikType = app->state.rowHitIk[row2d];
        if (ikType > 0) {
            ToggleModelKeyAllocated(app,
                rinmotiondance::mdl::DisplayKeys(ActiveModel(app))[ikType],
                shiftActive);
        } else if (ikType == -10) {                 
            ToggleModelKeyAllocated(app,
                rinmotiondance::mdl::DisplayKeys(ActiveModel(app))[0],
                shiftActive);
        }
        const std::int32_t morphType = app->state.rowHitMorph[row2d];
        if (morphType > 0) {
            ToggleModelKeyAllocated(app,
                rinmotiondance::mdl::MorphKeys(ActiveModel(app))[morphType],
                shiftActive);
        } else if (morphType == -10) {              
            ToggleModelKeyAllocated(app,
                rinmotiondance::mdl::MorphKeys(ActiveModel(app))[0],
                shiftActive);
        } else if (morphType == -1) {               
            StageRangeRow(app, morphType,
                          rinmotiondance::mdl::MorphKeys(ActiveModel(app)),
                          shiftActive);
        } else if (morphType == -2) {               
            StageRangeRow(app, morphType,
                          rinmotiondance::mdl::MorphKeys(ActiveModel(app)),
                          shiftActive);
        }
        const std::int32_t boneType = app->state.rowHitBone[row2d];
        if (boneType > 0) {
            ToggleModelKeyAllocated(app,
                rinmotiondance::mdl::BoneKeys(ActiveModel(app))[boneType],
                shiftActive);
        } else if (boneType == -10) {               
            ToggleModelKeyAllocated(app,
                rinmotiondance::mdl::BoneKeys(ActiveModel(app))[0],
                shiftActive);
        } else if (boneType == -1) {                
            StageRangeRow(app, boneType,
                          rinmotiondance::mdl::BoneKeys(ActiveModel(app)),
                          shiftActive);
        } else if (boneType == -2) {                
            StageRangeRow(app, boneType,
                          rinmotiondance::mdl::BoneKeys(ActiveModel(app)),
                          shiftActive);
        }
        if (ikType == 0 && morphType == 0 && boneType == 0 && !shiftActive)
            ClearActiveModelFlags(app);
        SelectionStats(app, x, y, hwnd);           
        return;
    }
}

static void HandleLButtonDown_FrameStripHit(MMDApp* app, HWND hwnd,
                                            std::int32_t x, std::int32_t y,
                                            std::int32_t sidebar) {
    if (app->PlaybackActive() != 0)
        return;
    app->CameraAttachmentTransformSuppressed() = 0;
    for (int i = 0; i < kModelSlotCount; ++i) {
        unsigned char* model = ModelAtSlot(app, i);
        if (model == nullptr)
            continue;
        mdl::ModelRecord* modelRecord = mdl::Mdl(model);
        const std::int32_t n =
            static_cast<std::int32_t>(modelRecord->boneCount);
        if (n <= 0)
            continue;
        unsigned char* flags = modelRecord->bonePhysicsState;
        int j = 0;
        for (; j < n; ++j)
            if (flags[j] != 0)
                break;
        if (j < n) {
            SnapshotPoseBeforeFrameChange(model, app->state.currentFrame);
            modelRecord = mdl::Mdl(model);
            flags = modelRecord->bonePhysicsState;
            for (j = 0; j < n; ++j)
                flags[j] = 0;
        }
    }
    app->state.currentFrame =
        Div13(x - 0x64) + app->state.timelineStartFrame;
    const LRESULT textLen = GetWindowTextLengthA(GetDlgItem(hwnd, panel::kCurrentFrameEdit));
    SendMessageA(GetDlgItem(hwnd, panel::kCurrentFrameEdit), EM_SETSEL, 0, textLen);
    char frameText[0x100];
    sprintf_s(frameText, 0x100u, "%d",
              app->state.currentFrame);
    SendMessageA(GetDlgItem(hwnd, panel::kCurrentFrameEdit), EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(frameText));
    for (int i = 0; i < kModelSlotCount; ++i) {
        unsigned char* model = ModelAtSlot(app, i);
        if (model == nullptr)
            continue;
        SeekModelFrame(model,
                  app->state.currentFrame,
                  app->PlaybackPhysicsMode());
        if (i == static_cast<int>(app->SelectedModelSlot())) {
            SyncModelEditControls(model);
        }
    }
    if (app->state.optflag[0] != 0) {  
        ReloadModels(app);                            
        RefreshLightPanel(app);                               
        RefreshSelfShadowPanel(app);                               
        ApplyGravityTrack(app);                               
        for (int i = 0; i < 0xFF; ++i) {
            if (app->ObjectSlot(i) != nullptr)
                ApplyAccessoryTrack(app, i);                    
        }
        SyncAccessoryEditPanel(app);                               
    } else if (app->state.followCameraEnabled != 0) {  
        app->ViewOffsetX() = 0.0f;
        app->ViewOffsetY() = 0.0f;
        ReloadModels(app);
        RefreshLightPanel(app);
        RefreshSelfShadowPanel(app);
        ApplyGravityTrack(app);
        for (int i = 0; i < 0xFF; ++i) {
            if (app->ObjectSlot(i) != nullptr)
                ApplyAccessoryTrack(app, i);
        }
        SyncAccessoryEditPanel(app);
        const std::int32_t sel =
            app->CameraParentModel();
        if (sel >= 0) {
            unsigned char* selModel = ModelAtSlot(app, sel);
            ModelApplyMorphs(selModel);                        
            SetPhysicsMode(selModel, 0, app->ModelSlots(),
                           app->PlaybackPhysicsMode());                    
        }
        PostModelReload(app);                         
    } else {
        EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);  
        EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE); 
    }
    const std::int32_t pages = (sidebar - 0x54) / 26;
    const std::uint32_t frameU =
        static_cast<std::uint32_t>(app->state.currentFrame);
    if (frameU <= static_cast<std::uint32_t>(pages))  
        app->state.timelineStartFrame = 0;
    else
        app->state.timelineStartFrame =
            static_cast<std::int32_t>(frameU - static_cast<std::uint32_t>(pages));
    PanelPaint(app);                                  
    if (app->state.waveEnabled != 0) {  
        TimelineDrawTicks(app->state.timelineStartFrame,
                          app->SidebarWidth());
        RECT rc2;
        rc2.left = 6;
        rc2.top = 95;
        rc2.right = sidebar - 3;
        rc2.bottom = 146;
        InvalidateRect(hwnd, &rc2, 0);
        if (app->state.wavPlaysOnFrameMove != 0) {  
            const bool gate =
                app->state.automaticFrameAdvanceEnabled == 0;  
            app->AudioSeekReady() = 1;  
            if (gate) {
                SetFrameNormalized(app->FrameNormalization()); 
                const std::int32_t frameMinus1 =
                    app->state.currentFrame - 1;
                double t = static_cast<double>(static_cast<std::uint32_t>(frameMinus1)) / 30.0;
                if (t < 0.0)                          
                    t = 0.0;
                WaveRestartAt(app->Audio(), t);
            }
        }
    }
    if (app->state.aviBackgroundEnabled == 1)  
        AviBgOverlayRefresh(app);                               
    app->PhysicsResetPending() = 1;
    return;
}

static void HandleLButtonDown_NameRowRectHit(MMDApp* app, std::int32_t x,
                                             std::int32_t y,
                                             std::int32_t bottom) {
    const std::int32_t boneOff =
        static_cast<std::int8_t>(app->state.interpCurveControlCache[1]) + bottom;
    const std::int32_t boneTop =
        static_cast<std::int8_t>(app->state.interpCurveControlCache[0]);
    if (x < boneTop + 0xD && x > boneTop + 3 &&
        y > boneOff - 0x8C && y < boneOff - 0x82 &&
        app->state.interpCurveUniformFound != 0) {
        if (app->PlaybackActive() != 0)
            return;
        unsigned char* m = ActiveModel(app);
        if (m != nullptr) {
            SnapshotSelectedKeysForUndo(m, app->state.currentFrame);
        }
        app->PendingTimelineSelectionRow() = TimelineSelectionRow::Bone;
        return;
    }
    const std::int32_t morphOff =
        static_cast<std::int8_t>(app->state.interpCurveControlCache[3]) + bottom;
    const std::int32_t morphTop =
        static_cast<std::int8_t>(app->state.interpCurveControlCache[2]);
    if (x < morphTop + 0xD && x > morphTop + 3 &&
        y > morphOff - 0x8C && y < morphOff - 0x82 &&
        app->state.interpCurveUniformFound != 0) {
        if (app->PlaybackActive() != 0)
            return;
        unsigned char* m = ActiveModel(app);
        if (m != nullptr) {
            SnapshotSelectedKeysForUndo(m, app->state.currentFrame);
        }
        app->PendingTimelineSelectionRow() = TimelineSelectionRow::Morph;
        return;
    }
}

void HandleLButtonDown(MMDApp* app) {
    if (app->FullscreenMode() != 0)
        return;                                              

    const HWND hwnd = static_cast<HWND>(app->state.hwnd);  
    RECT rc;
    GetClientRect(hwnd, &rc);
    const std::int32_t y = app->MouseY();
    const std::int32_t x = app->MouseX();
    const std::int32_t sidebar = app->SidebarWidth();

    if (y <= rc.bottom - 0x9E && x <= sidebar + 6 &&
        app->FloatingWindow() == nullptr && sidebar <= x) {
        app->SidebarResizeDragging() = 1;
        app->WindowLayoutReady() = 0;
        return;
    }
    const bool rowGateY = y > 0xA0 && y < rc.bottom - 0xF8;

    if (rowGateY && x > 4 && x < 0x14) {
        HandleLButtonDown_MarginColumnToggle(app, y);
        return;
    }

    if (rowGateY && x > 0x14 && x <= 0x5F) {
        HandleLButtonDown_NameColumnHit(app, hwnd, y);
        return;
    }

    if (rowGateY && x > 0x5F && x < sidebar - 0x12) {
        HandleLButtonDown_TimelineAreaHit(app, hwnd, x, y);
        return;
    }

    if (x < sidebar - 0x12 && y > 0x90 && y < 0xA0 && x > 0x64) {
        HandleLButtonDown_FrameStripHit(app, hwnd, x, y, sidebar);
        return;
    }

    HandleLButtonDown_NameRowRectHit(app, x, y, rc.bottom);
}

}  
