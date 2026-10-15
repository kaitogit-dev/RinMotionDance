#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <io.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

inline std::int32_t RdI32(unsigned char* p, std::size_t off) {
    return *reinterpret_cast<std::int32_t*>(p + off);
}
inline std::uint16_t RdU16(unsigned char* p, std::size_t off) {
    return *reinterpret_cast<std::uint16_t*>(p + off);
}
inline unsigned char*& RdPtr(unsigned char* p, std::size_t off) {
    return *reinterpret_cast<unsigned char**>(p + off);
}
inline std::int32_t& WrI32(unsigned char* p, std::size_t off) {
    return *reinterpret_cast<std::int32_t*>(p + off);
}

inline unsigned char* SlotModel(MMDApp* app) {
    return app->SelectedModel();
}

constexpr int kDisplayObjectCount = 255;
constexpr std::size_t kDisplayOrder = 1181;
constexpr std::size_t kDisplayActive = 1196;

inline unsigned char* DisplayObjectAt(MMDApp* app, int slot) {
    return static_cast<unsigned char*>(app->ObjectSlot(slot));
}
inline std::uint8_t& DisplayOrder(unsigned char* object) {
    return object[kDisplayOrder];
}
inline std::uint8_t& DisplayObjectActive(unsigned char* object) {
    return object[kDisplayActive];
}

inline HWND AppHwnd(MMDApp* app) {
    return app->state.hwnd;
}

inline void KeyEdgeScan(MMDApp* app, int vk, std::uint32_t* cell) {
    if ((GetKeyState(vk) & 0x80) == 0x80) {          
        if (*cell != 0u) {
            *cell = 3;                               
        } else {
            *cell = 1;                               
            app->state.messageSeen = 1;  
        }
    } else if (*cell == 1u || *cell == 3u) {
        *cell = 2;                                   
        app->state.messageSeen = 1;      
    } else {
        *cell = 0;                                   
    }
}

}  

int ReadBeWord(int fh, int* outVal, int nbytes) {
    std::int32_t acc = 0;
    int result;
    if (nbytes <= 0) {                                
        result = static_cast<int>(
            reinterpret_cast<std::uintptr_t>(outVal));  
        *outVal = 0;                                  
    } else {
        int byteBuf = 0;
        result = 0;
        do {
            result = _read(fh, &byteBuf, 1);          
            acc = static_cast<std::uint8_t>(byteBuf) +
                  (acc << 8);                         
            --nbytes;
        } while (nbytes != 0);
        *outVal = acc;                                
    }
    return result;
}

int ReadFixedString(int fh, char* out, int len) {
    char byteBuf = 0;
    char scratch[1000];                               
    int i = 0;
    for (; i < len; ++i) {
        _read(fh, &byteBuf, 1);                       
        scratch[i] = byteBuf;                         
    }
    scratch[i] = 0;                                   
    return sprintf_s(out, 0x3E8u, "%s", scratch);     
}

void PollKeyboardStates(MMDApp* app) {
    struct KeyCell { int vk; std::int32_t MMDAppState::* cell; };
    static constexpr KeyCell kNamedCells[] = {
        {VK_UP, &MMDAppState::upKeyState},
        {VK_DOWN, &MMDAppState::downKeyState},
        {VK_LEFT, &MMDAppState::leftKeyState},
        {VK_RIGHT, &MMDAppState::rightKeyState},
        {VK_SHIFT, &MMDAppState::shiftModifierState},
        {VK_SPACE, &MMDAppState::spaceKeyState},
        {VK_CONTROL, &MMDAppState::ctrlModifierState},
        {VK_DELETE, &MMDAppState::deleteKeyState},
        {VK_ESCAPE, &MMDAppState::escKeyState},
        {VK_TAB, &MMDAppState::tabKeyState},
        {VK_LBUTTON, &MMDAppState::leftMouseButtonState},
        {VK_RBUTTON, &MMDAppState::rightMouseButtonState},
        {VK_MBUTTON, &MMDAppState::middleMouseButtonState},
        {VK_RETURN, &MMDAppState::enterKeyState},
        {VK_MENU, &MMDAppState::menuKeyState},
        {221, &MMDAppState::keyState221},
        {226, &MMDAppState::keyState226},
    };
    for (const KeyCell& e : kNamedCells)
        KeyEdgeScan(app, e.vk,
                    reinterpret_cast<std::uint32_t*>(&(app->state.*e.cell)));

    static constexpr struct { int key; int flagIndex; } kLetterKeys[] = {
        {'x', 0}, {'X', 0}, {'z', 1}, {'Z', 1},
        {'c', 2}, {'C', 2}, {'v', 3}, {'V', 3},
        {'d', 4}, {'D', 4}, {'a', 5}, {'A', 5},
        {'b', 6}, {'B', 6}, {'g', 7}, {'G', 7},
        {'s', 8}, {'S', 8}, {'h', 10}, {'H', 10},
        {'i', 9}, {'I', 9}, {'k', 11}, {'K', 11},
        {'p', 12}, {'P', 12}, {'u', 13}, {'U', 13},
        {'j', 14}, {'J', 14}, {'f', 15}, {'F', 15},
        {'r', 16}, {'R', 16}, {'l', 17}, {'L', 17},
    };
    for (const auto& e : kLetterKeys)
        KeyEdgeScan(app, e.key,
                    reinterpret_cast<std::uint32_t*>(
                        &app->state.dialogFlags[e.flagIndex]));

    for (int i = 0; i < 10; ++i)
        KeyEdgeScan(app, VK_NUMPAD0 + i,
                    reinterpret_cast<std::uint32_t*>(
                        &app->state.numpadKeyState[i]));
}

