#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <unknwn.h>

#include <cstdint>
#include <new>

#include "btBulletDynamicsCommon.h"

#include "rinmotiondance/model.hpp"
#include "rinmotiondance/physics_scene.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

template <typename T>
inline void DeleteOwnedStorage(T*& pointer) {
    ::operator delete(pointer);
    pointer = nullptr;
}

inline void ReleaseCom(void*& storage) {
    IUnknown*& object = mdl::ResourceAs<IUnknown>(storage);
    if (object != nullptr) {
        object->Release();
        object = nullptr;
    }
}

void ReleasePhysConstraint(PhysicsScene* physics, int constraintId) {
    if (physics == nullptr || physics->world == nullptr)
        return;
    btDiscreteDynamicsWorld* const world = physics->world;
    for (int i = world->getNumConstraints() - 1; i >= 0; --i) {
        btTypedConstraint* const item = world->getConstraint(i);
        if (item != nullptr && item->getUid() == constraintId) {
            world->removeConstraint(item);
            delete item;
            return;
        }
    }
}

void ReleasePhysRigid(PhysicsScene* physics, void* bodyPointer) {
    auto* body = static_cast<btRigidBody*>(bodyPointer);
    if (physics == nullptr || physics->world == nullptr || body == nullptr)
        return;
    physics->world->removeRigidBody(body);
    delete body->getMotionState();
    delete body->getCollisionShape();
    delete body;
}

}  

