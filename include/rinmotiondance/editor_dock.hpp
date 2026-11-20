#pragma once
#include <array>
#include "rinmotiondance/mmd_app.hpp"

namespace rinmotiondance {

enum class DockSection : int {
    Interpolation,
    Model,
    Edit,
    Secondary,
    Shadow,
    Accessory,
    Playback,
    Count
};

struct EditorDock {
    bool enabled{};
    bool camera{};
    std::array<int, 7> left{};
    std::array<int, 7> width{};
    int top{};
    int bottom{};
};

inline EditorDock ComputeEditorDock(MMDApp* app, int clientWidth,
                                    int clientHeight) {
    EditorDock dock{};
    dock.camera = app->state.optflag[0] != 0;
    dock.enabled = app->FloatingWindow() == nullptr &&
                   clientWidth >= 1500 && clientHeight >= 600;
    dock.top = clientHeight - 160;
    dock.bottom = clientHeight;
    if (!dock.enabled)
        return dock;

    const std::array<int, 7> cameraWidths{
        240, 220, 235, 245, 165, 250, 295
    };
    const std::array<int, 7> modelWidths{
        240, 230, 325, 565, 0, 0, 290
    };
    const auto& desired = dock.camera ? cameraWidths : modelWidths;
    int x = 0;
    for (int index = 0; index < 7; ++index) {
        dock.left[index] = x;
        if (desired[index] <= 0)
            continue;
        dock.width[index] = index == 6 ? clientWidth - x :
            static_cast<int>(static_cast<long long>(clientWidth) *
                             desired[index] / 1650);
        x += dock.width[index];
    }
    return dock;
}

inline int DockIndex(DockSection section) {
    return static_cast<int>(section);
}

}
