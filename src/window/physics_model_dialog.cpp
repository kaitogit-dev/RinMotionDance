#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include "btBulletDynamicsCommon.h"

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

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

constexpr int kAddBodyButton = 685;        
constexpr int kBodyPageFirst = 685;
constexpr int kBodyPageLast = 734;
constexpr int kDeleteBodyButton = 706;     
constexpr int kBodyListCombo = 704;
constexpr int kBodyNameEdit = 705;
constexpr int kBoneCombo = 707;            
constexpr int kGroupCombo = 708;           
constexpr int kSizeXEdit = 709;
constexpr int kSizeYEdit = 710;
constexpr int kSizeZEdit = 711;
constexpr int kPosXEdit = 712;
constexpr int kPosYEdit = 713;
constexpr int kPosZEdit = 714;
constexpr int kRotXEdit = 715;
constexpr int kRotYEdit = 716;
constexpr int kRotZEdit = 717;
constexpr int kMassEdit = 718;
constexpr int kGroupTextEdit = 719;
constexpr int kLinDampEdit = 720;
constexpr int kAngDampEdit = 721;
constexpr int kRestitutionEdit = 722;
constexpr int kFrictionEdit = 723;
constexpr int kCopyBodyButton = 724;       
constexpr int kPasteBodyButton = 725;      
constexpr int kShapeSphereRadio = 726;     
constexpr int kShapeBoxRadio = 727;        
constexpr int kShapeCapsuleRadio = 728;    
constexpr int kPhysicalRadio = 729;        
constexpr int kBoneFollowRadio = 730;      
constexpr int kBoneAlignCheckbox = 731;    
constexpr int kSizeXLabel = 732;
constexpr int kSizeYLabel = 733;
constexpr int kSizeZLabel = 734;
constexpr int kJointPageFirst = 735;
constexpr int kJointPageLast = 799;
constexpr int kAddJointButton = 735;       
constexpr int kJointListCombo = 736;
constexpr int kJointNameEdit = 740;
constexpr int kDeleteJointButton = 737;    
constexpr int kBodyACombo = 741;
constexpr int kBodyBCombo = 742;
constexpr int kPivotBoneCombo = 743;
constexpr int kCopyJointButton = 738;      
constexpr int kPasteJointButton = 739;     
constexpr int kJointPosXEdit = 744;
constexpr int kJointPosYEdit = 745;
constexpr int kJointPosZEdit = 746;
constexpr int kLinLowerXEdit = 747;      
constexpr int kSpringLinXEdit = 748;     
constexpr int kSpringLinYEdit = 749;
constexpr int kSpringLinZEdit = 750;
constexpr int kSpringAngXEdit = 751;     
constexpr int kSpringAngYEdit = 752;
constexpr int kSpringAngZEdit = 753;
constexpr int kJointRotXEdit = 754;
constexpr int kJointRotYEdit = 755;
constexpr int kJointRotZEdit = 756;
constexpr int kLinUpperXEdit = 757;      
constexpr int kLinLowerYEdit = 758;
constexpr int kLinUpperYEdit = 759;
constexpr int kLinLowerZEdit = 760;
constexpr int kLinUpperZEdit = 761;
constexpr int kAngLowerXEdit = 762;      
constexpr int kAngUpperXEdit = 763;      
constexpr int kAngLowerYEdit = 764;      
constexpr int kAngUpperYEdit = 765;      
constexpr int kAngLowerZEdit = 766;
constexpr int kAngUpperZEdit = 767;
constexpr int kBodyPageRadio = 800;        
constexpr int kJointPageRadio = 801;       

#if defined(_M_X64)
constexpr int kMaxEditRecords = 100000;    
#else
constexpr int kMaxEditRecords = 10000;     
#endif
constexpr float kPiF = 3.1415927f;         
static_assert(__builtin_bit_cast(std::uint32_t, kPiF) == 0x40490FDBu,
              "float-rounded pi bit-exact");
constexpr float kPiShort = 3.141592f;      
static_assert(__builtin_bit_cast(std::uint32_t, kPiShort) == 0x40490FD8u,
              "truncated pi bit-exact");

static const char kMsgEnhanceModelJp[] =
    "\x95\xd2\x8f\x57\x8c\x8b\x89\xca\x82\xcd\x27\x8a\x67\x92\xa3"
    "\x83\x82\x83\x66\x83\x8b\x95\xdb\x91\xb6\x27\x82\xc5\x90\x56"
    "\x82\xb5\x82\xa2\x83\x82\x83\x66\x83\x8b\x82\xc6\x82\xb5\x82"
    "\xc4\x95\xdb\x91\xb6\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2";
static const char kCaptionEnhanceModelJp[] =
    "\x83\x82\x83\x66\x83\x8b\x8a\x87\x92\xa3";
static const char kCaptionAngleLimitJp[] =
    "Joint\x8a\x70\x93\x78\x90\xa7\x8c\xc0";
static const char kMsgAngleLimitJp[] =
    "Bullet\x82\xcc\x8e\x64\x97\x6c\x8f\xe3\x81\x41y\x8e\xb2\x82"
    "\xcc\x8a\x70\x93\x78\x90\xa7\x8c\xc0\x82\xcd-90<y<90\x82\xcc"
    "\x8a\xd4\x82\xb5\x82\xa9\x90\xdd\x92\xe8\x82\xc5\x82\xab\x82"
    "\xdc\x82\xb9\x82\xf1";
static const char kCaptionAddJointJp[] =
    "\x83\x57\x83\x87\x83\x43\x83\x93\x83\x67\x92\xc7\x89\xc1";
static const char kMsgNoBodyJp[] =
    "\x8d\x84\x91\xcc\x82\xaa\x88\xea\x82\xc2\x82\xe0\x82\xa0\x82"
    "\xe8\x82\xdc\x82\xb9\x82\xf1\x0a\x90\xe6\x82\xc9\x8d\x84\x91"
    "\xcc\x82\xf0\x92\xc7\x89\xc1\x82\xb5\x82\xc4\x89\xba\x82\xb3"
    "\x82\xa2";
static const char kGroup16Jp[] = "16(\x8f\xb0)";
static const char kLabelRadiusJp[] = "\x94\xbc\x8c\x61";
static const char kLabelWidthJp[] = "\x81\x40\x95\x9d";
static const char kLabelHeightJp[] = "\x8d\x82\x82\xb3";
static const char kLabelDepthJp[] = "\x89\x9c\x8d\x73";

