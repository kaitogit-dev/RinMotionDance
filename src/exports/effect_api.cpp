#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstring>
#include <cstdint>

#include "rinmotiondance/charset_conv.hpp"
#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"

namespace rinmotiondance {
int GetPmdNum(MMDApp* app);
}  

namespace {

using rinmotiondance::MMDApp;


unsigned char* ModelByIndex(MMDApp* app, int index) {  
    int occupied = -1;
    for (int i = 0; i < rinmotiondance::kModelSlotCount; ++i) {
        unsigned char* slot = app->ModelSlot(i);
        if (slot != nullptr && ++occupied == index)
            return slot;
    }
    return nullptr;
}

rinmotiondance::mdl::AccessoryRecord* AcsByIndex(MMDApp* app, int index) {
    int occupied = -1;
    for (int i = 0; i < 0xFF; ++i) {
        rinmotiondance::mdl::AccessoryRecord* slot = app->AccessorySlot(i);
        if (slot != nullptr && ++occupied == index)
            return slot;
    }
    return nullptr;
}

int CountAcs(MMDApp* app) {
    int count = 0;
    for (int i = 0; i < 0xFF; ++i) {
        if (app->AccessorySlot(i) != nullptr)
            ++count;
    }
    return count;
}


void Identity(rinmotiondance::d3dx::D3DXMATRIXF* m) {
    std::memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = 1.0f;
}

struct D3dx {
    rinmotiondance::d3dx::Api& api;
    D3dx() : api(rinmotiondance::d3dx::Get()) {}
    void Translation(rinmotiondance::d3dx::D3DXMATRIXF* m, float x, float y, float z) {
        api.translation(m, x, y, z);
    }
    void Multiply(rinmotiondance::d3dx::D3DXMATRIXF* o, const rinmotiondance::d3dx::D3DXMATRIXF* a,
                  const rinmotiondance::d3dx::D3DXMATRIXF* b) {
        api.multiply(o, a, b);
    }
    void Rot(float angle, int axis, rinmotiondance::d3dx::D3DXMATRIXF* m) {
        (axis == 0 ? api.rotX : axis == 1 ? api.rotY : api.rotZ)(m, angle);
    }
    void Scaling(rinmotiondance::d3dx::D3DXMATRIXF* m, float x, float y, float z) {
        api.scaling(m, x, y, z);
    }
};

float* AccWorldMatrix(const rinmotiondance::mdl::AccessoryRecord& acc, float* dst16,
                      float px, float py, float pz,
                      const float* boneMat16) {
    auto* dst = reinterpret_cast<rinmotiondance::d3dx::D3DXMATRIXF*>(dst16);
    auto* bone = reinterpret_cast<const rinmotiondance::d3dx::D3DXMATRIXF*>(boneMat16);
    rinmotiondance::d3dx::D3DXMATRIXF step;
    D3dx d3;

    const float si10 = static_cast<float>(acc.scale * 10.0);
    d3.Scaling(dst, si10, si10, si10);            
    d3.Rot(acc.rotation[2], 2, &step);            
    d3.Multiply(dst, dst, &step);
    d3.Rot(acc.rotation[0], 0, &step);            
    d3.Multiply(dst, dst, &step);
    d3.Rot(acc.rotation[1], 1, &step);            
    d3.Multiply(dst, dst, &step);
    d3.Translation(&step, acc.position[0], acc.position[1], acc.position[2]);
    d3.Multiply(dst, dst, &step);
    d3.Translation(&step, px, py, pz);
    d3.Multiply(dst, dst, &step);
    d3.Multiply(dst, dst, bone);
    return dst16;
}

}  

