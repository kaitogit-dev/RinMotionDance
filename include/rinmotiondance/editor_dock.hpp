#pragma once
#include <array>
#include <algorithm>
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
                   clientWidth >= 1480 && clientHeight >= 600;
    dock.top = clientHeight - 160;
    dock.bottom = clientHeight;
    if (!dock.enabled)
        return dock;

    const std::array<int, 7> cameraWidths{
        195, 190, app->state.optflag[1] != 0 ? 230 : 100,
        app->state.optflag[2] != 0 ? 240 : 110,
        app->state.optflag[6] != 0 ? 185 : 110,
        app->state.optflag[3] != 0 ? 280 : 110, 245
    };
    const std::array<int, 7> modelWidths{
        200, 215, app->state.optflag[4] != 0 ? 265 : 110,
        app->state.optflag[5] != 0 ? 475 : 110, 0, 0, 250
    };
    const auto& desired = dock.camera ? cameraWidths : modelWidths;
    int total = 0;
    for (int width : desired)
        total += width;
    int x = 0;
    for (int index = 0; index < 7; ++index) {
        dock.left[index] = x;
        if (desired[index] == 0)
            continue;
        const int sectionWidth = index == 6 ? clientWidth - x :
            static_cast<int>(static_cast<long long>(clientWidth) *
                             desired[index] / total);
        dock.width[index] = sectionWidth;
        x += sectionWidth;
    }
    if (x != clientWidth) {
        dock.width[6] += clientWidth - x;
    }
    x = 0;
    for (int index = 0; index < 7; ++index) {
        dock.left[index] = x;
        x += dock.width[index];
    }
    return dock;
}

inline int DockIndex(DockSection section) {
    return static_cast<int>(section);
}

}
