#include <cstdint>

#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

std::uint32_t ColorLerp(std::uint32_t a, std::uint32_t b, float t) {
    const unsigned char rA = static_cast<unsigned char>(a & 0xFF);
    const unsigned char gA = static_cast<unsigned char>((a >> 8) & 0xFF);
    const unsigned char bA = static_cast<unsigned char>((a >> 16) & 0xFF);
    const unsigned char rB = static_cast<unsigned char>(b & 0xFF);
    const unsigned char gB = static_cast<unsigned char>((b >> 8) & 0xFF);
    const unsigned char bB = static_cast<unsigned char>((b >> 16) & 0xFF);

    const int r = static_cast<int>(static_cast<double>(rA) -
                                   static_cast<double>(rA - rB) * t);
    const int g = static_cast<int>(static_cast<double>(gA) -
                                   static_cast<double>(gA - gB) * t);
    const int bl = static_cast<int>(static_cast<double>(bA) -
                                    static_cast<double>(bA - bB) * t);

    return static_cast<std::uint32_t>((r & 0xFF) | ((g & 0xFF) << 8) |
                                      ((bl & 0xFF) << 16));
}

}  
