#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <new>
#include <cstdlib>
#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {


unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

rinmotiondance::mdl::UndoRecord& CurrentUndo(unsigned char* model) {
    return rinmotiondance::mdl::Mdl(model)->undoRings[0].slots[
        rinmotiondance::mdl::Mdl(model)->undoState[0]];
}

void BeginUndoEdit(unsigned char* model, std::uint32_t frame) {
    mdl::Mdl(model)->undoDirty = 1;
    mdl::Mdl(model)->redoDirty = 0;
    std::int32_t cursor = rinmotiondance::mdl::Mdl(model)->undoState[0] + 1;
    if (cursor >= 0x1E) cursor = 0;
    rinmotiondance::mdl::Mdl(model)->undoState[0] = cursor;
    rinmotiondance::mdl::Mdl(model)->undoState[1] = cursor;
    auto& undo = CurrentUndo(model);
    undo.operation = 2;
    undo.frame = frame;
}

void SnapshotPose(unsigned char* model) {
    auto& undo = CurrentUndo(model);
    auto*& slot = undo.bonePose;
    if (slot != nullptr) {
        ::operator delete(slot);
        slot = nullptr;
    }
    const std::int32_t count = mdl::Mdl(model)->boneCount;
    slot = static_cast<rinmotiondance::mdl::BonePoseSnapshot*>(::operator new(
        static_cast<std::size_t>(count) *
        sizeof(rinmotiondance::mdl::BonePoseSnapshot)));
    std::memset(slot, 0, static_cast<std::size_t>(count) * sizeof(*slot));
    rinmotiondance::mdl::BoneRecord* bones = rinmotiondance::mdl::Bones(model);
    unsigned char* selected = mdl::Mdl(model)->boneSelection;
    for (std::int32_t i = 0; i < count; ++i) {
        auto& out = slot[i];
        rinmotiondance::mdl::BoneRecord* bone = &bones[i];
        out.boneIndex = i;
        std::memcpy(out.position, bone->trans, sizeof out.position);
        std::memcpy(out.rotation, bone->rotQuat, sizeof out.rotation);
        out.physicsDisabled = selected[i];
    }
}

void AllocUndoKeys(unsigned char* model, std::size_t bytes) {
    auto& undo = CurrentUndo(model);
    undo.dirty = 0;
    void*& slot = undo.auxiliaryPose;
    if (slot != nullptr) {
        ::operator delete(slot);
        slot = nullptr;
    }
    slot = static_cast<unsigned char*>(::operator new(bytes));
    std::memset(slot, 0, bytes);
}

void ClearBoneKeyRecord(mdl::BoneKey& key) {
    key.frame = 0;
    key.allocated = 0;
    std::memset(key.position, 0, sizeof(key.position));
    key.rotation[0] = 0.0f;
    key.rotation[1] = 0.0f;
    key.rotation[2] = 0.0f;
    key.rotation[3] = 1.0f;
}

void CopyBoneKeyPayload(mdl::BoneKey& dst, const mdl::BoneKey& src) {
    std::memcpy(dst.interpolation, src.interpolation,
                sizeof(dst.interpolation));
    std::memcpy(dst.position, src.position, sizeof(dst.position));
    std::memcpy(dst.rotation, src.rotation, sizeof(dst.rotation));
    dst.allocated = src.allocated;
}

void CopyCameraKeyPayload(mdl::CameraKey& dst, const mdl::CameraKey& src) {
    dst.distance = src.distance;
    std::memcpy(dst.eye, src.eye, sizeof(dst.eye));
    std::memcpy(dst.target, src.target, sizeof(dst.target));
    for (int c = 0; c < 4; ++c)
        std::memcpy(dst.interpolation[c], src.interpolation[c], 4);
    dst.perspective = src.perspective;
    dst.fov = src.fov;
}

void ClearCameraKeyRecord(mdl::CameraKey& key) {
    key.next = 0;
    key.previous = 0;
    key.frame = 0;
    key.perspective = 0;
    key.distance = 0.0f;
    key.fov = 0;
    key.eye[0] = key.eye[1] = 0.0f;
    key.target[0] = key.target[1] = 0.0f;
    for (auto& row : key.interpolation)
        std::memset(row, 0, 4);
    key.selected = 0;
}

}  

