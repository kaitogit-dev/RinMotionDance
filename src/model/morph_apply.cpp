#include <cmath>
#include <cstdint>
#include <cstring>

#include "rinmotiondance/model.hpp"

namespace rinmotiondance {
namespace {

void QuatMul(float out[4], const float a[4], const float b[4]) {
    float r[4];
    r[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
    r[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
    r[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
    r[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
    out[0] = r[0];
    out[1] = r[1];
    out[2] = r[2];
    out[3] = r[3];
}

void AccumBoneEntry(mdl::BoneMorphOffsetRecord* work,
                    const mdl::PmxBoneMorphEntry& ent, float w, float refW) {
    mdl::BoneMorphOffsetRecord& rec = work[ent.boneIndex];
    float x = ent.rotation[0];
    float y = ent.rotation[1];
    float z = ent.rotation[2];
    float qw = ent.rotation[3];
    if (qw > 1.0f)
        qw = 1.0f;
    else if (qw < -1.0f)
        qw = -1.0f;
#if defined(_M_IX86)
    double angle = std::acos(static_cast<double>(qw));
    if (qw < 0.0f)
        angle -= 3.140000104904175;
    const double len =
        std::sqrt(static_cast<double>(x * x + y * y + z * z));
    float q[4];
    if (angle == 0.0 || len < 0.00000011920929) {
        q[0] = 0.0f;
        q[1] = 0.0f;
        q[2] = 0.0f;
        q[3] = 1.0f;
    } else {
        const double scaled = (angle * w) * refW;
        const double s = std::sin(scaled) / len;
        q[0] = static_cast<float>(s * x);
        q[1] = static_cast<float>(s * y);
        q[2] = static_cast<float>(s * z);
        q[3] = static_cast<float>(std::cos(scaled));
    }
#else
    float angle = std::acos(qw);
    if (qw < 0.0f)
        angle -= 3.140000104904175f;
    const float len = std::sqrt(x * x + y * y + z * z);
    float q[4];
    if (angle == 0.0f || len < 0.00000011920929f) {
        q[0] = 0.0f;
        q[1] = 0.0f;
        q[2] = 0.0f;
        q[3] = 1.0f;
    } else {
        const float scaled = (angle * w) * refW;
        const float s = std::sin(scaled) / len;
        q[0] = s * x;
        q[1] = s * y;
        q[2] = s * z;
        q[3] = std::cos(scaled);
    }
#endif
    QuatMul(rec.rotation, rec.rotation, q);
#if defined(_M_IX86)
    rec.translation[0] = static_cast<float>(
        rec.translation[0] +
        (w * static_cast<double>(ent.translation[0])) * refW);
    rec.translation[1] = static_cast<float>(
        rec.translation[1] +
        (w * static_cast<double>(ent.translation[1])) * refW);
    rec.translation[2] = static_cast<float>(
        rec.translation[2] +
        (w * static_cast<double>(ent.translation[2])) * refW);
#else
    rec.translation[0] += (ent.translation[0] * w) * refW;
    rec.translation[1] += (ent.translation[1] * w) * refW;
    rec.translation[2] += (ent.translation[2] * w) * refW;
#endif
}

template <std::size_t Count>
void ApplyMaterialMorphChannels(float (&add)[Count], float (&mul)[Count],
                                const float (&delta)[Count], float weight,
                                bool additive) {
    for (std::size_t i = 0; i < Count; ++i) {
        if (additive)
            add[i] += delta[i] * weight;
        else
            mul[i] = (1.0f - (1.0f - delta[i]) * weight) * mul[i];
    }
}

void ApplyMaterialMorphChannels(float& add, float& mul, float delta,
                                float weight, bool additive) {
    if (additive)
        add += delta * weight;
    else
        mul = (1.0f - (1.0f - delta) * weight) * mul;
}

void ApplyMatSlot(mdl::MaterialMorphPool& add, mdl::MaterialMorphPool& mul,
                  const mdl::PmxMaterialMorphEntry& entry, float weight) {
    const bool additive = entry.operation != 0;
    const mdl::MaterialMorphChannels& delta = entry.channels;
    mdl::MaterialMorphChannels& addChannels = add.channels;
    mdl::MaterialMorphChannels& mulChannels = mul.channels;
    ApplyMaterialMorphChannels(addChannels.diffuse, mulChannels.diffuse,
                               delta.diffuse, weight, additive);
    ApplyMaterialMorphChannels(addChannels.specular, mulChannels.specular,
                               delta.specular, weight, additive);
    ApplyMaterialMorphChannels(addChannels.specularPower,
                               mulChannels.specularPower,
                               delta.specularPower, weight, additive);
    ApplyMaterialMorphChannels(addChannels.ambient, mulChannels.ambient,
                               delta.ambient, weight, additive);
    ApplyMaterialMorphChannels(addChannels.edgeColor, mulChannels.edgeColor,
                               delta.edgeColor, weight, additive);
    ApplyMaterialMorphChannels(addChannels.edgeSize, mulChannels.edgeSize,
                               delta.edgeSize, weight, additive);
    ApplyMaterialMorphChannels(addChannels.textureTint,
                               mulChannels.textureTint, delta.textureTint,
                               weight, additive);
    ApplyMaterialMorphChannels(addChannels.sphereTint, mulChannels.sphereTint,
                               delta.sphereTint, weight, additive);
    ApplyMaterialMorphChannels(addChannels.toonTint, mulChannels.toonTint,
                               delta.toonTint, weight, additive);
}

void AccumMatMorph(mdl::MaterialMorphPool* addP,
                   mdl::MaterialMorphPool* mulP, int matCount,
                   const mdl::MorphRecord& morph, float w) {
    const int n = morph.materialCount;
    for (int k = 0; k < n; ++k) {
        const mdl::PmxMaterialMorphEntry& ent = morph.materialEntries[k];
        const std::int32_t mi = ent.materialIndex;
        if (mi == -1) {
            for (int s = 0; s < matCount; ++s)
                ApplyMatSlot(addP[s], mulP[s], ent, w);
        } else {
            ApplyMatSlot(addP[mi], mulP[mi], ent, w);
        }
    }
}

}  

void ModelApplyMorphs(unsigned char* m) {
    mdl::ModelRecord& model = *mdl::Mdl(m);
    if (model.physicsMode != 2)
        return;

    mdl::MorphRecord* morphs = model.morphs;
    const int morphCount = static_cast<int>(model.morphCount);
    mdl::BoneMorphOffsetRecord* work = mdl::BoneMorphOffsets(m);
    const int workCount = mdl::BoneMorphOffsetCount(m);

    for (int i = 0; i < workCount; ++i) {
        mdl::BoneMorphOffsetRecord& record = work[i];
        record.translation[0] = record.translation[1] =
            record.translation[2] = 0.0f;
        record.rotation[0] = record.rotation[1] =
            record.rotation[2] = 0.0f;
        record.rotation[3] = 1.0f;
    }
    for (int i = 0; i < morphCount; ++i) {
        const mdl::MorphRecord& morph = morphs[i];
        const float w = morph.value;
        if (w == 0.0f)
            continue;
        if (morph.type != 0) {
            if (morph.type == 2) {                    
                const int n = morph.boneCount;
                for (int k = 0; k < n; ++k)
                    AccumBoneEntry(work, morph.boneEntries[k], w, 1.0f);
            }
        } else {                                      
            for (int k = 0; k < morph.groupCount; ++k) {
                const mdl::PmxGroupMorphEntry& ref = morph.groupEntries[k];
                const mdl::MorphRecord& target = morphs[ref.morphIndex];
                if (target.type == 2) {
                    const int en = target.boneCount;
                    for (int e = 0; e < en; ++e)
                        AccumBoneEntry(work, target.boneEntries[e], w,
                                       ref.weight);
                }
            }
        }
    }

    const int matCount = static_cast<int>(model.materialCount);
    auto* addP = mdl::MaterialMorphAdd(m);
    auto* mulP = mdl::MaterialMorphMul(m);
    std::memset(addP, 0, sizeof(*addP) * static_cast<std::size_t>(matCount));
    for (int i = 0; i < matCount; ++i) {
        mdl::MaterialMorphChannels& slot = mulP[i].channels;
        for (float& value : slot.diffuse) value = 1.0f;
        for (float& value : slot.specular) value = 1.0f;
        slot.specularPower = 1.0f;
        for (float& value : slot.ambient) value = 1.0f;
        for (float& value : slot.edgeColor) value = 1.0f;
        slot.edgeSize = 1.0f;
        for (float& value : slot.textureTint) value = 1.0f;
        for (float& value : slot.sphereTint) value = 1.0f;
        for (float& value : slot.toonTint) value = 1.0f;
    }
    for (int i = 0; i < morphCount; ++i) {
        const mdl::MorphRecord& morph = morphs[i];
        const float w = morph.value;
        if (w == 0.0f)
            continue;
        if (morph.type != 0) {
            if (morph.type == 8)                       
                AccumMatMorph(addP, mulP, matCount, morph, w);
        } else {                                      
            for (int k = 0; k < morph.groupCount; ++k) {
                const mdl::PmxGroupMorphEntry& ref = morph.groupEntries[k];
                const mdl::MorphRecord& target = morphs[ref.morphIndex];
                if (target.type == 8)
                    AccumMatMorph(addP, mulP, matCount, target,
                                  w * ref.weight);
            }
        }
    }

    auto* mats = model.materials;
    auto* baseP = mdl::MaterialMorphBase(m);
    for (int i = 0; i < matCount; ++i) {
        mdl::ModelMaterialRecord& mat = mats[i];
        const mdl::MaterialMorphChannels& add = addP[i].channels;
        const mdl::MaterialMorphChannels& mul = mulP[i].channels;
        const mdl::MaterialMorphChannels& base = baseP[i].channels;
        const auto compose = [](float multiplier, float original, float added) {
            return multiplier * original + added;
        };
        for (int c = 0; c < 3; ++c) {
            mat.diffuseMirror[c] = compose(mul.diffuse[c], base.diffuse[c],
                                            add.diffuse[c]);
            mat.specular[c] = compose(mul.specular[c], base.specular[c],
                                      add.specular[c]);
            mat.ambient[c] = compose(mul.ambient[c], base.ambient[c],
                                     add.ambient[c]);
            mat.edgeColor[c] = compose(mul.edgeColor[c], base.edgeColor[c],
                                       add.edgeColor[c]);
        }
        mat.diffuse[3] = compose(mul.diffuse[3], base.diffuse[3],
                                 add.diffuse[3]);
        mat.specularPower = compose(mul.specularPower, base.specularPower,
                                    add.specularPower);
        mat.edgeColor[3] = compose(mul.edgeColor[3], base.edgeColor[3],
                                   add.edgeColor[3]);
        mat.edgeSize = compose(mul.edgeSize, base.edgeSize, add.edgeSize);
    }
}

}  
