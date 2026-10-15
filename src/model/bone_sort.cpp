#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "rinmotiondance/model.hpp"

namespace rinmotiondance {

void InitBoneSortOrder(unsigned char* m) {
    mdl::BoneOrderCount(m) = 1;
    if (rinmotiondance::mdl::Mdl(m)->boneCount > 0) {
        rinmotiondance::mdl::BoneRecord* bones = rinmotiondance::mdl::Bones(m);
        for (std::uint32_t i = 0; i < rinmotiondance::mdl::Mdl(m)->boneCount; ++i) {
            const mdl::BoneType t = bones[i].type;
            if (t == mdl::BoneType::Move || t == mdl::BoneType::Ik)
                ++mdl::BoneOrderCount(m);
        }
    }

    if (rinmotiondance::mdl::Mdl(m)->boneOrderTable != nullptr) {
        delete[] rinmotiondance::mdl::Mdl(m)->boneOrderTable;
        rinmotiondance::mdl::Mdl(m)->boneOrderTable = nullptr;
    }
    const std::uint32_t slots = mdl::BoneOrderCount(m);
    auto* tbl = new mdl::BoneOrderEntry[slots]{};
    rinmotiondance::mdl::BoneOrder(m) = tbl;
    tbl[0].boneIndex = -1;
    tbl[0].linkedModel = -1;

    mdl::BoneOrderCount(m) = 1;
    if (rinmotiondance::mdl::Mdl(m)->boneCount > 0) {
        rinmotiondance::mdl::BoneRecord* bones = rinmotiondance::mdl::Bones(m);
        const std::uint32_t n = rinmotiondance::mdl::Mdl(m)->boneCount;
        for (std::uint32_t boneIdx = 0; boneIdx < n; ++boneIdx) {
            rinmotiondance::mdl::BoneRecord* bone = &bones[boneIdx];
            const mdl::BoneType t = bone->type;
            if (t == mdl::BoneType::Move || t == mdl::BoneType::Ik) {
                auto& entry = tbl[mdl::BoneOrderCount(m)];
                entry.boneIndex = boneIdx;
                entry.linkedModel = -1;
                bone->slotIndex = mdl::BoneOrderCount(m)++;
            }
        }
    }
}

}  
