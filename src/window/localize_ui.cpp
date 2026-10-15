#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "ui_controls.inc"

namespace rinmotiondance {
namespace {

struct TextEntry { int id; const char* en; };

constexpr TextEntry kEnTexts[] = {
    {400, "undo"}, {401, "redo"}, {418, "<"}, {419, ">"}, {429, "current"},
    {416, "v-sel"}, {423, "delete"}, {420, "copy"}, {421, "paste"},
    {422, "revers"}, {415, "range-sel"}, {424, "expand"}, {432, "liner"},
    {431, "paste"}, {430, "copy"}, {435, "load"}, {437, "delete"},
    {440, "shadow"}, {441, "Add-syn"}, {442, "OP"}, {438, "register"},
    {451, "reset"}, {446, "perspect"}, {452, "register"}, {454, "+"},
    {453, "-"}, {467, "reset"}, {468, "register"}, {562, "off"},
    {563, "mode1"}, {564, "mode2"}, {565, "register"}, {470, "+"},
    {469, "-"}, {489, "+"}, {488, "-"}, {472, "load"}, {473, "delete"},
    {477, "Add-syn"}, {487, "register"}, {402, "front"}, {403, "back"},
    {404, "top"}, {405, "left"}, {406, "right"}, {407, "btm"},
    {408, "play"}, {535, "track"}, {490, "select"}, {493, "rotate"},
    {492, "move"}, {491, "BOX-sel"}, {494, "select all"},
    {501, "unregisted"}, {496, "copy"}, {497, "paste"}, {498, "revers"},
    {500, "register"}, {499, "physics"}, {495, "reset"}, {503, "+"},
    {502, "-"}, {529, "+"}, {528, "-"}, {567, "+"}, {566, "-"},
    {524, "register"}, {525, "register"}, {527, "register"},
    {526, "register"}, {543, "dist"}, {551, "info"}, {552, "low pow"},
    {557, "axis"}, {555, "set"}, {556, "Fshadow"},
};

HWND Dlg(MMDApp* app, int id) {
    return GetDlgItem(static_cast<HWND>(app->Hwnd()), id);
}

const wchar_t* JpControlText(int id) {
    static wchar_t buf[64];
    for (const ui::ControlSpec& c : ui::kControls) {
        if (c.id != id)
            continue;
        if (c.wtext != nullptr)
            return c.wtext;
        int i = 0;
        for (; i < 63 && c.atext != nullptr && c.atext[i] != '\0'; ++i)
            buf[i] = static_cast<unsigned char>(c.atext[i]);
        buf[i] = L'\0';
        return buf;
    }
    return L"";
}

}  

void LocalizeUI(MMDApp* app) {
    auto& s = *app;
    const bool english = s.EnglishUI() != 0;                       
    const bool modelMode = s.state.optflag[0] != 0;  

    HWND combo433 = Dlg(app, 433);
    SendMessageA(combo433, CB_RESETCONTENT, 0, 0);

    if (english) {
        for (const TextEntry& t : kEnTexts)
            SetWindowTextA(Dlg(app, t.id), t.en);
        SetWindowTextA(Dlg(app, 536), modelMode ? "To model" : "camera");
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"x axis move");
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"y axis move");
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"z axis move");
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"rotation");
        if (modelMode) {
            SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"distance");
            SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"view angle");
        }
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"all");
    } else {
        for (const TextEntry& t : kEnTexts)
            SetWindowTextW(Dlg(app, t.id), JpControlText(t.id));
        SetWindowTextW(Dlg(app, 536), JpControlText(536));
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\xff38\x79fb\x52d5");               
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\xff39\x79fb\x52d5");               
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\xff3a\x79fb\x52d5");               
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\x56de\x3000\x8ee2");               
        if (modelMode) {
            SendMessageW(combo433, CB_ADDSTRING, 0,
                         (LPARAM)L"\x8ddd\x3000\x96e2");           
            SendMessageW(combo433, CB_ADDSTRING, 0,
                         (LPARAM)L"\x8996\x91ce\x89d2");           
        }
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\x3059\x3079\x3066");               
    }
    SendMessageA(combo433, CB_SETCURSEL, 3, 0);

    for (int i = 0; i < kModelSlotCount; ++i) {
        unsigned char* slot = s.ModelSlot(i);
        if (slot != nullptr)
            mdl::Mdl(slot)->physicsFlags =
                static_cast<unsigned char>(english);
    }
    if (s.Audio() != nullptr)
        s.Audio()->englishUI = static_cast<unsigned char>(english);

    InvalidateRect(static_cast<HWND>(s.Hwnd()), nullptr, FALSE);
    PostLanguageSweep(app);                                        
    PostLanguageSweep2(app);                                       

    if (modelMode) {
        HWND combo434 = Dlg(app, 434);
        SendMessageA(combo434, CB_DELETESTRING, 0, 0);
        SendMessageA(combo434, CB_DELETESTRING, 0, 0);
        SendMessageA(combo434, CB_DELETESTRING, 0, 0);
        if (english) {
            SendMessageA(combo434, CB_INSERTSTRING, 0, (LPARAM)"gravity");
            SendMessageA(combo434, CB_INSERTSTRING, 0, (LPARAM)"s shadow");
            SendMessageA(combo434, CB_INSERTSTRING, 0, (LPARAM)"light");
            SendMessageA(combo434, CB_INSERTSTRING, 0, (LPARAM)"camera");
        } else {
            SendMessageW(combo434, CB_INSERTSTRING, 0,
                         (LPARAM)L"\x91cd\x529b");                 
            SendMessageW(combo434, CB_INSERTSTRING, 0,
                         (LPARAM)L"\x30bb\x30eb\x30d5\x5f71");     
            SendMessageW(combo434, CB_INSERTSTRING, 0,
                         (LPARAM)L"\x7167\x660e");                 
            SendMessageW(combo434, CB_INSERTSTRING, 0,
                         (LPARAM)L"\x30ab\x30e1\x30e9");           
        }
        SendMessageA(combo434, CB_SETCURSEL, 0, 0);
    } else {
        PostLoadInit(s.SelectedModel());                           
    }

    HWND combo436 = Dlg(app, 436), combo474 = Dlg(app, 474), combo449 = Dlg(app, 449);
    LRESULT sel436 = SendMessageA(combo436, CB_GETCURSEL, 0, 0);
    LRESULT sel474 = SendMessageA(combo474, CB_GETCURSEL, 0, 0);
    LRESULT sel449 = SendMessageA(combo449, CB_GETCURSEL, 0, 0);
    SendMessageA(combo436, CB_RESETCONTENT, 0, 0);
    SendMessageA(combo474, CB_RESETCONTENT, 0, 0);
    SendMessageA(combo449, CB_RESETCONTENT, 0, 0);
    if (english) {
        SendMessageA(combo436, CB_ADDSTRING, 0, (LPARAM)"camera/light/accessory");  
        SendMessageA(combo474, CB_ADDSTRING, 0, (LPARAM)"ground");                 
        SendMessageA(combo449, CB_ADDSTRING, 0, (LPARAM)"non");                    
    } else {
        SendMessageW(combo436, CB_ADDSTRING, 0,
                     (LPARAM)L"\xff76\xff92\xff97\xff65\x7167\x660e"
                              L"\xff65\xff71\xff78\xff7e\xff7b\xff98");  
        SendMessageW(combo474, CB_ADDSTRING, 0,
                     (LPARAM)L"\x5730\x9762");                           
        SendMessageW(combo449, CB_ADDSTRING, 0,
                     (LPARAM)L"\x306a\x3057");                           
    }

    for (int order = 1; order < kModelSlotCount; ++order) {
        int slotIdx = -1;
        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* slot = s.ModelSlot(i);
            if (slot != nullptr &&
                rinmotiondance::mdl::Mdl(slot)->comboSelIndex == order) {
                slotIdx = i;
                break;
            }
        }
        if (slotIdx < 0)
            continue;
        unsigned char* slot = s.ModelSlot(slotIdx);
        mdl::ModelRecord* model = mdl::Mdl(slot);
        LPARAM name = reinterpret_cast<LPARAM>(
            english ? model->nameEn : model->name);
        SendMessageA(combo436, CB_ADDSTRING, 0, name);
        SendMessageA(combo474, CB_ADDSTRING, 0, name);
        SendMessageA(combo449, CB_ADDSTRING, 0, name);
    }

    SendMessageA(combo436, CB_SETCURSEL, sel436, 0);
    SendMessageA(combo474, CB_SETCURSEL, sel474, 0);
    SendMessageA(combo449, CB_SETCURSEL, sel449, 0);
}

}  
