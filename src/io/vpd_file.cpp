#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

const char kHdr[] = "Vocaloid Pose Data file\n\n";               
const char kModelFmt[] =                                         
    "%s.osm;\t\t// \x90\x65\x83\x74\x83\x40\x83\x43\x83\x8b\x96\xbc\n";
const char kCountFmt[] =                                         
    "%d;\t\t\t\t// \x91\x8d\x83\x7c\x81\x5b\x83\x59\x83\x7b\x81\x5b\x83"
    "\x93\x90\x94\n\n";
const char kBoneFmt[] = "Bone%d{%s\n";                           
const char kTransFmt[] =                                         
    "  %f,%f,%f;\t\t\t\t// trans x,y,z\n";
const char kQuatFmt[] =                                          
    "  %f,%f,%f,%f;\t\t// Quaternion x,y,z,w\n";
const char kCloseFmt[] = "}\n\n";                                
const wchar_t kModeWrite[] = L"wt";                              
const wchar_t kModeRead[] = L"r";                                
const char kCountScan[] = "%d;";                                 
const char kBrace[] = "{";                                       
const char kTransScan[] = "  %f,%f,%f;";                         
const char kQuatScan[] = "  %f,%f,%f,%f;";                       

}  

void SaveVpdFile(const wchar_t* path) {
    MMDApp* app = g_Block;
    if (app == nullptr) return;
    unsigned char* const model = ActiveModel(app);
    if (model == nullptr) return;

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, path, kModeWrite) != 0 || fp == nullptr) return;

    std::fprintf(fp, "%s", kHdr);
    mdl::ModelRecord& modelRecord = *mdl::Mdl(model);
    std::fprintf(fp, kModelFmt, modelRecord.name);

    const int boneCnt = modelRecord.boneCount;
    unsigned char* const selBits = modelRecord.boneSelection;
    int selected = 0;
    for (int i = 0; i < boneCnt; ++i)
        if (selBits[i] != 0) ++selected;
    std::fprintf(fp, kCountFmt, selected);

    rinmotiondance::mdl::BoneRecord* const bones = rinmotiondance::mdl::Bones(model);
    const int physMode = app->PlaybackPhysicsMode();
    int running = 0;
    for (int i = 0; i < boneCnt; ++i) {
        if (selBits[i] == 0) continue;
        rinmotiondance::mdl::BoneRecord* const bone = bones + i;
        std::fprintf(fp, kBoneFmt, running, bone->name);
        ++running;
        const float* trans;
        const float* quat;
        if (bone->hasRigidBody != 0 && bone->physicsDisabled == 0 && physMode >= 2) {
            trans = bone->ikBackup;
            quat = bone->ikBackup + 3;
        } else {
            trans = bone->trans;
            quat = bone->rotQuat;
        }
        std::fprintf(fp, kTransFmt, trans[0], trans[1], trans[2]);
        std::fprintf(fp, kQuatFmt, quat[0], quat[1], quat[2], quat[3]);
        std::fprintf(fp, "%s", kCloseFmt);
    }
    std::fclose(fp);
}