namespace {

using Rigid = mdl::RigidRecord;
using Joint = mdl::JointRecord;

Rigid* EditBodies() {
    MMDApp* app = g_Block;
    return static_cast<Rigid*>(app->state.rigidScratchArray);
}
Joint* EditJoints() {
    MMDApp* app = g_Block;
    return static_cast<Joint*>(app->state.jointScratchArray);
}

int BodyComboIndex(const Rigid& rb) {
    return static_cast<int>(reinterpret_cast<std::intptr_t>(rb.keyData));
}
void SetBodyComboIndex(Rigid& rb, int index) {
    rb.keyData = reinterpret_cast<void*>(static_cast<std::intptr_t>(index));
}

Rigid s_bodyCopyScratch;
Joint s_jointCopyScratch;
unsigned char s_jointPageVisible = 0;

void RemovePhysJointByUid(PhysicsScene* scene, int uid) {
    if (scene == nullptr || scene->world == nullptr)
        return;
    btDiscreteDynamicsWorld* const world = scene->world;
    for (int i = world->getNumConstraints() - 1; i >= 0; --i) {
        btTypedConstraint* const item = world->getConstraint(i);
        if (item != nullptr && item->getUid() == uid) {
            world->removeConstraint(item);
            delete item;
            return;
        }
    }
}

void RemovePhysRigidBody(PhysicsScene* scene, btRigidBody* body) {
    if (scene == nullptr || scene->world == nullptr || body == nullptr)
        return;
    scene->world->removeRigidBody(body);
    delete body->getMotionState();
    delete body->getCollisionShape();
    delete body;
}

void TransformPoint(float out[3], const float point[3], const float m[16]) {
    for (int column = 0; column < 3; ++column) {
        out[column] = point[0] * m[column] + point[1] * m[4 + column] +
                      point[2] * m[8 + column] + m[12 + column];
    }
}

void ReadEditFloat(HWND hWnd, float& field) {
    char text[256];
    GetWindowTextA(hWnd, text, 256);
    field = static_cast<float>(atof(text));
}
void ReadEditFloatClamped(HWND hWnd, float& field) {
    char text[256];
    GetWindowTextA(hWnd, text, 256);
    if (atof(text) < 0.1) {
        SetWindowTextA(hWnd, "0.1");
        GetWindowTextA(hWnd, text, 256);
    }
    field = static_cast<float>(atof(text));
}
void ReadEditDegrees(HWND hWnd, float& field) {
    char text[256];
    GetWindowTextA(hWnd, text, 256);
    field = static_cast<float>(atof(text)) / 180.0f * kPiF;
}

void CommitNameToCombo(HWND hList, int current, const char* text) {
    SendMessageA(hList, CB_DELETESTRING, current, 0);
    SendMessageA(hList, CB_INSERTSTRING, current,
                 reinterpret_cast<LPARAM>(text));
}

float CheckYAngleLimit(HWND hDlg, HWND hEdit, float value) {
    MMDApp* app = g_Block;
    char text[256];
    if (value <= -90.0f || value >= 90.0f) {
        if (app->state.englishUI != 0) {
            MessageBoxA(hDlg, "y-axis limit must between -90 to 90",
                        "angle limit", 0);
        } else {
            MessageBoxA(hDlg, kMsgAngleLimitJp, kCaptionAngleLimitJp, 0);
        }
        if (value > -90.0f) {
            value = 80.0f;
            strcpy_s(text, sizeof text, "80.00");
        } else {
            value = -80.0f;
            strcpy_s(text, sizeof text, "-80.00");
        }
        SetWindowTextA(hEdit, text);
    }
    return value;
}

}  

LRESULT CALLBACK PhysicsEditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {  
    MMDApp* app = g_Block;
    if (msg != WM_KEYDOWN || wParam != VK_RETURN)
        return CallWindowProcA(app->FrameCopyEditProc(), hWnd, msg, wParam,
                               lParam);
    app->state.timeNowHigh = 1;
    HWND hDlg = app->state.frameCopyDialog;
    Rigid* bodies = EditBodies();
    Joint* joints = EditJoints();
    Rigid& rb = bodies[app->state.selectedRigidIndex];
    Joint& jt = joints[app->state.selectedJointIndex];

    if (hWnd == GetDlgItem(hDlg, kBodyNameEdit)) {
        char text[256];
        GetWindowTextA(hWnd, text, 256);
        if (std::strlen(text) > 20)
            text[19] = '\0';
        const int current = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_GETCURSEL, 0, 0));
        CommitNameToCombo(GetDlgItem(hDlg, kBodyListCombo), current, text);
        CommitNameToCombo(GetDlgItem(hDlg, kBodyACombo), current, text);
        CommitNameToCombo(GetDlgItem(hDlg, kBodyBCombo), current, text);
        SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_SETCURSEL,
                     current, 0);
        sprintf_s(rb.name, sizeof rb.name, "%s", text);
        return 0;
    }
    if (hWnd == GetDlgItem(hDlg, kSizeXEdit)) { ReadEditFloatClamped(hWnd, rb.size[0]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kSizeYEdit)) { ReadEditFloatClamped(hWnd, rb.size[1]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kSizeZEdit)) { ReadEditFloatClamped(hWnd, rb.size[2]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kPosXEdit)) { ReadEditFloat(hWnd, rb.position[0]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kPosYEdit)) { ReadEditFloat(hWnd, rb.position[1]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kPosZEdit)) { ReadEditFloat(hWnd, rb.position[2]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kRotXEdit)) { ReadEditDegrees(hWnd, rb.rotation[0]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kRotYEdit)) { ReadEditDegrees(hWnd, rb.rotation[1]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kRotZEdit)) { ReadEditDegrees(hWnd, rb.rotation[2]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kMassEdit)) { ReadEditFloatClamped(hWnd, rb.mass); return 0; }
    if (hWnd == GetDlgItem(hDlg, kLinDampEdit)) { ReadEditFloat(hWnd, rb.linearDamping); return 0; }
    if (hWnd == GetDlgItem(hDlg, kAngDampEdit)) { ReadEditFloat(hWnd, rb.angularDamping); return 0; }
    if (hWnd == GetDlgItem(hDlg, kRestitutionEdit)) { ReadEditFloat(hWnd, rb.restitution); return 0; }
    if (hWnd == GetDlgItem(hDlg, kFrictionEdit)) { ReadEditFloat(hWnd, rb.friction); return 0; }

    if (hWnd == GetDlgItem(hDlg, kJointNameEdit)) {
        char text[256];
        GetWindowTextA(hWnd, text, 256);
        if (std::strlen(text) > 20)
            text[19] = '\0';
        const int current = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_GETCURSEL, 0, 0));
        CommitNameToCombo(GetDlgItem(hDlg, kJointListCombo), current, text);
        SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_SETCURSEL,
                     current, 0);
        sprintf_s(jt.name, sizeof jt.name, "%s", text);
        return 0;
    }
    if (hWnd == GetDlgItem(hDlg, kJointPosXEdit)) { ReadEditFloat(hWnd, jt.position[0]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kJointPosYEdit)) { ReadEditFloat(hWnd, jt.position[1]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kJointPosZEdit)) { ReadEditFloat(hWnd, jt.position[2]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kJointRotXEdit)) { ReadEditDegrees(hWnd, jt.rotation[0]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kJointRotYEdit)) { ReadEditDegrees(hWnd, jt.rotation[1]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kJointRotZEdit)) { ReadEditDegrees(hWnd, jt.rotation[2]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kLinUpperXEdit)) { ReadEditFloat(hWnd, jt.limits[0]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kLinUpperYEdit)) { ReadEditFloat(hWnd, jt.limits[1]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kLinUpperZEdit)) { ReadEditFloat(hWnd, jt.limits[2]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kLinLowerXEdit)) { ReadEditFloat(hWnd, jt.limits[3]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kLinLowerYEdit)) { ReadEditFloat(hWnd, jt.limits[4]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kLinLowerZEdit)) { ReadEditFloat(hWnd, jt.limits[5]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kAngUpperXEdit)) { ReadEditDegrees(hWnd, jt.limits[6]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kAngUpperZEdit)) { ReadEditDegrees(hWnd, jt.limits[8]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kAngLowerXEdit)) { ReadEditDegrees(hWnd, jt.limits[9]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kAngLowerZEdit)) { ReadEditDegrees(hWnd, jt.limits[11]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kSpringLinXEdit)) { ReadEditFloat(hWnd, jt.springs[0]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kSpringLinYEdit)) { ReadEditFloat(hWnd, jt.springs[1]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kSpringLinZEdit)) { ReadEditFloat(hWnd, jt.springs[2]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kSpringAngXEdit)) { ReadEditFloat(hWnd, jt.springs[3]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kSpringAngYEdit)) { ReadEditFloat(hWnd, jt.springs[4]); return 0; }
    if (hWnd == GetDlgItem(hDlg, kSpringAngZEdit)) { ReadEditFloat(hWnd, jt.springs[5]); return 0; }

    if (hWnd == GetDlgItem(hDlg, kAngUpperYEdit)) {
        char text[256];
        GetWindowTextA(hWnd, text, 256);
        float value = static_cast<float>(atof(text));
        value = CheckYAngleLimit(hDlg, hWnd, value);
        jt.limits[7] = value / 180.0f * kPiShort;
        return 0;
    }
    if (hWnd == GetDlgItem(hDlg, kAngLowerYEdit)) {
        char text[256];
        GetWindowTextA(hWnd, text, 256);
        float value = static_cast<float>(atof(text));
        value = CheckYAngleLimit(hDlg, hWnd, value);
        jt.limits[10] = value / 180.0f * kPiShort;
        return 0;
    }
    return CallWindowProcA(app->FrameCopyEditProc(), hWnd, msg, wParam,
                           lParam);
}

