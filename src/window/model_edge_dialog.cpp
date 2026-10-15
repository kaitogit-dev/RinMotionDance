#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

constexpr int kEditModelNameEn    = 667;  
constexpr int kEditModelCommentEn = 668;  
constexpr int kComboBoneName      = 669;  
constexpr int kEditBoneNameEn     = 672;  
constexpr int kComboMorphName     = 673;  
constexpr int kEditMorphNameEn    = 676;  
constexpr int kComboGroupName     = 677;  
constexpr int kEditGroupNameEn    = 680;  

constexpr int kMainComboModel  = 436;  
constexpr int kMainComboGround = 474;  
constexpr int kMainComboNormal = 449;  
constexpr int kMainRadioFirst  = 490;  

const char kHeadModelCombo[] = "camera/light/accessory";
const char kHeadGroundCombo[] = "ground";
const char kHeadNormalCombo[] = "normal";

int MorphNameSlot(MMDApp* app) {
    return app->ModelEdgeComboCursor(1) + 1;
}

int GroupNameSlot(MMDApp* app, mdl::ModelRecord* model) {
    return app->ModelEdgeComboCursor(2) +
           (model->facialFrameCount != 0 ? 3 : 2);
}

void MirrorFacialFrameNames(mdl::ModelRecord* model) {
    mdl::FrameGroup* frames = model->displayFrames;
    mdl::MorphRecord* morphs = model->morphs;
    for (int i = 0; i < model->facialFrameCount; ++i)
        strcpy_s(frames[i].nameEn, 0x14, morphs[frames[i].targetIndex].nameEn);
}

void MirrorBoneFrameNames(mdl::ModelRecord* model) {
    mdl::FrameGroup* frames =
        static_cast<mdl::FrameGroup*>(model->rbGroups);
    mdl::BoneRecord* bones = model->boneTable;
    const std::uint32_t count = model->rigidBodyCount;
    for (std::uint32_t i = 0; i < count; ++i)
        strcpy_s(frames[i].nameEn, 0x14, bones[frames[i].targetIndex].nameEn);
}

void RefreshNameEdit(HWND edit, const char* text) {
    SendMessageA(edit, EM_SETSEL, 0, GetWindowTextLengthA(edit));
    SendMessageA(edit, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(text));
    SendMessageA(edit, EM_SETSEL, 0, GetWindowTextLengthA(edit));
    SetFocus(edit);
}

}  

void CollectEnglishNameEdit(MMDApp* app, HWND hEdit, int idx) {  
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());
    char text[256];

    if (idx == 0) {  
        GetWindowTextA(hEdit, text, 256);
        if (std::strlen(text) >= 0x14)
            text[19] = '\0';
        strcpy_s(model->nameEn, 0x32, text);

        if (app->state.englishUI != 0) {
            HWND modelCombo = GetDlgItem(app->state.hwnd, kMainComboModel);
            HWND groundCombo = GetDlgItem(app->state.hwnd, kMainComboGround);
            HWND normalCombo = GetDlgItem(app->state.hwnd, kMainComboNormal);
            const int selModel =
                static_cast<int>(SendMessageA(modelCombo, CB_GETCURSEL, 0, 0));
            const int selGround =
                static_cast<int>(SendMessageA(groundCombo, CB_GETCURSEL, 0, 0));
            const int selNormal =
                static_cast<int>(SendMessageA(normalCombo, CB_GETCURSEL, 0, 0));
            SendMessageA(modelCombo, CB_RESETCONTENT, 0, 0);
            SendMessageA(groundCombo, CB_RESETCONTENT, 0, 0);
            SendMessageA(normalCombo, CB_RESETCONTENT, 0, 0);
            SendMessageA(modelCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kHeadModelCombo));
            SendMessageA(groundCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kHeadGroundCombo));
            SendMessageA(normalCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kHeadNormalCombo));
            for (int order = 1; order < kModelSlotCount; ++order) {
                mdl::ModelRecord* ordered = nullptr;
                for (int slot = 0; slot < kModelSlotCount; ++slot) {
                    unsigned char* other = app->ModelSlot(slot);
                    if (other != nullptr &&
                        mdl::Mdl(other)->comboSelIndex == order) {
                        ordered = mdl::Mdl(other);
                        break;
                    }
                }
                if (ordered == nullptr)
                    continue;
                SendMessageA(modelCombo, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(ordered->nameEn));
                SendMessageA(groundCombo, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(ordered->nameEn));
                SendMessageA(normalCombo, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(ordered->nameEn));
            }
            SendMessageA(modelCombo, CB_SETCURSEL, selModel, 0);
            SendMessageA(groundCombo, CB_SETCURSEL, selGround, 0);
            SendMessageA(normalCombo, CB_SETCURSEL, selNormal, 0);
        }
        return;
    }

    switch (idx) {
    case 1: {  
        GetWindowTextA(hEdit, text, 256);
        if (std::strlen(text) >= 0x14)
            text[19] = '\0';
        mdl::BoneRecord* bones = model->boneTable;
        strcpy_s(bones[app->ModelEdgeComboCursor(0)].nameEn, 0x14, text);
        MirrorBoneFrameNames(model);
        break;
    }
    case 2: {  
        if (model->facialFrameCount != 0) {
            GetWindowTextA(hEdit, text, 256);
            if (std::strlen(text) >= 0x14)
                text[19] = '\0';
            strcpy_s(model->morphs[MorphNameSlot(app)].nameEn, 0x14, text);
            PostLanguageSweep(app);                        
            MirrorFacialFrameNames(model);
            PostLoadInit(app->SelectedModel());            
        }
        break;
    }
    case 3: {  
        GetWindowTextA(hEdit, text, 256);
        if (std::strlen(text) >= 0x32)
            text[49] = '\0';
        mdl::DisplayGroup* groups = mdl::DisplayGroups(app->SelectedModel());
        strcpy_s(groups[GroupNameSlot(app, model)].nameEn, 0x32, text);
        break;
    }
    default:
        break;
    }
}

