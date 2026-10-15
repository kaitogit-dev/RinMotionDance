#pragma once

#include <cstddef>

namespace rinmotiondance {

struct PathResolutionWorkspace {
    wchar_t projectDirectory[256];
    wchar_t executableDirectory[256];
    unsigned char scratch[2000];
    wchar_t resolvedPath[256];
};

static_assert(offsetof(PathResolutionWorkspace, executableDirectory) == 512);
static_assert(offsetof(PathResolutionWorkspace, resolvedPath) == 3024);
static_assert(sizeof(PathResolutionWorkspace) == 3536);

}  
