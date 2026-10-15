#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"

#include "keyframe_common.hpp"

namespace rinmotiondance {
namespace {

using kfa::Rd32;
using kfa::RdI32;
using kfa::RdF32;
using kfa::Wr32;
using kfa::WrF32;

const char kJpOverflow[] =
    "\x93\x6f\x98\x5e\x83\x7c\x83\x43\x83\x93\x83\x67\x90\x94\x82\xaa%d"
    "\x8c\xc2\x82\xf0\x89\x7a\x82\xa6\x82\xdc\x82\xb5\x82\xbd\n"
    "\x82\xb1\x82\xea\x88\xc8\x8f\xe3\x82\xcc\x93\x6f\x98\x5e\x82\xcd\x8d"
    "\x73\x82\xa6\x82\xdc\x82\xb9\x82\xf1\n"
    "\x81\x75\xcc\xda\xb0\xd1\x95\xd2\x8f\x57\x81\x76\x82\xcc\x81\x75\x95"
    "\x73\x97\x70\xcc\xda\xb0\xd1\x8d\xed\x8f\x9c\x81\x76\x82\xf0\x8e\xc0"
    "\x8d\x73\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2";
const char kJpFrameRegTitle[] =  
    "\xcc\xda\xb0\xd1\x93\x6f\x98\x5e";

void OverflowBox(unsigned char* m, int limit) {
    char text[256];
    if (rinmotiondance::mdl::Mdl(m)->physicsFlags != 0) {
        sprintf_s(text, 0x100,
                  "You cannot regist over %d point\n"
                  "Please execute 'delete unused frame'", limit);
        MessageBoxA(*reinterpret_cast<HWND*>(m), text,
                    "register frame", 0);
    } else {
        sprintf_s(text, 0x100, kJpOverflow, limit);
        MessageBoxA(*reinterpret_cast<HWND*>(m), text,
                    kJpFrameRegTitle, 0);
    }
}

void FillBoneRecord(mdl::BoneKey& dst, const unsigned char* rec,
                    bool writePos) {
    std::memcpy(dst.rotation, rec + 36, sizeof dst.rotation);
    if (writePos) std::memcpy(dst.position, rec + 52, sizeof dst.position);
    dst.physicsDisabled = rec[64];
    std::memcpy(dst.interpolation, rec + 65, sizeof dst.interpolation);
    dst.allocated = 1;
}

void FillMirroredBoneRecord(mdl::BoneKey& dst, const unsigned char* rec) {
    std::memcpy(dst.rotation, rec + 36, sizeof dst.rotation);
    dst.rotation[1] = -dst.rotation[1];
    dst.rotation[2] = -dst.rotation[2];
    std::memcpy(dst.position, rec + 52, sizeof dst.position);
    dst.position[0] = -dst.position[0];
    dst.physicsDisabled = rec[64];
    std::memcpy(dst.interpolation, rec + 65, sizeof dst.interpolation);
    dst.allocated = 1;
}

int FindMirroredBone(unsigned char* model, const unsigned char* rec) {
    static const char kRight[] = "\x89\x45";
    static const char kLeft[] = "\x8D\xB6";
    const char* const name = reinterpret_cast<const char*>(rec);
    const mdl::ModelRecord& state = *mdl::Mdl(model);
    const mdl::BoneRecord* const bones = state.boneTable;
    const int boneCount = static_cast<int>(state.boneCount);

    const auto findOpposite = [&](const char* marker,
                                  const char* opposite) -> int {
        const char* hit = std::strstr(name, marker);
        if (hit == nullptr || boneCount <= 0) return -1;
        const char* const suffix = hit + 2;
        for (int i = 0; i < boneCount; ++i) {
            const char* const boneName = bones[i].name;
            const char* other = std::strstr(boneName, opposite);
            if (other != nullptr && std::strcmp(suffix, other + 2) == 0)
                return i;
        }
        return -1;
    };

    int index = findOpposite(kLeft, kRight);
    if (index >= 0) return index;
    index = findOpposite(kRight, kLeft);
    if (index >= 0) return index;
    for (int i = 0; i < boneCount; ++i) {
        if (std::strcmp(name, bones[i].name) == 0)
            return i;
    }
    return -1;
}

const unsigned char kJpDispIkOp[] =   
    {0x95, 0x5C, 0x8E, 0xA6, 0xA5, 0x49, 0x4B,
     0xA5, 0x8A, 0x4F, 0x90, 0x65, 0x00};
const unsigned char kJpBone01[] =     
    {0xCE, 0xDE, 0xB0, 0xDD, 0x30, 0x31, 0x00};

template <typename Key>
int MarkTrackRange(Key* keys, int root, std::uint32_t from,
                   std::uint32_t to) {
    int cur = root;
    if (keys[cur].frame < from) {
        for (;;) {
            const int next = static_cast<int>(keys[cur].next);
            if (next == 0) return 0;
            cur = next;
            if (keys[cur].frame >= from) break;
        }
    }
    if (keys[cur].frame > to) return 0;                    
    int last = 0;
    for (;;) {
        keys[cur].allocated = 1;                           
        last = cur;
        const int next = static_cast<int>(keys[cur].next);
        if (next == 0) break;                              
        if (keys[next].frame > to) break;                  
        cur = next;
    }
    return last;
}

}  

void ResetBoneKeyCursor(unsigned char* model) {
    unsigned char* const m = model;
    mdl::BoneKey* const keys = mdl::BoneKeys(m);
    int& cursor = mdl::Mdl(m)->searchCursor;
    cursor = static_cast<int>(mdl::Mdl(m)->boneCount);
    if (keys[cursor].frame != 0) {
        do {
            if (++cursor >= static_cast<int>(mdl::kBoneKeyCapacity))
                break;
        } while (keys[cursor].frame != 0);
    }
}

int ResetMorphKeyCursor(unsigned char* model) {
    unsigned char* const m = model;
    mdl::MorphKey* const keys = mdl::MorphKeys(m);
    int& cursor = mdl::Mdl(m)->searchCursor;
    cursor = static_cast<int>(mdl::Mdl(m)->morphCount);
    int result = 5 * cursor;
    if (keys[cursor].frame != 0) {
        do {
            if (++cursor >= 0x4E20) break;
            result = 5 * cursor;
        } while (keys[cursor].frame != 0);
    }
    return result;
}

void RegisterMorphKeyCurrent(unsigned char* model, int morph, int frameArg) {
    unsigned char* const m = model;
    if (m == nullptr || morph < 0 ||
        (rinmotiondance::mdl::Mdl(m)->physicsMode != 2 && morph == 0) || rinmotiondance::mdl::Morphs(m) == nullptr)
        return;

    mdl::MorphKey* const keys = mdl::MorphKeys(m);
    const std::uint32_t frame = static_cast<std::uint32_t>(frameArg);
    const float value = rinmotiondance::mdl::Morphs(m)[morph].value;

    const auto fill = [&](int index) {
        keys[index].frame = frame;
        keys[index].value = value;
        keys[index].allocated = 1;
        if (frame > mdl::Mdl(m)->maxFrame)
            mdl::Mdl(m)->maxFrame = frame;
    };
    const auto allocate = [&]() -> int {
        int index = static_cast<int>(mdl::Mdl(m)->morphCount);
        while (index < 20000 && keys[index].frame != 0)
            ++index;
        if (index >= 20000) {
            OverflowBox(m, 20000);
            return -1;
        }
        return index;
    };

    int current = morph;
    if (keys[current].frame < frame) {
        for (;;) {
            const int next = static_cast<int>(keys[current].next);
            if (next == 0)
                break;
            current = next;
            if (keys[current].frame >= frame)
                break;
        }
        if (keys[current].frame < frame) {
            const int fresh = allocate();
            if (fresh < 0) return;
            keys[current].next = static_cast<std::uint32_t>(fresh);
            keys[fresh].previous = static_cast<std::uint32_t>(current);
            fill(fresh);
            return;
        }
    }

    if (keys[current].frame == frame) {
        keys[current].value = value;
        keys[current].allocated = 1;
        return;
    }

    const int fresh = allocate();
    if (fresh < 0) return;
    const int previous = static_cast<int>(keys[current].previous);
    keys[previous].next = static_cast<std::uint32_t>(fresh);
    keys[fresh].previous = static_cast<std::uint32_t>(previous);
    keys[current].previous = static_cast<std::uint32_t>(fresh);
    keys[fresh].next = static_cast<std::uint32_t>(current);
    fill(fresh);
}

bool RegisterBoneKey(unsigned char* model, unsigned char* rec, int frameOffset,
                     unsigned char useSelected) {
    unsigned char* const m = model;
    mdl::BoneKey* const keys = mdl::BoneKeys(m);
    const std::uint32_t frame =
        static_cast<std::uint32_t>(frameOffset) + Rd32(rec + 32);

    int boneIdx;
    if (useSelected != 0) {
        boneIdx = mdl::Mdl(m)->selectedBone;
    } else {
        boneIdx = 0;
        const int boneCnt = static_cast<int>(mdl::Mdl(m)->boneCount);
        if (boneCnt <= 0) return true;
        const mdl::BoneRecord* bone = mdl::Bones(m);
        while (std::strcmp(reinterpret_cast<const char*>(rec),
                           bone->name) != 0) {
            if (++boneIdx >= boneCnt) return true;
            ++bone;
        }
    }
    if (boneIdx == -1) return true;

    mdl::BoneRecord& bone = mdl::Bones(m)[boneIdx];
    const mdl::BoneType btype = bone.type;
    const bool writePos = btype == mdl::BoneType::RotateMove ||
                          btype == mdl::BoneType::Move ||
                          btype == mdl::BoneType::Ik;

    if (btype == mdl::BoneType::FixedAxis ||
        (btype == mdl::BoneType::UnderIk &&
         (bone.flags & mdl::kBoneFlagFixedAxis) == mdl::kBoneFlagFixedAxis)) {
        float boneAxis[3];
        if (rinmotiondance::mdl::Mdl(m)->physicsMode == 2) {  
            std::memcpy(boneAxis, bone.axis, sizeof boneAxis);
        } else {
            const mdl::BoneRecord& tail = mdl::Bones(m)[bone.tailBone];
            boneAxis[0] = tail.position[0] - bone.position[0];
            boneAxis[1] = tail.position[1] - bone.position[1];
            boneAxis[2] = tail.position[2] - bone.position[2];
            auto* d3 = &d3dx::Get();
            d3->vec3Normalize(boneAxis, boneAxis);
        }
        float* const q = reinterpret_cast<float*>(rec + 36);
        float axisOut[3], angle;
        {
            auto* d3 = &d3dx::Get();
            if (d3->quatToAxisAngle == nullptr) return false;
            d3->quatToAxisAngle(q, axisOut, &angle);
        }
        if (!_finite(q[3])) q[3] = 1.0f;  
        float sinHalf;
        const float ww = q[3] * q[3];
        if (ww <= 1.0f) {
            sinHalf = sqrtf(1.0f - ww);
        } else {
            q[3] = 1.0f;
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
        const float cosT = dot / (nB * nA);
        if (cosT < 0.0f) sinHalf = -sinHalf;
        q[0] = sinHalf * axisOut[0];
        q[1] = sinHalf * axisOut[1];
        q[2] = sinHalf * axisOut[2];
        auto* d3 = &d3dx::Get();
        if (d3->quatNormalize != nullptr) d3->quatNormalize(q, q);
    }

    int cur = boneIdx;
    if (keys[cur].frame < frame) {
        for (;;) {
            const int next = static_cast<int>(keys[cur].next);
            if (next == 0) break;
            cur = next;
            if (keys[cur].frame >= frame) break;
        }
        if (keys[cur].frame < frame) {
            int free_ = mdl::Mdl(m)->searchCursor;
            if (keys[free_].frame != 0) {
                for (;;) {
                    ++mdl::Mdl(m)->searchCursor;
                    ++free_;
                    if (free_ >= static_cast<int>(mdl::kBoneKeyCapacity)) {
                        OverflowBox(m, static_cast<int>(mdl::kBoneKeyCapacity));
                        return false;
                    }
                    if (keys[free_].frame == 0) break;
                }
            }
            AppendBoneKeyToUndo(m, cur);
            AppendBoneKeyToUndo(m, free_);
            keys[cur].next = static_cast<std::uint32_t>(free_);
            keys[free_].previous = static_cast<std::uint32_t>(cur);
            keys[free_].frame = frame;
            FillBoneRecord(keys[free_], rec, writePos);
            if (frame > mdl::Mdl(m)->maxFrame)
                mdl::Mdl(m)->maxFrame = frame;
            return true;
        }
    }

    if (frame == keys[cur].frame) {
        AppendBoneKeyToUndo(m, cur);
        FillBoneRecord(keys[cur], rec, writePos);
    } else {
        int free_ = mdl::Mdl(m)->searchCursor;
        if (keys[free_].frame != 0) {
            for (;;) {
                ++mdl::Mdl(m)->searchCursor;
                ++free_;
                if (free_ >= static_cast<int>(mdl::kBoneKeyCapacity)) {
                    OverflowBox(m, static_cast<int>(mdl::kBoneKeyCapacity));
                    return false;
                }
                if (keys[free_].frame == 0) break;
            }
        }
        const int prev = static_cast<int>(keys[cur].previous);
        AppendBoneKeyToUndo(m, prev);
        AppendBoneKeyToUndo(m, cur);
        AppendBoneKeyToUndo(m, free_);
        keys[prev].next = static_cast<std::uint32_t>(free_);
        keys[free_].previous = static_cast<std::uint32_t>(prev);
        keys[cur].previous = static_cast<std::uint32_t>(free_);
        keys[free_].next = static_cast<std::uint32_t>(cur);
        keys[free_].frame = frame;
        FillBoneRecord(keys[free_], rec, writePos);
    }
    if (static_cast<std::uint32_t>(frameOffset) > mdl::Mdl(m)->maxFrame)
        mdl::Mdl(m)->maxFrame = static_cast<std::uint32_t>(frameOffset);
    return true;
}

bool RegisterMirroredBoneKey(unsigned char* model, unsigned char* rec,
                             int frameOffset) {
    unsigned char* const m = model;
    mdl::BoneKey* const keys = mdl::BoneKeys(m);
    const std::uint32_t frame =
        static_cast<std::uint32_t>(frameOffset) + Rd32(rec + 32);
    const int boneIdx = FindMirroredBone(m, rec);
    if (boneIdx < 0) return true;

    mdl::BoneRecord& bone = mdl::Bones(m)[boneIdx];
    const mdl::BoneType btype = bone.type;
    if (btype == mdl::BoneType::FixedAxis ||
        (btype == mdl::BoneType::UnderIk &&
         (bone.flags & mdl::kBoneFlagFixedAxis) == mdl::kBoneFlagFixedAxis)) {
        float boneAxis[3];
        if (rinmotiondance::mdl::Mdl(m)->physicsMode == 2) {
            std::memcpy(boneAxis, bone.axis, sizeof boneAxis);
        } else {
            const mdl::BoneRecord& tail = mdl::Bones(m)[bone.tailBone];
            boneAxis[0] = tail.position[0] - bone.position[0];
            boneAxis[1] = tail.position[1] - bone.position[1];
            boneAxis[2] = tail.position[2] - bone.position[2];
            auto* d3 = &d3dx::Get();
            if (d3->vec3Normalize != nullptr)
                d3->vec3Normalize(boneAxis, boneAxis);
        }

        float* const q = reinterpret_cast<float*>(rec + 36);
        float sourceAxis[3], angle;
        auto* d3 = &d3dx::Get();
        if (d3->quatToAxisAngle == nullptr) return false;
        d3->quatToAxisAngle(q, sourceAxis, &angle);
        if (!_finite(q[3])) q[3] = 1.0f;
        float sinHalf;
        const float ww = q[3] * q[3];
        if (ww <= 1.0f) {
            sinHalf = sqrtf(1.0f - ww);
        } else {
            q[3] = 1.0f;
            sinHalf = 0.0f;
        }
        sourceAxis[0] = -sourceAxis[0];
        sinHalf = -sinHalf;
        const float dot = boneAxis[0] * sourceAxis[0] +
                          boneAxis[1] * sourceAxis[1] +
                          boneAxis[2] * sourceAxis[2];
        const float nA = sqrtf(boneAxis[0] * boneAxis[0] +
                               boneAxis[1] * boneAxis[1] +
                               boneAxis[2] * boneAxis[2]);
        const float nB = sqrtf(sourceAxis[0] * sourceAxis[0] +
                               sourceAxis[1] * sourceAxis[1] +
                               sourceAxis[2] * sourceAxis[2]);
        if (dot / (nA * nB) < 0.0f) sinHalf = -sinHalf;
        q[0] = sinHalf * boneAxis[0];
        q[1] = -sinHalf * boneAxis[1];
        q[2] = -sinHalf * boneAxis[2];
        if (d3->quatNormalize != nullptr) d3->quatNormalize(q, q);
    }

    int cur = boneIdx;
    if (keys[cur].frame < frame) {
        for (;;) {
            const int next = static_cast<int>(keys[cur].next);
            if (next == 0) break;
            cur = next;
            if (keys[cur].frame >= frame) break;
        }
        if (keys[cur].frame < frame) {
            int free_ = mdl::Mdl(m)->searchCursor;
            if (keys[free_].frame != 0) {
                for (;;) {
                    ++mdl::Mdl(m)->searchCursor;
                    ++free_;
                    if (free_ >= static_cast<int>(mdl::kBoneKeyCapacity)) {
                        OverflowBox(m, static_cast<int>(mdl::kBoneKeyCapacity));
                        return false;
                    }
                    if (keys[free_].frame == 0) break;
                }
            }
            AppendBoneKeyToUndo(m, cur);
            AppendBoneKeyToUndo(m, free_);
            keys[cur].next = static_cast<std::uint32_t>(free_);
            keys[free_].previous = static_cast<std::uint32_t>(cur);
            keys[free_].frame = frame;
            FillMirroredBoneRecord(keys[free_], rec);
            if (frame > mdl::Mdl(m)->maxFrame)
                mdl::Mdl(m)->maxFrame = frame;
            return true;
        }
    }

    if (frame == keys[cur].frame) {
        AppendBoneKeyToUndo(m, cur);
        FillMirroredBoneRecord(keys[cur], rec);
    } else {
        int free_ = mdl::Mdl(m)->searchCursor;
        if (keys[free_].frame != 0) {
            for (;;) {
                ++mdl::Mdl(m)->searchCursor;
                ++free_;
                if (free_ >= static_cast<int>(mdl::kBoneKeyCapacity)) {
                    OverflowBox(m, static_cast<int>(mdl::kBoneKeyCapacity));
                    return false;
                }
                if (keys[free_].frame == 0) break;
            }
        }
        const int prev = static_cast<int>(keys[cur].previous);
        AppendBoneKeyToUndo(m, prev);
        AppendBoneKeyToUndo(m, cur);
        AppendBoneKeyToUndo(m, free_);
        keys[prev].next = static_cast<std::uint32_t>(free_);
        keys[free_].previous = static_cast<std::uint32_t>(prev);
        keys[cur].previous = static_cast<std::uint32_t>(free_);
        keys[free_].next = static_cast<std::uint32_t>(cur);
        keys[free_].frame = frame;
        FillMirroredBoneRecord(keys[free_], rec);
    }
    if (static_cast<std::uint32_t>(frameOffset) > mdl::Mdl(m)->maxFrame)
        mdl::Mdl(m)->maxFrame = static_cast<std::uint32_t>(frameOffset);
    return true;
}

bool RegisterMorphKeyFromRecord(unsigned char* model, const unsigned char* rec,
                                int frameOffset) {
    unsigned char* const m = model;
    mdl::MorphKey* const keys = mdl::MorphKeys(m);
    const std::uint32_t frame =
        static_cast<std::uint32_t>(frameOffset) + Rd32(rec + 32);

    int morphIdx = 0;
    const int morphCnt = static_cast<int>(mdl::Mdl(m)->morphCount);
    if (morphCnt <= 0) return true;
    const rinmotiondance::mdl::MorphRecord* mrec = rinmotiondance::mdl::Morphs(m);
    while (std::strcmp(reinterpret_cast<const char*>(rec),
                       reinterpret_cast<const char*>(mrec)) != 0) {
        if (++morphIdx >= morphCnt) return true;
        mrec += 136;
    }
    if (morphIdx == -1) return true;

    int cur = morphIdx;
    if (keys[cur].frame < frame) {
        for (;;) {
            const int next = static_cast<int>(keys[cur].next);
            if (next == 0) break;
            cur = next;
            if (keys[cur].frame >= frame) break;
        }
        if (keys[cur].frame < frame) {
            int free_ = mdl::Mdl(m)->searchCursor;
            if (keys[free_].frame != 0) {
                for (;;) {
                    ++mdl::Mdl(m)->searchCursor;
                    ++free_;
                    if (20 * free_ >= 400000) {
                        OverflowBox(m, 20000);
                        return false;
                    }
                    if (keys[free_].frame == 0) break;
                }
            }
            keys[cur].next = static_cast<std::uint32_t>(free_);
            keys[free_].previous = static_cast<std::uint32_t>(cur);
            keys[free_].frame = frame;
            keys[free_].value = RdF32(rec + 36);
            keys[free_].allocated = 1;
            if (frame > mdl::Mdl(m)->maxFrame)
                mdl::Mdl(m)->maxFrame = frame;
            return true;
        }
    }

    if (frame == keys[cur].frame) {
        keys[cur].value = RdF32(rec + 36);
        keys[cur].allocated = 1;
        return true;
    }
    int free_ = mdl::Mdl(m)->searchCursor;
    if (keys[free_].frame != 0) {
        for (;;) {
            ++mdl::Mdl(m)->searchCursor;
            ++free_;
            if (20 * free_ >= 400000) {
                OverflowBox(m, 20000);
                return false;
            }
            if (keys[free_].frame == 0) break;
        }
    }
    const int prev = static_cast<int>(keys[cur].previous);
    keys[prev].next = static_cast<std::uint32_t>(free_);
    keys[free_].previous = static_cast<std::uint32_t>(prev);
    keys[cur].previous = static_cast<std::uint32_t>(free_);
    keys[free_].next = static_cast<std::uint32_t>(cur);
    keys[free_].frame = frame;
    keys[free_].value = RdF32(rec + 36);
    keys[free_].allocated = 1;
    if (frame > mdl::Mdl(m)->maxFrame)
        mdl::Mdl(m)->maxFrame = frame;
    return true;
}

bool RegisterDisplayKeyFromRecord(unsigned char* model, int frameArg,
                                  unsigned char view, int cnt,
                                  const unsigned char* entries21, int cnt2,
                                  const unsigned char* entries28,
                                  int frameOffset) {
    unsigned char* const m = model;
    mdl::DisplayKey* const keys = mdl::DisplayKeys(m);
    const std::uint32_t frame =
        static_cast<std::uint32_t>(frameOffset + frameArg);

    auto FillEntries = [&](int index) {
        const int ikCnt = static_cast<int>(mdl::Mdl(m)->ikChainCount);
        auto* const ikBase = mdl::IkStates(keys[index]);
        for (int i = 0; i < ikCnt; ++i) {
            if (cnt > 0) {
                const unsigned char* e = entries21;
                const unsigned char* const chainBone =
                    reinterpret_cast<const unsigned char*>(
                        mdl::Bones(m) + mdl::IkChains(m)[i].boneIndex);
                int k = 0;
                while (std::strcmp(reinterpret_cast<const char*>(e),
                                   reinterpret_cast<const char*>(
                                       chainBone)) != 0) {
                    ++k;
                    e += 21;
                    if (k >= cnt)
                        break;
                }
                if (k < cnt)
                    ikBase[i] = entries21[21 * k + 20];
            }
        }
        const int selCnt = static_cast<int>(mdl::Mdl(m)->boneOrderCount);
        mdl::BoneReference* const pairBase =
            mdl::SelectorStates(keys[index]);
        const mdl::BoneOrderEntry* const relationships = mdl::BoneOrder(m);
        for (int i = 0; i < selCnt; ++i) {
            if (cnt2 > 0) {
                const unsigned char* e = entries28;
                const unsigned char* const selBone =
                    reinterpret_cast<const unsigned char*>(mdl::Bones(m) +
                                                            relationships[i].boneIndex);
                int k = 0;
                while (std::strcmp(reinterpret_cast<const char*>(e),
                                   reinterpret_cast<const char*>(
                                       selBone)) != 0) {
                    ++k;
                    e += 28;
                    if (k >= cnt2)
                        break;
                }
                if (k < cnt2) {
                    pairBase[i].modelIndex =
                        RdI32(entries28 + 28 * k + 20);
                    pairBase[i].boneIndex =
                        RdI32(entries28 + 28 * k + 24);
                }
            }
        }
    };

    int cur = 0;
    if (keys[0].frame < frame) {
        for (;;) {
            const int next = static_cast<int>(keys[cur].next);
            if (next == 0) break;
            cur = next;
            if (keys[cur].frame >= frame) break;
        }
        if (keys[cur].frame < frame) {
            int free_ = mdl::Mdl(m)->searchCursor;
            if (keys[free_].frame != 0) {
                for (;;) {
                    ++mdl::Mdl(m)->searchCursor;
                    ++free_;
                    if (free_ >= 1000) {
                        OverflowBox(m, 1000);
                        return false;
                    }
                    if (keys[free_].frame == 0) break;
                }
            }
            keys[cur].next = static_cast<std::uint32_t>(free_);
            keys[free_].previous = static_cast<std::uint32_t>(cur);
            keys[free_].frame = frame;
            keys[free_].visible = view;
            FillEntries(free_);
            keys[free_].allocated = 1;
            if (frame > mdl::Mdl(m)->maxFrame)
                mdl::Mdl(m)->maxFrame = frame;
            return true;
        }
    }

    if (frame == keys[cur].frame) {
        keys[cur].visible = view;
        FillEntries(cur);
        keys[cur].allocated = 1;
        return true;
    }
    int free_ = mdl::Mdl(m)->searchCursor;
    if (keys[free_].frame != 0) {
        for (;;) {
            ++mdl::Mdl(m)->searchCursor;
            ++free_;
            if (free_ >= 1000) {
                OverflowBox(m, 1000);
                return false;
            }
            if (keys[free_].frame == 0) break;
        }
    }
    const int prev = static_cast<int>(keys[cur].previous);
    keys[prev].next = static_cast<std::uint32_t>(free_);
    keys[free_].previous = static_cast<std::uint32_t>(prev);
    keys[cur].previous = static_cast<std::uint32_t>(free_);
    keys[free_].next = static_cast<std::uint32_t>(cur);
    keys[free_].frame = frame;
    keys[free_].visible = view;
    FillEntries(free_);
    keys[free_].allocated = 1;
    if (frame > mdl::Mdl(m)->maxFrame)
        mdl::Mdl(m)->maxFrame = frame;
    return true;
}

void RegisterDisplayKeyCurrent(unsigned char* model, int frameArg) {
    unsigned char* const m = model;
    mdl::DisplayKey* const keys = mdl::DisplayKeys(m);
    const std::uint32_t frame = static_cast<std::uint32_t>(frameArg);

    const auto fill = [&](int index) {
        mdl::DisplayKey& rec = keys[index];
        rec.visible = rinmotiondance::mdl::Mdl(m)->loadComplete;
        auto* const ik = mdl::IkStates(rec);
        mdl::IkChain* const chains = mdl::IkChains(m);
        for (std::uint32_t i = 0; i < mdl::Mdl(m)->ikChainCount; ++i)
            ik[i] = chains[i].enabled;
        mdl::BoneReference* const pairs = mdl::SelectorStates(rec);
        mdl::BoneOrderEntry* const relationships = mdl::BoneOrder(m);
        for (std::uint32_t i = 0; i < mdl::Mdl(m)->boneOrderCount; ++i) {
            pairs[i].modelIndex = relationships[i].linkedModel;
            pairs[i].boneIndex = relationships[i].linkedBone;
        }
        rec.allocated = 1;
    };

    int current = 0;
    if (keys[0].frame < frame) {
        for (;;) {
            const int next = static_cast<int>(keys[current].next);
            if (next == 0)
                break;
            current = next;
            if (keys[current].frame >= frame)
                break;
        }
    }
    if (keys[current].frame == frame) {
        fill(current);
        return;
    }

    int freeIndex = 1;
    while (freeIndex < 1000 && keys[freeIndex].frame != 0)
        ++freeIndex;
    if (freeIndex >= 1000) {
        OverflowBox(m, 1000);
        return;
    }

    if (keys[current].frame < frame) {
        keys[current].next = static_cast<std::uint32_t>(freeIndex);
        keys[freeIndex].previous = static_cast<std::uint32_t>(current);
    } else {
        const int previous = static_cast<int>(keys[current].previous);
        keys[previous].next = static_cast<std::uint32_t>(freeIndex);
        keys[freeIndex].previous = static_cast<std::uint32_t>(previous);
        keys[current].previous = static_cast<std::uint32_t>(freeIndex);
        keys[freeIndex].next = static_cast<std::uint32_t>(current);
    }
    keys[freeIndex].frame = frame;
    fill(freeIndex);
    if (frame > mdl::Mdl(m)->maxFrame)
        mdl::Mdl(m)->maxFrame = frame;
}

int MarkKeyTrackRangeByName(unsigned char* model, std::uint32_t from,
                            std::uint32_t to, const char* name) {
    mdl::ModelRecord& record = *mdl::Mdl(model);

    const int facialCount = record.facialFrameCount;
    if (facialCount > 0) {
        mdl::FrameGroup* const groups = mdl::DisplayFrames(model);
        for (int g = 0; g < facialCount; ++g) {
            if (std::strcmp(name, groups[g].name) == 0)
                return MarkTrackRange(mdl::MorphKeys(model),
                                      groups[g].targetIndex, from, to);
        }
    }

    if (std::memcmp(name, kJpDispIkOp, 13) == 0 ||
        std::strcmp(name, "disp/IK/OP") == 0) {
        return MarkTrackRange(mdl::DisplayKeys(model), 0, from, to);
    }

    const mdl::BoneRecord* const rootBone = mdl::Bones(model);
    if (std::strcmp(name, rootBone->name) == 0 ||
        std::strcmp(name, rootBone->nameEn) == 0) {
        return MarkTrackRange(mdl::BoneKeys(model), 0, from, to);
    }

    if (std::memcmp(name, kJpBone01, 7) == 0 ||
        std::strcmp(name, "bone01") == 0) {
        return MarkTrackRange(mdl::BoneKeys(model), 0, from, to);
    }

    const std::int32_t groupCount =
        static_cast<std::int32_t>(record.rigidBodyCount);
    if (groupCount > 0) {
        mdl::FrameGroup* const groups = mdl::RigidGroups(model);
        for (std::int32_t g = 0; g < groupCount; ++g) {
            if (std::strcmp(name, groups[g].name) == 0 ||
                std::strcmp(name, groups[g].nameEn) == 0)
                return MarkTrackRange(mdl::BoneKeys(model),
                                      groups[g].targetIndex, from, to);
        }
    }
    return 0;
}

}  