extern "C" {

__declspec(dllexport) int ExpGetPmdNum() { return rinmotiondance::GetPmdNum(rinmotiondance::g_Block); }

__declspec(dllexport) char* ExpGetPmdFilename(int index) {
    MMDApp* app = rinmotiondance::g_Block;
    unsigned char* model = ModelByIndex(app, index);
    if (model == nullptr)
        return nullptr;                      
    char* out = reinterpret_cast<char*>(app->state.sjisOut);
    rinmotiondance::WideToSjis(app->Renderer(), out, rinmotiondance::mdl::Mdl(model)->path, 0x100);
    return out;
}

const wchar_t* MmdModelPathW(int index) {
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    return (model != nullptr) ? rinmotiondance::mdl::Mdl(model)->path : nullptr;
}

__declspec(dllexport) int ExpGetPmdOrder(int index) {
    MMDApp* app = rinmotiondance::g_Block;
    int base = CountAcs(app);
    const int pre = app->state.accessoryRenderSplitOrder;
    if (pre < base)
        base = pre;
    int occupied = -1;
    for (int i = 0; i < rinmotiondance::kModelSlotCount; ++i) {
        unsigned char* slot = app->ModelSlot(i);
        if (slot != nullptr && ++occupied == index)
            return rinmotiondance::mdl::Mdl(slot)->comboSelIndex + base;
    }
    return 0;
}

__declspec(dllexport) int ExpGetPmdMatNum(int index) {
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model == nullptr)
        return 0;
    return static_cast<int>(rinmotiondance::mdl::Mdl(model)->materialCount);
}

__declspec(dllexport) float* ExpGetPmdMaterial(float* out, int index,
                                               int mat) {
    std::memset(out, 0, 0x44);
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model == nullptr)
        return out;
    const std::uint32_t count =
        rinmotiondance::mdl::Mdl(model)->materialCount;
    if (static_cast<std::uint32_t>(mat) <= count) {
        const rinmotiondance::mdl::ModelMaterialRecord& rec =
            rinmotiondance::mdl::Materials(model)[
                static_cast<std::size_t>(mat)];
        std::memcpy(out, &rec, 0x44);
    }
    return out;
}

__declspec(dllexport) int ExpGetPmdBoneNum(int index) {
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model == nullptr)
        return 0;
    return static_cast<int>(rinmotiondance::mdl::Mdl(model)->boneCount);
}

__declspec(dllexport) char* ExpGetPmdBoneName(int index, int bone) {
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model == nullptr)
        return nullptr;
    return rinmotiondance::mdl::Bones(model)[bone].name;
}

__declspec(dllexport) float* ExpGetPmdBoneWorldMat(float* out, int index,
                                                   int bone) {
    rinmotiondance::d3dx::D3DXMATRIXF m;
    Identity(&m);
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model != nullptr) {
        rinmotiondance::mdl::BoneRecord& boneRec =
            rinmotiondance::mdl::Bones(model)[bone];
        rinmotiondance::d3dx::D3DXMATRIXF t;
        D3dx d3;
        d3.Translation(&t, boneRec.position[0], boneRec.position[1],
                       boneRec.position[2]);
        d3.Multiply(&m, &m, &t);
        d3.Multiply(&m, &m,
                    reinterpret_cast<const rinmotiondance::d3dx::D3DXMATRIXF*>(boneRec.matInit));
    }
    std::memcpy(out, &m, 0x40);
    return out;
}

__declspec(dllexport) int ExpGetPmdMorphNum(int index) {
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model == nullptr)
        return 0;
    return static_cast<int>(rinmotiondance::mdl::Mdl(model)->morphCount);
}

__declspec(dllexport) char* ExpGetPmdMorphName(int index, int morph) {
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model == nullptr)
        return nullptr;
    return rinmotiondance::mdl::Morphs(model)[morph].name;
}

__declspec(dllexport) float ExpGetPmdMorphValue(int index, int morph) {
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model == nullptr)
        return 0.0f;
    return rinmotiondance::mdl::Morphs(model)[morph].value;
}

__declspec(dllexport) int ExpGetPmdDisp(int index) {
    unsigned char* model = ModelByIndex(rinmotiondance::g_Block, index);
    if (model == nullptr)
        return 0;
    return static_cast<unsigned char>(rinmotiondance::mdl::Mdl(model)->loadComplete);
}

