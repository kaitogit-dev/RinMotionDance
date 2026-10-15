// VA 0x0054593C - the single `Block` application pointer.
// WinMain assigns it right after operator new; ported functions reach the
// state through rinmotiondance::g_Block exactly like the original global.
#include "rinmotiondance/mmd_app.hpp"

namespace rinmotiondance {

MMDApp* g_Block = nullptr;

}  // namespace rinmotiondance
