#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void PostLanguageSweep2(MMDApp* app) {
    HWND hwnd = reinterpret_cast<HWND>(app->state.floatingWindow);
    RECT client;
    int x;
    if (hwnd != nullptr) {                          
        GetClientRect(hwnd, &client);
        x = 0;
    } else {                                        
        hwnd = reinterpret_cast<HWND>(app->state.hwnd);
        GetClientRect(hwnd, &client);
        x = app->state.sidebarWidth + 9;  
    }

    const std::int32_t hideTop =
        app->state.hideTop;  
    RECT rc;
    rc.left = x + 10;
    rc.bottom = hideTop - 1;
    rc.top = hideTop - 22;
    rc.right = client.right - 450;
    InvalidateRect(hwnd, &rc, FALSE);               

    rc.top = app->state.hideBottom;  
    rc.left = x + 68;
    rc.right = x + 139;
    rc.bottom = rc.top + 30;
    InvalidateRect(hwnd, &rc, FALSE);               
}

void PostViewRefresh(MMDApp* app) {
    HWND hwnd = reinterpret_cast<HWND>(app->state.floatingWindow);
    if (hwnd == nullptr)                            
        hwnd = reinterpret_cast<HWND>(app->state.hwnd);

    char text[260];  

    if (app->state.optflag[0] != 0) {   
        sprintf_s(text, 0x100, "%1.2f",
                  static_cast<double>(app->CameraPositionX()));
        SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutPosXEdit), text);
        sprintf_s(text, 0x100, "%1.2f",
                  static_cast<double>(app->CameraPositionY()));
        SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutPosYEdit), text);
        float posz = app->CameraPositionZ();
        if (app->CameraParentModel() >= 0)
            posz = -posz;
        sprintf_s(text, 0x100, "%1.2f", static_cast<double>(posz));
        SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutPosZEdit), text);
        float ang = static_cast<float>(                       
            -static_cast<double>(app->CameraPitch()) /
            3.141592f * 180.0f);
        if (std::fabs(static_cast<double>(ang)) < 1e-7f)      
            ang = 0.0f;
        sprintf_s(text, 0x100, "%1.1f", static_cast<double>(ang));
        SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutRotXEdit), text);
        sprintf_s(text, 0x100, "%1.1f",                       
                  static_cast<double>(app->CameraYaw()) /
                      3.141592f * 180.0f);
        SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutRotYEdit), text);
        sprintf_s(text, 0x100, "%1.1f",                       
                  static_cast<double>(app->CameraRoll()) /
                      3.141592f * 180.0f);
        SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutRotZEdit), text);
        if (app->CameraParentModel() < 0) {
            float camAngle = static_cast<float>(
                -static_cast<double>(app->CameraDistance()));
            if (std::fabs(static_cast<double>(camAngle)) < 1e-7f)
                camAngle = 0.0f;
            sprintf_s(text, 0x100, "%1.2f", static_cast<double>(camAngle));
        } else {
            sprintf_s(text, 0x100, "%1.2f", 0.0);
        }
        SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutDistEdit), text);
        return;
    }

    unsigned char* model = app->SelectedModel();
    const std::int32_t boneIdx = rinmotiondance::mdl::Mdl(model)->selectedBone;
    if (boneIdx < 0) {                                     
        for (int id = 0x220; id <= 0x226; ++id)
            SetWindowTextA(GetDlgItem(hwnd, id), "-----");
        return;
    }
    rinmotiondance::mdl::BoneRecord* bone =
        &rinmotiondance::mdl::Bones(model)[boneIdx];
    const float* display;
    const float* quat;
    if (((app->PlaybackPhysicsMode() == 3) &                 
         (*reinterpret_cast<unsigned char*>(&bone->hasRigidBody) != 0)) != 0) {
        display = reinterpret_cast<const float*>(bone->ikBackup);  
        quat = reinterpret_cast<const float*>(bone->ikBackup + 3);
    } else {
        display = reinterpret_cast<const float*>(bone->trans);  
        quat = reinterpret_cast<const float*>(bone->rotQuat);
    }
    sprintf_s(text, 0x100, "%1.2f", static_cast<double>(display[0]));
    SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutPosXEdit), text);
    sprintf_s(text, 0x100, "%1.2f", static_cast<double>(display[1]));
    SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutPosYEdit), text);
    sprintf_s(text, 0x100, "%1.2f", static_cast<double>(display[2]));
    SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutPosZEdit), text);

    d3dx::D3DXMATRIXF m;  
    auto* d3dx = &d3dx::Get();
    d3dx->matrixRotationQuaternion(&m, quat);       

    const float m12 = m.m[0][1];
    const float m22 = m.m[1][1];
    const float m31 = m.m[2][0];
    const float m32 = m.m[2][1];
    const float m33 = m.m[2][2];

    float rz = static_cast<float>(std::atan2(static_cast<double>(m12),
                                             static_cast<double>(m22)));
    app->BoneRotationEditDegreesZ() = rz;               
    float rx = static_cast<float>(std::asin(-static_cast<double>(m32)));
    app->BoneRotationEditDegreesX() = rx;               
    float ry = static_cast<float>(std::atan2(static_cast<double>(m31),
                                             static_cast<double>(m33)));
    app->BoneRotationEditDegreesY() = ry;               
    float c = static_cast<float>(std::cos(static_cast<double>(rx)));

    if (std::fabs(static_cast<double>(c)) < 1e-6f) {    
        rz = static_cast<float>(static_cast<double>(rz) +
                                (m12 > 0.0f ? 3.141592f : -3.141592f));
        ry = static_cast<float>(static_cast<double>(ry) +
                                (m31 > 0.0f ? 3.141592f : -3.141592f));
    }
    if (std::fabs(static_cast<double>(rx)) < 1e-6f)
        rx = 0.0f;
    if (std::fabs(static_cast<double>(ry)) < 1e-6f)
        ry = 0.0f;
    if (std::fabs(static_cast<double>(rz)) < 1e-6f)
        rz = 0.0f;

    rx = static_cast<float>(static_cast<double>(rx) / 3.141592f * 180.0f);
    ry = static_cast<float>(-static_cast<double>(ry) / 3.141592f * 180.0f);
    rz = static_cast<float>(180.0f * (-static_cast<double>(rz) / 3.141592f));
    app->BoneRotationEditDegreesX() = rx;               
    app->BoneRotationEditDegreesY() = ry;               
    app->BoneRotationEditDegreesZ() = rz;               

    float d = app->BoneRotationEditDegreesX();
    if (std::fabs(static_cast<double>(d)) < 1e-7f)      
        d = 0.0f;
    sprintf_s(text, 0x100, "%1.1f", static_cast<double>(d));
    SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutRotXEdit), text);
    d = app->BoneRotationEditDegreesY();
    if (std::fabs(static_cast<double>(d)) < 1e-7f)
        d = 0.0f;
    sprintf_s(text, 0x100, "%1.1f", static_cast<double>(d));
    SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutRotYEdit), text);
    d = app->BoneRotationEditDegreesZ();
    if (std::fabs(static_cast<double>(d)) < 1e-7f)
        d = 0.0f;
    sprintf_s(text, 0x100, "%1.1f", static_cast<double>(d));
    SetWindowTextA(GetDlgItem(hwnd, panel::kReadoutRotZEdit), text);
}

}  
