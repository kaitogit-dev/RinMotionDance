#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "rinmotiondance/model.hpp"

namespace rinmotiondance {
namespace {

const wchar_t* kJpAllFrame = L"\x5168\xFF8C\xFF9A\xFF70\xFF91";          
const wchar_t* kJpDispIkOp =
    L"\x8868\x793A\xFF65I\x004B\xFF65\x5916\x89AA";                      
const wchar_t* kJpSelBone =
    L"\x9078\x629E\xFF8E\xFF9E\xFF70\xFF9D";                             
const wchar_t* kJpSelFacial = L"\x9078\x629E\x8868\x60C5";               
const wchar_t* kJpAllFacial =
    L"\x5168\x8868\x60C5\xFF8C\xFF9A\xFF70\xFF91";                       
const wchar_t* kJpAllBone =
    L"\x5168\x30DC\x30FC\x30F3\xFF8C\xFF9A\xFF70\xFF91";                 
const char kJpBoneFmt[] =
    "\xCE\xDE\xB0\xDD" "%02d";                                           
const char kDummyBoneName[13] = {
    char(0x83), char(0x5F), char(0x83), char(0x7E), char(0x81), char(0x5B),
    char(0x83), char(0x7B), char(0x81), char(0x5B), char(0x83), char(0x93),
    '\0'};                                        

HWND Dlg(unsigned char* m, int id) {
    return GetDlgItem(*reinterpret_cast<HWND*>(m), id);
}

void RefreshMorphGroup(unsigned char* m, mdl::MorphPanel panel, int comboId,
                       int sliderId, int editId, std::size_t selector) {
    HWND main = *reinterpret_cast<HWND*>(m);
    HWND combo = Dlg(m, comboId);
    SendMessage(combo, CB_RESETCONTENT, 0, 0);
    mdl::ModelRecord* model = mdl::Mdl(m);
    const int n = static_cast<int>(model->morphCount);
    mdl::MorphRecord* morphs = model->morphs;
    const bool useEnglishNames = model->physicsFlags != 0;
    if (n > 0) {
        for (int i = 0; i < n; ++i) {
            const mdl::MorphRecord& morph = morphs[i];
            if (morph.panel == panel)
                SendMessageA(combo, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(
                                 useEnglishNames ? morph.nameEn : morph.name));
        }
    }
    if (morphs != nullptr) {
        const char* text;
        char buf[256];
        const std::int32_t sel = model->selectedMorphs[selector];
        if (sel < 0) {
            SendMessage(Dlg(m, sliderId), TBM_SETPOS, TRUE, 0);
            text = "0.0000";
        } else {
            const float val = morphs[sel].value;
            SendMessage(Dlg(m, sliderId), TBM_SETPOS, TRUE,
                        static_cast<LPARAM>(val * 100.0f));
            sprintf_s(buf, 0x100, "%5.4f", val);
            text = buf;
        }
        SetWindowTextA(Dlg(m, editId), text);
    }
    if (model->selectedMorphs[selector] != -1) {
        const std::int32_t sel = model->selectedMorphs[selector];
        if (n > 0) {
            int catIdx = 0;
            for (int i = 0; i < n; ++i) {
                if (morphs[i].panel == panel) {
                    if (i == sel) {
                        SendMessageA(combo, CB_SETCURSEL, catIdx, 0);
                        return;
                    }
                    ++catIdx;
                }
            }
        }
        return;
    }
    SendMessageA(combo, CB_SETCURSEL, 0, 0);
    if (n > 0) {
        for (int i = 0; i < n; ++i) {
            if (morphs[i].panel == panel) {
                model->selectedMorphs[selector] = i;
                return;
            }
        }
    }
}

}  

void PostLoadInit(unsigned char* m) {
    HWND main = *reinterpret_cast<HWND*>(m);

    HWND boneCombo = Dlg(m, 443);
    SendMessage(boneCombo, CB_RESETCONTENT, 0, 0);
    mdl::ModelRecord* model = mdl::Mdl(m);
    rinmotiondance::mdl::BoneRecord* bones = model->boneTable;
    const bool useEnglishNames = model->physicsFlags != 0;
    const int chainCount = static_cast<int>(model->ikChainCount);
    if (chainCount > 0) {
        for (int i = 0; i < chainCount; ++i) {
            const std::int32_t boneIdx = mdl::IkChains(m)[i].boneIndex;
            SendMessageA(boneCombo, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>(
                    useEnglishNames ? bones[boneIdx].nameEn
                                    : bones[boneIdx].name));
        }
    }
    if (mdl::IkChains(m) != nullptr) {
        SendMessageA(boneCombo, CB_SETCURSEL, 0, 0);
        CheckRadioButton(main, 444, 445,
                         mdl::IkChains(m)[0].enabled != 0 ? 444 : 445);
    }

    SendMessage(Dlg(m, 439), BM_SETCHECK,
                model->loadComplete != 0 ? 1 : 0, 0);
    SendMessage(Dlg(m, 441), BM_SETCHECK,
                model->postLoadFlag2 != 0 ? 1 : 0, 0);
    SendMessage(Dlg(m, 440), BM_SETCHECK,
                model->toonFlag != 0 ? 1 : 0, 0);

    RefreshMorphGroup(m, mdl::MorphPanel::eyebrow, 504, 505, 506, 0);
    RefreshMorphGroup(m, mdl::MorphPanel::eye, 509, 510, 511, 1);
    RefreshMorphGroup(m, mdl::MorphPanel::mouth, 514, 515, 516, 2);
    RefreshMorphGroup(m, mdl::MorphPanel::other, 519, 520, 521, 3);

    HWND frameCombo = Dlg(m, 434);
    SendMessage(frameCombo, CB_RESETCONTENT, 0, 0);
    char buf[256];
    if (std::memcmp(model->name, kDummyBoneName, 13) == 0) {
        if (useEnglishNames) {
            SendMessageA(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("All frame"));
            SendMessageA(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("disp/IK/OP"));
            SendMessageA(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("Sel Bone"));
        } else {
            SendMessageW(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kJpAllFrame));
            SendMessageW(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kJpDispIkOp));
            SendMessageW(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kJpSelBone));
        }
        for (int i = 1; i <= 15; ++i) {
            if (useEnglishNames)
                sprintf_s(buf, 0x100, "bone%02d", i);
            else
                sprintf_s(buf, 0x100, kJpBoneFmt, i);
            SendMessageA(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(buf));
        }
    } else if (useEnglishNames) {
        SendMessageA(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("All frame"));
        SendMessageA(frameCombo, CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(
                bones[model->displayRootBone].nameEn));
        SendMessageA(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("disp/IK/OP"));
        SendMessageA(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("Sel Bone"));
        SendMessageA(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("Sel facial"));
        SendMessageA(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("All facial"));
        const auto* groups =
            model->displayFrames;
        for (std::uint8_t g = 0; g < model->facialFrameCount; ++g)
            SendMessageA(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(groups[g].nameEn));
        SendMessageA(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("All bone"));
        const auto* frames =
            static_cast<const mdl::FrameGroup*>(model->rbGroups);
        const std::int32_t frameCount = model->rigidBodyCount;
        for (int f = 0; f < frameCount; ++f)
            SendMessageA(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(frames[f].nameEn));
    } else {
        SendMessageW(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kJpAllFrame));
        SendMessageA(frameCombo, CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(
                bones[model->displayRootBone].name));
        SendMessageW(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kJpDispIkOp));
        SendMessageW(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kJpSelBone));
        SendMessageW(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kJpSelFacial));
        SendMessageW(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kJpAllFacial));
        const auto* groups =
            model->displayFrames;
        for (std::uint8_t g = 0; g < model->facialFrameCount; ++g)
            SendMessageA(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(groups[g].name));
        SendMessageW(frameCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kJpAllBone));
        const auto* frames =
            static_cast<const mdl::FrameGroup*>(model->rbGroups);
        const std::int32_t frameCount = model->rigidBodyCount;
        for (int f = 0; f < frameCount; ++f)
            SendMessageA(frameCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(frames[f].name));
    }
    SendMessageA(frameCombo, CB_SETCURSEL,
                 model->frameRegistrationSelection, 0);
}

}  
