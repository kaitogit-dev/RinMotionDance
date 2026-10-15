#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"

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

}  

int SeekModelFrame(unsigned char* model, int frameArg, int physicsMode) {
    unsigned char* const m = model;
    const std::uint32_t frameU32 = static_cast<std::uint32_t>(frameArg);
    int result = 0;

    {
        mdl::DisplayKey* const keys = mdl::DisplayKeys(m);
        int found = 0;                                 
        bool resolved = false;
        if (keys[0].frame < frameU32) {
            for (;;) {
                const int next = static_cast<int>(keys[found].next);
                if (next == 0) break;
                found = next;
                if (keys[found].frame >= frameU32) {
                    resolved = true;
                    break;
                }
            }
            if (!resolved) {
                rinmotiondance::mdl::Mdl(m)->loadComplete = keys[found].visible;
                CopyIkDisplayFlags(m, keys[found]);
                RefreshIkSelectors(m, keys, found, found, false, 0, 0);
            }
        } else {
            resolved = true;  
        }
        if (resolved) {
            if (keys[found].frame == frameU32) {
                rinmotiondance::mdl::Mdl(m)->loadComplete = keys[found].visible;
                CopyIkDisplayFlags(m, keys[found]);
                RefreshIkSelectors(m, keys, found, found, true, found, found);
            } else {
                const int prevIdx = static_cast<int>(keys[found].previous);
                rinmotiondance::mdl::Mdl(m)->loadComplete = keys[prevIdx].visible;
                CopyIkDisplayFlags(m, keys[prevIdx]);
                RefreshIkSelectors(m, keys, prevIdx, prevIdx, true, found,
                                   found);
            }
        }
    }

    const int morphCnt = static_cast<int>(mdl::Mdl(m)->morphCount);
    if (morphCnt > 0) {
        mdl::MorphKey* const keys = mdl::MorphKeys(m);
        rinmotiondance::mdl::MorphRecord* const vals = rinmotiondance::mdl::Morphs(m);  
        for (int k = 0; k < morphCnt; ++k) {
            int found = k;              
            if (keys[found].frame < frameU32) {
                bool resolved = false;
                for (;;) {
                    const int next = static_cast<int>(keys[found].next);
                    if (next == 0) break;
                    found = next;
                    if (keys[found].frame >= frameU32) {
                        resolved = true;
                        break;
                    }
                }
                if (!resolved) {
                    vals[k].value = keys[found].value;
                    result = k + 1;
                    continue;
                }
            }
            const mdl::MorphKey& rec = keys[found];
            float v;
            if (rec.frame == frameU32) {
                v = rec.value;
            } else {
                const mdl::MorphKey& prev = keys[rec.previous];
#if defined(_M_IX86)
                const float tF = (float)((double)(frameU32 - prev.frame) /
                                         (double)(std::uint32_t)(
                                             rec.frame - prev.frame));
                const float delta =
                    rec.value - prev.value;
                v = (float)((double)tF * (double)delta +
                            (double)prev.value);
#else
                const float tF =
                    static_cast<float>(frameU32 - prev.frame) /
                    static_cast<float>(static_cast<int>(rec.frame) -
                                       static_cast<int>(prev.frame));
                v = tF * (rec.value - prev.value) + prev.value;
#endif
            }
            vals[k].value = v;
            result = k + 1;
        }
    }

    const int boneCnt = static_cast<int>(mdl::Mdl(m)->boneCount);
    if (boneCnt > 0) {
        mdl::BoneKey* const keys = mdl::BoneKeys(m);
        rinmotiondance::mdl::BoneRecord* const bones = mdl::Bones(m);
        for (int b = 0; b < boneCnt; ++b) {
            rinmotiondance::mdl::BoneRecord* const bone = &bones[b];
            const std::int8_t btype = static_cast<std::int8_t>(bone->type);
            if (!(btype <= static_cast<std::int8_t>(mdl::BoneType::Effector) ||
                  bone->type == mdl::BoneType::FixedAxis))
                continue;

            int found = b;              
            bool resolved = false;
            if (keys[found].frame < frameU32) {
                for (;;) {
                    const int next = static_cast<int>(keys[found].next);
                    if (next == 0) break;
                    found = next;
                    if (keys[found].frame >= frameU32) {
                        resolved = true;
                        break;
                    }
                }
                if (!resolved) {
                    const mdl::BoneKey& rec = keys[found];
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
                    if (selIdx >= 0) {
                        const mdl::BoneOrderEntry& window =
                            mdl::BoneOrder(m)[selIdx];
                        const std::uint32_t selStart = window.windowStart;
                        if (rec.frame < selStart && selStart <= frameU32) {
                            for (int o = 0; o < 6; ++o)
                                bone->trans[o] = 0.0f;
                            bone->rotQuat[3] = 1.0f;
                        }
                    }
                    result = b + 1;
                    continue;
                }
            } else {
                resolved = true;
            }

            const mdl::BoneKey& rec = keys[found];
            const std::uint32_t curFrame = rec.frame;

            if (curFrame == frameU32) {
                CopyBoneKeyVerbatim(bone, rec);
                if (bone->hasRigidBody != 0) {  
                    if (physicsMode != 3) {
                        const unsigned char mode = rec.physicsDisabled;
                        if (bone->physicsDisabled != mode && physicsMode == 2) {
                            NotifyBonePhysicsMode(m, b, mode);  
                            bone->physicsDisabled = rec.physicsDisabled;
                        }
                        result = b + 1;
                        continue;
                    }
                    const int nextIdx = static_cast<int>(rec.next);
                    const unsigned char mode = rec.physicsDisabled;
                    if (nextIdx <= 0) {
                        if (bone->physicsDisabled != mode)
                            NotifyBonePhysicsMode(m, b, mode);  
                        bone->physicsDisabled = rec.physicsDisabled;
                        result = b + 1;
                        continue;
                    }
                    const mdl::BoneKey& nrec = keys[nextIdx];
                    if (nrec.physicsDisabled != 1 || mode != 0) {
                        if (bone->physicsDisabled != mode)
                            NotifyBonePhysicsMode(m, b, mode);  
                        bone->physicsDisabled = rec.physicsDisabled;
                        result = b + 1;
                        continue;
                    }
                    if (bone->physicsDisabled == 0) NotifyBonePhysicsMode(m, b, 1);
                    bone->physicsDisabled = 1;
                    bone->rigidIdx = static_cast<std::int32_t>(curFrame);
                    MirrorBackupsToWorking(bone);
                    MirrorBackupsToCurrent(bone);
                }
                result = b + 1;
                continue;
            }

            const int prevIdx = static_cast<int>(rec.previous);
            const mdl::BoneKey& prevRec = keys[prevIdx];
            std::uint32_t prevFrame = prevRec.frame;
            float ppos[3] = {prevRec.position[0], prevRec.position[1],
                             prevRec.position[2]};
            float pq[4] = {prevRec.rotation[0], prevRec.rotation[1],
                           prevRec.rotation[2], prevRec.rotation[3]};

            if (bone->hasRigidBody != 0) {  
                if (physicsMode == 3) {
                    if (rec.physicsDisabled == 1 &&
                        prevRec.physicsDisabled == 0) {
                        const std::uint32_t start =
                            static_cast<std::uint32_t>(bone->rigidIdx);
                        if (start >= curFrame || start < prevFrame) {
                            if (bone->physicsDisabled == 0) NotifyBonePhysicsMode(m, b, 1);
                            bone->physicsDisabled = 1;
                            bone->rigidIdx = static_cast<std::int32_t>(prevFrame);
                            MirrorBackupsToWorking(bone);
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
                        if (bone->physicsDisabled != pmode)
                            NotifyBonePhysicsMode(m, b, pmode);  
                        bone->physicsDisabled = prevRec.physicsDisabled;
                    }
                } else {
                    const unsigned char pmode = prevRec.physicsDisabled;
                    if (bone->physicsDisabled != pmode && physicsMode == 2) {
                        NotifyBonePhysicsMode(m, b, pmode);  
                        bone->physicsDisabled = prevRec.physicsDisabled;
                    }
                }
            }

            bool doSnap = false;
            std::uint32_t snapRef = 0;
            const int selIdx = bone->slotIndex;
            if (selIdx >= 0) {
                const mdl::BoneOrderEntry& window =
                    mdl::BoneOrder(m)[selIdx];
                const std::uint32_t selStart = window.windowStart;
                const std::uint32_t selEnd = window.windowEnd;
                if (selStart < curFrame && prevFrame < selStart) {
                    doSnap = true;
                    snapRef = selStart;
                } else if (!(curFrame < selEnd || prevFrame >= selEnd)) {
                    doSnap = true;
                    snapRef = selEnd;
                }
            }
            if (doSnap) {
                if (frameU32 >= snapRef)
                    CopyBoneKeyVerbatim(bone, rec);  
                else
                    WriteBonePrevVerbatim(bone, pq, ppos);  
                result = b + 1;
                continue;
            }

            const float cq[4] = {rec.rotation[0], rec.rotation[1],
                                 rec.rotation[2], rec.rotation[3]};
            const float tF =
                static_cast<float>(frameU32 - prevFrame) /
                static_cast<float>(static_cast<int>(curFrame) -
                                   static_cast<int>(prevFrame));
            const float eRot = BoneEase(m, 3, found, tF);

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
                    const float s = (float)std::sin((double)th2);
                    const float a0 =
                        (float)((1.0 - (double)eRot) * (double)th2);
                    const float sin0 =
                        (float)std::sin((double)a0);
                    const float w0 =
                        (float)((double)sin0 / (double)s);
                    const float aEnd = (float)((double)eRot * (double)th2);
                    const float sin1 =
                        (float)std::sin((double)aEnd);
                    const float w1 =
                        (float)((double)sin1 / (double)s);
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] =
                            (float)((double)pq[c] * (double)w0 -
                                    (double)w1 * (double)cq[c]);
                } else {
                    const float s = (float)std::sin((double)th);
                    const float a0 =
                        (float)((1.0 - (double)eRot) * (double)th);
                    const float sin0 =
                        (float)std::sin((double)a0);
                    const float w0 =
                        (float)((double)sin0 / (double)s);
                    const float aEnd = (float)((double)eRot * (double)th);
                    const float sin1 =
                        (float)std::sin((double)aEnd);
                    const float w1 =
                        (float)((double)sin1 / (double)s);
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
                    const float s = std::sin(th2);
                    const float w0 = std::sin((1.0f - eRot) * th2) / s;
                    const float w1 = std::sin(eRot * th2) / s;
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] = pq[c] * w0 - w1 * cq[c];
                } else {
                    const float s = std::sin(th);
                    const float w0 = std::sin((1.0f - eRot) * th) / s;
                    const float w1 = std::sin(eRot * th) / s;
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] = pq[c] * w0 + w1 * cq[c];
                }
            }
#endif

            if (btype <= static_cast<std::int8_t>(mdl::BoneType::Effector) ||
                bone->type == mdl::BoneType::FixedAxis) {
                for (int axis = 0; axis < 3; ++axis) {
                    const float delta = rec.position[axis] - ppos[axis];
                    if (delta == 0.0f) {
                        bone->trans[axis] = ppos[axis];
                    } else {
                        const float e = BoneEase(m, axis, found, tF);
                        bone->trans[axis] =
                            (float)((double)e * (double)delta +
                                    (double)ppos[axis]);
                    }
                }
            }
            result = b + 1;
        }
    }
    return result;
}

void SeekSelectedModelToCurrentFrame(MMDApp* app) {
    unsigned char* model = app->SelectedModel();
    SeekModelFrame(model, app->state.currentFrame,
              app->PlaybackPhysicsMode());                         
    app->RecordedFrameCountByte1() = 1;                                        
}

}  
