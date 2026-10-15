#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include "rinmotiondance/charset_conv.hpp"
#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"
#include "rinmotiondance/physics_scene.hpp"

#include "btBulletDynamicsCommon.h"

#include "dialog_scaffold.hpp"

namespace rinmotiondance {

void RegisterGravityKeyCurrent(MMDApp* app, std::int32_t frame);  

void AddRigidBody(HWND hDlg);                    
LRESULT CALLBACK PhysicsEditSubclassProc(HWND, UINT, WPARAM, LPARAM);  
void InitPhysicsModelDialog(HWND hDlg);          
void CollectBodyEdits(MMDApp* app);              
void CollectJointEdits(MMDApp* app);             
void AddJoint(HWND hDlg);                        
void ApplyBodyRecordToEdits(HWND hDlg, int idx);  
void ApplyJointRecordToEdits(HWND hDlg, int idx);  
void FlipPhysicsDialogPage(HWND hDlg, int flag);  
void UpdateShapeControls(HWND hDlg, int mode, int idx);  
void CommitPhysicsEdits(HWND hDlg);              
void PickPivotBone(HWND hDlg);                   
INT_PTR CALLBACK PhysicsModelDlgProc(HWND, UINT, WPARAM, LPARAM);  
void InitModelEdgeDialog(HWND hDlg);             
void ApplyModelEdgeDialog(HWND hDlg);            
void SelectModelEdgeBone(HWND hDlg);             
void SelectModelEdgeMorph(HWND hDlg);            
void SelectModelEdgeGroup(HWND hDlg);             
void CollectEnglishNameEdit(MMDApp* app, HWND hEdit, int idx);  
int CollectToonFileNames(HWND hDlg);             
void RefreshMenuLanguage(MMDApp* app);           
void ApplyCameraFrameScaleAdd(MMDApp* app, HWND hDlg);  
void ApplyBoneFrameScaleAdd(MMDApp* app, HWND hDlg);    
void ApplyMorphScaleAdd(MMDApp* app, HWND hDlg);  
void InitModelOrderDialog(int count, HWND hDlg);  
void InitGravityDialog(HWND hDlg);               
void ApplyPhysicsOnOff(int on);                  

void SeekSelectedModelToCurrentFrame(MMDApp* app);  

namespace {

constexpr double kPiTruncatedDouble = static_cast<double>(3.141592f);  
static_assert(__builtin_bit_cast(std::uint64_t, kPiTruncatedDouble) ==
                  0x400921FB00000000ull,
              "dbl_52B768 bit-exact");
constexpr double kDegreesScale = 180.0;                           
static_assert(__builtin_bit_cast(std::uint64_t, kDegreesScale) ==
                  0x4066800000000000ull,
              "dbl_52B760 bit-exact");
constexpr double kPiWidenedFloat = 3.141594886779785;               
static_assert(__builtin_bit_cast(std::uint64_t, kPiWidenedFloat) ==
                  0x400921FC80000000ull,
              "dbl_52E678 bit-exact");
constexpr float kAngleEpsilon = 1e-6f;  
static_assert(__builtin_bit_cast(std::uint32_t, kAngleEpsilon) == 0x358637BDu,
              "flt_52B740 bit-exact");
constexpr float kNegativePiFloat = -3.141592f;  
static_assert(__builtin_bit_cast(std::uint32_t, kNegativePiFloat) == 0xC0490FD8u,
              "flt_52B73C bit-exact");
constexpr float kPiFloat = 3.141592f;  
static_assert(__builtin_bit_cast(std::uint32_t, kPiFloat) == 0x40490FD8u,
              "flt_52B738 bit-exact");


static unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

double AngleFabs(float x) { return static_cast<double>(std::fabs(x)); }
double AngleAsin(float x) { return static_cast<double>(std::asin(x)); }
double AngleCos(float x)  { return static_cast<double>(std::cos(x)); }
double AngleAtan2(float y, float x) {
    return static_cast<double>(static_cast<float>(std::atan2(y, x)));
}

void BindCaptureRenderTarget(MMDApp* app) {
    D3DRenderer* locale = app->Renderer();
    locale->device->SetRenderTarget(0, locale->captureSurface);  
}

void SetMultisampleAntialias(MMDApp* app, int on) {
    D3DRenderer* locale = app->Renderer();
    locale->device->SetRenderState(
        D3DRS_MULTISAMPLEANTIALIAS, static_cast<DWORD>(on));
}

void BoneRotationWriteback(MMDApp* app, const d3dx::D3DXMATRIXF& m) {
    unsigned char* model = ActiveModel(app);
    const std::int32_t cur = mdl::Mdl(model)->selectedBone;
    mdl::BoneRecord* const bones = mdl::Bones(model);
    float quat[4];
    d3dx::Get().quatFromMatrix(quat, &m);
    std::memcpy(bones[cur].rotQuat, quat, sizeof(quat));
    mdl::Mdl(model)->bonePhysicsState[cur] = 1;
    app->SceneModified() = 1;
    PostViewRefresh(app);
}


static const char kMsgEnhanceModelJp[] =
    "\x95\xD2\x8F\x57\x8C\x8B\x89\xCA\x82\xCD\x27\x8A\x67\x92\xA3"
    "\x83\x82\x83\x66\x83\x8B\x95\xDB\x91\xB6\x27\x82\xC5\x90\x56"
    "\x82\xB5\x82\xA2\x83\x82\x83\x66\x83\x8B\x82\xC6\x82\xB5\x82"
    "\xC4\x95\xDB\x91\xB6\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2";
static const char kCaptionEnhanceModelJp[] =
    "\x83\x82\x83\x66\x83\x8B\x8A\x67\x92\xA3";
static const char kCaptionPhysEngineJp[] =
    "\x95\xA8\x97\x9D\x83\x47\x83\x93\x83\x57\x83\x93\x82\xC9\x82\xC2"
    "\x82\xA2\x82\xC4";
static const char kCaptionCreateWndJp[] =
    "\x98\x5E\x89\xE6\x83\x45\x83\x42\x83\x93\x83\x68\x83\x45\x8D\xEC"
    "\x90\xAC";
static const char kRecWndTitle[] =
    "\x98\x5E\x89\xE6\x92\x86(0 frame)";
static const char kMsgOpenOniJp[] =
    "\x83\x4A\x83\x81\x83\x89\x81\x45\x8F\xC6\x96\xBE\x83\x82\x81\x5B"
    "\x83\x68\x82\xC5\x82\xCD" "oni" "\x83\x66\x81\x5B\x83\x5E\x82\xCD"
    "\x93\xC7\x82\xDF\x82\xDC\x82\xB9\x82\xF1";

static const wchar_t kTitleSaveModelJp[] =
    L"\x62E1\x5F35\x30E2\x30C7\x30EB\x4FDD\x5B58";
static const wchar_t kFilterPicJp[] =
    L"\x5168\x3066\x306E\x5BFE\x5FDC\x30D5\x30A9\x30FC\x30DE\x30C3\x30C8"
    L"\x00\x00"
    L"*.bmp;*.jpg;*.png;*.dds;*.dib;*.pfm;*.hdr\x00\x00"
    L"Bmp files(*.bmp)\x00\x00*.bmp\x00\x00";
static const wchar_t kTitleRenderJp[] =
    L"\x753B\x50CF\x30D5\x30A1\x30A4\x30EB\x51FA\x529B";
static const wchar_t kTitleOpenJp[] =
    L"\x30D5\x30A1\x30A4\x30EB\x3092\x958B\x304F";


void SetEdgeThickness(MMDApp* app, float thickness);      
void SetGravityChannel(MMDApp* app, int channel, float value);  
LRESULT CALLBACK EdgeThicknessEditSubclassProc(  
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {  
        const HWND dlg = app->state.edgeThicknessDialog;
        if (hwnd == GetDlgItem(dlg, panel::kEdgeThicknessEdit  )) {
            char buf[8];
            GetWindowTextA(hwnd, buf, 8);
            const float v = static_cast<float>(atof(buf));
            SetEdgeThickness(app, v);                   
            SendMessageA(GetDlgItem(dlg, panel::kEdgeThicknessSlider  ),
                         TBM_SETPOS, 1,
                         static_cast<int>(v * 100.0));  
            return 0;
        }
    }
    return CallWindowProcA(app->EdgeThicknessEditProc(),
                           hwnd, msg, wParam, lParam);
}
float GetEdgeThickness(MMDApp* app) {  
    if (app->state.optflag[0] != 0)
        return 1.0f;
    unsigned char* model = app->SelectedModel();
    return mdl::Mdl(model)->edgeScale;
}
void SetEdgeThickness(MMDApp* app, float thickness) {  
    if (app->state.optflag[0] != 0)
        return;
    app->state.messageSeen = 1;
    app->SceneModified() = 1;
    unsigned char* model = app->SelectedModel();
    mdl::Mdl(model)->edgeScale = thickness;
}
void FillEnhanceModelNameEdits(MMDApp* app, HWND hDlg) {  
    unsigned char* model = app->SelectedModel();
    for (int i = 0; i < 10; ++i)
        SendMessageA(GetDlgItem(hDlg, panel::kToon01Edit + i), EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(mdl::Mdl(model)->pmdToonFileNames[i]));
}
LRESULT CALLBACK ModelEdgeEditSubclassProc(  
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        app->state.messageSeen = 1;
        const HWND dlg = app->FrameRangeDialog();
        if (hwnd == GetDlgItem(dlg, panel::kModelNameEnEdit)) {
            CollectEnglishNameEdit(app, hwnd, 0);                        
            return 0;
        }
        if (hwnd == GetDlgItem(dlg, panel::kBoneNameEnEdit)) {
            CollectEnglishNameEdit(app, hwnd, 1);
            PostLanguageSweep(app);                         
            return 0;
        }
        if (hwnd == GetDlgItem(dlg, panel::kMorphNameEnEdit)) {
            CollectEnglishNameEdit(app, hwnd, 2);
            return 0;
        }
        if (hwnd == GetDlgItem(dlg, panel::kGroupNameEnEdit)) {
            CollectEnglishNameEdit(app, hwnd, 3);
            return 0;
        }
    }
    return CallWindowProcA(app->ModelEdgeEditProc(),
                           hwnd, msg, wParam, lParam);
}
void ApplyModelCalculateOrderDialog(MMDApp* app, int count, HWND hDlg) {  
    (void)hDlg;  
    std::int32_t* order =
        app->DialogOrders().modelIndices.get();
    for (int i = 1; i < count; ++i) {
        unsigned char* model = app->ModelSlot(order[i]);
        mdl::Mdl(model)->comboSelIndex = static_cast<unsigned char>(i);
    }
}
LRESULT CALLBACK GravitySettingEditSubclassProc(  
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        const HWND dlg = app->GravitySettingDialog();
        char buf[0x100];
        for (int ch = 0; ch < 5; ++ch) {
            if (hwnd != GetDlgItem(dlg, panel::kToon01Edit + ch))
                continue;
            GetWindowTextA(hwnd, buf, 0x100);
            const float v = static_cast<float>(atof(buf));
            if (ch >= 1 && ch <= 3)
                SendMessageA(GetDlgItem(dlg, panel::kGravityDirXSlider + ch),
                             TBM_SETPOS, 1,
                             static_cast<int>(v * 100.0));  
            SetGravityChannel(app, ch, v);                   
            return 0;
        }
    }
    return CallWindowProcA(app->AccessoryFrameEditProc(),
                           hwnd, msg, wParam, lParam);
}
void SetGravityChannel(MMDApp* app, int channel, float value) {  
    switch (channel) {
    case 0:
        app->state.gravityMagnitude = value;
        if (app->state.gravityX == 0.0f &&
            app->state.gravityY == 0.0f &&
            app->state.gravityZ == 0.0f)
            app->state.gravityY = 0.1f;  
        break;
    case 1:
        app->state.gravityX = value;
        if (value == 0.0f && app->state.gravityY == 0.0f &&
            app->state.gravityZ == 0.0f)
            app->state.gravityY = 0.1f;
        break;
    case 2:
        app->state.gravityY = value;
        if (app->state.gravityX == 0.0f &&
            app->state.gravityZ == 0.0f && value == 0.0f)
            app->state.gravityY = 0.1f;
        break;
    case 3:
        app->state.gravityZ = value;
        if (app->state.gravityX == 0.0f &&
            app->state.gravityY == 0.0f && value == 0.0f)
            app->state.gravityY = 0.1f;
        break;
    default:
        if (channel == 4)
            app->state.gravityNoise =
                static_cast<std::int32_t>(value);
        RefreshRequest(-4);                                 
        return;
    }

    float dir[3] = {app->state.gravityX,
                    app->state.gravityY,
                    app->state.gravityZ};
    d3dx::Get().vec3Normalize(dir, dir);
    const float mag = app->state.gravityMagnitude;
    float vec[4] = {static_cast<float>(dir[0] * mag * 10.0),
                    static_cast<float>(dir[1] * mag * 10.0),
                    static_cast<float>(dir[2] * mag * 10.0), 0.0f};
    if (PhysicsScene* physicsScene = app->Physics()) {
        if (btDiscreteDynamicsWorld* world = physicsScene->world)
            world->setGravity(btVector3(vec[0], vec[1], vec[2]));
    }
    RefreshRequest(-4);                                     
}
void ApplyGravitySettingDialog(MMDApp* app) {  
    app->SceneModified() = 1;
    mdl::CameraKey* camera = app->CameraKeys();
    mdl::LightKey* light = app->LightKeys();
    mdl::SelfShadowKey* shadow = app->ShadowKeys();
    mdl::GravityKey* gravity = app->GravityKeys();
    for (std::size_t i = 0; i < 10000; ++i) {
        camera[i].selected = 0;
        light[i].selected = 0;
        shadow[i].selected = 0;
        gravity[i].selected = 0;
    }
    for (int slot = 0; slot < 255; ++slot) {
        mdl::AccessoryKey* acc = app->AccessoryKeys(slot);
        if (acc == nullptr)
            continue;
        for (std::size_t i = 0; i < 10000; ++i)
            acc[i].selected = 0;
    }
    RegisterGravityKeyCurrent(app, app->state.currentFrame);  
    RefreshRequest(-4);                                       
    PanelPaint(app);                                          
}
void BuildModelOrderArray(MMDApp* app, int count) {  
    std::int32_t* order =
        app->DialogOrders().modelIndices.get();
    for (int ord = 1; ord < count; ++ord) {
        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr &&
                mdl::Mdl(model)->comboSelIndex == ord) {
                order[ord] = i + 1;
                break;
            }
        }
    }
}
void ApplyModelDisplayOrderDialog(MMDApp* app, int count, HWND hDlg) {  
    (void)hDlg;
    std::int32_t* order =
        app->DialogOrders().modelIndices.get();
    for (int i = 1; i < count; ++i) {
        unsigned char* model = app->ModelSlot(order[i] - 1);
        mdl::Mdl(model)->comboSelIndex = static_cast<unsigned char>(i);
    }
    ApplyModelComboSelection(app);                                       
}

