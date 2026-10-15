#pragma once
#include <cstddef>

namespace rinmotiondance {
class D3DRenderer;

void ConvertAnsiToWide(const D3DRenderer* renderer, const char* source,
                       wchar_t* destination, int destinationWords);
void WideToSjis(const D3DRenderer* renderer, char* destination,
                const wchar_t* source, std::size_t destinationBytes);
}  