__declspec(dllexport) void* ExpGetPmdID(int index) {
    return ModelByIndex(rinmotiondance::g_Block, index);
}

__declspec(dllexport) int ExpGetAcsNum() { return CountAcs(rinmotiondance::g_Block); }

__declspec(dllexport) int ExpGetPreAcsNum() {
    return rinmotiondance::g_Block->state.accessoryRenderSplitOrder;
}

__declspec(dllexport) char* ExpGetAcsFilename(int index) {
    MMDApp* app = rinmotiondance::g_Block;
    rinmotiondance::mdl::AccessoryRecord* acc = AcsByIndex(app, index);
    if (acc == nullptr)
        return nullptr;
    char* out = reinterpret_cast<char*>(app->state.sjisOut);
    rinmotiondance::WideToSjis(app->Renderer(), out, acc->sourcePath, 0x100);
    return out;
}

const wchar_t* MmdAcsPathW(int index) {
    rinmotiondance::mdl::AccessoryRecord* acc =
        AcsByIndex(rinmotiondance::g_Block, index);
    return (acc != nullptr) ? acc->sourcePath : nullptr;
}

__declspec(dllexport) int ExpGetAcsOrder(int index) {
    MMDApp* app = rinmotiondance::g_Block;
    rinmotiondance::mdl::AccessoryRecord* acc = AcsByIndex(app, index);
    if (acc == nullptr)
        return 0;
    int order = acc->order + 1;
    if (order < app->state.accessoryRenderSplitOrder + 1)
        return -order;
    return order + rinmotiondance::GetPmdNum(app);
}

__declspec(dllexport) float* ExpGetAcsWorldMat(float* out, int index) {
    MMDApp* app = rinmotiondance::g_Block;
    std::memset(out, 0, 0x40);
    rinmotiondance::mdl::AccessoryRecord* acc = AcsByIndex(app, index);
    if (acc != nullptr) {
        const std::int32_t parentSlot = acc->parentModel;
        float px = 0.0f, py = 0.0f, pz = 0.0f;
        rinmotiondance::d3dx::D3DXMATRIXF bone;
        if (parentSlot != -1) {
            unsigned char* model = app->ModelSlot(parentSlot);
            rinmotiondance::mdl::BoneRecord& boneRec =
                rinmotiondance::mdl::Bones(model)[acc->parentBone];
            px = boneRec.position[0];
            py = boneRec.position[1];
            pz = boneRec.position[2];
            std::memcpy(&bone, boneRec.matInit, sizeof(bone));
        } else {
            Identity(&bone);
        }
        AccWorldMatrix(*acc, out, px, py, pz, &bone.m[0][0]);
    }
    return out;
}

__declspec(dllexport) float ExpGetAcsX(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    return acc != nullptr ? acc->position[0] : 0.0f;
}
__declspec(dllexport) float ExpGetAcsY(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    return acc != nullptr ? acc->position[1] : 0.0f;
}
__declspec(dllexport) float ExpGetAcsZ(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    return acc != nullptr ? acc->position[2] : 0.0f;
}
__declspec(dllexport) float ExpGetAcsRx(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    return acc != nullptr ? acc->rotation[0] : 0.0f;
}
__declspec(dllexport) float ExpGetAcsRy(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    return acc != nullptr ? acc->rotation[1] : 0.0f;
}
__declspec(dllexport) float ExpGetAcsRz(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    return acc != nullptr ? acc->rotation[2] : 0.0f;
}
__declspec(dllexport) float ExpGetAcsSi(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    return acc != nullptr
               ? static_cast<float>(acc->scale * 10.0)
               : 0.0f;
}
__declspec(dllexport) float ExpGetAcsTr(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    return acc != nullptr ? acc->opacity : 0.0f;
}