int g_calculateOrderDialogCount = 0;  
int g_displayOrderDialogCount = 0;    

}  
void RegisterGravityKeyCurrent(MMDApp* app, std::int32_t frame) {  
    static const char kJpOverflow[] =
        "\x93\x6f\x98\x5e\x83\x7c\x83\x43\x83\x93\x83\x67\x90\x94\x82\xaa%d"
        "\x8c\xc2\x82\xf0\x89\x7a\x82\xa6\x82\xdc\x82\xb5\x82\xbd\n"
        "\x82\xb1\x82\xea\x88\xc8\x8f\xe3\x82\xcc\x93\x6f\x98\x5e\x82\xcd\x8d"
        "\x73\x82\xa6\x82\xdc\x82\xb5\x82\xf1\n"
        "\x81\x75\xcc\xda\xb0\xd1\x95\xd2\x8f\x57\x81\x76\x82\xcc\x81\x75\x95"
        "\x73\x97\x70\xcc\xda\xb0\xd1\x8d\xed\x8f\x9c\x81\x76\x82\xf0\x8e\xc0"
        "\x8d\x73\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2";
    static const char kJpFrameRegTitle[] =  
        "\xcc\xda\xb0\xd1\x93\x6f\x98\x5e";

    mdl::GravityKey* keys = app->GravityKeys();
    const std::uint32_t target = static_cast<std::uint32_t>(frame);

    std::uint32_t current = 0;
    bool spliceBefore = keys[0].frame >= target;
    if (!spliceBefore) {
        std::uint32_t node = 0;
        for (;;) {
            const std::uint32_t next = keys[node].next;
            if (next == 0)
                break;
            current = next;
            node = next;
            if (keys[current].frame >= target) {
                spliceBefore = true;
                break;
            }
        }
    }

    if (spliceBefore && keys[current].frame == target) {
        mdl::GravityKey& key = keys[current];
        key.acceleration = app->state.gravityMagnitude;
        key.direction[0] = app->state.gravityX;
        key.direction[1] = app->state.gravityY;
        key.direction[2] = app->state.gravityZ;
        key.noise = app->state.gravityNoise;
        key.noiseEnabled = app->GravityNoiseEnabled();
        key.selected = 1;
        return;
    }

    std::uint32_t freeIndex = 1;
    if (keys[1].frame != 0) {
        bool freeFound = false;
        for (std::uint32_t index = 2; index < mdl::kTimelineKeyCapacity;
             ++index) {
            if (keys[index].frame == 0) {
                freeIndex = index;
                freeFound = true;
                break;
            }
        }
        if (!freeFound) {
            char text[256];
            if (app->EnglishUI() != 0) {
                sprintf_s(text, 0x100,
                          "You cannot regist over %dpoint.\n"
                          "Please execute 'delete unused frame'",
                          static_cast<int>(mdl::kTimelineKeyCapacity));
                MessageBoxA(static_cast<HWND>(app->Hwnd()), text,
                            "register frame", 0);
            } else {
                sprintf_s(text, 0x100, kJpOverflow,
                          static_cast<int>(mdl::kTimelineKeyCapacity));
                MessageBoxA(static_cast<HWND>(app->Hwnd()), text,
                            kJpFrameRegTitle, 0);
            }
            return;
        }
    }

    mdl::GravityKey& added = keys[freeIndex];
    if (spliceBefore) {
        const std::uint32_t previous = keys[current].previous;
        keys[previous].next = freeIndex;
        added.previous = previous;
        keys[current].previous = freeIndex;
        added.next = current;
    } else {
        keys[current].next = freeIndex;
        added.previous = current;
    }
    added.frame = target;
    added.acceleration = app->state.gravityMagnitude;
    added.direction[0] = app->state.gravityX;
    added.direction[1] = app->state.gravityY;
    added.direction[2] = app->state.gravityZ;
    added.noise = app->state.gravityNoise;
    added.noiseEnabled = app->GravityNoiseEnabled();
    added.selected = 1;
    if (target > app->state.lastRegisteredFrame)
        app->state.lastRegisteredFrame = target;
}