void InitPhysicsModelDialog(HWND hDlg) {  
    MMDApp* app = g_Block;
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());

    EnableWindow(GetDlgItem(app->state.hwnd, panel::kPlayButton), FALSE);

    app->state.rigidScratchArray =
        std::calloc(kMaxEditRecords, sizeof(Rigid));
    Rigid* bodies = EditBodies();
    for (int i = 0; i < kMaxEditRecords; ++i) {
        SetBodyComboIndex(bodies[i], -1);
        bodies[i].body = nullptr;
    }
    app->state.jointScratchArray =
        std::calloc(kMaxEditRecords, sizeof(Joint));
    Joint* joints = EditJoints();
    for (int i = 0; i < kMaxEditRecords; ++i) {
        joints[i].rigidA = 0;
        joints[i].rigidB = 0;
        joints[i].constraint = -1;
    }

    for (int id = kJointPageFirst; id <= kJointPageLast; ++id)
        ShowWindow(GetDlgItem(hDlg, id), SW_HIDE);
    CheckRadioButton(hDlg, kBodyPageRadio, kJointPageRadio, kBodyPageRadio);
    s_jointPageVisible = 0;

    SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_RESETCONTENT,
                 0, 0);
    for (std::uint32_t i = 0; i < model->rigidCount; ++i) {
        Rigid& dst = bodies[i];
        std::memcpy(&dst, &model->rigidTable[i], sizeof(Rigid));
        if (model->rigidTable[i].staticFlag != 0)
            dst.mode = 0;
        else if (model->rigidTable[i].kinematicFlag != 0)
            dst.mode = 2;
        else
            dst.mode = 1;
        dst.noCollapse = static_cast<std::uint16_t>(~dst.noCollapse);
        const LPARAM name = reinterpret_cast<LPARAM>(dst.name);
        SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_ADDSTRING, 0, name);
        SendMessageA(GetDlgItem(hDlg, kBodyACombo), CB_ADDSTRING, 0, name);
        SendMessageA(GetDlgItem(hDlg, kBodyBCombo), CB_ADDSTRING, 0, name);
        SetBodyComboIndex(dst, static_cast<int>(i));
    }
    for (std::uint32_t i = 0; i < model->jointCount; ++i) {
        Joint& dst = joints[i];
        std::memcpy(&dst, &model->jointTable[i], sizeof(Joint));
        SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(dst.name));
        dst.constraint = static_cast<int>(i);
    }

    SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_SETCURSEL, 0, 0);
    SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_SETCURSEL, 0, 0);
    SendMessageA(GetDlgItem(hDlg, kBodyACombo), CB_SETCURSEL, 0, 0);
    SendMessageA(GetDlgItem(hDlg, kBodyBCombo), CB_SETCURSEL, 0, 0);

    SendMessageA(GetDlgItem(hDlg, kBoneCombo), CB_RESETCONTENT, 0, 0);
    if (app->state.englishUI != 0) {
        SendMessageA(GetDlgItem(hDlg, kBoneCombo), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("none"));
    } else {
        SendMessageW(GetDlgItem(hDlg, kBoneCombo), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(L"\x306A\x3057"));
    }
    mdl::BoneRecord* bones = model->boneTable;
    for (std::uint32_t b = 0; b < model->boneCount; ++b) {
        const char* name =
            app->state.englishUI != 0 ? bones[b].nameEn : bones[b].name;
        SendMessageA(GetDlgItem(hDlg, kBoneCombo), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(name));
        SendMessageA(GetDlgItem(hDlg, kPivotBoneCombo), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(name));
    }

    SendMessageA(GetDlgItem(hDlg, kGroupCombo), CB_RESETCONTENT, 0, 0);
    char text[8];
    for (int n = 1; n <= 15; ++n) {
        sprintf_s(text, sizeof text, "%d", n);
        SendMessageA(GetDlgItem(hDlg, kGroupCombo), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(text));
    }
    SendMessageA(GetDlgItem(hDlg, kGroupCombo), CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(
                     app->state.englishUI != 0 ? "16(F)" : kGroup16Jp));

    for (std::uint32_t b = 0; b < model->boneCount; ++b) {
        bones[b].trans[0] = bones[b].trans[1] = bones[b].trans[2] = 0.0f;
        bones[b].rotQuat[0] = bones[b].rotQuat[1] = bones[b].rotQuat[2] = 0.0f;
        bones[b].rotQuat[3] = 1.0f;
    }

    if (model->rigidCount != 0) {
        ApplyBodyRecordToEdits(hDlg, 0);
        app->state.selectedRigidIndex = 0;
    } else {
        ApplyBodyRecordToEdits(hDlg, -1);
        app->state.selectedRigidIndex = -1;
    }
    if (model->jointCount != 0) {
        ApplyJointRecordToEdits(hDlg, 0);
        app->state.selectedJointIndex = 0;
    } else {
        ApplyJointRecordToEdits(hDlg, -1);
        app->state.selectedJointIndex = -1;
    }
}

void CollectBodyEdits(MMDApp* app) {  
    if (app->state.selectedRigidIndex == -1)
        return;
    Rigid& rb = EditBodies()[app->state.selectedRigidIndex];
    HWND hDlg = app->state.frameCopyDialog;
    char text[256];

    GetWindowTextA(GetDlgItem(hDlg, kBodyNameEdit), text, 100);
    strcpy_s(rb.name, sizeof rb.name, text);

    ReadEditFloatClamped(GetDlgItem(hDlg, kSizeXEdit), rb.size[0]);
    ReadEditFloatClamped(GetDlgItem(hDlg, kSizeYEdit), rb.size[1]);
    ReadEditFloatClamped(GetDlgItem(hDlg, kSizeZEdit), rb.size[2]);
    ReadEditFloat(GetDlgItem(hDlg, kPosXEdit), rb.position[0]);
    ReadEditFloat(GetDlgItem(hDlg, kPosYEdit), rb.position[1]);
    ReadEditFloat(GetDlgItem(hDlg, kPosZEdit), rb.position[2]);
    ReadEditDegrees(GetDlgItem(hDlg, kRotXEdit), rb.rotation[0]);
    ReadEditDegrees(GetDlgItem(hDlg, kRotYEdit), rb.rotation[1]);
    ReadEditDegrees(GetDlgItem(hDlg, kRotZEdit), rb.rotation[2]);
    ReadEditFloat(GetDlgItem(hDlg, kMassEdit), rb.mass);
    ReadEditFloat(GetDlgItem(hDlg, kLinDampEdit), rb.linearDamping);
    ReadEditFloat(GetDlgItem(hDlg, kAngDampEdit), rb.angularDamping);
    ReadEditFloat(GetDlgItem(hDlg, kRestitutionEdit), rb.restitution);
    ReadEditFloat(GetDlgItem(hDlg, kFrictionEdit), rb.friction);

    GetWindowTextA(GetDlgItem(hDlg, kGroupTextEdit), text, 100);
    std::uint16_t mask = 0;
    char* cursor = text;
    for (char* space = std::strstr(text, " "); space != nullptr;
         space = std::strstr(space + 1, " ")) {
        *space = '\0';
        const int bit = std::atoi(cursor) - 1;
        if (bit >= 0 && bit <= 15)
            mask |= static_cast<std::uint16_t>(1 << bit);
        cursor = space + 1;
    }
    const int bit = std::atoi(cursor) - 1;
    if (bit >= 0 && bit <= 15)
        mask |= static_cast<std::uint16_t>(1 << bit);
    rb.noCollapse = mask;
}