void InsertBoneCameraFrameLine(MMDApp* app) {
    const std::uint32_t cur = static_cast<std::uint32_t>(app->CurrentFrame());

    if (app->state.optflag[0] == 0) {
        unsigned char* model = ActiveModel(app);
        const std::int32_t boneCount = mdl::Mdl(model)->boneCount;
        mdl::BoneKey* keys = mdl::BoneKeys(model);

        int affected = 0;  
        for (int i = 0; i < static_cast<int>(mdl::kBoneKeyCapacity); ++i) {
            const std::uint32_t f = keys[i].frame;
            if (f != 0 && i >= boneCount && f >= cur) ++affected;
        }
        if (affected == 0) return;

        EnableWindow(GetDlgItem(app->state.hwnd, panel::kUndoButton),
                     TRUE);
        EnableWindow(GetDlgItem(app->state.hwnd, panel::kRedoButton),
                     FALSE);

        BeginUndoEdit(model, cur);                     
        SnapshotPose(model);                           
        AllocUndoKeys(model,                           
                      static_cast<std::size_t>(affected) * 0x40);
        std::memset(mdl::Mdl(model)->keyVisitMap, 0,
                  sizeof(mdl::Mdl(model)->keyVisitMap));  

        for (int i = 0; i < static_cast<int>(mdl::kBoneKeyCapacity); ++i) {             
            mdl::BoneKey& key = keys[i];
            const std::uint32_t f = key.frame;
            if (f != 0 && i >= boneCount && f >= cur) {
                AppendBoneKeyToUndo(model, i);                   
                key.frame = f + 1;
                if (static_cast<std::int32_t>(mdl::Mdl(model)->maxFrame) <
                    static_cast<std::int32_t>(f) + 1)
                    mdl::Mdl(model)->maxFrame =
                        static_cast<std::int32_t>(f) + 1;
            }
        }

        SeekModelFrame(model, static_cast<int>(cur),
                  app->PlaybackPhysicsMode());
        PanelPaint(app);
        SelectionReeval(app);
        app->SceneModified() = 1;
        return;
    }

    auto* keys = app->CameraKeys();
    for (int r = 0; r < 10000; ++r) {
        mdl::CameraKey& key = keys[r];
        const std::uint32_t f = key.frame;
        if (f != 0 && f >= cur) {
            key.frame = f + 1;
            if (app->LastRegisteredFrame() < static_cast<std::int32_t>(f + 1))
                app->LastRegisteredFrame() = static_cast<std::int32_t>(f + 1);
        }
    }
    ReloadModels(app);   
    PanelPaint(app);
    app->SceneModified() = 1;
}