namespace {  

static const char kOnXMarkJp[] = "ON (X\x88\xF3)";
static const char kOff[] = "OFF";
static const wchar_t kJpCamLightAcc[] = {
    0xFF76, 0xFF92, 0xFF97, 0xFF65, 0x7167, 0x660E, 0xFF65, 0xFF71, 0xFF78,
    0xFF7E, 0xFF7B, 0xFF98, 0x0000};
static const wchar_t kJpScreen[] = {0x5730, 0x9762, 0x0000};
static const wchar_t kJpNone[] = {0x306A, 0x3057, 0x0000};
static const char kCaptionFineShadowJp[] =
    "\x94\xFC\x89\x65\x83\x82\x81\x5B\x83\x68";
static const char kMsgFineShadowJp[] =
    "\x82\xB1\x82\xCC\x89\x42\x82\xB5\x83\x52\x83\x7D\x83\x93\x83\x68\x82\xCD"
    "\x89\xE6\x96\xCA\x89\x45\x8F\xE3\x82\xCC\x81\x75\x94\xFC\x81\x40\x89\x65"
    "\x81\x76\x83\x7B\x83\x5E\x83\x93\x82\xC9\x8E\xE6\x82\xC1\x82\xC4\x95\xCF"
    "\x82\xED\x82\xE8\x82\xDC\x82\xB5\x82\xBD";

INT_PTR CALLBACK BoneFrameMultiplyDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                          LPARAM lParam) {  
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MMDApp* app = g_Block;
        MakeDialogTopmostIfRequested(app, hDlg);
        for (int i = 0; i < 12; i += 2) {
            PrefillEdit(hDlg, panel::kBoneMulPosXScaleEdit + i, "1.0");
        }
        for (int j = 0; j < 12; j += 2) {
            PrefillEdit(hDlg, panel::kBoneMulPosXOffsetEdit + j, "0.0");
        }
        SelectAllEdit(hDlg, panel::kBoneMulPosXScaleEdit);
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            ApplyBoneFrameScaleAdd(g_Block, hDlg);  
            EndDialog(hDlg, 1);
            return 0;
        }
        if (id == 2) {
            EndDialog(hDlg, 2);
            return 0;
        }
    }
    return 0;
}

INT_PTR CALLBACK FacialMultiplyDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                       LPARAM lParam) {  
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MMDApp* app = g_Block;
        MakeDialogTopmostIfRequested(app, hDlg);
        PrefillEdit(hDlg, panel::kMorphMulScaleEdit, "1.0");
        PrefillEdit(hDlg, panel::kMorphMulOffsetEdit, "0.0");
        SelectAllEdit(hDlg, panel::kMorphMulScaleEdit);
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            ApplyMorphScaleAdd(g_Block, hDlg);  
            EndDialog(hDlg, 1);
            return 0;
        }
        if (id == 2) {
            EndDialog(hDlg, 2);
            return 0;
        }
    }
    return 0;
}

INT_PTR CALLBACK EdgeThicknessDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                      LPARAM lParam) {  
    (void)lParam;
    MMDApp* app = g_Block;
    switch (msg) {
    case WM_INITDIALOG: {
        MakeDialogTopmostIfRequested(app, hDlg);
        app->EdgeThicknessEditProc() =
            reinterpret_cast<WNDPROC>(
                GetWindowLongPtrA(GetDlgItem(hDlg, panel::kEdgeThicknessEdit), GWLP_WNDPROC));
        SetWindowLongPtrA(GetDlgItem(hDlg, panel::kEdgeThicknessEdit), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)EdgeThicknessEditSubclassProc);
        const float fps = GetEdgeThickness(app);  
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", static_cast<double>(fps));
        PrefillEdit(hDlg, panel::kEdgeThicknessEdit, buf);
        SendMessageA(GetDlgItem(hDlg, panel::kEdgeThicknessSlider), TBM_SETRANGEMIN, 0, 0);
        SendMessageA(GetDlgItem(hDlg, panel::kEdgeThicknessSlider), TBM_SETRANGEMAX, 0, 200);
        SendMessageA(GetDlgItem(hDlg, panel::kEdgeThicknessSlider), TBM_SETTICFREQ, 0x64, 0);
        SendMessageA(GetDlgItem(hDlg, panel::kEdgeThicknessSlider), TBM_SETPOS, 1,
                     (int)(fps * 100.0));
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 2) {
            DestroyWindow(hDlg);
            app->state.edgeThicknessDialog = nullptr;
            return 1;
        }
        break;
    case WM_HSCROLL: {
        const double v =
            static_cast<double>(
                SendMessageA(GetDlgItem(hDlg, panel::kEdgeThicknessSlider), TBM_GETPOS, 0, 0)) /
            100.0;
        SetEdgeThickness(app, static_cast<float>(v));  
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", v);
        SendMessageA(GetDlgItem(hDlg, panel::kEdgeThicknessEdit), EM_SETSEL, 0,
                     GetWindowTextLengthA(GetDlgItem(hDlg, panel::kEdgeThicknessEdit)));
        SendMessageA(GetDlgItem(hDlg, panel::kEdgeThicknessEdit), EM_REPLACESEL, 0,
                     (LPARAM)buf);
        return 1;
    }
    default:
        break;
    }
    return 0;
}

INT_PTR CALLBACK EnhanceModelDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                     LPARAM lParam) {  
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MMDApp* app = g_Block;
        MakeDialogTopmostIfRequested(app, hDlg);
        FillEnhanceModelNameEdits(app, hDlg);  
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    const WORD id = LOWORD(wParam);
    if (id == 1) {
        if (CollectToonFileNames(hDlg) != 0) {  
            EndDialog(hDlg, 1);
        } else {
            EndDialog(hDlg, 2);
        }
        return 0;
    }
    if (id == 2) {
        EndDialog(hDlg, 2);
        return 0;
    }
    return 0;
}

INT_PTR CALLBACK ModelEdgeDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                  LPARAM lParam) {  
    MMDApp* app = g_Block;
    const HWND hCtrl = reinterpret_cast<HWND>(lParam);
    if (msg == WM_INITDIALOG) {
        app->ModelEdgeEditProc() =
            reinterpret_cast<WNDPROC>(
                GetWindowLongPtrA(GetDlgItem(hDlg, panel::kModelNameEnEdit), GWLP_WNDPROC));
        SetWindowLongPtrA(GetDlgItem(hDlg, panel::kModelNameEnEdit), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)ModelEdgeEditSubclassProc);
        SetWindowLongPtrA(GetDlgItem(hDlg, panel::kBoneNameEnEdit), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)ModelEdgeEditSubclassProc);
        SetWindowLongPtrA(GetDlgItem(hDlg, panel::kMorphNameEnEdit), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)ModelEdgeEditSubclassProc);
        SetWindowLongPtrA(GetDlgItem(hDlg, panel::kGroupNameEnEdit), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)ModelEdgeEditSubclassProc);
        InitModelEdgeDialog(hDlg);  
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    reinterpret_cast<std::int32_t&>(app->state.timeNowHigh) = 1;
    switch (LOWORD(wParam)) {
    case 1:
    case 2:
        ApplyModelEdgeDialog(hDlg);  
        DestroyWindow(hDlg);
        app->FrameRangeDialog() = nullptr;
        EnableWindow(GetDlgItem(app->state.hwnd, panel::kMainComboModel), TRUE);
        if (LOWORD(wParam) == 1) {
            if (app->state.englishUI != 0) {
                MessageBoxA(app->state.hwnd,
                            "Please preserve the edit result as a new model by "
                            "'save enhanced model'.",
                            "enhance model", 0);
            } else {
                MessageBoxA(app->state.hwnd, kMsgEnhanceModelJp,
                            kCaptionEnhanceModelJp, 0);
            }
            app->EnhancedModelDirty() = 1;
        }
        return 1;
    case 0x29E: {  
        const int cur = app->ModelEdgeComboCursor(0);
        const int count = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_GETCOUNT, 0, 0));
        SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_SETCURSEL,
                     cur + 1 >= count ? 0 : cur + 1, 0);
        SelectModelEdgeBone(hDlg);  
        return 0;
    }
    case 0x29F: {  
        int sel = app->ModelEdgeComboCursor(0) - 1;
        if (sel < 0) {
            sel = static_cast<int>(
                      SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_GETCOUNT,
                                   0, 0)) -
                  1;
        }
        SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_SETCURSEL, sel, 0);
        SelectModelEdgeBone(hDlg);  
        return 0;
    }
    case 0x2A2: {  
        const int cur = app->ModelEdgeComboCursor(1);
        const int count = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, panel::kMorphNameCombo), CB_GETCOUNT, 0, 0));
        SendMessageA(GetDlgItem(hDlg, panel::kMorphNameCombo), CB_SETCURSEL,
                     cur + 1 >= count ? 0 : cur + 1, 0);
        SelectModelEdgeMorph(hDlg);  
        return 0;
    }
    case 0x2A3: {  
        int sel = app->ModelEdgeComboCursor(1) - 1;
        if (sel < 0) {
            sel = static_cast<int>(
                      SendMessageA(GetDlgItem(hDlg, panel::kMorphNameCombo), CB_GETCOUNT,
                                   0, 0)) -
                  1;
        }
        SendMessageA(GetDlgItem(hDlg, panel::kMorphNameCombo), CB_SETCURSEL, sel, 0);
        SelectModelEdgeMorph(hDlg);  
        return 0;
    }
    case 0x2A6: {  
        const int cur = app->ModelEdgeComboCursor(2);
        const int count = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, panel::kGroupNameCombo), CB_GETCOUNT, 0, 0));
        SendMessageA(GetDlgItem(hDlg, panel::kGroupNameCombo), CB_SETCURSEL,
                     cur + 1 >= count ? 0 : cur + 1, 0);
        SelectModelEdgeGroup(hDlg);  
        return 0;
    }
    case 679: {  
        int sel = app->ModelEdgeComboCursor(2) - 1;
        if (sel < 0) {
            sel = static_cast<int>(
                      SendMessageA(GetDlgItem(hDlg, panel::kGroupNameCombo), CB_GETCOUNT,
                                   0, 0)) -
                  1;
        }
        SendMessageA(GetDlgItem(hDlg, panel::kGroupNameCombo), CB_SETCURSEL, sel, 0);
        SelectModelEdgeGroup(hDlg);  
        return 0;
    }
    default:
        break;
    }
    if (HIWORD(wParam) == 1  ) {
        if (hCtrl == GetDlgItem(hDlg, panel::kBoneNameCombo)) {
            SelectModelEdgeBone(hDlg);  
            return 0;
        }
        if (hCtrl == GetDlgItem(hDlg, panel::kMorphNameCombo)) {
            SelectModelEdgeMorph(hDlg);  
            return 0;
        }
        if (hCtrl == GetDlgItem(hDlg, panel::kGroupNameCombo)) {
            SelectModelEdgeGroup(hDlg);  
        }
    }
    return 0;
}

