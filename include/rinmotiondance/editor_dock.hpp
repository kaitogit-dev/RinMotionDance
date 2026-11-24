#pragma once
#include <algorithm>
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

inline DockSection modelInspectorPage = DockSection::Edit;
inline DockSection cameraInspectorPage = DockSection::Edit;

inline DockSection CurrentInspectorPage(bool camera) {
    return camera ? cameraInspectorPage : modelInspectorPage;
}

inline int InspectorPageCount(bool camera) {
    return camera ? 4 : 2;
}

inline DockSection InspectorPageAt(bool camera, int index) {
    if (!camera)
        return index == 0 ? DockSection::Edit : DockSection::Secondary;
    constexpr DockSection cameraPages[] = {
        DockSection::Edit,
        DockSection::Secondary,
        DockSection::Shadow,
        DockSection::Accessory
    };
    return cameraPages[std::clamp(index, 0, 3)];
}

inline const wchar_t* InspectorPageLabel(bool camera, int index) {
    constexpr const wchar_t* modelNames[] = {L"BONES", L"MORPHS"};
    constexpr const wchar_t* cameraNames[] = {
        L"CAMERA", L"LIGHT", L"SHADOW", L"ACCESSORY"
    };
    const int clamped = std::clamp(index, 0, InspectorPageCount(camera) - 1);
    return camera ? cameraNames[clamped] : modelNames[clamped];
}

struct EditorDock {
    bool enabled{};
    bool camera{};
    DockSection page{DockSection::Edit};
    std::array<int, 7> left{};
    std::array<int, 7> width{};
    int top{};
    int bottom{};
};

inline EditorDock ComputeEditorDock(MMDApp* app, int clientWidth,
                                    int clientHeight) {
    EditorDock dock{};
    dock.camera = app->state.optflag[0] != 0;
    dock.page = CurrentInspectorPage(dock.camera);
    dock.enabled = app->FloatingWindow() == nullptr &&
                   clientWidth >= 1080 && clientHeight >= 590;
    dock.top = clientHeight - 160;
    dock.bottom = clientHeight;
    if (!dock.enabled)
        return dock;

    const int interpolationWidth = clientWidth < 1280
        ? 200 : std::clamp(clientWidth / 8, 245, 260);
    const int modelWidth = std::clamp(clientWidth / 7, 195, 253);
    const int playbackWidth = std::clamp(clientWidth / 6, 260, 310);
    const int inspectorLeft = interpolationWidth + modelWidth;

    dock.left[0] = 0;
    dock.width[0] = interpolationWidth;
    dock.left[1] = interpolationWidth;
    dock.width[1] = modelWidth;
    const int active = static_cast<int>(dock.page);
    dock.left[active] = inspectorLeft;
    dock.width[active] = clientWidth - inspectorLeft - playbackWidth;
    dock.left[6] = clientWidth - playbackWidth;
    dock.width[6] = playbackWidth;
    return dock;
}

inline int InspectorTabWidth(const EditorDock& dock) {
    const int index = static_cast<int>(dock.page);
    const int usable = std::max(1, dock.width[index] - 24);
    return std::min(112, usable / InspectorPageCount(dock.camera));
}

inline bool HitEditorDockTab(MMDApp* app, int x, int y,
                             int clientWidth, int clientHeight) {
    const EditorDock dock =
        ComputeEditorDock(app, clientWidth, clientHeight);
    if (!dock.enabled || y < dock.top + 2 || y >= dock.top + 24)
        return false;
    const int active = static_cast<int>(dock.page);
    const int start = dock.left[active] + 9;
    const int tabWidth = InspectorTabWidth(dock);
    const int count = InspectorPageCount(dock.camera);
    if (x < start || x >= start + tabWidth * count)
        return false;
    const int choice = (x - start) / tabWidth;
    if (dock.camera)
        cameraInspectorPage = InspectorPageAt(true, choice);
    else
        modelInspectorPage = InspectorPageAt(false, choice);
    return true;
}

inline int DockIndex(DockSection section) {
    return static_cast<int>(section);
}

}