void AddRigidBody(HWND hDlg) {  
    MMDApp* app = g_Block;
    Rigid* bodies = EditBodies();

    int slot = 0;
    while (slot < kMaxEditRecords && BodyComboIndex(bodies[slot]) >= 0)
        ++slot;

    Rigid& rb = bodies[slot];
    sprintf_s(rb.name, sizeof rb.name, "BODY_%d", slot);
    const LRESULT comboIndex = SendMessageA(
        GetDlgItem(hDlg, kBodyListCombo), CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(rb.name));
    SendMessageA(GetDlgItem(hDlg, kBodyACombo), CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(rb.name));
    SendMessageA(GetDlgItem(hDlg, kBodyBCombo), CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(rb.name));
    SetBodyComboIndex(rb, static_cast<int>(comboIndex));
    SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_SETCURSEL, comboIndex, 0);

    rb.boneIndex = 0;
    rb.mode = 0;
    rb.kinematicFlag = 0;
    rb.group = 0;
    rb.mass = 1.0f;
    rb.linearDamping = 0.5f;
    rb.angularDamping = 0.5f;
    rb.restitution = 0.0f;
    rb.friction = 0.5f;
    rb.shape = 0;
    rb.position[0] = rb.position[1] = rb.position[2] = 0.0f;
    rb.rotation[0] = rb.rotation[1] = rb.rotation[2] = 0.0f;
    rb.size[0] = rb.size[1] = rb.size[2] = 2.0f;

    app->state.selectedRigidIndex = slot;
    ApplyBodyRecordToEdits(hDlg, slot);
}

void CollectJointEdits(MMDApp* app) {  
    if (app->state.selectedJointIndex == -1)
        return;
    Joint& jt = EditJoints()[app->state.selectedJointIndex];
    HWND hDlg = app->state.frameCopyDialog;
    char text[256];

    GetWindowTextA(GetDlgItem(hDlg, kJointNameEdit), text, 100);
    strcpy_s(jt.name, sizeof jt.name, text);

    ReadEditFloat(GetDlgItem(hDlg, kJointPosXEdit), jt.position[0]);
    ReadEditFloat(GetDlgItem(hDlg, kJointPosYEdit), jt.position[1]);
    ReadEditFloat(GetDlgItem(hDlg, kJointPosZEdit), jt.position[2]);
    ReadEditDegrees(GetDlgItem(hDlg, kJointRotXEdit), jt.rotation[0]);
    ReadEditDegrees(GetDlgItem(hDlg, kJointRotYEdit), jt.rotation[1]);
    ReadEditDegrees(GetDlgItem(hDlg, kJointRotZEdit), jt.rotation[2]);
    ReadEditFloat(GetDlgItem(hDlg, kLinUpperXEdit), jt.limits[0]);
    ReadEditFloat(GetDlgItem(hDlg, kLinUpperYEdit), jt.limits[1]);
    ReadEditFloat(GetDlgItem(hDlg, kLinUpperZEdit), jt.limits[2]);
    ReadEditFloat(GetDlgItem(hDlg, kLinLowerXEdit), jt.limits[3]);
    ReadEditFloat(GetDlgItem(hDlg, kLinLowerYEdit), jt.limits[4]);
    ReadEditFloat(GetDlgItem(hDlg, kLinLowerZEdit), jt.limits[5]);

    ReadEditDegrees(GetDlgItem(hDlg, kAngUpperXEdit), jt.limits[6]);
    {   
        GetWindowTextA(GetDlgItem(hDlg, kAngUpperYEdit), text, 10);
        float value = static_cast<float>(atof(text));
        value = CheckYAngleLimit(hDlg, GetDlgItem(hDlg, kAngUpperYEdit), value);
        jt.limits[7] = value / 180.0f * kPiShort;
    }
    ReadEditDegrees(GetDlgItem(hDlg, kAngUpperZEdit), jt.limits[8]);
    ReadEditDegrees(GetDlgItem(hDlg, kAngLowerXEdit), jt.limits[9]);
    {   
        GetWindowTextA(GetDlgItem(hDlg, kAngLowerYEdit), text, 10);
        float value = static_cast<float>(atof(text));
        value = CheckYAngleLimit(hDlg, GetDlgItem(hDlg, kAngLowerYEdit), value);
        jt.limits[10] = value / 180.0f * kPiShort;
    }
    ReadEditDegrees(GetDlgItem(hDlg, kAngLowerZEdit), jt.limits[11]);

    ReadEditFloat(GetDlgItem(hDlg, kSpringLinXEdit), jt.springs[0]);
    ReadEditFloat(GetDlgItem(hDlg, kSpringLinYEdit), jt.springs[1]);
    ReadEditFloat(GetDlgItem(hDlg, kSpringLinZEdit), jt.springs[2]);
    ReadEditFloat(GetDlgItem(hDlg, kSpringAngXEdit), jt.springs[3]);
    ReadEditFloat(GetDlgItem(hDlg, kSpringAngYEdit), jt.springs[4]);
    ReadEditFloat(GetDlgItem(hDlg, kSpringAngZEdit), jt.springs[5]);
}

void AddJoint(HWND hDlg) {  
    MMDApp* app = g_Block;
    Rigid* bodies = EditBodies();
    Joint* joints = EditJoints();

    int liveBodies = 0;
    for (int i = 0; i < kMaxEditRecords; ++i)
        if (BodyComboIndex(bodies[i]) >= 0)
            ++liveBodies;
    if (liveBodies == 0) {
        if (app->state.englishUI != 0) {
            MessageBoxA(hDlg, "There is no Body.\nPlease add body first.",
                        "add joint", 0);
        } else {
            MessageBoxA(hDlg, kMsgNoBodyJp, kCaptionAddJointJp, 0);
        }
        return;
    }

    int slot = 0;
    while (slot < kMaxEditRecords && joints[slot].constraint >= 0)
        ++slot;

    Joint& jt = joints[slot];
    sprintf_s(jt.name, sizeof jt.name, "JOINT_%d", slot);
    const LRESULT comboIndex = SendMessageA(
        GetDlgItem(hDlg, kJointListCombo), CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(jt.name));
    jt.constraint = static_cast<int>(comboIndex);
    SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_SETCURSEL, comboIndex, 0);

    jt.rigidA = 0;
    jt.rigidB = 0;
    jt.position[0] = jt.position[1] = jt.position[2] = 0.0f;
    jt.rotation[0] = jt.rotation[1] = jt.rotation[2] = 0.0f;
    for (float& limit : jt.limits)
        limit = 0.0f;
    for (float& spring : jt.springs)
        spring = 0.0f;

    app->state.selectedJointIndex = slot;
    ApplyJointRecordToEdits(hDlg, slot);
}

void UpdateShapeControls(HWND hDlg, int shape, int idx) {  
    MMDApp* app = g_Block;
    const bool english = app->state.englishUI != 0;
    Rigid* bodies = EditBodies();
    char text[256];

    if (shape == 0) {          
        CheckRadioButton(hDlg, kShapeSphereRadio, kShapeCapsuleRadio,
                         kShapeSphereRadio);
        SetWindowTextA(GetDlgItem(hDlg, kSizeXLabel),
                       english ? "radius" : kLabelRadiusJp);
        ShowWindow(GetDlgItem(hDlg, kSizeYLabel), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, kSizeYEdit), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, kSizeZLabel), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, kSizeZEdit), SW_HIDE);
        if (idx != -1) {
            sprintf_s(text, sizeof text, "%3.2f", bodies[idx].size[0]);
            SetWindowTextA(GetDlgItem(hDlg, kSizeXEdit), text);
        } else {
            SetWindowTextA(GetDlgItem(hDlg, kSizeXEdit), "");
        }
        return;
    }
    if (shape == 1) {          
        CheckRadioButton(hDlg, kShapeSphereRadio, kShapeCapsuleRadio,
                         kShapeBoxRadio);
        SetWindowTextA(GetDlgItem(hDlg, kSizeXLabel),
                       english ? "width" : kLabelWidthJp);
        SetWindowTextA(GetDlgItem(hDlg, kSizeYLabel),
                       english ? "height" : kLabelHeightJp);
        SetWindowTextA(GetDlgItem(hDlg, kSizeZLabel),
                       english ? "depth" : kLabelDepthJp);
        ShowWindow(GetDlgItem(hDlg, kSizeYLabel), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, kSizeYEdit), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, kSizeZLabel), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, kSizeZEdit), SW_SHOW);
        if (idx >= 0 && idx < kMaxEditRecords) {
            sprintf_s(text, sizeof text, "%3.2f", bodies[idx].size[0]);
            SetWindowTextA(GetDlgItem(hDlg, kSizeXEdit), text);
            sprintf_s(text, sizeof text, "%3.2f", bodies[idx].size[1]);
            SetWindowTextA(GetDlgItem(hDlg, kSizeYEdit), text);
            sprintf_s(text, sizeof text, "%3.2f", bodies[idx].size[2]);
            SetWindowTextA(GetDlgItem(hDlg, kSizeZEdit), text);
        }
        return;
    }
    CheckRadioButton(hDlg, kShapeSphereRadio, kShapeCapsuleRadio,
                     kShapeCapsuleRadio);
    SetWindowTextA(GetDlgItem(hDlg, kSizeXLabel),
                   english ? "radius" : kLabelRadiusJp);
    SetWindowTextA(GetDlgItem(hDlg, kSizeYLabel),
                   english ? "height" : kLabelHeightJp);
    ShowWindow(GetDlgItem(hDlg, kSizeYLabel), SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, kSizeYEdit), SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, kSizeZLabel), SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, kSizeZEdit), SW_HIDE);
    if (idx >= 0 && idx < kMaxEditRecords) {
        sprintf_s(text, sizeof text, "%3.2f", bodies[idx].size[0]);
        SetWindowTextA(GetDlgItem(hDlg, kSizeXEdit), text);
        sprintf_s(text, sizeof text, "%3.2f", bodies[idx].size[1]);
        SetWindowTextA(GetDlgItem(hDlg, kSizeYEdit), text);
    }
}