INT_PTR CALLBACK ModelCalculateOrderDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                            LPARAM lParam) {  
    (void)lParam;
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        g_calculateOrderDialogCount = static_cast<int>(
            SendMessageA(GetDlgItem(app->state.hwnd, panel::kMainComboModel),
                         CB_GETCOUNT, 0, 0)) -
            1;
        InitModelOrderDialog(g_calculateOrderDialogCount, hDlg);  
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    const WORD id = LOWORD(wParam);
    if (id == 630) {  
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETCURSEL, 0, 0));
        if (sel >= 1) {
            char buf[100];
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, sel,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_DELETESTRING, sel,
                         0);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_INSERTSTRING,
                         sel - 1, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL, sel - 1,
                         0);
            std::int32_t* arr =
                app->DialogOrders().modelIndices.get();
            const std::int32_t tmp = arr[sel - 1];
            arr[sel - 1] = arr[sel];
            arr[sel] = tmp;
        }
        return 0;
    }
    if (id == 631) {  
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETCURSEL, 0, 0));
        if (sel != -1 && sel < g_calculateOrderDialogCount - 1) {
            char buf[100];
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, sel,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_DELETESTRING, sel,
                         0);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_INSERTSTRING,
                         sel + 1, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL, sel + 1,
                         0);
            std::int32_t* arr =
                app->DialogOrders().modelIndices.get();
            const std::int32_t tmp = arr[sel + 1];
            arr[sel + 1] = arr[sel];
            arr[sel] = tmp;
        }
        return 0;
    }
    if (id == 632) {  
        ApplyModelCalculateOrderDialog(app, g_calculateOrderDialogCount, hDlg);  
        EndDialog(hDlg, 1);
        if (app->DialogOrders().modelIndices == nullptr) {
            return 0;
        }
        app->DialogOrders().modelIndices.reset();
        return 0;
    }
    if (id == 2) {  
        EndDialog(hDlg, 2);
        if (app->DialogOrders().modelIndices == nullptr) {
            return 0;
        }
        app->DialogOrders().modelIndices.reset();
        return 0;
    }
    return 0;
}

INT_PTR CALLBACK GravitySettingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                       LPARAM lParam) {  
    MMDApp* app = g_Block;
    const HWND hCtrl = reinterpret_cast<HWND>(lParam);
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        reinterpret_cast<WNDPROC&>(app->BoneFrameScratch()[28]) =
            reinterpret_cast<WNDPROC>(
                GetWindowLongPtrA(GetDlgItem(hDlg, panel::kGravityAccelEdit), GWLP_WNDPROC));
        for (int i = 709; i <= 713; ++i) {
            SetWindowLongPtrA(GetDlgItem(hDlg, i), GWLP_WNDPROC,
                           (LONG)(LONG_PTR)GravitySettingEditSubclassProc);
        }
        InitGravityDialog(hDlg);  
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 731) {
            if (IsDlgButtonChecked(hDlg, panel::kGravityNoiseCheckbox) == 1) {
                app->state.gravityNoiseEnabled = 1;
                EnableWindow(GetDlgItem(hDlg, panel::kGravityNoiseEdit), TRUE);
            } else {
                app->state.gravityNoiseEnabled = 0;
                EnableWindow(GetDlgItem(hDlg, panel::kGravityNoiseEdit), FALSE);
            }
        } else if (HIWORD(wParam) == 768  ) {
            char buf[0x100];
            if (hCtrl == GetDlgItem(app->GravitySettingDialog(), panel::kGravityAccelEdit)) {
                GetWindowTextA(hCtrl, buf, 256);
                SetGravityChannel(app,0, static_cast<float>(atof(buf)));  
            } else if (hCtrl == GetDlgItem(app->GravitySettingDialog(), panel::kGravityDirXEdit)) {
                GetWindowTextA(hCtrl, buf, 256);
                const float f = static_cast<float>(atof(buf));
                SendMessageA(GetDlgItem(app->GravitySettingDialog(), panel::kNumInputPosXEdit),
                             TBM_SETPOS, 1, (int)(f * 100.0));
                SetGravityChannel(app,1, f);  
            } else if (hCtrl == GetDlgItem(app->GravitySettingDialog(), panel::kGravityDirYEdit)) {
                GetWindowTextA(hCtrl, buf, 256);
                const float f = static_cast<float>(atof(buf));
                SendMessageA(GetDlgItem(app->GravitySettingDialog(), panel::kNumInputPosYEdit),
                             TBM_SETPOS, 1, (int)(f * 100.0));
                SetGravityChannel(app,2, f);  
            } else if (hCtrl == GetDlgItem(app->GravitySettingDialog(), panel::kGravityDirZEdit)) {
                GetWindowTextA(hCtrl, buf, 256);
                const float f = static_cast<float>(atof(buf));
                SendMessageA(GetDlgItem(app->GravitySettingDialog(), panel::kNumInputPosZEdit),
                             TBM_SETPOS, 1, (int)(f * 100.0));
                SetGravityChannel(app,3, f);  
            } else if (hCtrl == GetDlgItem(app->GravitySettingDialog(), panel::kGravityNoiseEdit)) {
                GetWindowTextA(hCtrl, buf, 256);
                SetGravityChannel(app,4, static_cast<float>(atof(buf)));  
            }
        } else if (id == 1) {
            ApplyGravitySettingDialog(app);  
        } else if (id == 2) {
            DestroyWindow(hDlg);
            app->GravitySettingDialog() = 0;
            ApplyGravityTrack(app);  
            return 1;
        }
        return 0;
    }
    if (msg != WM_HSCROLL) {
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, panel::kNumInputPosXEdit)) {
        const double v =
            static_cast<double>(
                SendMessageA(hCtrl, TBM_GETPOS, 0, 0)) /
            100.0;
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", v);
        SetWindowTextA(GetDlgItem(hDlg, panel::kGravityDirXEdit), buf);
        SetGravityChannel(app,1, static_cast<float>(v));  
    } else if (hCtrl == GetDlgItem(hDlg, panel::kNumInputPosYEdit)) {
        const double v =
            static_cast<double>(
                SendMessageA(hCtrl, TBM_GETPOS, 0, 0)) /
            100.0;
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", v);
        SetWindowTextA(GetDlgItem(hDlg, panel::kGravityDirYEdit), buf);
        SetGravityChannel(app,2, static_cast<float>(v));  
    } else if (hCtrl == GetDlgItem(hDlg, panel::kNumInputPosZEdit)) {
        const double v =
            static_cast<double>(
                SendMessageA(hCtrl, TBM_GETPOS, 0, 0)) /
            100.0;
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", v);
        SetWindowTextA(GetDlgItem(hDlg, panel::kGravityDirZEdit), buf);
        SetGravityChannel(app,3, static_cast<float>(v));  
    }
    return 1;
}

