#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void RefreshSelectNavDisplay(MMDApp* app, HWND hDlg);                

void SyncSelectAttachCombo(MMDApp* app, HWND hDlg);                
void RebuildTargetBoneList(MMDApp* app, HWND hDlg, int keepSelection); 

namespace {



const char kJpRoot[] = "\x83\x8b\x81\x5b\x83\x67";
const char kJpNone[] = "\x82\xc8\x82\xb5";
const char kJpGround[] = "\x92\x6e\x96\xca";

unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

unsigned char* ModelAt(MMDApp* app, int slot) {
    return app->ModelSlot(slot);
}

bool English(MMDApp* app) {
    return app->state.englishUI != 0;  
}

mdl::BoneOrderEntry* ModelSelectRecords(unsigned char* model) {
    return mdl::BoneOrder(model);
}

mdl::BoneOrderEntry* SelRecord(MMDApp* app, int item) {
    return app->state.selectNavRecords + item;
}

LPARAM StrParam(const void* s) {
    return reinterpret_cast<LPARAM>(const_cast<void*>(s));
}

}  

void InitSelectNavDialog(MMDApp* app, HWND hDlg) {  
    app->AccessoryApplyGate() = 0;                     
    if (app->state.selectNavRecords != nullptr) {      
        delete[] app->state.selectNavRecords;
        app->state.selectNavRecords = nullptr;
    }

    unsigned char* model = ActiveModel(app);
    const int count = static_cast<int>(
        mdl::Mdl(model)->boneOrderCount);              
    auto* const records = mdl::CopyBoneBindings(
        ModelSelectRecords(model), static_cast<std::size_t>(count));
    app->state.selectNavRecords = records;

    const HWND combo669 = GetDlgItem(hDlg, panel::kBoneNameCombo);                    
    const HWND combo673 = GetDlgItem(hDlg, panel::kMorphNameCombo);                    
    SendMessageA(combo669, CB_RESETCONTENT, 0, 0);        
    SendMessageA(combo673, CB_RESETCONTENT, 0, 0);                            
    SendMessageA(combo669, CB_ADDSTRING, 0,               
                 English(app) ? StrParam("root")          
                              : StrParam(kJpRoot));      

    int selItem = 0;                                                 
    int item = 1;                                                    
    if (count > 1) {
        mdl::BoneRecord* const bones = mdl::Bones(model);
        do {
            const int boneIdx = records[item].boneIndex;             
            const char* name =
                English(app) ? bones[boneIdx].nameEn : bones[boneIdx].name;
            SendMessageA(combo669, CB_ADDSTRING, 0, StrParam(name));        
            if (mdl::Mdl(model)->selectedBone == boneIdx)  
                selItem = item;
            ++item;
        } while (item < count);
    }
    const LRESULT cnt669 =
        SendMessageA(combo669, CB_GETCOUNT, 0, 0);         
    if (selItem <= 0)
        SendMessageA(combo669, CB_SETCURSEL, cnt669 > 0, 0); 
    else
        SendMessageA(combo669, CB_SETCURSEL, selItem, 0);                   

    if (English(app)) {                                              
        SendMessageA(combo673, CB_ADDSTRING, 0, StrParam("non"));     
        SendMessageA(combo673, CB_ADDSTRING, 0, StrParam("ground"));   
    } else {
        SendMessageA(combo673, CB_ADDSTRING, 0, StrParam(kJpNone));    
        SendMessageA(combo673, CB_ADDSTRING, 0, StrParam(kJpGround));  
    }

    const HWND mainCombo = GetDlgItem(
        static_cast<HWND>(app->state.hwnd), panel::kMainComboModel); 
    const int modelCount =
        static_cast<int>(SendMessageA(mainCombo, CB_GETCOUNT, 0, 0)) - 1;  
    if (app->DialogOrders().modelIndices != nullptr) {                    
        app->DialogOrders().modelIndices.reset();
    }
    app->DialogOrders().modelIndices.reset(
        new std::int32_t[static_cast<std::size_t>(modelCount)]);
    std::int32_t* const order = app->DialogOrders().modelIndices.get();

    char Buffer[256];                                                
    int nOrder = 0;
    for (int ord = 0; ord < kModelSlotCount; ++ord) {                
        int slot = 0;
        unsigned char* m = nullptr;
        for (; slot < kModelSlotCount; ++slot) {                     
            unsigned char* cand = ModelAt(app, slot);
            if (cand != nullptr &&
                mdl::Mdl(cand)->comboSelIndex2 ==
                    static_cast<unsigned char>(ord)) {  
                m = cand;
                break;
            }
        }
        if (m == nullptr)
            continue;                                                
        const mdl::ModelRecord* const mrec = mdl::Mdl(m);
        const char* name =
            English(app) ? mrec->nameEn : mrec->name;  
        if (slot == app->SelectedModelSlot()) {                      
            sprintf_s(Buffer, 0x100, "(%s)", name);                  
            SendMessageA(combo673, CB_ADDSTRING, 0, StrParam(Buffer));
        } else {
            SendMessageA(combo673, CB_ADDSTRING, 0, StrParam(name));
        }
        order[nOrder++] = slot;                                      
    }

    SyncSelectAttachCombo(app, hDlg);                                            
}