__declspec(dllexport) int ExpGetAcsDisp(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    if (acc == nullptr)
        return 0;
    return acc->visible;
}

__declspec(dllexport) void* ExpGetAcsID(int index) {
    return AcsByIndex(rinmotiondance::g_Block, index);
}

__declspec(dllexport) int ExpGetAcsMatNum(int index) {
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    if (acc == nullptr)
        return 0;
    return static_cast<int>(acc->materialCount);
}

__declspec(dllexport) float* ExpGetAcsMaterial(float* out, int index,
                                               int mat) {
    std::memset(out, 0, 0x44);
    const auto* acc = AcsByIndex(rinmotiondance::g_Block, index);
    if (acc == nullptr)
        return out;
    if (mat < static_cast<std::int32_t>(acc->materialCount)) {
        const float* rec = static_cast<const float*>(acc->materials) +
                           17 * static_cast<std::size_t>(mat);
        std::memcpy(out, rec, 0x44);
    }
    return out;
}

__declspec(dllexport) int ExpGetCurrentObject() {
    MMDApp* app = rinmotiondance::g_Block;
    unsigned char* cur = static_cast<unsigned char*>(app->state.activeRenderObject);
    if (cur == nullptr)
        return 0;
    for (int i = 0; i < 0xFF; ++i) {
        rinmotiondance::mdl::AccessoryRecord* slot = app->AccessorySlot(i);
        if (slot == reinterpret_cast<rinmotiondance::mdl::AccessoryRecord*>(cur)) {
            int order = slot->order + 1;
            if (order >= app->state.accessoryRenderSplitOrder + 1)
                return order + rinmotiondance::GetPmdNum(app);
            return -order;
        }
    }
    int base = CountAcs(app);
    const int pre = app->state.accessoryRenderSplitOrder;
    if (pre < base)
        base = pre;
    for (int i = 0; i < rinmotiondance::kModelSlotCount; ++i) {
        unsigned char* slot = app->ModelSlot(i);
        if (slot == cur)
            return rinmotiondance::mdl::Mdl(slot)->comboSelIndex + base;
    }
    return 0;
}

__declspec(dllexport) int ExpGetCurrentMaterial() {
    MMDApp* app = rinmotiondance::g_Block;
    unsigned char* cur = static_cast<unsigned char*>(app->state.activeRenderObject);
    if (cur == nullptr)
        return -1;
    for (int i = 0; i < 0xFF; ++i) {
        rinmotiondance::mdl::AccessoryRecord* slot = app->AccessorySlot(i);
        if (slot == reinterpret_cast<rinmotiondance::mdl::AccessoryRecord*>(cur))
            return slot->currentMaterial;
    }
    for (int i = 0; i < rinmotiondance::kModelSlotCount; ++i) {
        unsigned char* slot = app->ModelSlot(i);
        if (slot == cur)
            return static_cast<std::int32_t>(
                rinmotiondance::mdl::Mdl(cur)->toonShared);
    }
    return -1;
}

__declspec(dllexport) int ExpGetCurrentTechnic() {
    return rinmotiondance::g_Block->state.activeRenderPass;
}

__declspec(dllexport) void ExpSetRenderRepeatCount(int count) {
    rinmotiondance::g_Block->state.renderPassCount = count;
}

__declspec(dllexport) int ExpGetRenderRepeatCount() {
    return rinmotiondance::g_Block->state.renderPassCount;
}

__declspec(dllexport) int ExpGetEnglishMode() {
    return rinmotiondance::g_Block->state.englishUI;
}

__declspec(dllexport) float ExpGetFrameTime() {
    MMDApp* app = rinmotiondance::g_Block;
    if (app->state.playbackActive != 0)
        return app->state.playbackCursorSeconds;
    const std::int32_t frame = app->state.currentFrame;
    return static_cast<float>(static_cast<double>(
                                  static_cast<std::uint32_t>(frame)) /
                              30.0);
}

}  
