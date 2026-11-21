#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstddef>
#include <cstdint>

#include "rinmotiondance/ui_autokey.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

void PanelPaint(MMDApp* app);
void RegisterSelectedBoneKeys(unsigned char* model, int frame, int mode);
void RegisterMorphKeyCurrent(unsigned char* model, int index, int frame);

void AutoKeyBoneEdit(MMDApp* app) {
    if (app == nullptr || app->PlaybackActive() != 0 ||
        app->state.optflag[0] != 0)
        return;
    unsigned char* model = app->SelectedModel();
    if (model == nullptr)
        return;
    mdl::ModelRecord* record = mdl::Mdl(model);
    if (record == nullptr || record->boneSelection == nullptr ||
        record->boneCount == 0)
        return;
    bool selected = false;
    for (std::uint32_t i = 0; i < record->boneCount; ++i) {
        if (record->boneSelection[i] != 0) {
            selected = true;
            break;
        }
    }
    if (!selected && record->selectedBone >= 0 &&
        record->selectedBone < static_cast<std::int32_t>(record->boneCount)) {
        record->boneSelection[record->selectedBone] = 1;
        selected = true;
    }
    if (!selected)
        return;
    app->SceneModified() = 1;
    RegisterSelectedBoneKeys(model, app->state.currentFrame,
                             app->PlaybackPhysicsMode());
    if (app->state.lastRegisteredFrame <
        static_cast<std::int32_t>(record->maxFrame))
        app->state.lastRegisteredFrame =
            static_cast<std::int32_t>(record->maxFrame);
    PanelPaint(app);
}

void AutoKeyMorphEdit(MMDApp* app, std::size_t lane) {
    if (app == nullptr || app->PlaybackActive() != 0 ||
        app->state.optflag[0] != 0 || lane >= 4)
        return;
    unsigned char* model = app->SelectedModel();
    if (model == nullptr)
        return;
    mdl::ModelRecord* record = mdl::Mdl(model);
    if (record == nullptr || record->morphs == nullptr)
        return;
    const int index = record->selectedMorphs[lane];
    if (index < 0 || index >= static_cast<int>(record->morphCount))
        return;
    app->SceneModified() = 1;
    RegisterMorphKeyCurrent(model, index, app->state.currentFrame);
    if (app->state.lastRegisteredFrame <
        static_cast<std::int32_t>(record->maxFrame))
        app->state.lastRegisteredFrame =
            static_cast<std::int32_t>(record->maxFrame);
    PanelPaint(app);
}

}