INT_PTR CALLBACK PhysicsOnOffFrameDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                          LPARAM lParam) {  
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_RESETCONTENT, 0, 0);
        const char* first =
            g_Block->state.englishUI != 0
                ? "ON (X mark)"
                : kOnXMarkJp;
        SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_ADDSTRING, 0,
                     (LPARAM)first);
        SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_ADDSTRING, 0,
                     (LPARAM)kOff);
        SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_SETCURSEL, 0, 0);
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            if (SendMessageA(GetDlgItem(hDlg, panel::kBoneNameCombo), CB_GETCURSEL, 0,
                             0) != 0) {
                ApplyPhysicsOnOff(1);  
            } else {
                ApplyPhysicsOnOff(0);  
            }
            EndDialog(hDlg, 1);
            return 0;
        }
        if (id == 2) {
            EndDialog(hDlg, 1);
            return 0;
        }
    }
    return 0;
}

INT_PTR CALLBACK CameraNumericInputDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                           LPARAM lParam) {  
    (void)lParam;
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        static const std::size_t kEditIds[7] = {637, 638, 639, 640, 641, 642, 644};
        char buf[20];
        for (int i = 0; i < 7; ++i) {
            sprintf_s(buf, 0x14, "%7.3f",
                      static_cast<double>(app->RotationDialogTemp(i)));
            PrefillEdit(hDlg, static_cast<int>(kEditIds[i]), buf);
        }
        SelectAllEdit(hDlg, panel::kNumInputPosXEdit);
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            static const std::size_t kEditIds[7] = {637, 638, 639, 640, 641, 642, 644};
            for (int i = 0; i < 7; ++i) {
                app->RotationDialogTemp(i) =
                    ReadFloatFromEdit(hDlg, static_cast<int>(kEditIds[i]));
            }
            EndDialog(hDlg, 1);
        } else if (id == 2) {
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

INT_PTR CALLBACK BoneNumericInputDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                         LPARAM lParam) {  
    (void)lParam;
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        static const std::size_t kEditIds[6] = {637, 638, 639, 640, 641, 642};
        char buf[20];
        for (int i = 0; i < 6; ++i) {
            sprintf_s(buf, 0x14, "%7.3f",
                      static_cast<double>(app->RotationDialogTemp(i)));
            PrefillEdit(hDlg, static_cast<int>(kEditIds[i]), buf);
        }
        SelectAllEdit(hDlg, panel::kNumInputPosXEdit);
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            static const std::size_t kEditIds[6] = {637, 638, 639, 640, 641, 642};
            for (int i = 0; i < 6; ++i) {
                app->RotationDialogTemp(i) =
                    ReadFloatFromEdit(hDlg, static_cast<int>(kEditIds[i]));
            }
            EndDialog(hDlg, 1);
        } else if (id == 2) {
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

INT_PTR CALLBACK ModelDisplayOrderDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                          LPARAM lParam) {  
    (void)lParam;
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        MakeDialogTopmostIfRequested(app, hDlg);
        const int count = static_cast<int>(
            SendMessageA(GetDlgItem(app->state.hwnd, panel::kMainComboModel),
                         CB_GETCOUNT, 0, 0));
        g_displayOrderDialogCount = count;
        app->DialogOrders().modelIndices.reset(new std::int32_t[count]);
        char buf[0x100];
        for (int i = 1; i < count; ++i) {
            SendMessageA(GetDlgItem(app->state.hwnd, panel::kMainComboModel),
                         CB_GETLBTEXT, i, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_ADDSTRING, 0,
                         (LPARAM)buf);
        }
        BuildModelOrderArray(app, count);  
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    const WORD id = LOWORD(wParam);
    if (id == 630) {  
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETCURSEL, 0, 0));
        if (sel >= 1) {
            char buf[0x100];
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, sel,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_DELETESTRING,
                         sel, 0);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_INSERTSTRING,
                         sel - 1, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL,
                         sel - 1, 0);
            std::int32_t* arr =
                app->DialogOrders().modelIndices.get();
            const std::int32_t tmp = arr[sel + 1];
            arr[sel + 1] = arr[sel];
            arr[sel] = tmp;
        }
        return 0;
    }
    if (id == 631) {  
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETCURSEL, 0, 0));
        if (sel != -1 && sel < g_displayOrderDialogCount - 1) {
            char buf[0x100];
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, sel,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_DELETESTRING,
                         sel, 0);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_INSERTSTRING,
                         sel + 1, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_SETCURSEL,
                         sel + 1, 0);
            std::int32_t* arr =
                app->DialogOrders().modelIndices.get();
            const std::int32_t tmp = arr[sel + 2];
            arr[sel + 2] = arr[sel + 1];
            arr[sel + 1] = tmp;
        }
        return 0;
    }
    if (id == 632) {  
        HWND mainWnd = app->state.hwnd;
        SendMessageA(GetDlgItem(mainWnd, panel::kMainComboModel), CB_RESETCONTENT, 0,
                     0);
        SendMessageA(GetDlgItem(mainWnd, panel::kMainComboGround), CB_RESETCONTENT, 0,
                     0);
        SendMessageA(GetDlgItem(mainWnd, panel::kMainComboNormal), CB_RESETCONTENT, 0,
                     0);
        if (app->state.englishUI != 0) {
            SendMessageA(GetDlgItem(mainWnd, panel::kMainComboModel), CB_ADDSTRING, 0,
                         (LPARAM)"camera/light/accessory");
            SendMessageA(GetDlgItem(mainWnd, panel::kMainComboGround), CB_ADDSTRING, 0,
                         (LPARAM)"ground");
            SendMessageA(GetDlgItem(mainWnd, panel::kMainComboNormal), CB_ADDSTRING, 0,
                         (LPARAM)"non");
        } else {
            SendMessageW(GetDlgItem(mainWnd, panel::kMainComboModel), CB_ADDSTRING, 0,
                         (LPARAM)kJpCamLightAcc);
            SendMessageW(GetDlgItem(mainWnd, panel::kMainComboGround), CB_ADDSTRING, 0,
                         (LPARAM)kJpScreen);
            SendMessageW(GetDlgItem(mainWnd, panel::kMainComboNormal), CB_ADDSTRING, 0,
                         (LPARAM)kJpNone);
        }
        char buf[0x100];
        for (int i = 0; i < g_displayOrderDialogCount - 1; ++i) {
            SendMessageA(GetDlgItem(hDlg, panel::kOrderListBox), LB_GETTEXT, i,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(mainWnd, panel::kMainComboModel), CB_ADDSTRING, 0,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(mainWnd, panel::kMainComboGround), CB_ADDSTRING, 0,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(mainWnd, panel::kMainComboNormal), CB_ADDSTRING, 0,
                         (LPARAM)buf);
        }
        SendMessageA(GetDlgItem(mainWnd, panel::kMainComboModel), CB_SETCURSEL, 0, 0);
        ApplyModelDisplayOrderDialog(app, g_displayOrderDialogCount, hDlg);  
        EndDialog(hDlg, 1);
        if (app->DialogOrders().modelIndices == nullptr) {
            return 0;
        }
        app->DialogOrders().modelIndices.reset();
        return 0;
    }
    if (id == 2) {  
        EndDialog(hDlg, 2);
        if (app->DialogOrders().modelIndices == nullptr) {
            return 0;
        }
        app->DialogOrders().modelIndices.reset();
        return 0;
    }
    return 0;
}

}  

void FineShadowModeNotice(MMDApp* app) {
    const bool english = app->state.englishUI != 0;
    MessageBoxA(app->state.hwnd,
                english
                    ? "This command is changed for \"Fshadow\" button."
                    : kMsgFineShadowJp,
                english ? "fine shadow mode" : kCaptionFineShadowJp,
                0);
}