void ScrollDisplayBoneIntoView(MMDApp* app, int boneIdx) {
    unsigned char* m = SlotModel(app);                          
    const auto* record = mdl::Mdl(m);
    if (record->displayRootBone == static_cast<std::uint32_t>(boneIdx) ||
        boneIdx < 0)                                            
        return;

    int rows = -1;                                              
    const unsigned tblCnt =
        static_cast<unsigned char>(rinmotiondance::mdl::Mdl(m)->groupCount);                    
    if (tblCnt <= 1u)
        return;
    const mdl::DisplayGroup* const tbl = mdl::DisplayGroups(m);

    for (unsigned k = 1; k < tblCnt; ++k) {                     
        ++rows;                                                 
        if (tbl[k].flags == 0)                                  
            continue;
        const unsigned char sep = rinmotiondance::mdl::Mdl(m)->facialFrameCount;                 
        if (sep != 0 && k == 2) {                               
            rows += sep;                                        
            continue;
        }
        const int n = static_cast<int>(mdl::Mdl(m)->rigidBodyCount);
        if (n == 0)
            continue;
        const mdl::FrameGroup* const grps = mdl::RigidGroups(m);
        for (int j = 0; j < n; ++j) {                           
            if (grps[j].groupIndex != k)
                continue;
            ++rows;                                             
            if (grps[j].targetIndex !=
                static_cast<std::uint16_t>(boneIdx))            
                continue;
            const int delta = rows - record->boneListPos;       
            HWND hwnd = AppHwnd(app);
            if (delta >= 0) {
                RECT rc;
                GetClientRect(hwnd, &rc);                       
                const int slack =
                    (rc.bottom - 408) / 14 - delta - 2;         
                if (slack < 0) {
                    int cnt2 = 2 - ((rc.bottom - 408) / 14 - delta);
                    for (; cnt2 != 0; --cnt2)                   
                        SendMessageA(hwnd, WM_VSCROLL, 1,
                                     reinterpret_cast<LPARAM>(
                                         GetDlgItem(hwnd, panel::kTimelineVScroll)));
                }
            } else {
                int cnt2 = -delta;
                for (; cnt2 != 0; --cnt2)                       
                    SendMessageA(hwnd, WM_VSCROLL, 0,
                                 reinterpret_cast<LPARAM>(
                                     GetDlgItem(hwnd, panel::kTimelineVScroll)));
            }
            return;                                             
        }
    }
}

void SelectDisplayBone(MMDApp* app, int boneIdx) {
    unsigned char* m = SlotModel(app);
    auto* record = mdl::Mdl(m);
    record->selectedBone = boneIdx;                             
    const int cap = static_cast<int>(record->boneCount);
    unsigned char* flags = record->boneSelection;
    for (int i = 0; i < cap; ++i)                               
        flags[i] = 0;
    flags[boneIdx] = 1;                                         
    ScrollDisplayBoneIntoView(app, boneIdx);                    
    PostLanguageSweep(app);                                     
    PostLanguageSweep2(app);                                    
}

void SelectPreviousDisplayBone(MMDApp* app) {
    unsigned char* m = SlotModel(app);
    const auto* record = mdl::Mdl(m);
    const int cur = record->selectedBone;                       
    if (cur < 0) {
        SelectDisplayBone(app, record->displayRootBone);                
        return;
    }
    const int rootBone = record->displayRootBone;
    if (rootBone == cur)
        return;                                                 
    const int n = static_cast<int>(mdl::Mdl(m)->rigidBodyCount);
    if (n == 0)
        return;
    const mdl::FrameGroup* const grps = mdl::RigidGroups(m);
    const mdl::DisplayGroup* const tbl = mdl::DisplayGroups(m);

    for (int i = 0; i < n; ++i) {                               
        if (grps[i].targetIndex != static_cast<std::uint16_t>(cur))
            continue;                                           
        int g = grps[i].groupIndex;                             

        if (tbl[g].flags != 0 && i >= 1) {                      
            for (int j = i - 1; j >= 0; --j) {                  
                if (grps[j].groupIndex == g) {
                    SelectDisplayBone(app, grps[j].targetIndex);        
                    return;
                }
            }
        }

        if (g > 0) {
            for (--g; g > 0 && tbl[g].flags == 0; --g) {        
            }
        }
        if (g <= 2) {
            SelectDisplayBone(app, rootBone);                           
            return;
        }
        for (int j = n - 1; j >= 0; --j) {                      
            if (grps[j].groupIndex == g) {
                SelectDisplayBone(app, grps[j].targetIndex);            
                return;
            }
        }
    }
}