void DeleteBoneCameraFrameLine(MMDApp* app) {
    const std::uint32_t cur = static_cast<std::uint32_t>(app->CurrentFrame());

    if (app->state.optflag[0] == 0) {
        unsigned char* model = ActiveModel(app);
        const std::int32_t boneCount = mdl::Mdl(model)->boneCount;
        mdl::BoneKey* keys = mdl::BoneKeys(model);

        int affected = 0;  
        for (int i = 0; i < static_cast<int>(mdl::kBoneKeyCapacity); ++i) {
            const std::uint32_t f = keys[i].frame;
            if (i >= boneCount && f != 0) {
                if (f == cur) affected += 3;
                else if (cur < f) affected += 1;
            }
        }
        if (affected == 0) return;

        EnableWindow(GetDlgItem(app->state.hwnd, panel::kUndoButton),
                     TRUE);
        EnableWindow(GetDlgItem(app->state.hwnd, panel::kRedoButton),
                     FALSE);

        BeginUndoEdit(model, cur);                     
        SnapshotPose(model);                           
        AllocUndoKeys(model,                           
                      static_cast<std::size_t>(affected) * 0x40);
        std::memset(mdl::Mdl(model)->keyVisitMap, 0,
                  sizeof(mdl::Mdl(model)->keyVisitMap));  

        const bool autoInterp =
            SendMessageA(GetDlgItem(app->state.hwnd,
                                    panel::kPhysicsFrameCheckbox),
                         BM_GETCHECK, 0, 0) == 1;

        for (int i = 0; i < static_cast<int>(mdl::kBoneKeyCapacity); ++i) {             
            mdl::BoneKey& key = keys[i];
            const std::uint32_t f = key.frame;

            if (f != 0 && i >= boneCount && f == cur) {
                const std::int32_t prev = static_cast<std::int32_t>(key.previous);
                const std::int32_t next = static_cast<std::int32_t>(key.next);
                AppendBoneKeyToUndo(model, prev);                
                AppendBoneKeyToUndo(model, i);                   
                AppendBoneKeyToUndo(model, next);                
                keys[prev].next = next;
                keys[next].previous = prev;
                ClearBoneKeyRecord(key);               
                if (autoInterp) {                      
                    for (int lane = 0; lane < 4; ++lane) {
                        const std::int32_t n =
                            static_cast<std::int32_t>(key.next);
                        RebuildBoneKeyInterpolation(model, n == 0
                                          ? static_cast<std::int32_t>(
                                                key.previous) : n,
                                  lane);
                    }
                }
                key.next = 0;
                key.previous = 0;
                continue;
            }

            if (f != 0 && i >= boneCount && cur < f) {
                AppendBoneKeyToUndo(model, i);                   
                const std::uint32_t nf = f - 1;
                key.frame = nf;
                if (nf == 0) {
                    const std::int32_t prev =
                        static_cast<std::int32_t>(key.previous);
                    mdl::BoneKey& dst = keys[prev];
                    dst.next = key.next;
                    CopyBoneKeyPayload(dst, key);
                    const std::int32_t next =
                        static_cast<std::int32_t>(key.next);
                    if (next != 0)
                        keys[next].previous = prev;
                    if (autoInterp) {                  
                        for (int lane = 0; lane < 4; ++lane) {
                            const std::int32_t n =
                                static_cast<std::int32_t>(key.next);
                            RebuildBoneKeyInterpolation(model,
                                      n == 0
                                          ? static_cast<std::int32_t>(
                                                key.previous) : n,
                                      lane);
                        }
                    }
                    ClearBoneKeyRecord(key);           
                }
            }
        }

        SeekModelFrame(model, static_cast<int>(cur),
                  app->PlaybackPhysicsMode());
        PanelPaint(app);
        SelectionReeval(app);
        app->SceneModified() = 1;
        return;
    }

    auto* keys = app->CameraKeys();
    for (int r = 0; r < 10000; ++r) {
        mdl::CameraKey& key = keys[r];
        const std::uint32_t f = key.frame;

        if (f != 0 && f == cur) {
            const std::int32_t prev = key.previous;
            const std::int32_t next = key.next;
            keys[prev].next = next;
            keys[next].previous = prev;
            ClearCameraKeyRecord(key);
            continue;
        }

        const std::uint32_t f2 = key.frame;
        if (f2 != 0 && cur < f2) {
            const std::uint32_t nf = f2 - 1;
            key.frame = nf;
            if (nf == 0) {
                const std::int32_t prev = key.previous;
                mdl::CameraKey& dst = keys[prev];
                dst.next = key.next;
                CopyCameraKeyPayload(dst, key);
                const std::int32_t next = key.next;
                if (next != 0)
                    keys[next].previous = prev;
            }
        }
    }
    ReloadModels(app);
    PanelPaint(app);
    app->SceneModified() = 1;
}

void InsertFacialLightFrameLine(MMDApp* app) {
    const std::uint32_t cur = static_cast<std::uint32_t>(app->CurrentFrame());

    if (app->state.optflag[0] != 0) {
        auto* keys = app->LightKeys();
        for (int r = 0; r < 10000; ++r) {
            mdl::LightKey& key = keys[r];
            const std::uint32_t f = key.frame;
            if (f != 0 && f >= cur) {
                key.frame = f + 1;
                if (app->LastRegisteredFrame() < static_cast<std::int32_t>(f + 1))
                    app->LastRegisteredFrame() = static_cast<std::int32_t>(f + 1);
            }
        }
        RefreshLightPanel(app);
        PanelPaint(app);
        app->SceneModified() = 1;
        return;
    }

    unsigned char* model = ActiveModel(app);
    const std::int32_t morphCount = mdl::Mdl(model)->morphCount;
    mdl::MorphKey* keys = mdl::MorphKeys(model);
    for (int i = 0; i < 20000; ++i) {
        mdl::MorphKey& key = keys[i];
        const std::uint32_t f = key.frame;
        if (f != 0 && i >= morphCount && f >= cur) {
            key.frame = f + 1;
            if (static_cast<std::int32_t>(mdl::Mdl(model)->maxFrame) <
                static_cast<std::int32_t>(f) + 1)
                mdl::Mdl(model)->maxFrame =
                    static_cast<std::int32_t>(f) + 1;
        }
    }
    SeekModelFrame(model, static_cast<int>(cur),
              app->PlaybackPhysicsMode());
    PanelPaint(app);
    SelectionReeval(app);
    app->SceneModified() = 1;
}