void SyncSelectAttachCombo(MMDApp* app, HWND hDlg) {  
    const HWND combo673 = GetDlgItem(hDlg, panel::kMorphNameCombo);                     
    const HWND combo669 = GetDlgItem(hDlg, panel::kBoneNameCombo);                     
    const int attach = SelRecord(
        app,
        static_cast<int>(SendMessageA(combo669, CB_GETCURSEL, 0, 0)))
        ->linkedModel;

    if (attach == -1) {                                              
        SendMessageA(combo673, CB_SETCURSEL, 0, 0);
    } else if (attach == -2) {                                       
        SendMessageA(combo673, CB_SETCURSEL, 1, 0);
    } else {
        const int n = static_cast<int>(
                         SendMessageA(combo673, CB_GETCOUNT, 0, 0)) - 2;
        if (n > 0) {                                                 
            const int* order = app->DialogOrders().modelIndices.get();
            int idx = 0;
            while (attach != order[idx]) {
                ++idx;
                if (idx >= n) {
                    RebuildTargetBoneList(app, hDlg, 1);                         
                    return;
                }
            }
            SendMessageA(combo673, CB_SETCURSEL, idx + 2, 0);               
        }
    }
    RebuildTargetBoneList(app, hDlg, 1);                                         
}

void RebuildTargetBoneList(MMDApp* app, HWND hDlg, int keepSelection) {  
    const HWND combo677 = GetDlgItem(hDlg, panel::kGroupNameCombo);                     
    const int boneSel = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, panel::kBoneNameCombo), CB_GETCURSEL, 0, 0));       
    const int attach = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, panel::kMorphNameCombo), CB_GETCURSEL, 0, 0));                        
    mdl::BoneOrderEntry* const rec = SelRecord(app, boneSel);

    if (attach == 0 || attach == 1) {                                
        rec->linkedModel = attach == 0 ? -1 : -2;                
        rec->linkedBone = 0;
        SendMessageA(combo677, CB_RESETCONTENT, 0, 0);
        SendMessageA(combo677, CB_ADDSTRING, 0, StrParam("------"));        
        SendMessageA(combo677, CB_SETCURSEL, 0, 0);
        RefreshSelectNavDisplay(app, hDlg);                                        
        return;
    }

    const int modelSlot = app->DialogOrders().modelIndices.get()[attach - 2];
    rec->linkedModel = modelSlot;                                
    SendMessageA(combo677, CB_RESETCONTENT, 0, 0);                             

    unsigned char* const model = ModelAt(app, modelSlot);
    mdl::BoneRecord* const bones = mdl::Bones(model);
    const int boneCount =
        static_cast<int>(mdl::Mdl(model)->boneCount);                 

    int nSelectable = 0;                                             
    for (int i = 0; i < boneCount; ++i) {                            
        const mdl::BoneType type = bones[i].type;
        if (type < mdl::BoneType::InertTip ||
            type == mdl::BoneType::FixedAxis)
            ++nSelectable;
    }

    if (app->AccessoryEditArray() != nullptr) {                     
        free(app->AccessoryEditArray());
        app->AccessoryEditArray() = nullptr;
    }
    int* const boneIds = static_cast<int*>(
        operator new(static_cast<std::size_t>(4 * nSelectable)));    
    app->AccessoryEditArray() = boneIds;

    int n = 0;                                                       
    for (int i = 0; i < boneCount; ++i) {                            
        const mdl::BoneType type = bones[i].type;
        if (type < mdl::BoneType::InertTip ||
            type == mdl::BoneType::FixedAxis) {
            const char* name = English(app) ? bones[i].nameEn : bones[i].name;
            SendMessageA(combo677, CB_ADDSTRING, 0, StrParam(name));        
            boneIds[n++] = i;                                        
        }
    }

    if (keepSelection) {                                             
        if (n > 0) {
            int idx = 0;
            while (rec->linkedBone != boneIds[idx]) {
                ++idx;
                if (idx >= n) {
                    RefreshSelectNavDisplay(app, hDlg);                            
                    return;
                }
            }
            SendMessageA(combo677, CB_SETCURSEL, idx, 0);                   
        }
    } else {
        SendMessageA(combo677, CB_SETCURSEL, 0, 0);                         
        rec->linkedBone = 0;                                    
    }
    RefreshSelectNavDisplay(app, hDlg);                                            
}

