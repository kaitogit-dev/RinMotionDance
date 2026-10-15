#pragma once

namespace rinmotiondance {
namespace mdl { struct ModelRecord; }

bool PickBoneAtPoint(mdl::ModelRecord& model, int x, int y,
                     int physicsMode, bool additive);
}
