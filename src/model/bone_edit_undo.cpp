#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdlib>
#include <cstring>
#include <new>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void PushBoneEditUndo(MMDApp* app) {  
    if (app == nullptr)
        return;
    const unsigned modelSlot = app->state.slotIdx;
    auto* model = static_cast<unsigned char*>(
        app->state.modelSlots[modelSlot]);
    if (model == nullptr)
        return;

    const int boneCount = rinmotiondance::mdl::Mdl(model)->boneCount;
    auto* selected = rinmotiondance::mdl::Mdl(model)->boneSelection;
    auto* dirty = rinmotiondance::mdl::Mdl(model)->bonePhysicsState;
    auto* bones = rinmotiondance::mdl::Bones(model);
    if (boneCount <= 0 || selected == nullptr || bones == nullptr)
        return;

    int selectedCount = 0;
    for (int i = 0; i < boneCount; ++i)
        selectedCount += selected[i] != 0;
    if (selectedCount == 0)
        return;

    HWND window = app->state.hwnd;
    if (window == nullptr)
        window = static_cast<HWND>(app->Hwnd());
    EnableWindow(GetDlgItem(window, panel::kUndoButton), TRUE);
    EnableWindow(GetDlgItem(window, panel::kRedoButton), FALSE);

    rinmotiondance::mdl::Mdl(model)->undoDirty = 1;
    rinmotiondance::mdl::Mdl(model)->redoDirty = 0;
    auto& ringIndex = rinmotiondance::mdl::Mdl(model)->undoState[0];
    if (++ringIndex >= 30)
        ringIndex = 0;
    rinmotiondance::mdl::Mdl(model)->undoState[1] = ringIndex;

    auto& undo = rinmotiondance::mdl::Mdl(model)->undoRings[0].slots[ringIndex];
    undo.operation = 1;
    undo.dirty = selectedCount;
    auto*& oldSnapshot = undo.bonePose;
    if (oldSnapshot != nullptr) {
        ::operator delete(oldSnapshot);
        oldSnapshot = nullptr;
    }

    auto* snapshot = static_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
        ::operator new(sizeof(rinmotiondance::mdl::BonePoseSnapshot) * selectedCount));
    oldSnapshot = snapshot;
    std::memset(snapshot, 0, static_cast<std::size_t>(36) * selectedCount);

    int output = 0;
    for (int i = 0; i < boneCount; ++i) {
        if (selected[i] == 0)
            continue;
        auto& record = snapshot[output++];
        rinmotiondance::mdl::BoneRecord* bone = &bones[i];
        record.boneIndex = i;
        std::memcpy(record.position, bone->trans, sizeof(record.position));
        std::memcpy(record.rotation, bone->rotQuat, sizeof(record.rotation));
        record.physicsDisabled = dirty != nullptr ? dirty[i] : 0;
    }
}

}  