void CommitTargetBonePick(MMDApp* app, HWND hDlg) {  
    const int boneSel = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, panel::kBoneNameCombo), CB_GETCURSEL, 0, 0));       
    const int attach = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, panel::kMorphNameCombo), CB_GETCURSEL, 0, 0));                        
    mdl::BoneOrderEntry* const rec = SelRecord(app, boneSel);
    if (attach >= 2) {                                               
        const int pick = static_cast<int>(SendMessageA(
            GetDlgItem(hDlg, panel::kGroupNameCombo), CB_GETCURSEL, 0, 0));                    
        rec->linkedBone =
            static_cast<int*>(app->AccessoryEditArray())[pick];
    } else {
        rec->linkedBone = 0;                                    
    }
    RefreshSelectNavDisplay(app, hDlg);                                            
}

void ResetModelSelection(MMDApp* app) {  
    app->SceneModified() = 1;                                       

    unsigned char* const model = ActiveModel(app);

    mdl::BoneKey* const boneKeys = mdl::BoneKeys(model);
    for (std::size_t i = 0; i < mdl::kBoneKeyCapacity; ++i)          
        boneKeys[i].allocated = 0;
    mdl::MorphKey* const morphKeys = mdl::MorphKeys(model);
    for (std::size_t j = 0; j < mdl::kMorphKeyCapacity; ++j)         
        morphKeys[j].allocated = 0;
    mdl::DisplayKey* const displayKeys = mdl::DisplayKeys(model);
    for (std::size_t k = 0; k < mdl::kDisplayKeyCapacity; ++k)       
        displayKeys[k].allocated = 0;

    const int count = static_cast<int>(
        mdl::Mdl(model)->boneOrderCount);                            
    if (count > 0) {
        std::copy_n(app->state.selectNavRecords, count,
                    ModelSelectRecords(model));
    }

    RegisterDisplayKeyCurrent(model, app->state.currentFrame);    
    const auto registeredFrames = mdl::Mdl(model)->maxFrame;         
    if (app->LastRegisteredFrame() < registeredFrames)                
        app->LastRegisteredFrame() = registeredFrames;

    PanelPaint(app);                                                 
    app->AccessoryApplyGate() = 1;                         
}