void SelectNextDisplayBone(MMDApp* app) {
    unsigned char* m = SlotModel(app);
    const auto* record = mdl::Mdl(m);
    const int cur = record->selectedBone;                       
    if (cur < 0) {
        SelectDisplayBone(app, record->displayRootBone);                
        return;
    }
    const mdl::FrameGroup* const grps = mdl::RigidGroups(m);
    const mdl::DisplayGroup* const tbl = mdl::DisplayGroups(m);
    const int n = static_cast<int>(mdl::Mdl(m)->rigidBodyCount);
    const int tblLast = static_cast<unsigned char>(rinmotiondance::mdl::Mdl(m)->groupCount) - 1;  

    if (cur == record->displayRootBone) {                        
        for (int g = 1; g <= tblLast; ++g) {                    
            if (tbl[g].flags == 0)
                continue;
            for (int j = 0; j < n; ++j) {
                if (grps[j].groupIndex == g) {
                    SelectDisplayBone(app, grps[j].targetIndex);        
                    return;
                }
            }
        }
    }
    if (n == 0)                                                 
        return;

    for (int i = 0; i < n; ++i) {                               
        if (grps[i].targetIndex != static_cast<std::uint16_t>(cur))
            continue;                                           
        const int g = grps[i].groupIndex;                       

        if (tbl[g].flags != 0 && i + 1 < n) {
            for (int j = i + 1; j < n; ++j) {                   
                if (grps[j].groupIndex == g) {
                    SelectDisplayBone(app, grps[j].targetIndex);
                    return;
                }
            }
        }

        if (g < tblLast) {
            int g2 = g;
            for (;;) {                                          
                ++g2;
                if (tbl[g2].flags != 0) {
                    for (int j = 0; j < n; ++j) {
                        if (grps[j].groupIndex == g2) {
                            SelectDisplayBone(app, grps[j].targetIndex);
                            return;
                        }
                    }
                }
                if (g2 >= tblLast)                              
                    break;
            }
        }
    }
}

void ScrollModelListIntoView(MMDApp* app, unsigned char slotIdx) {
    unsigned char* m = DisplayObjectAt(app, slotIdx);           
    if (m == nullptr)
        return;
    const int listPos = app->DisplayObjectListScrollPosition();
    const int delta = DisplayOrder(m) - listPos;                 
    HWND hwnd = AppHwnd(app);
    if (delta >= 0) {
        RECT rc;
        GetClientRect(hwnd, &rc);                               
        const int slack = (rc.bottom - 452) / 14 - delta - 2;   
        if (slack < 0) {
            int cnt = 2 - ((rc.bottom - 452) / 14 - delta);
            for (; cnt != 0; --cnt)                             
                SendMessageA(hwnd, WM_VSCROLL, 1,
                             reinterpret_cast<LPARAM>(
                                 GetDlgItem(hwnd, panel::kTimelineVScroll)));
        }
    } else {
        int cnt = listPos - DisplayOrder(m);
        for (; cnt != 0; --cnt)                                 
            SendMessageA(hwnd, WM_VSCROLL, 0,
                         reinterpret_cast<LPARAM>(GetDlgItem(hwnd, panel::kTimelineVScroll)));
    }
}

namespace {

inline void ClearAllDisplayObjectFlags(MMDApp* app) {
    for (int slot = 0; slot < kDisplayObjectCount; ++slot) {
        unsigned char* object = DisplayObjectAt(app, slot);
        if (object != nullptr)
            DisplayObjectActive(object) = 0;                     
    }
}

inline void SelectDisplayObject(MMDApp* app, int idx) {         
    unsigned char* object = DisplayObjectAt(app, idx);
    DisplayObjectActive(object) = 1;
    HWND hwnd = AppHwnd(app);
    app->SelectedObjectSlot() = static_cast<unsigned char>(idx); 
    const WPARAM wp = DisplayOrder(object);                      
    SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo), CB_SETCURSEL, wp, 0);
    SyncAccessoryEditPanel(app);  
    ScrollModelListIntoView(app, app->SelectedObjectSlot());
    PostLanguageSweep(app);                                     
    PostLanguageSweep2(app);
}

}  