void InitModelEdgeDialog(HWND hDlg) {  
    MMDApp* app = g_Block;
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());
    mdl::BoneRecord* bones = model->boneTable;
    mdl::MorphRecord* morphs = model->morphs;
    char buf[512];

    strcpy_s(buf, 0x200, model->nameEn);
    SendMessageA(GetDlgItem(hDlg, kEditModelNameEn), EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(buf));

    std::size_t out = 0;
    for (const char* p = model->commentEn; *p != '\0';) {
        if (*p == '\r') {  
            buf[out++] = '\r';
            buf[out++] = '\n';
            p += 2;
        } else if (*p == '\n') {  
            buf[out++] = '\r';
            buf[out++] = '\n';
            ++p;
        } else {
            buf[out++] = *p++;
        }
    }
    buf[out] = '\0';
    SendMessageA(GetDlgItem(hDlg, kEditModelCommentEn), EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(buf));

    HWND boneCombo = GetDlgItem(hDlg, kComboBoneName);
    SendMessageA(boneCombo, CB_RESETCONTENT, 0, 0);
    for (std::uint32_t j = 0; j < model->boneCount; ++j)
        SendMessageA(boneCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(bones[j].nameEn));
    SendMessageA(boneCombo, CB_SETCURSEL, 0, 0);
    strcpy_s(buf, 0x200, bones[0].nameEn);
    SendMessageA(GetDlgItem(hDlg, kEditBoneNameEn), EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(buf));

    HWND morphCombo = GetDlgItem(hDlg, kComboMorphName);
    SendMessageA(morphCombo, CB_RESETCONTENT, 0, 0);
    for (std::uint32_t k = 1; k < model->morphCount; ++k)
        SendMessageA(morphCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(morphs[k].nameEn));
    SendMessageA(morphCombo, CB_SETCURSEL, 0, 0);
    if (model->facialFrameCount != 0) {
        strcpy_s(buf, 0x200, morphs[1].nameEn);
        SendMessageA(GetDlgItem(hDlg, kEditMorphNameEn), EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(buf));
    }

    HWND groupCombo = GetDlgItem(hDlg, kComboGroupName);
    SendMessageA(groupCombo, CB_RESETCONTENT, 0, 0);
    mdl::DisplayGroup* groups = mdl::DisplayGroups(app->SelectedModel());
    const std::uint8_t firstGroup =
        model->facialFrameCount != 0 ? 3 : 2;
    for (std::uint8_t g = firstGroup; g < model->groupCount; ++g)
        SendMessageA(groupCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(groups[g].nameEn));
    strcpy_s(buf, 0x100, groups[firstGroup].nameEn);
    SendMessageA(GetDlgItem(hDlg, kEditGroupNameEn), EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(buf));
    SendMessageA(groupCombo, CB_SETCURSEL, 0, 0);

    EnableWindow(GetDlgItem(app->state.hwnd, kMainComboModel), FALSE);
    SendMessageA(GetDlgItem(app->state.hwnd, kMainRadioFirst + 1),
                 BM_SETCHECK, 0, 0);
    SendMessageA(GetDlgItem(app->state.hwnd, kMainRadioFirst + 2),
                 BM_SETCHECK, 0, 0);
    SendMessageA(GetDlgItem(app->state.hwnd, kMainRadioFirst + 3),
                 BM_SETCHECK, 0, 0);
    SendMessageA(GetDlgItem(app->state.hwnd, kMainRadioFirst),
                 BM_SETCHECK, 1, 0);
    app->state.editMode = 0;

    unsigned char* selection = model->boneSelection;
    for (std::uint32_t i = 0; i < model->boneCount; ++i)
        selection[i] = 0;
    selection[0] = 1;
    model->selectedBone = 0;

    PostLanguageSweep(app);                                
    app->ModelEdgeComboCursor(0) = 0;
    app->ModelEdgeComboCursor(1) = 0;
    app->ModelEdgeComboCursor(2) = 0;
}