void ApplyBoneAttach(MMDApp* app, HWND hDlg) {  
    const int boneSel = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, panel::kBoneNameCombo), CB_GETCURSEL, 0, 0));       
    const int attach = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, panel::kMorphNameCombo), CB_GETCURSEL, 0, 0));                        
    const mdl::BoneOrderEntry* const rec = SelRecord(app, boneSel);

    unsigned char* const model = ActiveModel(app);
    mdl::BoneRecord* const bones = mdl::Bones(model);                
    mdl::BoneRecord& bone = bones[rec->boneIndex];

    auto* d3dx = &d3dx::Get();
    {                                              
        d3dx::D3DXMATRIXF X = {};                                    
        d3dx::D3DXMATRIXF T = {};                                    
        if (attach == 0) {
            const int parent = bone.parent;                           
            if (parent < 0) {
                X = *reinterpret_cast<d3dx::D3DXMATRIXF*>(
                    bone.matInit);                                   
            } else {
                d3dx->inverse(
                    &T, nullptr, reinterpret_cast<d3dx::D3DXMATRIXF*>(
                                     bones[parent].matInit));         
                d3dx->multiply(
                    &X, reinterpret_cast<d3dx::D3DXMATRIXF*>(bone.matInit),
                    &T);                                             
            }
        } else if (attach == 1) {
            X = *reinterpret_cast<d3dx::D3DXMATRIXF*>(bone.matInit); 
        } else {
            unsigned char* const tmodel =
                ModelAt(app, rec->linkedModel);                  
            mdl::BoneRecord& target =
                mdl::Bones(tmodel)[rec->linkedBone];
            const float* const tpos = target.position;
            X = *reinterpret_cast<d3dx::D3DXMATRIXF*>(
                target.matExtra);                                    
            d3dx->translation(&T, tpos[0], tpos[1], tpos[2]);        
            d3dx->multiply(&X, &T, &X);                              
            const float* const cpos = bone.position;
            d3dx->translation(&T, -cpos[0], -cpos[1], -cpos[2]);     
            d3dx->multiply(&X, &T, &X);                              
            d3dx->inverse(&X, nullptr, &X);                          
            T = *reinterpret_cast<d3dx::D3DXMATRIXF*>(bone.matInit); 
            d3dx->multiply(&X, &T, &X);                              
        }

        const float* const cpos2 = bone.position;                    
        float vin[3] = {cpos2[0], cpos2[1], cpos2[2]};
        float vout[4];                                               
        d3dx->vec3Transform(vout, vin, &X);                          
        bone.trans[0] = vout[0] - vin[0];                            
        bone.trans[1] = vout[1] - vin[1];                            
        bone.trans[2] = vout[2] - vin[2];                            

        X.m[3][0] = 0.0f;                                            
        X.m[3][1] = 0.0f;                                            
        X.m[3][2] = 0.0f;                                            
        d3dx->quatFromMatrix(bone.rotQuat, &X);                      
    }

    app->SceneModified() = 1;                                       

    unsigned char* const selFlags = mdl::Mdl(model)->boneSelection;
    const int boneCount = static_cast<int>(mdl::Mdl(model)->boneCount);
    for (int i = 0; i < boneCount; ++i)                              
        selFlags[i] = 0;
    selFlags[rec->boneIndex] = 1;                                    

    RegisterSelectedBoneKeys(model, app->state.currentFrame,     
              app->PlaybackPhysicsMode());
    const auto registeredFrames = mdl::Mdl(model)->maxFrame;         
    if (app->LastRegisteredFrame() < registeredFrames)                
        app->LastRegisteredFrame() = registeredFrames;

    PanelPaint(app);                                                 
    SelectionReeval(app);                                            
    app->AccessoryApplyGate() = 1;                         
}

