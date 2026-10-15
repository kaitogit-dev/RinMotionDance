#include <cstring>
#include <new>

#include "rinmotiondance/model.hpp"

namespace rinmotiondance {
void ModelInitDefaults(unsigned char*);
void ModelDispose(unsigned char*);
}

int main() {
    using namespace rinmotiondance;
    auto* bytes = static_cast<unsigned char*>(::operator new(mdl::kSize));
    std::memset(bytes, 0, mdl::kSize);
    ModelInitDefaults(bytes);
    ModelDispose(bytes);
    ::operator delete(bytes);

    bytes = static_cast<unsigned char*>(::operator new(mdl::kSize));
    std::memset(bytes, 0, mdl::kSize);
    ModelInitDefaults(bytes);
    auto& model = *mdl::Mdl(bytes);
    model.materials = static_cast<mdl::ModelMaterialRecord*>(
        ::operator new(sizeof(mdl::ModelMaterialRecord) * 2));
    model.boneKeyCursors = static_cast<std::uint32_t*>(::operator new(4));
    model.morphKeyCursors = static_cast<std::uint32_t*>(::operator new(4));
    model.reservedMorphTable = ::operator new(16);
    model.pmxTextBuffers[0] = static_cast<wchar_t*>(::operator new(32));
    model.boneOrderTable = new mdl::BoneOrderEntry[3]{};
    model.undoRings[0].slots[0].bonePose =
        static_cast<mdl::BonePoseSnapshot*>(::operator new(sizeof(mdl::BonePoseSnapshot)));
    ModelDispose(bytes);
    const bool released = model.materials == nullptr
        && model.boneKeyCursors == nullptr
        && model.morphKeyCursors == nullptr
        && model.reservedMorphTable == nullptr
        && model.pmxTextBuffers[0] == nullptr
        && model.boneOrderTable == nullptr
        && model.undoRings[0].slots[0].bonePose == nullptr;
    ::operator delete(bytes);
    return released ? 0 : 1;
}
