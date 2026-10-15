#pragma once

#include <cstddef>

namespace rinmotiondance {

inline constexpr std::size_t kTimelineHitGridStride = 200;

template<class Visit>
void VisitTimelineSelectionCells(int firstRow, int lastRow,
                                 int firstColumn, int lastColumn,
                                 Visit&& visit) {
    for (int row = firstRow; row < lastRow; ++row)
        for (int column = firstColumn; column < lastColumn; ++column)
            visit(static_cast<std::size_t>(row) * kTimelineHitGridStride +
                      static_cast<std::size_t>(column), column);
}

}  