void ModelDispose(unsigned char* m) {
    mdl::ModelRecord& model = *mdl::Mdl(m);
    PhysicsScene* scene = model.scenePtr;

    if (model.jointCount > 0) {
        for (std::uint32_t i = 0; i < model.jointCount; ++i) {
            mdl::JointRecord* jt = &model.jointTable[i];
            ReleasePhysConstraint(scene, jt->constraint);
            DeleteOwnedStorage(jt->jpText);
            DeleteOwnedStorage(jt->enText);
        }
    }
    DeleteOwnedStorage(model.jointTable);

    if (model.rigidCount > 0) {
        for (std::uint32_t i = 0; i < model.rigidCount; ++i) {
            mdl::RigidRecord* rb = &model.rigidTable[i];
            ReleasePhysRigid(scene, rb->body);
            rb->body = nullptr;
            DeleteOwnedStorage(rb->jpText);
            DeleteOwnedStorage(rb->enText);
        }
    }
    DeleteOwnedStorage(model.rigidTable);

    for (auto& ring : rinmotiondance::mdl::Mdl(m)->undoRings) {
        for (auto& undo : ring.slots) {
            if (undo.auxiliaryPose != nullptr) {
                ::operator delete(undo.auxiliaryPose);
                undo.auxiliaryPose = nullptr;
            }
            if (undo.bonePose != nullptr) {
                ::operator delete(undo.bonePose);
                undo.bonePose = nullptr;
            }
        }
    }

    DeleteOwnedStorage(mdl::MorphKeyIndices(m));
    DeleteOwnedStorage(mdl::BoneKeyIndices(m));

    if (mdl::DisplayKeys(m) != nullptr) {
        for (int i = 0; i < 1000; ++i) {
            DeleteOwnedStorage(mdl::IkStates(mdl::DisplayKeys(m)[i]));
            DeleteOwnedStorage(mdl::SelectorStates(mdl::DisplayKeys(m)[i]));
        }
    }
    ::operator delete(mdl::DisplayKeys(m));
    mdl::DisplayKeys(m) = nullptr;
    ::operator delete(mdl::MorphKeys(m));
    mdl::MorphKeys(m) = nullptr;
    ::operator delete(mdl::BoneKeys(m));
    mdl::BoneKeys(m) = nullptr;

    DeleteOwnedStorage(mdl::Mdl(m)->boneSelection);
    DeleteOwnedStorage(mdl::Mdl(m)->bonePhysicsState);
    DeleteOwnedStorage(mdl::Mdl(m)->rbGroups);
    DeleteOwnedStorage(mdl::Mdl(m)->groupNames);
    DeleteOwnedStorage(mdl::Mdl(m)->displayFrames);

    if (model.morphs != nullptr && model.morphCount > 0) {
        const std::uint32_t n = model.morphCount;
        for (std::uint32_t i = 0; i < n; ++i) {
            mdl::MorphRecord& morph = model.morphs[i];
            DeleteOwnedStorage(morph.jpText);
            DeleteOwnedStorage(morph.enText);
            DeleteOwnedStorage(morph.vertexEntries);
            for (auto& entries : morph.uvEntries)
                DeleteOwnedStorage(entries);
            DeleteOwnedStorage(morph.boneEntries);
            DeleteOwnedStorage(morph.groupEntries);
            DeleteOwnedStorage(morph.materialEntries);
        }
    }

    DeleteOwnedStorage(mdl::BaseVertexMorphTable(m));
    for (auto& table : mdl::UvMorphTables(m).byFamily)
        DeleteOwnedStorage(table);
    DeleteOwnedStorage(mdl::BoneMorphOffsets(m));
    DeleteOwnedStorage(model.reservedMorphTable);
    DeleteOwnedStorage(mdl::MaterialMorphBase(m));
    DeleteOwnedStorage(mdl::MaterialMorphAdd(m));
    DeleteOwnedStorage(mdl::MaterialMorphMul(m));

    DeleteOwnedStorage(model.morphs);

    if (mdl::Mdl(m)->ikChains != nullptr &&
        rinmotiondance::mdl::Mdl(m)->ikChainCount > 0) {
        const int n = rinmotiondance::mdl::Mdl(m)->ikChainCount;
        for (int i = 0; i < n; ++i)
            DeleteOwnedStorage(mdl::IkChains(m)[i].links);
    }
    DeleteOwnedStorage(mdl::Mdl(m)->ikChains);

    DeleteOwnedStorage(mdl::Mdl(m)->morphKeyCursors);
    DeleteOwnedStorage(mdl::Mdl(m)->morphTrackActive);
    DeleteOwnedStorage(mdl::Mdl(m)->boneKeyCursors);
    DeleteOwnedStorage(mdl::Mdl(m)->boneTrackActive);

    if (rinmotiondance::mdl::Mdl(m)->boneCount > 0) {
        rinmotiondance::mdl::BoneRecord* bones = mdl::Mdl(m)->boneTable;
        for (std::uint32_t i = 0; i < rinmotiondance::mdl::Mdl(m)->boneCount; ++i) {
            rinmotiondance::mdl::BoneRecord* bone = &bones[i];
            DeleteOwnedStorage(bone->jpText);
            DeleteOwnedStorage(bone->enText);
            DeleteOwnedStorage(bone->ikLinks);
        }
    }
    DeleteOwnedStorage(mdl::Mdl(m)->boneTable);

    DeleteOwnedStorage(mdl::Mdl(m)->materials);
    DeleteOwnedStorage(mdl::Mdl(m)->indices);
    DeleteOwnedStorage(mdl::Mdl(m)->rawVertices);
    DeleteOwnedStorage(mdl::Mdl(m)->pmxVertices);
    delete[] mdl::Mdl(m)->boneOrderTable;
    mdl::Mdl(m)->boneOrderTable = nullptr;

    ReleaseCom(mdl::Mdl(m)->vertexBuffer2);
    ReleaseCom(mdl::Mdl(m)->vertexBuffer);
    ReleaseCom(mdl::Mdl(m)->indexBuffer);

    DeleteOwnedStorage(mdl::PmxTextBuffer(m, mdl::PmxTextBufferSlot::japaneseName));
    DeleteOwnedStorage(mdl::PmxTextBuffer(m, mdl::PmxTextBufferSlot::englishName));
    DeleteOwnedStorage(mdl::PmxTextBuffer(m, mdl::PmxTextBufferSlot::japaneseComment));
    DeleteOwnedStorage(mdl::PmxTextBuffer(m, mdl::PmxTextBufferSlot::englishComment));
}

void DeleteModel(unsigned char* model, int freeFlag) {
    ModelDispose(model);                                    
    if ((freeFlag & 1) != 0)
        ::operator delete(model);                                   
}

}  
