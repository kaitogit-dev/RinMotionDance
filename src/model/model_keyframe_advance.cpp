#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include "btBulletDynamicsCommon.h"

#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"

#include "keyframe_common.hpp"

namespace rinmotiondance {
namespace {

using kfa::CopyIkDisplayFlags;
using kfa::RefreshIkSelectors;
using kfa::CopyBoneKeyVerbatim;
using kfa::WriteBonePrevVerbatim;
using kfa::MirrorBackupsToWorking;
using kfa::MirrorBackupsToCurrent;

constexpr double kPiHalfBits = static_cast<double>(3.141592f) * 0.5;
static_assert(__builtin_bit_cast(std::uint64_t, kPiHalfBits) ==
                  0x3FF921FB00000000ULL,
              "pi/2 double constant bit-exact");

double PiHalfBits() {
    return kPiHalfBits;
}

const float kQuatClamp = 0.999999f;

const double kCpScale = 0.02362200058996677;
const float kCpScaleF = 0.02362200058996677f;

}  

float BoneEase(unsigned char* model, int channel, int keyIdx, float t) {
    const mdl::BoneKey& rec = mdl::BoneKeys(model)[keyIdx];
    const unsigned char x1 = rec.interpolation[channel];
    const unsigned char y1 = rec.interpolation[channel + 4];
    const unsigned char x2 = rec.interpolation[channel + 8];
    const unsigned char y2 = rec.interpolation[channel + 12];
    if (x2 == y2 && x1 == y1) return t;  

#if defined(_M_IX86)
    const double x1c = static_cast<double>(
                           static_cast<signed char>(x1)) * kCpScale;
    const double x2c = static_cast<double>(
                           static_cast<signed char>(x2)) * kCpScale;

    float u = 0.5f, step = 0.5f;
    for (int i = 0; i < 12; ++i) {
        const float om = 1.0f - u;
        const double xu = (double)om * (double)om * (double)u * x1c +
                          (double)om * (double)u * (double)u * x2c +
                          (double)u * (double)u * (double)u;
        if (xu == (double)t) break;
        step *= 0.5f;
        u = xu >= (double)t ? (float)(u - step) : (float)(u + step);
    }

    const float om = 1.0f - u;
    const double y1c = static_cast<double>(
                           static_cast<signed char>(y1)) * kCpScale;
    const double y2c = static_cast<double>(
                           static_cast<signed char>(y2)) * kCpScale;
    return static_cast<float>(
        (double)u * ((double)u * (double)u) +
        (double)om * (double)om * (double)u * y1c +
        (double)om * (double)u * (double)u * y2c);
#else
    const float x1c = static_cast<float>(
                          static_cast<signed char>(x1)) * kCpScaleF;
    const float x2c = static_cast<float>(
                          static_cast<signed char>(x2)) * kCpScaleF;
    const float y1b = static_cast<float>(
        static_cast<signed char>(y1));
    const float y2b = static_cast<float>(
        static_cast<signed char>(y2));

    float u = 0.5f, step = 0.5f;
    for (int i = 0; i < 12; ++i) {
        const float om = 1.0f - u;
        const float gx = (om * om * u) * x1c + (om * u * u) * x2c +
                         u * u * u;
        if (gx == t) break;                              
        step *= 0.5f;
        u = gx >= t ? u - step : u + step;
    }

    const float om = 1.0f - u;
    return (om * om * u) * y1b * kCpScaleF + (om * u * u) * y2b * kCpScaleF +
           u * u * u;
#endif
}

void NotifyBonePhysicsMode(unsigned char* model, int boneIdx, unsigned char mode) {
    rinmotiondance::mdl::RigidRecord* const rigids = rinmotiondance::mdl::Rigids(model);
    const int cnt = static_cast<int>(mdl::Mdl(model)->rigidCount);
    for (int i = 0; i < cnt; ++i) {
        rinmotiondance::mdl::RigidRecord* const r = &rigids[i];
        if (r->staticFlag != 0 || r->boneIndex != boneIdx) continue;
        btRigidBody* const body = static_cast<btRigidBody*>(r->body);
        if (body == nullptr)
            continue;

        const int flags = body->getCollisionFlags();
        if (mode != 0) {
            body->setCollisionFlags(
                flags | btCollisionObject::CF_KINEMATIC_OBJECT);
            r->mode = 0;
        } else {
            body->setCollisionFlags(
                flags & ~btCollisionObject::CF_KINEMATIC_OBJECT);
            r->mode = r->kinematicFlag != 0 ? 2 : 1;
        }
    }
}

void InitModelTrackCursors(unsigned char* model, float cursor, int physicsMode) {
    unsigned char* const m = model;
    const double frame = static_cast<double>(cursor) * 30.0;

    mdl::DisplayKey* const ikKeys = mdl::DisplayKeys(m);
    mdl::ModelRecord& state = *mdl::Mdl(m);
    state.displayTrackActive = 1;
    state.displayKeyCursor = 0;
    if (static_cast<double>(ikKeys[0].frame) < frame) {
        for (;;) {
            const int cur = static_cast<int>(state.displayKeyCursor);
            const int next = static_cast<int>(ikKeys[cur].next);
            if (next == 0) {
                state.displayTrackActive = 0;
                const mdl::DisplayKey& rec = ikKeys[cur];
                rinmotiondance::mdl::Mdl(m)->loadComplete = rec.visible;
                kfa::CopyIkDisplayFlags(m, rec);
                kfa::RefreshIkSelectors(m, ikKeys, cur, cur, false, 0, 0);
                break;
            }
            state.displayKeyCursor = static_cast<std::uint32_t>(next);
            if (static_cast<double>(ikKeys[next].frame) >= frame)
                break;
        }
    }

    const int morphCount = static_cast<int>(mdl::Mdl(m)->morphCount);
    mdl::MorphKey* const morphKeys = mdl::MorphKeys(m);
    rinmotiondance::mdl::MorphRecord* const morphValues = rinmotiondance::mdl::Morphs(m);
    unsigned char* const morphActive = state.morphTrackActive;
    std::uint32_t* const morphCursor = state.morphKeyCursors;
    for (int morph = 0; morph < morphCount; ++morph) {
        morphActive[morph] = 1;
        morphCursor[morph] = static_cast<std::uint32_t>(morph);
        for (;;) {
            const int cur = static_cast<int>(morphCursor[morph]);
            if (static_cast<double>(morphKeys[cur].frame) >= frame)
                break;
            const int next = static_cast<int>(morphKeys[cur].next);
            if (next != 0) {
                morphCursor[morph] = static_cast<std::uint32_t>(next);
                continue;
            }
            morphActive[morph] = 0;
            morphValues[morph].value = morphKeys[cur].value;
            break;
        }
    }

    const int boneCount = static_cast<int>(mdl::Mdl(m)->boneCount);
    mdl::BoneKey* const boneKeys = mdl::BoneKeys(m);
    rinmotiondance::mdl::BoneRecord* const bones = mdl::Bones(m);
    unsigned char* const boneActive = state.boneTrackActive;
    std::uint32_t* const boneCursor = state.boneKeyCursors;
    for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex) {
        rinmotiondance::mdl::BoneRecord* const bone = &bones[boneIndex];
        boneActive[boneIndex] = 1;
        if (bone->type == mdl::BoneType::InertTip) {
            boneActive[boneIndex] = 0;
            continue;
        }
        boneCursor[boneIndex] = static_cast<std::uint32_t>(boneIndex);
        for (;;) {
            const int cur = static_cast<int>(boneCursor[boneIndex]);
            const mdl::BoneKey& rec = boneKeys[cur];
            if (static_cast<double>(rec.frame) >= frame)
                break;
            const int next = static_cast<int>(rec.next);
            if (next != 0) {
                boneCursor[boneIndex] = static_cast<std::uint32_t>(next);
                continue;
            }
            boneActive[boneIndex] = 0;
            std::memcpy(bone->rotQuat, rec.rotation, sizeof rec.rotation);
            std::memcpy(bone->trans, rec.position, sizeof rec.position);
            break;
        }

        const int cur = static_cast<int>(boneCursor[boneIndex]);
        const unsigned char mode = boneKeys[cur].physicsDisabled;
        if (bone->hasRigidBody != 0) {
            if (physicsMode >= 2)
                NotifyBonePhysicsMode(m, boneIndex, mode);
            bone->physicsDisabled = mode;
        }
    }
}