void LoadVpdFile(const wchar_t* path) {
    MMDApp* app = g_Block;
    if (app == nullptr) return;
    unsigned char* const model = ActiveModel(app);
    if (model == nullptr) return;

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, path, kModeRead) != 0 || fp == nullptr) return;

    char line[256], name[256], tail[256];
    std::fgets(line, 0x100, fp);   
    std::fgets(line, 0x100, fp);   
    std::fgets(line, 0x100, fp);   
    int count = 0;
    std::fscanf(fp, kCountScan, &count);
    std::fgets(tail, 0x100, fp);   
    std::fgets(tail, 0x100, fp);   

    const HWND hwnd = static_cast<HWND>(app->Hwnd());
    EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);
    EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);

    mdl::ModelRecord& modelRecord = *mdl::Mdl(model);
    modelRecord.undoDirty = 1;                          
    modelRecord.redoDirty = 0;                          
    int ring = ++mdl::Mdl(model)->undoState[0];
    if (ring >= 30) mdl::Mdl(model)->undoState[0] = 0;
    ring = mdl::Mdl(model)->undoState[0];
    mdl::Mdl(model)->undoState[1] = ring;

    mdl::UndoRecord& undo = mdl::Mdl(model)->undoRings[0].slots[ring];
    undo.operation = 1;
    undo.dirty = count;
    if (undo.bonePose != nullptr) {
        ::operator delete(undo.bonePose);
        undo.bonePose = nullptr;
    }
    mdl::BonePoseSnapshot* const snap =
        static_cast<mdl::BonePoseSnapshot*>(
            operator new(sizeof(mdl::BonePoseSnapshot) * count));
    undo.bonePose = snap;
    std::memset(snap, 0, sizeof(mdl::BonePoseSnapshot) * count);
    undo.dirty = count;

    const int boneCnt = modelRecord.boneCount;
    rinmotiondance::mdl::BoneRecord* const bones = rinmotiondance::mdl::Bones(model);
    unsigned char* const selBits = modelRecord.boneSelection;
    unsigned char* const regBits = modelRecord.bonePhysicsState;
    const bool pmx = modelRecord.physicsMode == 2;              

    for (int k = 0; k < count; ++k) {
        float trans[3] = {0, 0, 0};
        float quat[4] = {0, 0, 0, 1.0f};

        std::fgets(line, 0x100, fp);                    
        const char* brace = std::strstr(line, kBrace);
        if (brace == nullptr) continue;
        sprintf_s(name, 0x100, "%s", brace + 1);
        if (char* nl = strchr(name, 10)) *nl = 0;  

        std::fscanf(fp, kTransScan, &trans[0], &trans[1], &trans[2]);
        std::fgets(tail, 0x100, fp);                    
        std::fscanf(fp, kQuatScan, &quat[0], &quat[1], &quat[2], &quat[3]);
        std::fgets(tail, 0x100, fp);                    
        std::fgets(tail, 0x100, fp);                    
        std::fgets(tail, 0x100, fp);

        for (int i = 0; i < boneCnt; ++i) {
            rinmotiondance::mdl::BoneRecord* const bone = bones + i;
            if (std::strcmp(name, bone->name) != 0)
                continue;

            mdl::BonePoseSnapshot& rec = snap[k];
            rec.boneIndex = i;
            std::memcpy(rec.position, bone->trans, sizeof rec.position);
            std::memcpy(rec.rotation, bone->rotQuat, sizeof rec.rotation);
            rec.physicsDisabled = regBits[i];

            const mdl::BoneType btype = bone->type;
            if (btype == mdl::BoneType::FixedAxis ||
                (btype == mdl::BoneType::UnderIk &&
                 (bone->flags & mdl::kBoneFlagFixedAxis) ==
                     mdl::kBoneFlagFixedAxis)) {
                float boneAxis[3];
                if (pmx) {
                    std::memcpy(boneAxis, bone->axis, 12);
                } else {
                    const mdl::BoneRecord& tailBone = bones[bone->tailBone];
                    for (int c = 0; c < 3; ++c)
                        boneAxis[c] = tailBone.position[c] - bone->position[c];
                    auto* d3 = &d3dx::Get();
                    d3->vec3Normalize(boneAxis, boneAxis);
                }
                auto* d3 = &d3dx::Get();
                if (d3->quatToAxisAngle != nullptr) {
                    float axisOut[3], angle;
                    d3->quatToAxisAngle(quat, axisOut, &angle);
                    if (!_finite(quat[3])) quat[3] = 1.0f;   
                    float sinHalf;
                    const float ww = quat[3] * quat[3];
                    if (ww <= 1.0f) {
                        sinHalf = sqrtf(1.0f - ww);     
                    } else {
                        quat[3] = 1.0f;
                        sinHalf = 0.0f;
                    }
                    const float dot = boneAxis[0] * axisOut[0] +
                                      boneAxis[1] * axisOut[1] +
                                      boneAxis[2] * axisOut[2];
                    const float nA = sqrtf(boneAxis[0] * boneAxis[0] +
                                           boneAxis[1] * boneAxis[1] +
                                           boneAxis[2] * boneAxis[2]);
                    const float nB = sqrtf(axisOut[0] * axisOut[0] +
                                           axisOut[1] * axisOut[1] +
                                           axisOut[2] * axisOut[2]);
                    const float cosT = dot / (nA * nB);
                    if (cosT < 0.0f) sinHalf = -sinHalf;    
                    quat[0] = sinHalf * axisOut[0];
                    quat[1] = sinHalf * axisOut[1];
                    quat[2] = sinHalf * axisOut[2];
                    if (d3->quatNormalize != nullptr)
                        d3->quatNormalize(quat, quat);
                }
            }

            selBits[i] = 1;                              
            regBits[i] = 1;
            std::memcpy(bone->trans, trans, 12);        
            std::memcpy(bone->rotQuat, quat, 16);
        }
    }
    std::fclose(fp);
}

}  