void FlipPhysicsDialogPage(HWND hDlg, int bodyPage) {  
    if (bodyPage != 0) {
        for (int id = kJointPageFirst; id <= kJointPageLast; ++id)
            ShowWindow(GetDlgItem(hDlg, id), SW_HIDE);
        for (int id = kBodyPageFirst; id <= kBodyPageLast; ++id)
            ShowWindow(GetDlgItem(hDlg, id), SW_SHOW);
        s_jointPageVisible = 0;
    } else {
        for (int id = kBodyPageFirst; id <= kBodyPageLast; ++id)
            ShowWindow(GetDlgItem(hDlg, id), SW_HIDE);
        for (int id = kJointPageFirst; id <= kJointPageLast; ++id)
            ShowWindow(GetDlgItem(hDlg, id), SW_SHOW);
        s_jointPageVisible = 1;
    }
}

void PickPivotBone(HWND hDlg) {  
    MMDApp* app = g_Block;
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());
    const int sel = static_cast<int>(
        SendMessageA(GetDlgItem(hDlg, kPivotBoneCombo), CB_GETCURSEL, 0, 0));
    Joint& jt = EditJoints()[app->state.selectedJointIndex];
    const mdl::BoneRecord& bone = model->boneTable[sel];
    jt.position[0] = bone.position[0];
    jt.position[1] = bone.position[1];
    jt.position[2] = bone.position[2];
    char text[256];
    for (int axis = 0; axis < 3; ++axis) {
        sprintf_s(text, sizeof text, "%5.4f", jt.position[axis]);
        SetWindowTextA(GetDlgItem(hDlg, kJointPosXEdit + axis), text);
    }
}

void ApplyBodyRecordToEdits(HWND hDlg, int idx) {  
    MMDApp* app = g_Block;
    Rigid* bodies = EditBodies();
    char text[256];

    if (idx == -1) {
        for (int id = kBodyNameEdit; id <= kBoneAlignCheckbox; ++id)
            EnableWindow(GetDlgItem(hDlg, id), FALSE);
        SetWindowTextA(GetDlgItem(hDlg, kBodyNameEdit), "");
        SendMessageA(GetDlgItem(hDlg, kBoneCombo), CB_SETCURSEL, 1, 0);
        SendMessageA(GetDlgItem(hDlg, kGroupCombo), CB_SETCURSEL, 0, 0);
        UpdateShapeControls(hDlg, 0, -1);
        for (int id = kPosXEdit; id <= kFrictionEdit; ++id)
            SetWindowTextA(GetDlgItem(hDlg, id), "");
        CheckRadioButton(hDlg, kPhysicalRadio, kBoneFollowRadio,
                         kBoneFollowRadio);
        return;
    }

    const Rigid& rb = bodies[idx];
    for (int id = kBodyNameEdit; id <= kBoneAlignCheckbox; ++id)
        EnableWindow(GetDlgItem(hDlg, id), TRUE);
    SetWindowTextA(GetDlgItem(hDlg, kBodyNameEdit), rb.name);
    SendMessageA(GetDlgItem(hDlg, kBoneCombo), CB_SETCURSEL, rb.boneIndex + 1, 0);
    SendMessageA(GetDlgItem(hDlg, kGroupCombo), CB_SETCURSEL, rb.group, 0);
    UpdateShapeControls(hDlg, rb.shape, idx);

    for (int axis = 0; axis < 3; ++axis) {
        sprintf_s(text, sizeof text, "%3.2f", rb.position[axis]);
        SetWindowTextA(GetDlgItem(hDlg, kPosXEdit + axis), text);
        sprintf_s(text, sizeof text, "%3.2f",
                  rb.rotation[axis] / kPiShort * 180.0f);
        SetWindowTextA(GetDlgItem(hDlg, kRotXEdit + axis), text);
    }
    sprintf_s(text, sizeof text, "%4.3f", rb.mass);
    SetWindowTextA(GetDlgItem(hDlg, kMassEdit), text);
    sprintf_s(text, sizeof text, "%4.3f", rb.linearDamping);
    SetWindowTextA(GetDlgItem(hDlg, kLinDampEdit), text);
    sprintf_s(text, sizeof text, "%4.3f", rb.angularDamping);
    SetWindowTextA(GetDlgItem(hDlg, kAngDampEdit), text);
    sprintf_s(text, sizeof text, "%4.3f", rb.restitution);
    SetWindowTextA(GetDlgItem(hDlg, kRestitutionEdit), text);
    sprintf_s(text, sizeof text, "%4.3f", rb.friction);
    SetWindowTextA(GetDlgItem(hDlg, kFrictionEdit), text);

    text[0] = '\0';
    char group[16];
    for (int bit = 0; bit < 16; ++bit) {
        sprintf_s(group, sizeof group, "%d ", bit + 1);
        if ((rb.noCollapse & (1u << bit)) != 0)
            strcat_s(text, sizeof text, group);
    }
    SetWindowTextA(GetDlgItem(hDlg, kGroupTextEdit), text);

    if (rb.mode != 0) {
        CheckRadioButton(hDlg, kPhysicalRadio, kBoneFollowRadio,
                         kPhysicalRadio);
        EnableWindow(GetDlgItem(hDlg, kBoneAlignCheckbox), TRUE);
    } else {
        CheckRadioButton(hDlg, kPhysicalRadio, kBoneFollowRadio,
                         kBoneFollowRadio);
        SendMessageA(GetDlgItem(hDlg, kBoneAlignCheckbox),
                     BM_SETCHECK, 0, 0);
        EnableWindow(GetDlgItem(hDlg, kBoneAlignCheckbox), FALSE);
    }
    SendMessageA(GetDlgItem(hDlg, kBoneAlignCheckbox), BM_SETCHECK,
                 rb.mode == 2 ? 1 : 0, 0);
}

