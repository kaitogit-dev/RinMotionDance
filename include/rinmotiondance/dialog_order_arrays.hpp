#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include "rinmotiondance/accessory_layout.hpp"

namespace rinmotiondance {
struct DialogOrderArrays {
    std::unique_ptr<std::int32_t[]> modelIndices;
    std::unique_ptr<std::int32_t[]> accessoryIndices;

    void SwapAccessories(std::size_t first, std::size_t second) noexcept {
        std::swap(accessoryIndices[first], accessoryIndices[second]);
    }
};

template<class AccessoryAt>
void BuildAccessoryOrderIndices(std::int32_t* order, int count,
                               AccessoryAt accessoryAt) {
    for (int i = 0; i < count; ++i) {
        for (int slot = 0; slot < 255; ++slot) {
            const mdl::AccessoryRecord* accessory = accessoryAt(slot);
            if (accessory != nullptr && accessory->order == i) {
                order[i] = slot;
                break;
            }
        }
    }
}

template<class AccessoryAt, class RenameAccessory>
std::uint8_t ApplyAccessoryOrderIndices(const std::int32_t* order, int count,
                                       AccessoryAt accessoryAt,
                                       RenameAccessory renameAccessory) {
    for (int i = 0; i < count; ++i) {
        mdl::AccessoryRecord* accessory = accessoryAt(order[i]);
        if (accessory == nullptr)
            continue;
        accessory->order = static_cast<std::uint8_t>(i);
        renameAccessory(i, *accessory);
    }
    return static_cast<std::uint8_t>(order[0]);
}

}  