void CmdViewMenu(MMDApp* app, HWND hwnd, std::uint16_t id,
                 std::uint16_t notify) {
    (void)notify;  
    HINSTANCE hInst = static_cast<HINSTANCE>(app->HInstance());
    const bool english = app->state.englishUI != 0;

    switch (id) {
    case 251: {
        if (app->state.optflag[0] != 0) {
            return;
        }
        app->state.enterKeyState = 1;
        if (DialogBoxParamA(hInst,
                            MAKEINTRESOURCEA(english ? 0x294 : 0x27C),
                            hwnd, BoneFrameMultiplyDlgProc, 0) == 2) {
            return;
        }
        break;
    }

    case 252: {
        if (app->state.optflag[0] != 0) {
            return;
        }
        app->state.enterKeyState = 1;
        if (DialogBoxParamA(hInst,
                            MAKEINTRESOURCEA(english ? 0x295 : 0x283),
                            hwnd, FacialMultiplyDlgProc, 0) == 2) {
            return;
        }
        break;
    }

    case 253: {
        if (app->state.edgeThicknessDialog != nullptr) {
            break;  
        }
        if (app->state.optflag[0] != 0) {
            break;
        }
        HWND dlg = CreateDialogParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x296 : 0x285),
            hwnd, EdgeThicknessDlgProc, 0);
        app->state.edgeThicknessDialog = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        break;
    }

    case 254: {
        if (app->state.projectedShadowBlendEnabled != 0) {
            app->state.projectedShadowBlendEnabled = 0;
            CheckMenuItem(GetMenu(hwnd), 0xFE, MF_UNCHECKED);
        } else {
            app->state.projectedShadowBlendEnabled = 1;
            CheckMenuItem(GetMenu(hwnd), 0xFE, MF_CHECKED);
        }
        break;
    }

    case 255:
    case 256:
    case 257:
    case 258: {
        static const struct {
            int id;
            int flagIdx;
            void (*frameLineOp)(MMDApp*);
        } kFrameEditDialogs[] = {
            {255, 9, InsertBoneCameraFrameLine},    
            {256, 11, DeleteBoneCameraFrameLine},   
            {257, 13, InsertFacialLightFrameLine},  
            {258, 14, DeleteFacialLightFrameLine},  
        };
        for (const auto& entry : kFrameEditDialogs) {
            if (entry.id == id) {
                app->state.dialogFlags[entry.flagIdx] = 1;
                entry.frameLineOp(app);
                app->SceneModified() = 1;
                return;
            }
        }
        break;
    }

    case 259: {
        if (app->FrameRangeDialog() != nullptr) {
            break;
        }
        if (app->state.optflag[0] != 0) {
            break;
        }
        HWND dlg = CreateDialogParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x29A : 0x299),
            hwnd, ModelEdgeDlgProc, 0);
        app->FrameRangeDialog() = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        return;
    }

    case 260: {
        const std::uint8_t cur =
            app->state.englishUI;
        app->state.englishUI = (cur == 0) ? 1 : 0;
        LocalizeUI(app);                             
        RefreshMenuLanguage(app);                              
        return;
    }

    case 261: {
        if (app->state.optflag[0] != 0) {
            return;
        }
        app->state.enterKeyState = 1;
        const INT_PTR r = DialogBoxParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x2AA : 0x2A9),
            hwnd, EnhanceModelDlgProc, 0);
        if (r == 2) {
            return;
        }
        if (english) {
            MessageBoxA(hwnd,
                        "Please preserve the edit result as a new model by "
                        "'save enhanced model'.",
                        "enhance model", MB_TOPMOST);
        } else {
            MessageBoxA(hwnd, kMsgEnhanceModelJp, kCaptionEnhanceModelJp,
                        MB_TOPMOST);
        }
        app->EnhancedModelDirty() = 1;
        return;
    }

    case 262: {
        if (app->state.frameCopyDialog != nullptr) {
            break;
        }
        if (app->state.optflag[0] != 0) {
            return;
        }
        app->state.dialogFlags[6] = 1;
        app->state.enterKeyState = 1;
        HWND dlg = CreateDialogParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x2AC : 0x2AB),
            hwnd, PhysicsModelDlgProc, 0);
        app->state.frameCopyDialog = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        return;
    }

    case 263: {
        app->state.dialogFlags[12] = 1;
        app->state.enterKeyState = 1;
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t fileBuf[0x100];
        fileBuf[0] = L'\0';
        OPENFILENAMEW ofn;
        std::memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.nFilterIndex = 1;
        ofn.lpstrFilter =
            L"Polygon Model files(*.pmd)\x00\x00*.pmd\x00\x00"
            L"All Files(*.*)\x00\x00";
        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = 0x100;
        ofn.Flags = (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY);
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            ofn.lpstrInitialDir = app->DirModel();
        } else {
            ofn.lpstrInitialDir = L"UserFile\\Model";
        }
        ofn.lpstrDefExt = L"pmd";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = english ? L"save enhanced model" : kTitleSaveModelJp;
        if (!GetSaveFileNameW(&ofn)) {
            return;
        }
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            wchar_t* dir = ExtractDirFromPath(
                app->PathWorkspace().projectDirectory,
                fileBuf);
            CopyDirPathW(app->DirModel(), dir);
        }
        SaveEnhancedModel(app, fileBuf);                     
        return;
    }

    case 264: {
        app->state.dialogFlags[4] = 1;
        if (app->state.rigidBodyDisplayEnabled != 0) {
            app->state.rigidBodyDisplayEnabled = 0;
            CheckMenuItem(GetMenu(hwnd), 0x108, MF_UNCHECKED);
        } else {
            app->state.rigidBodyDisplayEnabled = 1;
            CheckMenuItem(GetMenu(hwnd), 0x108, MF_CHECKED);
        }
        return;
    }

    case 265: {
        app->PlaybackPhysicsMode() = 1;
        CheckMenuItem(GetMenu(hwnd), 0x10D, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x109, MF_CHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x10E, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x110, MF_UNCHECKED);
        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                ModelKinematicSync(model);  
            }
        }
        app->PhysicsResetPending() = 1;
        return;
    }

    case 269: {
        app->PlaybackPhysicsMode() = 2;
        CheckMenuItem(GetMenu(hwnd), 0x10D, MF_CHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x109, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x10E, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x110, MF_UNCHECKED);
        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                ModelKinematicSync(model);
            }
        }
        app->PhysicsResetPending() = 1;
        return;
    }

    case 270: {
        app->PlaybackPhysicsMode() = 3;
        CheckMenuItem(GetMenu(hwnd), 0x10D, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x109, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x10E, MF_CHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x110, MF_UNCHECKED);
        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                ModelKinematicSync(model);
            }
        }
        app->PhysicsResetPending() = 1;
        return;
    }

    case 272: {
        app->PlaybackPhysicsMode() = 0;
        CheckMenuItem(GetMenu(hwnd), 0x10D, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x109, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x10E, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x110, MF_CHECKED);
        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                ModelKinematicSync(model);
            }
        }
        return;
    }

    case 266: {
        if (app->GravitySettingDialog() != 0) {
            return;
        }
        app->state.dialogFlags[7] = 1;
        app->state.enterKeyState = 1;
        HWND dlg = CreateDialogParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x323 : 0x322),
            hwnd, GravitySettingDlgProc, 0);
        app->GravitySettingDialog() = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        return;
    }

    case 267: {
        app->state.dialogFlags[5] = 1;
        if (english) {
            MessageBoxA(hwnd,
                        "Bullet Physics Library Ver.2.75\n\n"
                        "http://www.bulletphysics.com/",
                        "about physical engine", MB_TOPMOST);
        } else {
            MessageBoxA(hwnd,
                        "Bullet Physics Library Ver.2.75\n\n"
                        "http://www.bulletphysics.com/",
                        kCaptionPhysEngineJp, MB_TOPMOST);
        }
        return;
    }

    case 268:
        app->state.dialogFlags[9] = 1;
        app->PhysicsResetPending() = 1;
        return;

    case 271: {
        app->state.dialogFlags[12] = 1;
        if ((GetMenuState(GetMenu(hwnd), 0x10F, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x10F, MF_CHECKED);
            app->state.playbackAlwaysOnOffMode = 1;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x10F, MF_UNCHECKED);
            app->state.playbackAlwaysOnOffMode = 0;
        }
        return;
    }

    case 273: {
        if (app->state.optflag[0] != 0) {
            return;
        }
        app->state.dialogFlags[6] = 1;
        unsigned char* model = ActiveModel(app);
        const std::int32_t boneCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);
        if (boneCount > 0) {
            mdl::BoneRecord* const bones = mdl::Bones(model);
            unsigned char* sel = mdl::Mdl(model)->boneSelection;
            for (std::int32_t i = 0; i < boneCount; ++i) {
                if (bones[i].hasRigidBody != 0) {
                    sel[i] = 1;
                    mdl::Mdl(model)->selectedBone = i;
                } else {
                    sel[i] = 0;
                }
            }
        }
        PostLanguageSweep(app);   
        PostLanguageSweep2(app);  
        return;
    }

    case 274: {
        if (app->state.optflag[0] != 0) {
            return;
        }
        app->state.dialogFlags[0] = 1;
        mdl::SelectPhysicsOnBoneKeys(*mdl::Mdl(ActiveModel(app)));
        PanelPaint(app);  
        return;
    }

    case 275:
        app->state.dialogFlags[2] = 1;
        app->state.enterKeyState = 1;
        DialogBoxParamA(hInst,
                        MAKEINTRESOURCEA(english ? 0x32D : 0x32C),
                        hwnd, PhysicsOnOffFrameDlgProc, 0);
        return;

    case 276: {
        app->state.dialogFlags[6] = 1;
        app->state.enterKeyState = 1;
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t fileBuf[0x100];
        fileBuf[0] = L'\0';
        OPENFILENAMEW ofn;
        std::memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = english
            ? L"All picture format\x00\x00"
              L"*.bmp;*.jpg;*.png;*.dds;*.dib;*.pfm;*.hdr\x00\x00"
              L"Bmp files(*.bmp)\x00\x00*.bmp\x00\x00"
            : kFilterPicJp;
        ofn.lpstrFile = fileBuf;
        ofn.nFilterIndex = 1;
        ofn.nMaxFile = 0x100;
        ofn.Flags = (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY);
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            ofn.lpstrInitialDir = app->DirUser();
        } else {
            ofn.lpstrInitialDir = L"UserFile";
        }
        ofn.lpstrDefExt = L"bmp";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = english ? L"render to picture file" : kTitleRenderJp;
        if (!GetSaveFileNameW(&ofn)) {
            return;
        }
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            wchar_t* dir = ExtractDirFromPath(
                app->PathWorkspace().projectDirectory,
                fileBuf);
            CopyDirPathW(app->DirUser(), dir);
        }
        wcscpy_s(app->CaptureSavePath(), 0x100, fileBuf);
        D3DRenderer* locale = app->Renderer();
        const std::int32_t oldW = locale->screenWidth;   
        const std::int32_t oldH = locale->screenHeight;  
        const std::int32_t renderW =
            app->RenderWidth();
        const std::int32_t renderH =
            app->RenderHeight();
        if (renderW > oldW || renderH > oldH) {
            locale->presentParameters.BackBufferWidth = renderW;   
            locale->presentParameters.BackBufferHeight = renderH;  
            locale->screenWidth = renderW;                         
            locale->screenHeight = renderH;                        
            PostDeviceReset(app);  
        }
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = renderW;
        rc.bottom = renderH;
        AdjustWindowRect(&rc, WS_POPUP | WS_CAPTION, FALSE);
        HWND recWnd = CreateWindowExA(
            0, "RecWindow", kRecWndTitle, 0x80C80000,
            app->SidebarWidth() + 0x32, 0x64,
            rc.right - rc.left, rc.bottom - rc.top, hwnd, nullptr, hInst,
            nullptr);
        app->RecordingWindow() = recWnd;                    
        if (recWnd == nullptr) {
            MessageBoxA(hwnd, "CreateWindow failed",
                        english ? "create main window" : kCaptionCreateWndJp,
                        MB_TOPMOST);
            return;
        }
        ShowWindow(recWnd, SW_SHOW);
        UpdateWindow(recWnd);
        if (locale->multisampleAvailable == 0) {  
            BindCaptureRenderTarget(app);
        }
        const bool needRefresh =
            app->CaptureMode() == ScreenCaptureMode::BackgroundRefresh ||
            app->state.aviBackgroundEnabled == 1;
        if (needRefresh && app->state.aviStream != 0) {
            AviBgOverlayRefresh(app);  
        }
        RefreshMainWindowViewport(app);  
        InvalidateRect(hwnd, &app->ViewportRect(), FALSE);
        app->state.timelineAdvanceRequested = 1;
        return;
    }

    case 277: {
        app->state.dialogFlags[10] = 1;
        if ((GetMenuState(GetMenu(hwnd), 0x115, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x115, MF_CHECKED);
            SetMultisampleAntialias(app, 1);
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x115, MF_UNCHECKED);
            SetMultisampleAntialias(app, 0);
        }
        return;
    }

    case 278:
        InitToonTextures(app);  
        ReloadTextureCache(app->Renderer());  
        return;

    case 279: {
        app->state.dialogFlags[12] = 1;
        if ((GetMenuState(GetMenu(hwnd), 0x117, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x117, MF_CHECKED);
            app->state.selfShadowEnabled = 1;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x117, MF_UNCHECKED);
            app->state.selfShadowEnabled = 0;
        }
        return;
    }

    case 280:
        if (app->state.floatingWindow != 0) {
            SaveFlagSubsystem(app);  
        } else {
            InitFlagSubsystem(app);  
        }
        return;

    case 281: {
        app->state.dialogFlags[15] = 1;
        if ((GetMenuState(GetMenu(hwnd), 0x119, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x119, MF_CHECKED);
            if (app->state.floatingWindow != 0) {
                SetWindowPos(app->state.floatingWindow,
                             HWND_TOPMOST, 0, 0, 0, 0, 0x43);
            }
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x119, MF_UNCHECKED);
            if (app->state.floatingWindow != 0) {
                SetWindowPos(app->state.floatingWindow,
                             HWND_NOTOPMOST, 0, 0, 0, 0, 0x43);
            }
        }
        return;
    }

    case 282: {
        app->state.dialogFlags[4] = 1;
        if (app->state.blackBackgroundEnabled != 0) {
            app->state.blackBackgroundEnabled = 0;
            CheckMenuItem(GetMenu(hwnd), 0x11A, MF_UNCHECKED);
        } else {
            app->state.blackBackgroundEnabled = 1;
            CheckMenuItem(GetMenu(hwnd), 0x11A, MF_CHECKED);
        }
        return;
    }

    case 283: {
        app->state.dialogFlags[5] = 1;
        if (app->state.modelNonDisplayMode != 0) {
            app->state.modelNonDisplayMode = 0;
            CheckMenuItem(GetMenu(hwnd), 0x11B, MF_UNCHECKED);
        } else {
            app->state.modelNonDisplayMode = 1;
            CheckMenuItem(GetMenu(hwnd), 0x11B, MF_CHECKED);
        }
        return;
    }

    case 284: {
        app->state.dialogFlags[15] = 1;
        if (app->state.wavPlaysOnFrameMove != 0) {
            app->state.wavPlaysOnFrameMove = 0;
            CheckMenuItem(GetMenu(hwnd), 0x11C, MF_UNCHECKED);
        } else {
            app->state.wavPlaysOnFrameMove = 1;
            CheckMenuItem(GetMenu(hwnd), 0x11C, MF_CHECKED);
        }
        return;
    }

    case 285: {
        app->state.dialogFlags[15] = 1;
        unsigned char* target = reinterpret_cast<unsigned char*>(
            app->Physics()->groundBody);   
        if (app->state.floorVisible != 0) {
            app->state.floorVisible = 0;
            CheckMenuItem(GetMenu(hwnd), 0x11D, MF_UNCHECKED);
            *reinterpret_cast<std::int32_t*>(target + 0xE0) = -1;
        } else {
            app->state.floorVisible = 1;
            CheckMenuItem(GetMenu(hwnd), 0x11D, MF_CHECKED);
            *reinterpret_cast<std::int32_t*>(target + 0xE0) = 1;
        }
        return;
    }

    case 286: {
        app->state.dialogFlags[6] = 1;
        app->state.enterKeyState = 1;
        CHOOSECOLORA cc;
        std::memset(&cc, 0, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = hwnd;
        const std::uint8_t r =
            app->state.modelOutlineColorRed;
        const std::uint16_t g =
            app->state.modelOutlineColorGreen;
        const std::uint8_t b =
            app->state.modelOutlineColorBlue;
        cc.rgbResult = (static_cast<COLORREF>(b) << 16) |
                       (static_cast<COLORREF>(g) << 8) | r;
        cc.lpCustColors =
            reinterpret_cast<COLORREF*>(app->state.customColorTable);
        cc.Flags = 3;  
        if (!ChooseColorA(&cc)) {
            return;
        }
        const std::uint8_t nr = cc.rgbResult & 0xFF;
        const std::uint8_t ng = (cc.rgbResult >> 8) & 0xFF;
        const std::uint8_t nb = (cc.rgbResult >> 16) & 0xFF;
        app->state.modelOutlineColorRed = nr;
        app->state.modelOutlineColorGreen = ng;
        app->state.modelOutlineColorBlue = nb;
        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                SetModelColor(reinterpret_cast<MMDApp*>(model), nr, ng, nb);
            }
        }
        return;
    }

    case 287: {
        if (app->WireframeRenderingEnabled() != 0) {
            CheckMenuItem(GetMenu(hwnd), 0x11F, MF_UNCHECKED);
            app->WireframeRenderingEnabled() = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x11F, MF_CHECKED);
            app->WireframeRenderingEnabled() = 1;
        }
        return;
    }

    case 288:
        app->state.enterKeyState = 1;
        DialogBoxParamA(hInst,
                        MAKEINTRESOURCEA(english ? 0x325 : 0x324),
                        hwnd, ModelDisplayOrderDlgProc, 0);
        return;

    case 289:
        app->state.dialogFlags[2] = 1;
        app->state.enterKeyState = 1;
        DialogBoxParamA(hInst,
                        MAKEINTRESOURCEA(english ? 0x32B : 0x32A),
                        hwnd, ModelCalculateOrderDlgProc, 0);
        return;

    case 290:
        app->FullscreenMode() = 1;
        ApplyFullscreenWindowState(app);  
        PostDeviceReset(app);  
        return;

    case 291:
        app->state.dialogFlags[11] = 1;
        if (app->state.depthDeviceEnabled != 0) {
            DisableKinect(app);  
        } else {
            OpenNiInit(app, nullptr);  
        }
        return;

    case 292: {
        app->state.dialogFlags[2] = 1;
        if (app->state.autoRepeat == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x124, MF_CHECKED);
            app->state.autoRepeat = 1;
            SetTimer(hwnd, 0x65, 0x5DC, nullptr);
            app->PhysicsResetPending() = 1;
            for (int i = 0; i < kModelSlotCount; ++i) {
                unsigned char* model = app->ModelSlot(i);
                if (model != nullptr) {
                    mdl::Mdl(model)->openniVersion = app->state.openniVersion;
                }
            }
        } else {
            KillTimer(hwnd, 0x65);
            CheckMenuItem(GetMenu(hwnd), 0x124, MF_UNCHECKED);
            app->state.autoRepeat = 0;
            app->SceneModified() = 1;
            app->PhysicsResetPending() = 1;
        }
        return;
    }

    case 293: {
        if (app->state.kinectMirrorEnabled != 0) {
            CheckMenuItem(GetMenu(hwnd), 0x125, MF_UNCHECKED);
            app->state.kinectMirrorEnabled = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x125, MF_CHECKED);
            app->state.kinectMirrorEnabled = 1;
        }
        return;
    }

    case 294: {
        app->state.dialogFlags[17] = 1;
        if (app->state.kinectInitLostBone != 0) {
            CheckMenuItem(GetMenu(hwnd), 0x126, MF_UNCHECKED);
            app->state.kinectInitLostBone = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x126, MF_CHECKED);
            app->state.kinectInitLostBone = 1;
        }
        return;
    }

    case 295: {
        app->state.dialogFlags[4] = 1;
        if (app->state.depthTextureCompositionEnabled != 0) {
            CheckMenuItem(GetMenu(hwnd), 0x127, MF_UNCHECKED);
            app->state.depthTextureCompositionEnabled = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x127, MF_CHECKED);
            app->state.depthTextureCompositionEnabled = 1;
        }
        return;
    }

    case 296: {
        if (app->state.optflag[0] != 0) {
            if (english) {
                MessageBoxA(hwnd, "Please select model!", "open oni data",
                            MB_TOPMOST);
            } else {
                MessageBoxA(hwnd, kMsgOpenOniJp, "oni",
                            MB_TOPMOST);
            }
            return;
        }
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t fileBuf[0x100];
        fileBuf[0] = L'\0';
        OPENFILENAMEW ofn;
        std::memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = app->state.floatingWindow != 0
                            ? app->state.floatingWindow
                            : hwnd;
        ofn.lpstrFilter =
            L"oni files(*.oni)\x00\x00*.oni\x00\x00All Files(*.*)\x00\x00";
        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = 0x100;
        ofn.Flags = OFN_FILEMUSTEXIST;
        ofn.lpstrInitialDir = L"UserFile";
        ofn.lpstrDefExt = L"oni";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = english ? L"open file data" : kTitleOpenJp;
        if (!GetOpenFileNameW(&ofn)) {
            return;
        }
        if (app->state.depthDeviceEnabled != 0) {
            DisableKinect(app);  
        }
        char sjisPath[0x100];
        WideToSjis(app->Renderer(), sjisPath, fileBuf, 0x100);  
        OpenNiInit(app, sjisPath);  
        return;
    }

    case 297: {
        app->state.dialogFlags[4] = 1;
        if (app->state.automaticFrameAdvanceEnabled != 0) {
            app->state.automaticFrameAdvanceEnabled = 0;
            CheckMenuItem(GetMenu(hwnd), 0x129, MF_UNCHECKED);
        } else {
            app->state.automaticFrameAdvanceEnabled = 1;
            CheckMenuItem(GetMenu(hwnd), 0x129, MF_CHECKED);
        }
        return;
    }

    case 298: {
        D3DRenderer* locale = app->Renderer();
        if ((GetMenuState(GetMenu(hwnd), 0x12A, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x12A, MF_CHECKED);
            locale->runtimeToggle = 1;  
            InitRenderStates(app);  
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x12A, MF_UNCHECKED);
            locale->runtimeToggle = 0;  
            InitRenderStates(app);
        }
        return;
    }

    case 299: {
        HWND owner = app->state.floatingWindow != 0
                         ? app->state.floatingWindow
                         : hwnd;
        if ((GetMenuState(GetMenu(hwnd), 0x12B, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x12B, MF_CHECKED);
            SendMessageA(GetDlgItem(owner, panel::kFrameVolumeCheckbox), BM_SETCHECK, 1, 0);
            app->FrameVolumeControlEnabled() = 1;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x12B, MF_UNCHECKED);
            SendMessageA(GetDlgItem(owner, panel::kFrameVolumeCheckbox), BM_SETCHECK, 0, 0);
            app->FrameVolumeControlEnabled() = 0;
        }
        return;
    }

    case 300: {
        const double kD1 = kPiTruncatedDouble;  
        const double kD2 = kDegreesScale;  
        const double kD3 = kPiWidenedFloat;  
        const float kPi = kPiFloat;   
        const float kNegPi = kNegativePiFloat;
        const float kThr = kAngleEpsilon;
        app->state.enterKeyState = 1;
        if (app->state.optflag[0] != 0) {
            const float t0 = app->CameraPositionX();
            const float t1 = app->CameraPositionY();
            const float t2 = app->CameraPositionZ();
            const float t3 = static_cast<float>(
                -static_cast<double>(app->CameraPitch()) *
                kD2 / kD1);
            const float t4 = static_cast<float>(
                static_cast<double>(app->CameraYaw()) *
                kD2 / kD1);
            const float t5 = static_cast<float>(
                static_cast<double>(app->CameraRoll()) *
                kD2 / kD1);
            const float t6 = static_cast<float>(
                -static_cast<double>(app->CameraDistance()));
            if (DialogBoxParamA(
                    hInst, MAKEINTRESOURCEA(english ? 0x298 : 0x289),
                    hwnd, CameraNumericInputDlgProc, 0) == 2) {
                return;
            }
            app->CameraPositionX() = t0;
            app->CameraPositionY() = t1;
            app->CameraPositionZ() = t2;
            app->CameraPitch() = static_cast<float>(
                -static_cast<double>(t3) * kD3 / kD2);
            app->CameraYaw() = static_cast<float>(
                static_cast<double>(t4) * kD3 / kD2);
            app->CameraRoll() = static_cast<float>(
                kD2 / (kD3 * static_cast<double>(t5)));
            app->CameraDistance() =
                static_cast<float>(-static_cast<double>(t6));
            app->SceneModified() = 1;
            PostViewRefresh(app);  
            return;
        }
        unsigned char* model = ActiveModel(app);
        const std::int32_t curBone = mdl::Mdl(model)->selectedBone;
        if (curBone == -1) {
            return;
        }
        rinmotiondance::mdl::BoneRecord* bone = mdl::Bones(model) + curBone;
        const float t0 = bone->trans[0];
        const float t1 = bone->trans[1];
        const float t2 = bone->trans[2];
        d3dx::D3DXMATRIXF m{};
        d3dx::Get().matrixRotationQuaternion(
            &m, reinterpret_cast<const float*>(bone->rotQuat));
        float t5 = static_cast<float>(AngleAtan2(m.m[1][0], m.m[0][1]));
        float t3 = static_cast<float>(AngleAsin(-m.m[2][0]));
        float t4 = static_cast<float>(AngleAtan2(m.m[2][1], m.m[1][3]));
        const double r1 = AngleFabs(static_cast<float>(AngleCos(t3)));
        if (r1 < static_cast<double>(kThr)) {
            t5 = static_cast<float>(static_cast<double>(t5) +
                                    (!(m.m[0][1] > 0.0f) ? kNegPi : kPi));
            t4 = static_cast<float>(static_cast<double>(t4) +
                                    (!(m.m[1][3] > 0.0f) ? kNegPi : kPi));
        }
        if (static_cast<float>(AngleFabs(t3)) < kThr) {
            t3 = 0.0f;
        }
        if (static_cast<float>(AngleFabs(t4)) < kThr) {
            t4 = 0.0f;
        }
        if (static_cast<float>(AngleFabs(t5)) < kThr) {
            t5 = 0.0f;
        }
        t3 = static_cast<float>(static_cast<double>(t3) * kD2 / kD1);
        t4 = static_cast<float>(-static_cast<double>(t4) * kD2 / kD1);
        t5 = static_cast<float>(-kD1 * kD2 / static_cast<double>(t5));
        if (DialogBoxParamA(
                hInst, MAKEINTRESOURCEA(english ? 0x297 : 0x288),
                hwnd, BoneNumericInputDlgProc, 0) == 2) {
            return;
        }
        PushBoneEditUndo(app);  
        model = ActiveModel(app);
        bone = mdl::Bones(model) + curBone;
        bone->trans[0] = t0;
        bone->trans[1] = t1;
        bone->trans[2] = t2;
        t3 = static_cast<float>(static_cast<double>(t3) * kD1 / kD2);
        t4 = static_cast<float>(-static_cast<double>(t4) * kD1 / kD2);
        t5 = static_cast<float>(-kD2 / (static_cast<double>(t5) * kD1));
        d3dx::D3DXMATRIXF tmp{};
        d3dx::D3DXMATRIXF rot{};
        d3dx::Get().rotZ(&rot, t5);
        d3dx::Get().rotX(&tmp, t3);
        d3dx::Get().multiply(&rot, &rot, &tmp);
        d3dx::Get().rotY(&tmp, t4);
        d3dx::Get().multiply(&rot, &rot, &tmp);
        BoneRotationWriteback(app, rot);
        return;
    }

    case 301: {
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x12D, MF_CHECKED);
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x12D, MF_UNCHECKED);
        }
        return;
    }

    case 302: {
        app->state.dialogFlags[1] = 1;
        if (app->state.optflag[0] != 0) {
            app->CameraPitch() = 0.0f;
            app->CameraYaw() = 0.0f;
            app->CameraRoll() = 0.0f;
            app->SceneModified() = 1;
            PostViewRefresh(app);
            return;
        }
        unsigned char* model = ActiveModel(app);
        if (mdl::Mdl(model)->selectedBone == -1) {
            return;
        }
        PushBoneEditUndo(app);  
        d3dx::D3DXMATRIXF ident{};
        for (int i = 0; i < 4; ++i) {
            ident.m[i][i] = 1.0f;
        }
        BoneRotationWriteback(app, ident);
        return;
    }

    default:
        break;
    }
}

}  