void ApplyJointRecordToEdits(HWND hDlg, int idx) {  
    MMDApp* app = g_Block;
    Rigid* bodies = EditBodies();
    Joint* joints = EditJoints();
    char text[256];

    if (idx == -1) {
        for (int id = kDeleteJointButton; id <= kAngUpperZEdit; ++id)
            EnableWindow(GetDlgItem(hDlg, id), FALSE);
        SendMessageA(GetDlgItem(hDlg, kBodyACombo), CB_SETCURSEL, 0, 0);
        SendMessageA(GetDlgItem(hDlg, kBodyBCombo), CB_SETCURSEL, 0, 0);
        SendMessageA(GetDlgItem(hDlg, kPivotBoneCombo), CB_SETCURSEL, -1, 0);
        for (int id = kJointPosXEdit; id <= kAngUpperZEdit; ++id)
            SetWindowTextA(GetDlgItem(hDlg, id), "");
        return;
    }

    const Joint& jt = joints[idx];
    for (int id = kDeleteJointButton; id <= kAngUpperZEdit; ++id)
        EnableWindow(GetDlgItem(hDlg, id), TRUE);
    SetWindowTextA(GetDlgItem(hDlg, kJointNameEdit), jt.name);
    SendMessageA(GetDlgItem(hDlg, kBodyACombo), CB_SETCURSEL,
                 BodyComboIndex(bodies[jt.rigidA]), 0);
    SendMessageA(GetDlgItem(hDlg, kBodyBCombo), CB_SETCURSEL,
                 BodyComboIndex(bodies[jt.rigidB]), 0);

    for (int axis = 0; axis < 3; ++axis) {
        sprintf_s(text, sizeof text, "%5.4f", jt.position[axis]);
        SetWindowTextA(GetDlgItem(hDlg, kJointPosXEdit + axis), text);
        sprintf_s(text, sizeof text, "%3.2f",
                  jt.rotation[axis] / kPiShort * 180.0f);
        SetWindowTextA(GetDlgItem(hDlg, kJointRotXEdit + axis), text);
    }
    const int kLinearUpperEdits[3] = {kLinUpperXEdit, kLinUpperYEdit,
                                      kLinUpperZEdit};
    const int kLinearLowerEdits[3] = {kLinLowerXEdit, kLinLowerYEdit,
                                      kLinLowerZEdit};
    for (int axis = 0; axis < 3; ++axis) {
        sprintf_s(text, sizeof text, "%3.2f", jt.limits[axis]);
        SetWindowTextA(GetDlgItem(hDlg, kLinearUpperEdits[axis]), text);
        sprintf_s(text, sizeof text, "%3.2f", jt.limits[3 + axis]);
        SetWindowTextA(GetDlgItem(hDlg, kLinearLowerEdits[axis]), text);
    }
    const int kAngUpperEdits[3] = {kAngUpperXEdit, kAngUpperYEdit,
                                   kAngUpperZEdit};
    const int kAngLowerEdits[3] = {kAngLowerXEdit, kAngLowerYEdit,
                                   kAngLowerZEdit};
    for (int axis = 0; axis < 3; ++axis) {
        sprintf_s(text, sizeof text, "%3.2f",
                  jt.limits[6 + axis] / kPiShort * 180.0f);
        SetWindowTextA(GetDlgItem(hDlg, kAngUpperEdits[axis]), text);
        sprintf_s(text, sizeof text, "%3.2f",
                  jt.limits[9 + axis] / kPiShort * 180.0f);
        SetWindowTextA(GetDlgItem(hDlg, kAngLowerEdits[axis]), text);
    }
    for (int axis = 0; axis < 3; ++axis) {
        sprintf_s(text, sizeof text, "%3.2f", jt.springs[axis]);
        SetWindowTextA(GetDlgItem(hDlg, kSpringLinXEdit + axis), text);
        sprintf_s(text, sizeof text, "%3.2f", jt.springs[3 + axis]);
        SetWindowTextA(GetDlgItem(hDlg, kSpringAngXEdit + axis), text);
    }
}

void CommitPhysicsEdits(HWND hDlg) {  
    (void)hDlg;
    MMDApp* app = g_Block;
    mdl::ModelRecord* model = mdl::Mdl(app->SelectedModel());
    PhysicsScene* scene = app->Physics();
    d3dx::Api& d3dx = d3dx::Get();
    Rigid* bodies = EditBodies();
    Joint* joints = EditJoints();
    using d3dx::D3DXMATRIXF;

    for (std::uint32_t i = 0; i < model->jointCount; ++i)
        RemovePhysJointByUid(scene, model->jointTable[i].constraint);
    ::operator delete(model->jointTable);
    model->jointTable = nullptr;
    for (std::uint32_t i = 0; i < model->rigidCount; ++i)
        RemovePhysRigidBody(
            scene, static_cast<btRigidBody*>(model->rigidTable[i].body));
    ::operator delete(model->rigidTable);
    model->rigidTable = nullptr;

    int liveBodies = 0;
    for (int i = 0; i < kMaxEditRecords; ++i)
        if (BodyComboIndex(bodies[i]) >= 0)
            ++liveBodies;
    model->rigidCount = static_cast<std::uint32_t>(liveBodies);
    model->rigidTable = static_cast<Rigid*>(::operator new(
        sizeof(Rigid) * (liveBodies > 0 ? liveBodies : 1)));
    std::memset(model->rigidTable, 0,
                sizeof(Rigid) * static_cast<std::size_t>(liveBodies));

    mdl::BoneRecord* bones = model->boneTable;
    for (std::uint32_t b = 0; b < model->boneCount; ++b)
        bones[b].hasRigidBody = 0;

    int bodyIndex = 0;
    for (int i = 0; i < kMaxEditRecords; ++i) {
        if (BodyComboIndex(bodies[i]) < 0)
            continue;
        Rigid& ed = bodies[i];
        ed.noCollapse = static_cast<std::uint16_t>(~ed.noCollapse);
        Rigid& dst = model->rigidTable[bodyIndex];
        std::memcpy(&dst, &ed, sizeof(Rigid));

        D3DXMATRIXF mat, tmp;
        d3dx.rotZ(&mat, ed.rotation[2]);
        d3dx.rotX(&tmp, ed.rotation[0]);
        d3dx.multiply(&mat, &mat, &tmp);
        d3dx.rotY(&tmp, ed.rotation[1]);
        d3dx.multiply(&mat, &mat, &tmp);
        d3dx.translation(&tmp, ed.position[0], ed.position[1],
                         ed.position[2]);
        d3dx.multiply(&mat, &mat, &tmp);
        const mdl::BoneRecord& linkedBone =
            bones[ed.boneIndex < 0 ? 0 : ed.boneIndex];
        d3dx.translation(&tmp, linkedBone.position[0],
                         linkedBone.position[1], linkedBone.position[2]);
        d3dx.multiply(&mat, &mat, &tmp);
        D3DXMATRIXF boneInit;
        std::memcpy(&boneInit, linkedBone.matInit, sizeof(boneInit));
        d3dx.multiply(&mat, &mat, &boneInit);

        void* bodyOut[2] = {nullptr, nullptr};
        CreateRigidBody(scene, bodyOut, ed.shape, ed.size[0], ed.size[1],
                        ed.size[2], &mat.m[0][0], ed.mode, ed.mass,
                        ed.linearDamping, ed.angularDamping, ed.restitution,
                        ed.friction, static_cast<char>(ed.group),
                        ed.noCollapse);
        dst.keyData = bodyOut[0];
        dst.body = bodyOut[1];
        SetBodyComboIndex(ed, bodyIndex);
        if (ed.mode > 0 && ed.boneIndex >= 0)
            bones[ed.boneIndex].hasRigidBody = 1;

        D3DXMATRIXF inv, t2;
        d3dx.translation(&inv, -linkedBone.position[0],
                         -linkedBone.position[1], -linkedBone.position[2]);
        d3dx.translation(&t2, -ed.position[0], -ed.position[1],
                         -ed.position[2]);
        d3dx.multiply(&inv, &inv, &t2);
        d3dx.rotY(&t2, -ed.rotation[1]);
        d3dx.multiply(&inv, &inv, &t2);
        d3dx.rotX(&t2, -ed.rotation[0]);
        d3dx.multiply(&inv, &inv, &t2);
        d3dx.rotZ(&t2, -ed.rotation[2]);
        d3dx.multiply(&inv, &inv, &t2);
        std::memcpy(dst.invTransform, &inv, sizeof(inv));

        ++bodyIndex;
    }

    int liveJoints = 0;
    for (int i = 0; i < kMaxEditRecords; ++i)
        if (joints[i].constraint >= 0)
            ++liveJoints;
    model->jointCount = static_cast<std::uint32_t>(liveJoints);
    model->jointTable = static_cast<Joint*>(::operator new(
        sizeof(Joint) * (liveJoints > 0 ? liveJoints : 1)));
    std::memset(model->jointTable, 0,
                sizeof(Joint) * static_cast<std::size_t>(liveJoints));

    int jointIndex = 0;
    for (int i = 0; i < kMaxEditRecords; ++i) {
        if (joints[i].constraint < 0)
            continue;
        Joint& ed = joints[i];
        ed.rigidA = BodyComboIndex(bodies[ed.rigidA]);
        ed.rigidB = BodyComboIndex(bodies[ed.rigidB]);
        Joint& dst = model->jointTable[jointIndex];
        std::memcpy(&dst, &ed, sizeof(Joint));

        const Rigid& rbA = model->rigidTable[ed.rigidA];
        const Rigid& rbB = model->rigidTable[ed.rigidB];

        float pivotA[3], pivotB[3];
        TransformPoint(pivotA, ed.position, rbA.invTransform);
        TransformPoint(pivotB, ed.position, rbB.invTransform);
        const float distA = std::sqrt(pivotA[0] * pivotA[0] +
                                      pivotA[1] * pivotA[1] +
                                      pivotA[2] * pivotA[2]);
        const float distB = std::sqrt(pivotB[0] * pivotB[0] +
                                      pivotB[1] * pivotB[1] +
                                      pivotB[2] * pivotB[2]);
        dst.radiusBound = distA + distB;

        float maxAbs[3];
        for (int axis = 0; axis < 3; ++axis) {
            float upper = std::fabs(ed.limits[axis]);
            const float lower = std::fabs(ed.limits[3 + axis]);
            if (lower > upper)
                upper = lower;
            maxAbs[axis] = upper;
        }
        dst.radiusBound += std::sqrt(maxAbs[0] * maxAbs[0] +
                                     maxAbs[1] * maxAbs[1] +
                                     maxAbs[2] * maxAbs[2]);

        float qa[4], qb[4];
        for (int side = 0; side < 2; ++side) {
            const Rigid& rb = side == 0 ? rbA : rbB;
            D3DXMATRIXF rot, r2;
            d3dx.rotZ(&rot, ed.rotation[2]);
            d3dx.rotX(&r2, ed.rotation[0]);
            d3dx.multiply(&rot, &rot, &r2);
            d3dx.rotY(&r2, ed.rotation[1]);
            d3dx.multiply(&rot, &rot, &r2);
            d3dx.rotY(&r2, -rb.rotation[1]);
            d3dx.multiply(&rot, &rot, &r2);
            d3dx.rotX(&r2, -rb.rotation[0]);
            d3dx.multiply(&rot, &rot, &r2);
            d3dx.rotZ(&r2, -rb.rotation[2]);
            d3dx.multiply(&rot, &rot, &r2);
            if (side == 0)
                d3dx.quatFromMatrix(qa, &rot);
            else
                d3dx.quatFromMatrix(qb, &rot);
        }

        dst.constraint = CreatePhysJoint(
            scene, rbA.body, rbB.body, pivotA[0], pivotA[1], pivotA[2],
            qa[0], qa[1], qa[2], qa[3], pivotB[0], pivotB[1], pivotB[2],
            qb[0], qb[1], qb[2], qb[3], ed.limits[0], ed.limits[1],
            ed.limits[2], ed.limits[3], ed.limits[4], ed.limits[5],
            ed.limits[6], ed.limits[7], ed.limits[8], ed.limits[9],
            ed.limits[10], ed.limits[11], ed.springs[0], ed.springs[1],
            ed.springs[2], ed.springs[3], ed.springs[4], ed.springs[5]);
        ++jointIndex;
    }
}

