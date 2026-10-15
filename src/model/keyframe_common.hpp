#ifndef RINMOTIONDANCE_SRC_MODEL_KEYFRAME_COMMON_HPP
#define RINMOTIONDANCE_SRC_MODEL_KEYFRAME_COMMON_HPP

#include <cstdint>
#include <cstring>

#include "rinmotiondance/model.hpp"

namespace rinmotiondance {
namespace kfa {

inline std::uint32_t Rd32(const unsigned char* p) {
    std::uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}
inline std::int32_t RdI32(const unsigned char* p) {
    std::int32_t v;
    std::memcpy(&v, p, 4);
    return v;
}
inline float RdF32(const unsigned char* p) {
    float v;
    std::memcpy(&v, p, 4);
    return v;
}
inline void Wr32(unsigned char* p, std::uint32_t v) { std::memcpy(p, &v, 4); }
inline void WrF32(unsigned char* p, float v) { std::memcpy(p, &v, 4); }
inline void CopyIkDisplayFlags(unsigned char* m, const mdl::DisplayKey& rec) {
    const auto* flags = mdl::IkStates(rec);
    mdl::IkChain* const chains = mdl::IkChains(m);
    const std::uint32_t count = mdl::Mdl(m)->ikChainCount;
    for (std::uint32_t k = 0; k < count; ++k)
        chains[k].enabled = flags[k];
}

inline void RefreshIkSelectors(unsigned char* m, mdl::DisplayKey* keys,
                               int pairIdx, int backStart, bool withForward,
                               int fwdStart, int fwdPairIdx) {
    mdl::BoneOrderEntry* const selectors = mdl::BoneOrder(m);
    const int cnt = static_cast<int>(mdl::Mdl(m)->boneOrderCount);
    for (int i = 0; i < cnt; ++i) {
        mdl::BoneOrderEntry& selector = selectors[i];
        const mdl::BoneReference& pair = mdl::SelectorStates(keys[pairIdx])[i];
        selector.linkedModel = pair.modelIndex;
        selector.linkedBone = pair.boneIndex;
        int walk = backStart;
        for (;;) {
            const mdl::DisplayKey& wrec = keys[walk];
            if (wrec.frame == 0) {  
                selector.windowStart = 0;
                break;
            }
            const int prev = static_cast<int>(wrec.previous);
            const mdl::BoneReference& previousPair =
                mdl::SelectorStates(keys[prev])[i];
            if (selector.linkedBone == previousPair.boneIndex &&
                selector.linkedModel == previousPair.modelIndex) {
                walk = prev;
                continue;
            }
            selector.windowStart = keys[keys[prev].next].frame;
            break;
        }
        if (!withForward) {
            selector.windowEnd = 0;
            continue;
        }
        const mdl::BoneReference& forwardPair =
            mdl::SelectorStates(keys[fwdPairIdx])[i];
        if (selector.linkedBone == forwardPair.boneIndex &&
            selector.linkedModel == forwardPair.modelIndex) {
            int f = fwdStart;
            bool toEnd = false;
            for (;;) {
                const int next = static_cast<int>(keys[f].next);
                if (next == 0) {
                    toEnd = true;
                    break;
                }
                f = next;
                const mdl::BoneReference& nextPair =
                    mdl::SelectorStates(keys[f])[i];
                if (!(selector.linkedBone == nextPair.boneIndex &&
                      selector.linkedModel == nextPair.modelIndex))
                    break;
            }
            selector.windowEnd = toEnd ? 0u : keys[f].frame;
        } else {
            selector.windowEnd = keys[fwdStart].frame;
        }
    }
}

inline void CopyBoneKeyVerbatim(rinmotiondance::mdl::BoneRecord* bone,
                                const mdl::BoneKey& rec) {
    for (int c = 0; c < 4; ++c)
        bone->rotQuat[c] = rec.rotation[c];
    for (int c = 0; c < 3; ++c)
        bone->trans[c] = rec.position[c];
}
inline void WriteBonePrevVerbatim(rinmotiondance::mdl::BoneRecord* bone,
                                  const float pq[4], const float ppos[3]) {
    for (int c = 0; c < 4; ++c) bone->rotQuat[c] = pq[c];
    for (int c = 0; c < 3; ++c) bone->trans[c] = ppos[c];
}

inline void MirrorBackupsToWorking(rinmotiondance::mdl::BoneRecord* bone) {
    for (int c = 0; c < 4; ++c)
        bone->ikWorkingQuat[c] = bone->ikBackup[3 + c];
    for (int c = 0; c < 3; ++c)
        bone->ikWorkingPos[c] = bone->ikBackup[c];
}
inline void MirrorBackupsToCurrent(rinmotiondance::mdl::BoneRecord* bone) {
    for (int c = 0; c < 4; ++c)
        bone->rotQuat[c] = bone->ikBackup[3 + c];
    for (int c = 0; c < 3; ++c)
        bone->trans[c] = bone->ikBackup[c];
}

}  
}  

#endif  