void AdvanceModelKeyframes(unsigned char* model, float cursor, int physicsMode) {
    unsigned char* const m = model;
    mdl::ModelRecord& state = *mdl::Mdl(m);

    double frame = static_cast<double>(cursor) * 30.0;
    {
        const double scaled = frame * 1000.0;
        if (scaled - static_cast<double>(static_cast<int>(scaled)) >= 0.5)
            frame = static_cast<double>(static_cast<int>(scaled) + 1) / 1000.0;
    }

    if (state.displayTrackActive != 0) {
        mdl::DisplayKey* const keys = mdl::DisplayKeys(m);
        while ((double)keys[state.displayKeyCursor].frame < frame &&
               state.displayTrackActive != 0) {
            const int cur = static_cast<int>(state.displayKeyCursor);
            const mdl::DisplayKey& rec = keys[cur];
            const int next = static_cast<int>(rec.next);
            if (next != 0) {
                state.displayKeyCursor = static_cast<std::uint32_t>(next);
                continue;
            }
            state.displayTrackActive = 0;
            rinmotiondance::mdl::Mdl(m)->loadComplete = rec.visible;
            CopyIkDisplayFlags(m, rec);
            RefreshIkSelectors(m, keys, cur, cur, false, 0, 0);
        }
        if (state.displayTrackActive != 0) {
            const int cur = static_cast<int>(state.displayKeyCursor);
            const mdl::DisplayKey& rec = keys[cur];
            if (frame == (double)rec.frame) {
                rinmotiondance::mdl::Mdl(m)->loadComplete = rec.visible;
                CopyIkDisplayFlags(m, rec);
                RefreshIkSelectors(m, keys, cur, cur, true, cur, cur);
            } else {
                const int prevIdx = static_cast<int>(rec.previous);
                const mdl::DisplayKey& prec = keys[prevIdx];
                rinmotiondance::mdl::Mdl(m)->loadComplete = prec.visible;
                CopyIkDisplayFlags(m, prec);
                RefreshIkSelectors(m, keys, prevIdx, prevIdx, true, cur, cur);
            }
        }
    }

    const int morphCnt = static_cast<int>(mdl::Mdl(m)->morphCount);
    if (morphCnt > 0) {
        mdl::MorphKey* const keys = mdl::MorphKeys(m);
        rinmotiondance::mdl::MorphRecord* const vals = rinmotiondance::mdl::Morphs(m);  
        unsigned char* const active = state.morphTrackActive;
        std::uint32_t* const cursors = state.morphKeyCursors;
        for (int k = 0; k < morphCnt; ++k) {
            if (active[k] == 0) continue;
            while ((double)keys[cursors[k]].frame < frame &&
                   active[k] != 0) {
                const mdl::MorphKey& rec = keys[cursors[k]];
                const int next = static_cast<int>(rec.next);
                if (next != 0)
                    cursors[k] = next;
                else {
                    active[k] = 0;
                    vals[k].value = rec.value;
                }
            }
            if (active[k] == 0) continue;
            const mdl::MorphKey& rec = keys[cursors[k]];
            float v;
            if (frame == (double)rec.frame) {
                v = rec.value;
            } else {
                const mdl::MorphKey& prev = keys[rec.previous];
#if defined(_M_IX86)
                const float tF = (float)((frame - (double)prev.frame) /
                                         (double)(std::uint32_t)(
                                             rec.frame - prev.frame));
                const float delta = rec.value - prev.value;
                v = (float)((double)tF * (double)delta +
                            (double)prev.value);
#else
                const float tF =
                    (static_cast<float>(frame) -
                     static_cast<float>(prev.frame)) /
                    static_cast<float>(static_cast<int>(rec.frame) -
                                       static_cast<int>(prev.frame));
                v = tF * (rec.value - prev.value) + prev.value;
#endif
            }
            vals[k].value = v;
        }
    }

    const int boneCnt = static_cast<int>(mdl::Mdl(m)->boneCount);
    if (boneCnt > 0) {
        mdl::BoneKey* const keys = mdl::BoneKeys(m);
        rinmotiondance::mdl::BoneRecord* const bones =
            mdl::Bones(m);
        unsigned char* const active = state.boneTrackActive;
        std::uint32_t* const cursors = state.boneKeyCursors;
        for (int b = 0; b < boneCnt; ++b) {
            rinmotiondance::mdl::BoneRecord* const bone = &bones[b];
            if (active[b] == 0) continue;

#ifdef RINMOTIONDANCE_DIAG
            int spin = 0;
#endif
            while ((double)keys[cursors[b]].frame < frame &&
                   active[b] != 0) {
#ifdef RINMOTIONDANCE_DIAG
                if (++spin > 100000 && getenv("RINMOTIONDANCE_SPIN_CANARY")) {
                    FILE* sf = fopen(getenv("RINMOTIONDANCE_SPIN_CANARY"), "a");
                    if (sf) {
                        fprintf(sf,
                                "BONE spin b=%d cursor=%d frame=%.3f\n",
                                b, cursors[b], frame);
                        fclose(sf);
                    }
                    spin = 0;
                }
#endif
                const mdl::BoneKey& rec = keys[cursors[b]];
                const int next = static_cast<int>(rec.next);
                if (next != 0) {
                    cursors[b] = next;
                    continue;
                }
                active[b] = 0;
                for (int c = 0; c < 4; ++c)
                    bone->rotQuat[c] = rec.rotation[c];  
                for (int c = 0; c < 3; ++c)
                    bone->trans[c] = rec.position[c];
                const unsigned char mode = rec.physicsDisabled;
                if (bone->hasRigidBody != 0 && bone->physicsDisabled != mode &&
                    physicsMode >= 2) {
                    NotifyBonePhysicsMode(m, b, mode);  
                    bone->physicsDisabled = rec.physicsDisabled;
                }
                const int selIdx = bone->slotIndex;
                if (selIdx < 0) {
                    active[b] = 0;
                } else {
                    const mdl::BoneOrderEntry& window =
                        mdl::BoneOrder(m)[selIdx];
                    const std::uint32_t selStart = window.windowStart;
                    if (rec.frame < selStart &&
                        (double)selStart <= frame) {
                        for (int c = 0; c < 3; ++c)
                            bone->trans[c] = 0.0f;
                        for (int c = 0; c < 3; ++c)
                            bone->rotQuat[c] = 0.0f;
                    }
                    if (window.windowEnd == 0) active[b] = 0;
                }
                break;
            }
            if (active[b] == 0) continue;

            const int cursorIdx = cursors[b];
            const mdl::BoneKey& rec = keys[cursorIdx];
            const std::uint32_t curFrame = rec.frame;

            if (frame == (double)curFrame) {
                CopyBoneKeyVerbatim(bone, rec);
                if (physicsMode >= 2 && bone->hasRigidBody != 0) {  
                    const int nextIdx = static_cast<int>(rec.next);
                    const unsigned char cmode = rec.physicsDisabled;
                    if (nextIdx <= 0) {
                        if (bone->physicsDisabled != cmode) NotifyBonePhysicsMode(m, b, cmode);
                        bone->physicsDisabled = cmode;
                        continue;
                    }
                    const mdl::BoneKey& nrec = keys[nextIdx];
                    if (nrec.physicsDisabled != 1 || cmode != 0) {
                        if (bone->physicsDisabled != cmode) NotifyBonePhysicsMode(m, b, cmode);
                        bone->physicsDisabled = cmode;
                        continue;
                    }
                    if (bone->physicsDisabled == 0) NotifyBonePhysicsMode(m, b, 1);
                    bone->physicsDisabled = 1;
                    bone->rigidIdx = static_cast<std::int32_t>(curFrame);
                    for (int c = 0; c < 4; ++c)
                        bone->ikWorkingQuat[c] = bone->ikBackup[3 + c];
                    for (int c = 0; c < 3; ++c)
                        bone->ikWorkingPos[c] = bone->ikBackup[c];
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] = bone->ikBackup[3 + c];  
                    for (int c = 0; c < 3; ++c)
                        bone->trans[c] = bone->ikBackup[c];
                }
                continue;
            }

            const int prevIdx = static_cast<int>(rec.previous);
            const mdl::BoneKey& prevRec = keys[prevIdx];
            std::uint32_t prevFrame = prevRec.frame;
            float ppos[3] = {prevRec.position[0], prevRec.position[1],
                             prevRec.position[2]};
            float pq[4] = {prevRec.rotation[0], prevRec.rotation[1],
                           prevRec.rotation[2], prevRec.rotation[3]};

            if (physicsMode >= 2 && bone->hasRigidBody != 0) {  
                if (rec.physicsDisabled == 1 &&
                    prevRec.physicsDisabled == 0) {
                    const std::uint32_t start =
                        static_cast<std::uint32_t>(bone->rigidIdx);
                    if (start < prevFrame || start >= curFrame) {
                        if (bone->physicsDisabled == 0) NotifyBonePhysicsMode(m, b, 1);
                        bone->physicsDisabled = 1;
                        bone->rigidIdx = static_cast<std::int32_t>(prevFrame);
                        std::memcpy(bone->ikWorkingQuat, bone->ikBackup + 3,
                                    sizeof(bone->ikWorkingQuat));
                        std::memcpy(bone->ikWorkingPos, bone->ikBackup,
                                    sizeof(bone->ikWorkingPos));
                    }
                    if (bone->physicsDisabled == 0) NotifyBonePhysicsMode(m, b, 1);
                    bone->physicsDisabled = 1;
                    pq[0] = bone->ikWorkingQuat[0];
                    pq[1] = bone->ikWorkingQuat[1];
                    pq[2] = bone->ikWorkingQuat[2];
                    pq[3] = bone->ikWorkingQuat[3];
                    ppos[0] = bone->ikWorkingPos[0];
                    ppos[1] = bone->ikWorkingPos[1];
                    ppos[2] = bone->ikWorkingPos[2];
                    prevFrame = static_cast<std::uint32_t>(
                        bone->rigidIdx);
                } else {
                    const unsigned char pmode = prevRec.physicsDisabled;
                    if (bone->physicsDisabled != pmode) NotifyBonePhysicsMode(m, b, pmode);
                    bone->physicsDisabled = pmode;
                }
            }

            bool fullInterp = true;
            const int selIdx = bone->slotIndex;
            if (selIdx >= 0) {
                const mdl::BoneOrderEntry& window =
                    mdl::BoneOrder(m)[selIdx];
                const std::uint32_t selStart = window.windowStart;
                const std::uint32_t selEnd = window.windowEnd;
                if (selStart < curFrame && prevFrame < selStart) {
                    if ((double)selStart <= frame) {
                        CopyBoneKeyVerbatim(bone, rec);
                        continue;
                    }
                    WriteBonePrevVerbatim(bone, pq, ppos);  
                    continue;
                }
                if (curFrame < selEnd || prevFrame >= selEnd) {
                    fullInterp = true;  
                } else {
                    if ((double)selEnd <= frame) {
                        CopyBoneKeyVerbatim(bone, rec);
                        continue;
                    }
                    WriteBonePrevVerbatim(bone, pq, ppos);  
                    continue;
                }
            }
            (void)fullInterp;  

            const float cq[4] = {rec.rotation[0], rec.rotation[1],
                                 rec.rotation[2], rec.rotation[3]};
#if defined(_M_IX86)
            const float tF =
                (float)((frame - (double)prevFrame) /
                        (double)(curFrame - prevFrame));
#else
            const float tF =
                (static_cast<float>(frame) - static_cast<float>(prevFrame)) /
                static_cast<float>(static_cast<int>(curFrame) -
                                   static_cast<int>(prevFrame));
#endif
            const float eRot = BoneEase(m, 3, cursorIdx, tF);

#if defined(_M_IX86)
            const double dotD = (double)pq[0] * cq[0] +
                                (double)pq[1] * cq[1] +
                                (double)pq[2] * cq[2] +
                                (double)pq[3] * cq[3];
            const float dot = (float)dotD;
            if (static_cast<float>(1.0 - (double)dot * (double)dot) ==
                0.0f) {
                for (int c = 0; c < 4; ++c) bone->rotQuat[c] = pq[c];
            } else {
                float dc = dot;
                if (dc > 1.0f)
                    dc = kQuatClamp;
                else if (dc < -1.0f)
                    dc = -kQuatClamp;
                const float th = (float)std::acos((double)dc);
                if ((double)th > PiHalfBits() && (double)dc < 0.0) {
                    const float th2 = (float)std::acos(-(double)dc);
                    const float invS =
                        (float)(1.0 / (double)(float)std::sin((double)th2));
                    const float aStart =
                        (float)((1.0 - (double)eRot) * (double)th2);
                    const float w0 =
                        (float)std::sin((double)aStart) * invS;
                    const float aEnd = (float)((double)eRot * (double)th2);
                    const float w1 =
                        (float)std::sin((double)aEnd) * invS;
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] =
                            (float)((double)pq[c] * (double)w0 -
                                    (double)w1 * (double)cq[c]);
                } else {
                    const float invS =
                        (float)(1.0 / (double)(float)std::sin((double)th));
                    const float aStart =
                        (float)((1.0 - (double)eRot) * (double)th);
                    const float w0 =
                        (float)std::sin((double)aStart) * invS;
                    const float aEnd = (float)((double)eRot * (double)th);
                    const float w1 =
                        (float)std::sin((double)aEnd) * invS;
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] =
                            (float)((double)pq[c] * (double)w0 +
                                    (double)w1 * (double)cq[c]);
                }
            }