INT_PTR CALLBACK PhysicsModelDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {  
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        if (app->state.floatingWindow != 0)
            SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOSIZE | SWP_NOMOVE);  
        HWND nameEdit = GetDlgItem(hDlg, kBodyNameEdit);
        app->FrameCopyEditProc() = reinterpret_cast<WNDPROC>(
            GetWindowLongPtrA(nameEdit, GWLP_WNDPROC));
        SetWindowLongPtrA(nameEdit, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(PhysicsEditSubclassProc));
        for (int id = 709; id <= 723; ++id)
            SetWindowLongPtrA(GetDlgItem(hDlg, id), GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(PhysicsEditSubclassProc));
        for (int id = kJointPosXEdit; id <= kAngUpperZEdit; ++id)
            SetWindowLongPtrA(GetDlgItem(hDlg, id), GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(PhysicsEditSubclassProc));
        SetWindowLongPtrA(GetDlgItem(hDlg, kJointNameEdit), GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(PhysicsEditSubclassProc));
        InitPhysicsModelDialog(hDlg);
        return 0;
    }
    if (msg != WM_COMMAND)
        return 0;

    app->state.timeNowHigh = 1;

    const HWND hCtrl = reinterpret_cast<HWND>(lParam);
    Rigid* bodies = EditBodies();
    Joint* joints = EditJoints();

    switch (LOWORD(wParam)) {
    case 0x2AD:  
        CollectBodyEdits(app);
        AddRigidBody(hDlg);
        return 0;
    case 0x2DF:  
        CollectJointEdits(app);
        AddJoint(hDlg);
        return 0;
    case 0x2C2: {  
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_GETCURSEL, 0, 0));
        SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_DELETESTRING, sel, 0);
        SendMessageA(GetDlgItem(hDlg, kBodyACombo), CB_DELETESTRING, sel, 0);
        SendMessageA(GetDlgItem(hDlg, kBodyBCombo), CB_DELETESTRING, sel, 0);
        for (int i = 0; i < kMaxEditRecords; ++i) {
            if (joints[i].constraint >= 0 &&
                (joints[i].rigidA == app->state.selectedRigidIndex ||
                 joints[i].rigidB == app->state.selectedRigidIndex)) {
                SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_DELETESTRING,
                             joints[i].constraint, 0);
                for (int m = 0; m < kMaxEditRecords; ++m)
                    if (joints[m].constraint > joints[i].constraint)
                        --joints[m].constraint;
                joints[i].constraint = -1;
            }
        }
        SetBodyComboIndex(bodies[app->state.selectedRigidIndex], -1);
        for (int n = 0; n < kMaxEditRecords; ++n)
            if (BodyComboIndex(bodies[n]) > sel)
                SetBodyComboIndex(bodies[n], BodyComboIndex(bodies[n]) - 1);
        if (SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_GETCOUNT, 0, 0) != 0) {
            if (BodyComboIndex(bodies[0]) == 0)
                app->state.selectedRigidIndex = 0;
        } else {
            app->state.selectedRigidIndex = -1;
        }
        SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_SETCURSEL, 0, 0);
        ApplyBodyRecordToEdits(hDlg, app->state.selectedRigidIndex);
        if (SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_GETCOUNT, 0, 0) != 0) {
            if (joints[0].constraint == 0)
                app->state.selectedJointIndex = 0;
        } else {
            app->state.selectedJointIndex = -1;
        }
        SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_SETCURSEL, 0, 0);
        ApplyJointRecordToEdits(hDlg, app->state.selectedJointIndex);
        return 0;
    }
    case 0x2E1: {  
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_GETCURSEL, 0, 0));
        SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_DELETESTRING, sel, 0);
        joints[app->state.selectedJointIndex].constraint = -1;
        for (int i = 0; i < kMaxEditRecords; ++i)
            if (joints[i].constraint > sel)
                --joints[i].constraint;
        if (SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_GETCOUNT, 0, 0) != 0) {
            if (joints[0].constraint == 0)
                app->state.selectedJointIndex = 0;
        } else {
            app->state.selectedJointIndex = -1;
        }
        SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_SETCURSEL, 0, 0);
        ApplyJointRecordToEdits(hDlg, app->state.selectedJointIndex);
        return 0;
    }
    case 0x2D4:  
        CollectBodyEdits(app);
        std::memcpy(&s_bodyCopyScratch, &bodies[app->state.selectedRigidIndex],
                    sizeof(Rigid));
        return 0;
    case 0x2D5: {  
        Rigid& rb = bodies[app->state.selectedRigidIndex];
        const Rigid& src = s_bodyCopyScratch;
        rb.boneIndex = src.boneIndex;
        rb.mode = src.mode;
        rb.kinematicFlag = src.kinematicFlag;
        rb.mass = src.mass;
        rb.linearDamping = src.linearDamping;
        rb.angularDamping = src.angularDamping;
        for (int axis = 0; axis < 3; ++axis) {
            rb.position[axis] = src.position[axis];
            rb.rotation[axis] = src.rotation[axis];
            rb.size[axis] = src.size[axis];
        }
        rb.shape = src.shape;
        ApplyBodyRecordToEdits(hDlg, app->state.selectedRigidIndex);
        return 0;
    }
    case 0x2E2:  
        CollectJointEdits(app);
        std::memcpy(&s_jointCopyScratch, &joints[app->state.selectedJointIndex],
                    sizeof(Joint));
        return 0;
    case 0x2E3: {  
        Joint& jt = joints[app->state.selectedJointIndex];
        const Joint& src = s_jointCopyScratch;
        jt.rigidA = src.rigidA;
        jt.rigidB = src.rigidB;
        jt.position[0] = src.position[0];
        jt.position[1] = src.position[1];
        jt.position[2] = src.position[2];
        jt.rotation[0] = src.rotation[0];
        jt.rotation[1] = src.rotation[1];
        jt.rotation[2] = src.rotation[2];
        for (int f = 0; f < 12; ++f)
            jt.limits[f] = src.limits[f];
        for (int f = 0; f < 6; ++f)
            jt.springs[f] = src.springs[f];
        ApplyJointRecordToEdits(hDlg, app->state.selectedJointIndex);
        return 0;
    }
    case 0x2D6:  
        bodies[app->state.selectedRigidIndex].shape = 0;
        UpdateShapeControls(hDlg, 0, app->state.selectedRigidIndex);
        return 0;
    case 0x2D7:  
        bodies[app->state.selectedRigidIndex].shape = 1;
        UpdateShapeControls(hDlg, 1, app->state.selectedRigidIndex);
        return 0;
    case 0x2D8:  
        bodies[app->state.selectedRigidIndex].shape = 2;
        UpdateShapeControls(hDlg, 2, app->state.selectedRigidIndex);
        return 0;
    case 0x2D9:  
        EnableWindow(GetDlgItem(hDlg, kBoneAlignCheckbox), TRUE);
        if (IsDlgButtonChecked(hDlg, kBoneAlignCheckbox) == 1) {
            bodies[app->state.selectedRigidIndex].mode = 2;
            bodies[app->state.selectedRigidIndex].kinematicFlag = 1;
        } else {
            bodies[app->state.selectedRigidIndex].mode = 1;
            bodies[app->state.selectedRigidIndex].kinematicFlag = 0;
        }
        return 0;
    case 0x2DA:  
        EnableWindow(GetDlgItem(hDlg, kBoneAlignCheckbox), FALSE);
        bodies[app->state.selectedRigidIndex].mode = 0;
        bodies[app->state.selectedRigidIndex].kinematicFlag = 0;
        return 0;
    case 0x2DB:  
        if (bodies[app->state.selectedRigidIndex].mode == 2) {
            bodies[app->state.selectedRigidIndex].mode = 1;
            bodies[app->state.selectedRigidIndex].kinematicFlag = 0;
        } else if (bodies[app->state.selectedRigidIndex].mode == 1) {
            bodies[app->state.selectedRigidIndex].mode = 2;
            bodies[app->state.selectedRigidIndex].kinematicFlag = 1;
        }
        return 0;
    case 0x320:  
        CollectJointEdits(app);
        FlipPhysicsDialogPage(hDlg, 1);
        return 0;
    case 0x321:  
        CollectBodyEdits(app);
        FlipPhysicsDialogPage(hDlg, 0);
        return 0;
    default:
        break;
    }

    if (HIWORD(wParam) != 1  ) {
        if (LOWORD(wParam) == 1) {
            SeekSelectedModelToCurrentFrame(app);   
            CollectBodyEdits(app);
            CollectJointEdits(app);
            CommitPhysicsEdits(hDlg);
            if (app->state.rigidScratchArray != nullptr) {
                std::free(app->state.rigidScratchArray);
                app->state.rigidScratchArray = nullptr;
            }
            if (app->state.jointScratchArray != nullptr) {
                std::free(app->state.jointScratchArray);
                app->state.jointScratchArray = nullptr;
            }
            DestroyWindow(hDlg);
            app->state.frameCopyDialog = nullptr;
            EnableWindow(GetDlgItem(app->state.hwnd, panel::kMainComboModel), TRUE);
            EnableWindow(GetDlgItem(app->state.hwnd, panel::kPlayButton), TRUE);
            if (app->state.englishUI != 0) {
                MessageBoxA(app->state.hwnd,
                            "Please preserve the edit result as a new model "
                            "by 'save enhanced model'.",
                            "enhance model", 0);
            } else {
                MessageBoxA(app->state.hwnd, kMsgEnhanceModelJp,
                            kCaptionEnhanceModelJp, 0);
            }
            app->EnhancedModelDirty() = 1;
            return 1;
        }
        if (LOWORD(wParam) == 2) {
            SeekSelectedModelToCurrentFrame(app);   
            if (app->state.rigidScratchArray != nullptr) {
                std::free(app->state.rigidScratchArray);
                app->state.rigidScratchArray = nullptr;
            }
            if (app->state.jointScratchArray != nullptr) {
                std::free(app->state.jointScratchArray);
                app->state.jointScratchArray = nullptr;
            }
            DestroyWindow(hDlg);
            app->state.frameCopyDialog = nullptr;
            EnableWindow(GetDlgItem(app->state.hwnd, panel::kMainComboModel), TRUE);
            EnableWindow(GetDlgItem(app->state.hwnd, panel::kPlayButton), TRUE);
            return 0;
        }
        return 0;
    }

    if (hCtrl == GetDlgItem(hDlg, kBodyListCombo)) {
        CollectBodyEdits(app);
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kBodyListCombo), CB_GETCURSEL, 0, 0));
        for (int idx = 0; idx < kMaxEditRecords; ++idx) {
            if (BodyComboIndex(bodies[idx]) == sel) {
                ApplyBodyRecordToEdits(hDlg, idx);
                app->state.selectedRigidIndex = idx;
            }
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, kJointListCombo)) {
        CollectJointEdits(app);
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kJointListCombo), CB_GETCURSEL, 0, 0));
        for (int idx = 0; idx < kMaxEditRecords; ++idx) {
            if (joints[idx].constraint == sel) {
                ApplyJointRecordToEdits(hDlg, idx);
                app->state.selectedJointIndex = idx;
            }
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, kBodyACombo)) {
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kBodyACombo), CB_GETCURSEL, 0, 0));
        for (int idx = 0; idx < kMaxEditRecords; ++idx) {
            if (BodyComboIndex(bodies[idx]) == sel)
                joints[app->state.selectedJointIndex].rigidA = idx;
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, kBodyBCombo)) {
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kBodyBCombo), CB_GETCURSEL, 0, 0));
        for (int idx = 0; idx < kMaxEditRecords; ++idx) {
            if (BodyComboIndex(bodies[idx]) == sel)
                joints[app->state.selectedJointIndex].rigidB = idx;
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, kBoneCombo)) {
        bodies[app->state.selectedRigidIndex].boneIndex = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, kBoneCombo), CB_GETCURSEL, 0, 0) &
            0xFFFF) - 1;
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, kGroupCombo)) {
        bodies[app->state.selectedRigidIndex].group = static_cast<std::uint8_t>(
            SendMessageA(GetDlgItem(hDlg, kGroupCombo), CB_GETCURSEL, 0, 0));
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, kPivotBoneCombo)) {
        PickPivotBone(hDlg);
        return 0;
    }
    return 0;
}

}  