void RefreshSelectNavDisplay(MMDApp* app, HWND hDlg) {  
    auto& s = *app;
    const int sel = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, panel::kBoneNameCombo), CB_GETCURSEL, 0, 0));             
    const int kind = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, panel::kMorphNameCombo), CB_GETCURSEL, 0, 0));             
    if (sel == 0) {
        for (int i = 0; i < 6; ++i)                              
            SetWindowTextA(GetDlgItem(hDlg, 688 + i), "------");
        EnableWindow(GetDlgItem(hDlg, panel::kOrderUpButton), FALSE);              
        return;
    }

    unsigned char* model = s.SelectedModel();
    rinmotiondance::mdl::BoneRecord* bones =
        rinmotiondance::mdl::Bones(model);
    const mdl::BoneOrderEntry* const rec =
        SelRecord(app, sel);                             
    rinmotiondance::mdl::BoneRecord* bone = &bones[rec->boneIndex];

    auto& api = d3dx::Get();
    d3dx::D3DXMATRIXF m{}, t{}, t2{};
    if (kind == 1) {                                             
        std::memcpy(&m, bone->matInit, sizeof m);
    } else if (kind >= 2) {                                      
        unsigned char* srcModel = s.ModelSlot(rec->linkedModel);
        rinmotiondance::mdl::BoneRecord* srcBones =
            rinmotiondance::mdl::Bones(srcModel);
        rinmotiondance::mdl::BoneRecord& src =
            srcBones[rec->linkedBone];
        std::memcpy(&m, &src.matInit[0], sizeof m);
        const float* const p = src.position;  
        api.translation(&t, p[0], p[1], p[2]);
        api.multiply(&m, &t, &m);
        api.translation(&t,
                        -bone->position[0],
                        -bone->position[1],
                        -bone->position[2]);
        api.multiply(&m, &t, &m);
        api.inverse(&m, nullptr, &m);
        std::memcpy(&t2, bone->matInit, sizeof t2);
        api.multiply(&m, &t2, &m);
    } else {                                                     
        const std::int32_t parent = bone->parent;
        if (parent < 0) {                                        
            std::memcpy(&m, bone->matInit, sizeof m);
        } else {
            api.inverse(&t, nullptr,
                        reinterpret_cast<d3dx::D3DXMATRIXF*>(
                            &bones[parent].matInit[0]));
            api.multiply(&m,
                         reinterpret_cast<d3dx::D3DXMATRIXF*>(bone->matInit),
                         &t);
        }
    }

    const float* const pos = bone->position;
    float out[4]{};
    float world[3] = {pos[0], pos[1], pos[2]};
    api.vec3Transform(out, world, &m);                           
    float values[6];
    values[0] = out[0] - pos[0];
    values[1] = out[1] - pos[1];
    values[2] = out[2] - pos[2];

    float& pitch = values[3];                                    
    float& roll = values[4];                                     
    float& yaw = values[5];                                      
    yaw = atan2f(m.m[0][1], m.m[1][1]);
    pitch = asinf(-m.m[2][1]);
    roll = atan2f(m.m[2][0], m.m[2][2]);
    const float c = cosf(pitch);
    if (fabsf(c) < 0.000001f) {                                  
        yaw = static_cast<float>(
            static_cast<double>(yaw) +
            (static_cast<double>(m.m[0][1]) <= 0.0 ? -3.141592
                                                   : 3.141592));
        roll = static_cast<float>(
            static_cast<double>(roll) +
            (static_cast<double>(m.m[2][0]) <= 0.0 ? -3.141592
                                                   : 3.141592));
    }
    if (fabs(static_cast<double>(pitch)) < 0.000001) pitch = 0.0f;
    if (fabs(static_cast<double>(roll)) < 0.000001) roll = 0.0f;
    if (fabs(static_cast<double>(yaw)) < 0.000001) yaw = 0.0f;
    constexpr float kPiDeg = 3.141592025756836f;
    pitch = static_cast<float>(static_cast<double>(pitch) /
                               kPiDeg * 180.0);
    roll = static_cast<float>(-static_cast<double>(roll) /
                              kPiDeg * 180.0);
    yaw = static_cast<float>(180.0 *
                             (-static_cast<double>(yaw) / kPiDeg));
    const double eps = 0.0000001000000011686097;
    if (fabs(static_cast<double>(pitch)) < eps) pitch = 0.0f;
    if (fabs(static_cast<double>(roll)) < eps) roll = 0.0f;
    if (fabs(static_cast<double>(yaw)) < eps) yaw = 0.0f;

    for (int i = 0; i < 6; ++i) {                                
        char text[0x100];
        sprintf_s(text, 0x100, "%f", values[i]);
        SetWindowTextA(GetDlgItem(hDlg, 688 + i), text);
    }
    EnableWindow(GetDlgItem(hDlg, panel::kOrderUpButton), TRUE);                   
}

}  