void DeleteFacialLightFrameLine(MMDApp* app) {
    const std::uint32_t cur = static_cast<std::uint32_t>(app->CurrentFrame());

    if (app->state.optflag[0] == 0) {
        unsigned char* model = ActiveModel(app);
        const std::int32_t morphCount = mdl::Mdl(model)->morphCount;
        mdl::MorphKey* keys = mdl::MorphKeys(model);
        for (int i = 0; i < 20000; ++i) {
            mdl::MorphKey& key = keys[i];
            const std::uint32_t f = key.frame;

            if (f != 0 && i >= morphCount && f == cur) {
                const std::int32_t prev = key.previous;
                const std::int32_t next = key.next;
                keys[prev].next = next;
                keys[next].previous = prev;
                key.frame = 0;
                key.allocated = 0;
                key.value = 0.0f;
                key.next = 0;
                key.previous = 0;
                continue;
            }

            if (f != 0 && i >= morphCount && cur < f) {
                const std::uint32_t nf = f - 1;
                key.frame = nf;
                if (nf == 0) {
                    const std::int32_t prev = key.previous;
                    mdl::MorphKey& dst = keys[prev];
                    dst.next = key.next;
                    dst.allocated = key.allocated;
                    std::memcpy(dst.reserved, key.reserved,
                                sizeof(dst.reserved));
                    dst.value = key.value;
                    const std::int32_t next = key.next;
                    if (next != 0)
                        keys[next].previous = prev;
                }
            }
        }
        SeekModelFrame(model, static_cast<int>(cur),
                  app->PlaybackPhysicsMode());
        PanelPaint(app);
        SelectionReeval(app);
        app->SceneModified() = 1;
        return;
    }

    auto* keys = app->LightKeys();
    for (int r = 0; r < 10000; ++r) {
        mdl::LightKey& key = keys[r];
        const std::uint32_t f = key.frame;

        if (f != 0 && f == cur) {
            const std::int32_t prev = key.previous;
            const std::int32_t next = key.next;
            keys[prev].next = next;
            keys[next].previous = prev;
            key.next = 0;
            key.previous = 0;
            key.frame = 0;
            key.selected = 0;
            std::memset(key.reserved1, 0, sizeof(key.reserved1));
            continue;
        }

        const std::uint32_t f2 = key.frame;
        if (f2 != 0 && cur < f2) {
            const std::uint32_t nf = f2 - 1;
            key.frame = nf;
            if (nf == 0) {
                const std::int32_t prev = key.previous;
                mdl::LightKey& dst = keys[prev];
                dst.next = key.next;
                std::memcpy(dst.direction, key.direction,
                            sizeof(dst.direction));
                std::memcpy(dst.color, key.color, sizeof(dst.color));
                const std::int32_t next = key.next;
                if (next != 0)
                    keys[next].previous = prev;
            }
        }
    }
    RefreshLightPanel(app);
    PanelPaint(app);
    app->SceneModified() = 1;
}

void SelectFrameGroup(MMDApp* app, int group) {
    unsigned char* model = ActiveModel(app);

    if (group == 0) {
        mdl::BoneKey* keys = mdl::BoneKeys(model);
        const std::int32_t boneCount = mdl::Mdl(model)->boneCount;
        for (int i = 0; i < static_cast<int>(mdl::kBoneKeyCapacity); ++i) {
            mdl::BoneKey& key = keys[i];
            key.allocated = 0;
            if (i < boneCount || key.frame != 0) {
                key.allocated = 1;
            }
        }
    } else if (group == 1) {
        mdl::DisplayKey* frames = mdl::DisplayKeys(model);
        for (int i = 0; i < 1000; ++i) {
            frames[i].allocated = 0;
            if (frames[i].frame != 0) {
                frames[i].allocated = 1;
            }
        }
        frames[0].allocated = 1;
    } else {
        mdl::MorphKey* frames = mdl::MorphKeys(model);
        const std::int32_t morphCount = mdl::Mdl(model)->morphCount;
        for (int i = 0; i < 20000; ++i) {
            frames[i].allocated = 0;
            if (i < morphCount || frames[i].frame != 0) {
                frames[i].allocated = 1;
            }
        }
    }

    PanelPaint(app);
    SelectionReeval(app);
}

}  