void ApplyModelEdgeDialog(HWND hDlg) {  
    MMDApp* app = g_Block;
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());
    char text[256];

    GetWindowTextA(GetDlgItem(hDlg, kEditModelCommentEn), text, 256);
    if (std::strlen(text) >= 0x100)
        text[255] = '\0';
    char clean[256];
    char* d = clean;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p != '\r')
            *d++ = *p;
    }
    *d = '\0';
    strcpy_s(model->commentEn, 0x100, clean);

    CollectEnglishNameEdit(app, GetDlgItem(hDlg, kEditModelNameEn), 0);    
    CollectEnglishNameEdit(app, GetDlgItem(hDlg, kEditBoneNameEn), 1);     
    CollectEnglishNameEdit(app, GetDlgItem(hDlg, kEditMorphNameEn), 2);    

    GetWindowTextA(GetDlgItem(hDlg, kEditGroupNameEn), text, 256);
    if (std::strlen(text) >= 0x32)
        text[49] = '\0';
    mdl::DisplayGroup* groups = mdl::DisplayGroups(app->SelectedModel());
    strcpy_s(groups[GroupNameSlot(app, model)].nameEn, 0x32, text);

    MirrorFacialFrameNames(model);
    MirrorBoneFrameNames(model);

    PostLanguageSweep(app);                                
    PostLoadInit(app->SelectedModel());                    
    PostLanguageSweep2(app);                               
}

void SelectModelEdgeBone(HWND hDlg) {  
    MMDApp* app = g_Block;
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());

    CollectEnglishNameEdit(app, GetDlgItem(hDlg, kEditBoneNameEn), 1);    

    const int cursor = static_cast<int>(
        SendMessageA(GetDlgItem(hDlg, kComboBoneName), CB_GETCURSEL, 0, 0));
    app->ModelEdgeComboCursor(0) = cursor;

    unsigned char* selection = model->boneSelection;
    for (std::uint32_t i = 0; i < model->boneCount; ++i)
        selection[i] = 0;

    const mdl::BoneType kind = model->boneTable[cursor].type;
    if (kind == mdl::BoneType::Effector || kind == mdl::BoneType::InertTip ||
        kind == mdl::BoneType::CoRotate) {
        model->selectedBone = -1;
    } else {
        selection[cursor] = 1;
        model->selectedBone = cursor;
    }
    PostLanguageSweep(app);                                

    char buf[256];
    strcpy_s(buf, 0x100, model->boneTable[cursor].nameEn);
    RefreshNameEdit(GetDlgItem(hDlg, kEditBoneNameEn), buf);
}

void SelectModelEdgeMorph(HWND hDlg) {  
    MMDApp* app = g_Block;
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());
    if (model->facialFrameCount == 0)
        return;

    CollectEnglishNameEdit(app, GetDlgItem(hDlg, kEditMorphNameEn), 2);   

    app->ModelEdgeComboCursor(1) = static_cast<int>(
        SendMessageA(GetDlgItem(hDlg, kComboMorphName), CB_GETCURSEL, 0, 0));
    PostLanguageSweep(app);                                

    char buf[256];
    strcpy_s(buf, 0x100, model->morphs[MorphNameSlot(app)].nameEn);
    RefreshNameEdit(GetDlgItem(hDlg, kEditMorphNameEn), buf);
}

void SelectModelEdgeGroup(HWND hDlg) {  
    MMDApp* app = g_Block;
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());

    char text[256];
    GetWindowTextA(GetDlgItem(hDlg, kEditGroupNameEn), text, 256);
    if (std::strlen(text) >= 0x32)
        text[49] = '\0';
    mdl::DisplayGroup* groups = mdl::DisplayGroups(app->SelectedModel());
    strcpy_s(groups[GroupNameSlot(app, model)].nameEn, 0x32, text);

    app->ModelEdgeComboCursor(2) = static_cast<int>(
        SendMessageA(GetDlgItem(hDlg, kComboGroupName), CB_GETCURSEL, 0, 0));
    PostLanguageSweep(app);                                

    char buf[256];
    strcpy_s(buf, 0x100, groups[GroupNameSlot(app, model)].nameEn);
    RefreshNameEdit(GetDlgItem(hDlg, kEditGroupNameEn), buf);
}

}  