#else
            const float dot = pq[0] * cq[0] + pq[1] * cq[1] +
                              pq[2] * cq[2] + pq[3] * cq[3];
            if (1.0f - dot * dot == 0.0f) {
                for (int c = 0; c < 4; ++c) bone->rotQuat[c] = pq[c];
            } else {
                float dc = dot;
                if (dc > 1.0f)
                    dc = kQuatClamp;
                else if (dc < -1.0f)
                    dc = -kQuatClamp;
                const float th = std::acos(dc);
                if (th > 1.570796012878418f && dc < 0.0f) {
                    const float th2 = std::acos(-dc);
                    const float invS = 1.0f / std::sin(th2);
                    const float w0 = std::sin((1.0f - eRot) * th2) * invS;
                    const float w1 = std::sin(eRot * th2) * invS;
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] = pq[c] * w0 - w1 * cq[c];
                } else {
                    const float invS = 1.0f / std::sin(th);
                    const float w0 = std::sin((1.0f - eRot) * th) * invS;
                    const float w1 = std::sin(eRot * th) * invS;
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] = pq[c] * w0 + w1 * cq[c];
                }
            }
#endif

            const std::int8_t btype =
                static_cast<std::int8_t>(bone->type);
            if (btype <= static_cast<std::int8_t>(mdl::BoneType::Effector) ||
                bone->type == mdl::BoneType::FixedAxis) {
                for (int axis = 0; axis < 3; ++axis) {
                    const float delta = rec.position[axis] - ppos[axis];
                    if (delta == 0.0f) {
                        bone->trans[axis] = ppos[axis];
                    } else {
                        const float e = BoneEase(m, axis, cursorIdx, tF);
                        const float easedDelta = e * delta;
                        bone->trans[axis] = easedDelta + ppos[axis];
                    }
                }
            }
        }
    }
}

}  
