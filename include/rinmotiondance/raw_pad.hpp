#pragma once

#include <cstddef>

namespace rinmotiondance {

template <std::size_t N>
struct RawPad { unsigned char b[N]; };

struct EmptyPad {};

}  
