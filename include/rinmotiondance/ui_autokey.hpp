#pragma once
#include <cstddef>

namespace rinmotiondance {
class MMDApp;
void AutoKeyBoneEdit(MMDApp* app);
void AutoKeyMorphEdit(MMDApp* app, std::size_t lane);
}