void SelectPrevEditTarget(MMDApp* app) {
    if (app->GlobalTrackSelected(GlobalTimelineTrack::Camera) != 0) {
        ClearAllDisplayObjectFlags(app);
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
        PostLanguageSweep(app);                                 
        PostLanguageSweep2(app);
        return;
    }
    if (app->GlobalTrackSelected(GlobalTimelineTrack::Light) != 0) {
        ClearAllDisplayObjectFlags(app);
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
        PostLanguageSweep(app);
        PostLanguageSweep2(app);
        return;
    }
    if (app->GlobalTrackSelected(GlobalTimelineTrack::SelfShadow) != 0) {
        ClearAllDisplayObjectFlags(app);
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Light);
        PostLanguageSweep(app);
        PostLanguageSweep2(app);
        return;
    }
    if (app->GlobalTrackSelected(GlobalTimelineTrack::Gravity) != 0) {
        ClearAllDisplayObjectFlags(app);
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::SelfShadow);
        PostLanguageSweep(app);
        PostLanguageSweep2(app);
        return;
    }

    ClearAllDisplayObjectFlags(app);
    const unsigned char curSlot = app->SelectedObjectSlot();    
    app->ClearGlobalTimelineTrackSelection();
    unsigned char* current = DisplayObjectAt(app, curSlot);
    if (current == nullptr) {                                   
        app->SelectedObjectSlot() = 0;
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
        PostLanguageSweep(app);                                 
        PostLanguageSweep2(app);
        return;
    }
    const unsigned char frame = DisplayOrder(current);          
    if (frame != 0) {
        const unsigned char target = frame - 1;                 
        int i = 0;
        while (DisplayObjectAt(app, i) == nullptr ||
               DisplayOrder(DisplayObjectAt(app, i)) != target) { 
            ++i;
            if (i >= 255) {
                PostLanguageSweep(app);                         
                PostLanguageSweep2(app);
                return;
            }
        }
        SelectDisplayObject(app, i);                             
        return;
    }
    app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Gravity);
    ScrollModelListIntoView(app, curSlot);                                    
    PostLanguageSweep(app);                                     
    PostLanguageSweep2(app);
}

void SelectNextEditTarget(MMDApp* app) {
    if (app->GlobalTrackSelected(GlobalTimelineTrack::Camera) != 0) {
        ClearAllDisplayObjectFlags(app);
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Light);
        PostLanguageSweep(app);                                 
        PostLanguageSweep2(app);
        return;
    }
    if (app->GlobalTrackSelected(GlobalTimelineTrack::Light) != 0) {
        ClearAllDisplayObjectFlags(app);
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::SelfShadow);
        PostLanguageSweep(app);                                 
        PostLanguageSweep2(app);
        return;
    }
    if (app->GlobalTrackSelected(GlobalTimelineTrack::SelfShadow) != 0) {
        ClearAllDisplayObjectFlags(app);
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Gravity);
        PostLanguageSweep(app);                                 
        PostLanguageSweep2(app);
        return;
    }

    if (app->GlobalTrackSelected(GlobalTimelineTrack::Gravity) != 0) {
        ClearAllDisplayObjectFlags(app);
        app->ClearGlobalTimelineTrackSelection();
        int i = 0;
        while (DisplayObjectAt(app, i) == nullptr ||
               DisplayOrder(DisplayObjectAt(app, i)) != 0) {    
            ++i;
            if (i >= 255) {
                app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Gravity);
                PostLanguageSweep(app);                         
                PostLanguageSweep2(app);
                return;
            }
        }
        SelectDisplayObject(app, i);                             
        return;
    }

    ClearAllDisplayObjectFlags(app);
    const unsigned char curSlot = app->SelectedObjectSlot();    
    unsigned char* cur = DisplayObjectAt(app, curSlot);         
    app->ClearGlobalTimelineTrackSelection();
    if (cur == nullptr) {                                       
        app->SelectedObjectSlot() = 0;
        app->SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
        PostLanguageSweep(app);                                 
        PostLanguageSweep2(app);
        return;
    }
    const unsigned char target = DisplayOrder(cur) + 1;         
    int i = 0;
    while (DisplayObjectAt(app, i) == nullptr ||
           DisplayOrder(DisplayObjectAt(app, i)) != target) {   
        ++i;
        if (i >= 255) {
            DisplayObjectActive(cur) = 1;                        
            ScrollModelListIntoView(app, curSlot);
            PostLanguageSweep(app);                             
            PostLanguageSweep2(app);
            return;
        }
    }
    SelectDisplayObject(app, i);
}

}  
