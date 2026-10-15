#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"

namespace rinmotiondance {
namespace {

constexpr std::size_t kListLeft = 91;         


constexpr char kJpPlus[] = "\x81\x7b";                     
constexpr char kJpMinus[] = "\x81\x7c";                    
constexpr char kJpCamera[] = "\x83\x4a\x83\x81\x83\x89";   
constexpr char kJpLight[] = "\x8f\xc6\x81\x40\x96\xbe";    
constexpr char kJpShadow[] = "\x83\x5a\x83\x8b\x83\x74\x89\x65";  
constexpr char kJpGravity[] = "\x8f\x64\x81\x40\x97\xcd";  

unsigned char* CurrentModel(MMDApp* app, std::size_t slotIdx) {
    return app->ModelSlot(static_cast<int>(slotIdx));
}

void DrawPanelText(MMDApp* app, const char* text, HDC hdc, int size, int x,
                   int y, const std::uint32_t& color) {
    DrawGlyph(app, text, hdc, size, x, y,
              static_cast<std::uint8_t>(color & 0xFF),
              static_cast<std::uint8_t>((color >> 8) & 0xFF),
              static_cast<std::uint8_t>((color >> 16) & 0xFF), 1);
}

void DrawFrameName(MMDApp* app, const mdl::DisplayGroup& frame,
                   HDC hdc, int y, const std::uint32_t& color) {
    const bool english = app->EnglishUI() != 0;
    DrawPanelText(app,
                  english ? frame.nameEn : frame.name,
                  hdc, 12, 12, y, color);
}

void DrawRecordName(MMDApp* app, const mdl::FrameGroup& record, HDC hdc, int y,
                    const std::uint32_t& color) {
    const bool english = app->EnglishUI() != 0;
    DrawPanelText(app,
                  english ? record.nameEn : record.name,
                  hdc, 12, 15, y, color);
}

void DrawHeader(MMDApp* app, HDC hdc, const char* text, int y,
                const std::uint32_t& color) {
    DrawPanelText(app, text, hdc, 12, 12, y, color);
}

}  

void PanelPaint(MMDApp* app);  

void PostLanguageSweep(MMDApp* app) {
    auto& s = *app;
    const bool english = s.EnglishUI() != 0;                        
    const std::size_t slotIdx = s.SelectedModelSlot();  
    HDC panel = s.PanelDC();                                       
    const std::int32_t listH = s.Renderer()->screenHeight;

    HDC dc = CreateCompatibleDC(nullptr);
    SelectObject(dc, reinterpret_cast<HGDIOBJ>(s.state.bmpPanelSpare));  
    BitBlt(panel, 0, 0, kListLeft, listH, dc, 0, 0, SRCCOPY);
    DeleteDC(dc);

    std::memset(s.PanelRowFlags(), 0, 0xC8);     

    if (s.state.optflag[0] == 0) {
        unsigned char* model = CurrentModel(app, slotIdx);
        mdl::ModelRecord* const record = mdl::Mdl(model);
        auto* const boneLineByIndex =
            reinterpret_cast<std::int32_t*>(record->boneKeyIndices);
        auto* const morphLineByIndex =
            reinterpret_cast<std::int32_t*>(record->morphKeyIndices);
        const std::int32_t rootIdx =
            static_cast<std::int32_t>(record->displayRootBone);
        unsigned char* const flagTab = record->boneSelection;
        const mdl::BoneRecord& rootBone = mdl::Bones(model)[rootIdx];
        const char* rootName = english ? rootBone.nameEn : rootBone.name;
        if (flagTab[rootIdx] != 0) {
            DrawPanelText(app, rootName, panel, 12, 12, 17,
                          s.state.themeColors[34]);
            s.PanelRowFlags()[0] = 1;
        } else {
            DrawPanelText(app, rootName, panel, 12, 12, 17,
                          s.state.themeColors[33]);
        }

        std::memset(boneLineByIndex, 0,
                    sizeof(std::uint32_t) * record->boneCount);
        std::memset(morphLineByIndex, 0,
                    sizeof(std::uint32_t) * record->morphCount);
        record->boneListSelLine = 0;
        for (int line0 = 0; line0 < 200; ++line0) {
            record->boneListRowType[line0] = 0;
            record->boneListRowRecord[line0] = -999;
        }
        record->boneListRowRecord[0] = rootIdx;
        record->boneListRows = 1;

        const mdl::DisplayGroup* const frames = mdl::DisplayGroups(model);
        const mdl::FrameGroup* const faces = mdl::DisplayFrames(model);
        const mdl::FrameGroup* const groups = mdl::RigidGroups(model);
        const std::int32_t groupCnt =
            static_cast<std::int32_t>(record->rigidBodyCount);
        std::uint8_t frameIdx = 1;   
        std::uint8_t frameType = 1;  
        int line = 1;                
        std::uint8_t faceIdx = 0;    
        if (mdl::Mdl(model)->groupCount <= 1) {
            PanelPaint(app);
            return;
        }
        for (;;) {
            const int frameNo = frameIdx;  
            const bool visible = frames[frameIdx].flags == 0;
            const std::int32_t iter = record->boneListRows;
            const std::int32_t iterMax = record->boneListPos;

            if (visible) {
                if (iter > iterMax && line < 200) {
                    if (english) {
                        if (frameIdx > 1)
                            DrawGlyph(app, "+", panel, 12, 1, 14 * line + 17, 0, 0, 0, 1);
                        DrawFrameName(app, frames[frameIdx], panel, 14 * line + 17,
                                      s.state.themeColors[33]);
                    } else {
                        if (frameIdx > 1)
                            DrawGlyph(app, kJpPlus, panel, 12, 1, 14 * line + 17, 0, 0, 0, 1);
                        DrawFrameName(app, frames[frameIdx], panel, 14 * line + 17,
                                      s.state.themeColors[33]);
                    }
                    if (((frameType == 1) & record->displayKeyframesPresent) != 0) {
                        DrawFrameName(app, frames[frameIdx], panel, 14 * line + 17,
                                      s.state.themeColors[34]);
                        s.PanelRowFlags()[line] = 1;
                    }
                    if (record->facialFrameCount != 0 && frameType == 2) {
                        bool anyFaceSelected = false;
                        for (unsigned int f = 0;
                             f < mdl::Mdl(model)->facialFrameCount; ++f) {
                            if (faces[f].selected != 0) {
                                anyFaceSelected = true;
                                break;
                            }
                        }
                        if (anyFaceSelected) {
                            DrawFrameName(app, frames[frameIdx], panel,
                                          14 * line + 17,
                                          s.state.themeColors[34]);
                            s.PanelRowFlags()[line] = 1;
                        }
                    }
                    if (groupCnt) {
                        bool anyRigidVisible = false;
                        for (int g = 0; g < groupCnt; ++g) {
                            if (groups[g].groupIndex == frameNo &&
                                flagTab[groups[g].targetIndex] != 0) {
                                anyRigidVisible = true;
                                break;
                            }
                        }
                        if (anyRigidVisible) {
                            DrawFrameName(app, frames[frameIdx], panel,
                                          14 * line + 17,
                                          s.state.themeColors[34]);
                            s.PanelRowFlags()[line] = 1;
                        }
                    }
                    record->boneListRowType[line] = frameType;
                    if (frameType == 1)
                        record->boneListSelLine = line;
                    for (std::uint8_t j = 0;
                         j < mdl::Mdl(model)->facialFrameCount; ++j) {
                        const mdl::FrameGroup& face = faces[j];
                        if (face.groupIndex + 1 == frameNo)
                            morphLineByIndex[face.targetIndex] = -line;
                    }
                    if (groupCnt) {
                        for (unsigned int g = 0;
                             g < static_cast<unsigned int>(groupCnt); ++g) {
                            const mdl::FrameGroup& rigid = groups[g];
                            if (rigid.groupIndex == frameNo)
                                boneLineByIndex[rigid.targetIndex] = -line;
                        }
                    }
                    frameIdx = frameType;
                    ++line;
                }
                ++record->boneListRows;
            } else {
                if (iter > iterMax && line < 200) {
                    if (english) {
                        if (frameIdx > 1)
                            DrawGlyph(app, "-", panel, 12, 1, 14 * line + 17, 0, 0, 0, 1);
                        DrawFrameName(app, frames[frameIdx], panel, 14 * line + 17,
                                      s.state.themeColors[33]);
                    } else {
                        if (frameIdx > 1)
                            DrawGlyph(app, kJpMinus, panel, 12, 1, 14 * line + 17, 0, 0, 0, 1);
                        DrawFrameName(app, frames[frameIdx], panel, 14 * line + 17,
                                      s.state.themeColors[33]);
                    }
                    record->boneListRowType[line++] = frameType;
                    frameIdx = frameType;
                }
                ++record->boneListRows;
                if (frameIdx == 1) {
                    record->boneListSelLine = line - 1;
                } else if (record->facialFrameCount != 0 && frameIdx == 2) {
                    faceIdx = 0;  
                    int recOff = line;  
                    int rowY = 14 * line + 17;
                    for (;;) {
                        if (record->boneListRows > record->boneListPos &&
                            line < 200) {
                            const mdl::FrameGroup& face = faces[faceIdx];
                            if (english) {
                                DrawPanelText(app, face.nameEn, panel, 12, 15,
                                              rowY,
                                              s.state.themeColors[face.selected != 0 ? 34 : 33]);
                            } else {
                                DrawPanelText(app, face.name, panel, 12, 15,
                                              rowY,
                                              s.state.themeColors[face.selected != 0 ? 34 : 33]);
                            }
                            if (face.selected != 0)  
                                s.PanelRowFlags()[line] = 1;
                            record->boneListRowRecord[recOff] =
                                -1 - face.targetIndex;
                            morphLineByIndex[face.targetIndex] = line++;
                            ++recOff;
                            rowY += 14;
                        }
                        ++record->boneListRows;  
                        if (++faceIdx >= record->facialFrameCount)
                            break;               
                    }
                } else if (groupCnt) {
                    int recOff = line;  
                    int rowY = 14 * line + 17;
                    for (unsigned int g = 0;
                         g < static_cast<unsigned int>(groupCnt); ++g) {
                        const mdl::FrameGroup& rigid = groups[g];
                        if (rigid.groupIndex == frameNo) {
                            if (record->boneListRows > record->boneListPos &&
                                line < 200) {
                                if (flagTab[rigid.targetIndex] != 0) {
                                    DrawRecordName(app, rigid, panel, rowY,
                                                   s.state.themeColors[34]);
                                    s.PanelRowFlags()[line] = 1;
                                } else {
                                    DrawRecordName(app, rigid, panel, rowY,
                                                   s.state.themeColors[33]);
                                }
                                record->boneListRowRecord[recOff] =
                                    rigid.targetIndex;
                                boneLineByIndex[rigid.targetIndex] = line++;
                                ++recOff;
                                rowY += 14;
                            }
                            ++record->boneListRows;
                        }
                    }
                }
            }

            frameType = ++frameIdx;
            if (frameIdx >= mdl::Mdl(model)->groupCount) {
                PanelPaint(app);
                return;
            }
        }
    } else {
        if (english) {
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Camera)) {
                DrawHeader(app, panel, "camera", 17, s.state.themeColors[34]);
                s.PanelRowFlags()[0] = 1;
            } else {
                DrawHeader(app, panel, "camera", 17, s.state.themeColors[33]);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Light)) {
                DrawHeader(app, panel, "light", 31, s.state.themeColors[34]);
                s.PanelRowFlags()[1] = 1;
            } else {
                DrawHeader(app, panel, "light", 31, s.state.themeColors[33]);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::SelfShadow)) {
                s.PanelRowFlags()[2] = 1;
                DrawHeader(app, panel, "s shadow", 45, s.state.themeColors[34]);
            } else {
                DrawHeader(app, panel, "s shadow", 45, s.state.themeColors[33]);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Gravity)) {
                s.PanelRowFlags()[3] = 1;
                DrawHeader(app, panel, "gravity", 59, s.state.themeColors[34]);
            } else {
                DrawHeader(app, panel, "gravity", 59, s.state.themeColors[33]);
            }
        } else {
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Camera)) {
                DrawHeader(app, panel, kJpCamera, 17, s.state.themeColors[34]);
                s.PanelRowFlags()[0] = 1;
            } else {
                DrawHeader(app, panel, kJpCamera, 17, s.state.themeColors[33]);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Light)) {
                DrawHeader(app, panel, kJpLight, 31, s.state.themeColors[34]);
                s.PanelRowFlags()[1] = 1;
            } else {
                DrawHeader(app, panel, kJpLight, 31, s.state.themeColors[33]);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::SelfShadow)) {
                s.PanelRowFlags()[2] = 1;
                DrawHeader(app, panel, kJpShadow, 45, s.state.themeColors[34]);
            } else {
                DrawHeader(app, panel, kJpShadow, 45, s.state.themeColors[33]);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Gravity)) {
                s.PanelRowFlags()[3] = 1;
                DrawHeader(app, panel, kJpGravity, 59, s.state.themeColors[34]);
            } else {
                DrawHeader(app, panel, kJpGravity, 59, s.state.themeColors[33]);
            }
        }
    }

    std::memset(s.state.jointLineMap, 0xFF, 0x320);  
    s.DisplayObjectListMatchCount() = 1;
    std::uint8_t order = 0;                        
    int ordinal = 0;                               
    int line = 0;                                  
    do {
        int slot = 0;                              
        int rowY = 14 * line + 73;
        std::int32_t* map = s.state.jointLineMap + line;
        do {
            mdl::AccessoryRecord* accessory =
                static_cast<mdl::AccessoryRecord*>(s.ObjectSlot(slot));
            if (accessory != nullptr && accessory->order == order) {
                if (ordinal >= s.DisplayObjectListScrollPosition() &&
                    line < 200) {
                    if (accessory->rowSelected != 0) {
                        DrawPanelText(app, accessory->name,
                                      panel, 12, 12, rowY, s.state.themeColors[34]);
                        s.PanelRowFlags()[4 + line] = 1;
                    } else {
                        DrawPanelText(app, accessory->name,
                                      panel, 12, 12, rowY, s.state.themeColors[33]);
                    }
                    *map = slot;
                    ++line;
                    ++map;
                    rowY += 14;
                }
                ++s.DisplayObjectListMatchCount();
            }
            ++slot;
        } while (slot < 255);
        ++ordinal;
        ++order;
    } while (order != 0xFF);

    PanelPaint(app);  
}

}  
