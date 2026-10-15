#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "frame_state_dump.hpp"
#include "bone_selection.hpp"

namespace rinmotiondance {

double g_MouseScaleA = 0.5;                       
double g_MouseScaleB = 0.004999999888241291;      
double g_MouseScaleC = 0.05000000074505806;       
float g_MouseScaleD = 0.0020000000949949f;        

static void ViewRefreshGate(MMDApp* s) {
    const bool frameGate =
        (s->state.followCameraEnabled | s->state.playbackStartsAtCurrentFrame |
         s->state.projectedShadowBlendEnabled) != 0 &&
        s->state.slotIdx == s->state.cameraParentModel &&
        (s->state.optflag[0] | s->state.optflag[1] |
         s->state.optflag[2] | s->state.optflag[3]) == 0 &&
        s->state.cameraParentModel >= 0 &&
        s->state.playbackActive == 0;
    if (frameGate) {
        s->state.viewDirty = 1;                                   
        s->state.windowLayoutReady = 0;
        PostLanguageSweep2(s);                                    
        s->state.windowLayoutReady = 1;
    }
    PostViewRefresh(s);                                           
}

namespace {

bool directBoneUndoPending = false;

void PollKey(MMDApp* app, std::int32_t& keyState, int virtualKey) {
    const std::int32_t oldState = keyState;
    const bool down = (GetKeyState(virtualKey) & 0x8000) != 0;
    std::int32_t state = 0;
    if (down)
        state = oldState == 0 ? 1 : 3;
    else if (oldState == 1 || oldState == 3)
        state = 2;
    keyState = state;
    if (state == 1 || state == 2)
        app->state.messageSeen = 1;
}

float DragScale(MMDApp* app) {
    if (app->ShiftModifierActive())
        return 0.5f;                            
    if (app->CtrlModifierActive())
        return 0.005f;                          
    return 0.05f;                               
}

void PanCameraPosition(MMDApp* app, int dx, int dy) {
    const float scale = DragScale(app);
    auto& api = d3dx::Get();
    d3dx::D3DXMATRIXF mx{}, my{}, mz{}, matrix{};
    api.rotY(&my, app->CameraYaw());
    api.rotX(&mx, app->CameraPitch());
    api.multiply(&matrix, &mx, &my);
    api.rotZ(&mz, -app->CameraRoll());
    api.multiply(&matrix, &mz, &matrix);

    const float sx = static_cast<float>(-dx) * scale;
    const float sy = static_cast<float>(dy) * scale;
    app->CameraPositionX() += sx * matrix.m[0][0] +
                              sy * matrix.m[1][0] + matrix.m[3][0];
    app->CameraPositionY() += sx * matrix.m[0][1] +
                              sy * matrix.m[1][1] + matrix.m[3][1];
    app->CameraPositionZ() -= sx * matrix.m[0][2] +
                              sy * matrix.m[1][2] + matrix.m[3][2];
}


int ViewportToolAtPoint(MMDApp* app) {
    app->ViewportToolHovered() = 0;
    app->ViewportToolOperation() = ViewportToolAction::None;
    if (app->ViewportInputActive() == 0)
        return 0;

    const int x = app->MouseX();
    const int y = app->MouseY();
    const RECT view = app->ViewportRect();
    HWND window = app->FloatingWindow();
    if (window == nullptr)
        window = app->state.hwnd;
    RECT client{};
    GetClientRect(window, &client);

    int operation = 0;
    if (y > view.top - 25 && y < view.top) {
        if (x > client.right - 60 && x < client.right - 36)
            operation = 1;
        else if (x > client.right - 30 && x < client.right - 6)
            operation = 2;
    }

    float scale = 1.0f;
    if (D3DRenderer* r = app->Renderer())
        scale = r->viewScale;  
    if (!(scale > 0.0f))
        scale = 1.0f;

    if (operation == 0 && app->PlaybackActive() == 0) {
        const float fx = static_cast<float>(x);
        const float fy = static_cast<float>(y);
        const float x0 = view.right - 127.0f * scale;
        const float x1 = view.right - 92.0f * scale;
        const float x2 = view.right - 86.0f * scale;
        const float x3 = view.right - 51.0f * scale;
        const float x4 = view.right - 48.0f * scale;
        const float x5 = view.right - 13.0f * scale;
        const float upperTop = view.bottom - 80.0f * scale;
        const float upperBottom = view.bottom - 50.0f * scale;
        const float lowerTop = view.bottom - 40.0f * scale;
        const float lowerBottom = view.bottom - 10.0f * scale;
        int column = -1;
        if (fx > x0 && fx < x1)
            column = 0;
        else if (fx > x2 && fx < x3)
            column = 1;
        else if (fx > x4 && fx < x5)
            column = 2;
        if (column >= 0) {
            const bool modelPanel =
                app->state.optflag[0] != 0;
            const std::uint32_t axisMode =
                app->state.v32c;
            if (fy > upperTop && fy < upperBottom) {
                if (modelPanel)
                    operation = 18 + column;
                else if (axisMode == 0 || axisMode == 1)
                    operation = 9 + column;
            } else if (fy > lowerTop && fy < lowerBottom) {
                if (modelPanel)
                    operation = 15 + column;
                else if (axisMode == 0 || axisMode == 2)
                    operation = 12 + column;
            }
        }

        if (operation == 0 && app->EditMode() == ViewportEditMode::Camera) {
            const int cx = app->ViewportToolCenterX();
            const int cy = app->ViewportToolCenterY();
            const int px = x - cx;
            const int py = y - cy;
            const bool horizontal =
                ((px > 7 && px < 35) || (px > -35 && px < -7)) &&
                py > -7 && py < 7;
            const bool vertical =
                ((py > 7 && py < 35) || (py > -35 && py < -7)) &&
                px > -7 && px < 7;
            if (horizontal)
                operation = 3;
            else if (vertical)
                operation = 4;
            else {
                const int radius2 = px * px + py * py;
                if (radius2 > 1225 && radius2 < 2025)
                    operation = 5;
            }
        } else if (operation == 0 &&
                   app->EditMode() == ViewportEditMode::Light) {
            const int px = x - app->ViewportToolCenterX();
            const int py = y - app->ViewportToolCenterY();
            if (px > 41 && px < 61 && py > -8 && py < 8)
                operation = 6;
            else if (px > -13 && px < 8 && py > -61 && py < -42)
                operation = 7;
            else if (px > -13 && px < 11 && py > -16 && py < 8)
                operation = 8;
        }

        if (operation == 0 &&
            app->InteractionDragMode() == ViewportDragMode::None) {
            const float coordLeft = view.right - 120.0f * scale;
            const float coordTop = view.bottom - 105.0f * scale;
            if (fx > coordLeft && fx < coordLeft + 25.0f * scale &&
                fy > coordTop && fy < coordTop + 25.0f * scale)
                operation = 21;
        }
    }

    if (operation == 0 && app->PlaybackActive() == 0)
        operation = PickTransformGizmo(app, x, y);

    if (operation != 0) {
        app->ViewportToolHovered() = 1;
        app->ViewportToolOperation() =
            static_cast<ViewportToolAction>(operation);
    }
    return operation;
}

HWND ViewportWindow(MMDApp* app) {
    HWND window = app->FloatingWindow();
    if (window == nullptr)
        window = app->state.hwnd;
    return window;
}

float ViewportScale(MMDApp* app) {
    D3DRenderer* r = app->Renderer();
    const float scale = r != nullptr ? r->viewScale : 1.0f;  
    return scale > 0.0f ? scale : 1.0f;
}

void WarpCursor(MMDApp* app, POINT point) {
    HWND window = ViewportWindow(app);
    if (window == nullptr)
        return;
    ClientToScreen(window, &point);
    SetCursorPos(point.x, point.y);
    app->state.separateWindowMouseSeen = 1;
}

void BeginCenteredDrag(MMDApp* app, int operation) {
    const RECT view = app->ViewportRect();
    POINT point{(view.left + view.right) / 2,
                (view.top + view.bottom) / 2};
    app->MouseX() = point.x;
    app->MouseY() = point.y;
    app->PreviousMouseX() = point.x;
    app->PreviousMouseY() = point.y;
    app->ViewToolDragOperation() =
        static_cast<ViewportToolAction>(operation);
    app->state.mouseJumped = 0;
    WarpCursor(app, point);
    while (ShowCursor(FALSE) >= 0) {}
}

void RestoreViewportToolCursor(MMDApp* app, int operation) {
    const RECT view = app->ViewportRect();
    const float scale = ViewportScale(app);
    POINT point{};
    if (operation == 1 || operation == 2) {
        HWND mainWindow = app->state.hwnd;
        RECT client{};
        GetClientRect(mainWindow, &client);
        point.x = client.right - (operation == 1 ? 48 : 18);
        point.y = view.top - 12;
    } else {
        const int column = operation == 9 || operation == 12 ||
                           operation == 15 || operation == 18 ? 0
                         : operation == 10 || operation == 13 ||
                           operation == 16 || operation == 19 ? 1 : 2;
        const bool upper = operation == 9 || operation == 10 ||
                           operation == 11 || operation == 18 ||
                           operation == 19 || operation == 20;
        static constexpr float kCenterX[3] = {110.0f, 70.0f, 30.0f};
        point.x = view.right - static_cast<int>(kCenterX[column] * scale);
        point.y = view.bottom - static_cast<int>((upper ? 65.0f : 25.0f) * scale);
    }
    WarpCursor(app, point);
    while (ShowCursor(TRUE) < 0) {}
}

unsigned char* ActiveBoneModel(MMDApp* app) {
    return app->ModelSlot(app->state.slotIdx);
}

bool PickBoneAtCursor(MMDApp* app) {
    auto* model = mdl::Mdl(ActiveBoneModel(app));
    if (model && PickBoneAtPoint(*model, app->MouseX(), app->MouseY(),
                                app->PlaybackPhysicsMode(), app->ShiftModifierActive())) {
        PostLanguageSweep(app);
        PostLanguageSweep2(app);
        return true;
    }
    return false;
}

void BeginDirectBoneDrag(MMDApp* app) {
    if (!PickBoneAtCursor(app) || app->ShiftModifierActive())
        return;
    auto* model = mdl::Mdl(ActiveBoneModel(app));
    if (model == nullptr || model->boneTable == nullptr)
        return;
    const int selected = model->selectedBone;
    if (selected < 0 || selected >= static_cast<int>(model->boneCount))
        return;
    const mdl::BoneType type = model->boneTable[selected].type;
    const bool movable = type == mdl::BoneType::Move || type == mdl::BoneType::Ik;
    app->InteractionDragMode() = movable
        ? ViewportDragMode::DirectBoneTranslate
        : ViewportDragMode::DirectBoneRotate;
    directBoneUndoPending = true;
}

void UpdateBoneBoxSelection(MMDApp* app) {
    if (app->BoneBoxSelectionActive() == 0)
        return;
    unsigned char* model = ActiveBoneModel(app);
    if (model == nullptr)
        return;
    auto* bones = rinmotiondance::mdl::Bones(model);
    auto* selected = rinmotiondance::mdl::Mdl(model)->boneSelection;
    const int count = rinmotiondance::mdl::Mdl(model)->boneCount;
    if (bones == nullptr || selected == nullptr || count <= 0)
        return;

    const int left = std::min(app->BoneBoxStartX(),
                              app->MouseX());
    const int right = std::max(app->BoneBoxStartX(),
                               app->MouseX());
    const int top = std::min(app->BoneBoxStartY(),
                             app->MouseY());
    const int bottom = std::max(app->BoneBoxStartY(),
                                app->MouseY());
    const bool additive = app->ShiftModifierActive();
    bool changed = false;
    for (int i = 0; i < count; ++i) {
        const mdl::BoneRecord& bone = bones[i];
        const mdl::BoneType type = bone.type;
        const bool eligible =
            type <= mdl::BoneType::Effector ||
            type == mdl::BoneType::FixedAxis;
        const int x = bone.selState;
        const int y = bone.selState2;
        const bool inside = eligible && x > left && x < right &&
                            y > top && y < bottom;
        const std::uint8_t next = inside ? 1 : (additive ? selected[i] : 0);
        changed |= selected[i] != next;
        selected[i] = next;
    }
    if (changed)
        PostLanguageSweep(app);
}

void BeginOrEndViewportToolDrag(MMDApp* app, int operation) {
    const int leftState = app->LeftMouseButtonState();
    if (leftState == 1) {
        directBoneUndoPending = false;
        if (operation == 1 || operation == 2) {
            BeginCenteredDrag(app, operation);
            return;
        }
        static constexpr std::uint8_t kDragMode[21] = {
            0, 0, 0, 1, 2, 3, 7, 8, 9, 4, 5, 6,
            10, 11, 12, 13, 14, 15, 16, 17, 18};
        if (operation >= 3 && operation <= 20) {
            const int mode = kDragMode[operation];
            if (operation >= 9)
                BeginCenteredDrag(app, operation);
            app->InteractionDragMode() = static_cast<ViewportDragMode>(mode);
            if (operation == 5) {
                app->ViewportToolDragOriginX() = app->ViewportToolCenterX();
                app->ViewportToolDragOriginY() = app->ViewportToolCenterY();
            }
            if (operation <= 8) {
                app->EditMode() = ViewportEditMode::ToolDrag;
                PushBoneEditUndo(app);
            } else if (operation <= 14) {
                PushBoneEditUndo(app);
                app->state.a06B5 = 1;
            } else {
                const int target = app->state.coordinateSystem;
                RefreshRequest(target == 2
                    ? app->state.selectedObjectSlot : -1);
                app->state.a06B5 = 1;
            }
        } else if (operation >= 22 && operation <= 27) {
            app->InteractionDragMode() = static_cast<ViewportDragMode>(operation - 1);
            directBoneUndoPending = true;
        } else if (operation == 21) {
            int& target = app->state.coordinateSystem;
            const int limit = app->state.optflag[0] != 0 ? 3 : 2;
            if (++target >= limit)
                target = 0;
            PostLanguageSweep(app);
        } else if (operation == 0 &&
                   app->PlaybackActive() == 0) {
            const RECT view = app->ViewportRect();
            const int x = app->MouseX();
            const int y = app->MouseY();
            if (x > view.left && x < view.right &&
                y > view.top && y < view.bottom) {
                const ViewportEditMode panelMode = app->EditMode();
                if (panelMode == ViewportEditMode::Bone)
                    BeginDirectBoneDrag(app);
                else if (panelMode == ViewportEditMode::BoneBox) {
                    app->BoneBoxStartX() = x;
                    app->BoneBoxStartY() = y;
                    app->BoneBoxSelectionActive() = 1;
                }
            }
        }
    } else if (leftState == 2) {
        directBoneUndoPending = false;
        app->PendingTimelineSelectionRow() = TimelineSelectionRow::None;;
        const int viewOperation =
            static_cast<int>(app->ViewToolDragOperation());
        const int mode = static_cast<int>(app->InteractionDragMode());
        const bool directBoneDrag =
            mode == static_cast<int>(ViewportDragMode::DirectBoneRotate) ||
            mode == static_cast<int>(ViewportDragMode::DirectBoneTranslate);
        if (viewOperation == 1 || viewOperation == 2) {
            RestoreViewportToolCursor(app, viewOperation);
            app->ViewToolDragOperation() = ViewportToolAction::None;
        }
        if (mode != 0) {
            if (!directBoneDrag && mode >= 1 && mode <= 3)
                app->EditMode() = ViewportEditMode::Camera;
            else if (!directBoneDrag && mode >= 7 && mode <= 9)
                app->EditMode() = ViewportEditMode::Light;
            if (viewOperation >= 9 && viewOperation <= 20) {
                RestoreViewportToolCursor(app, viewOperation);
                app->ViewToolDragOperation() = ViewportToolAction::None;
            }
            app->InteractionDragMode() = ViewportDragMode::None;
            app->state.a06B5 = 0;
        }
        app->BoneBoxSelectionActive() = 0;
    }
}

}  

bool PickBoneAtPoint(mdl::ModelRecord& model, int x, int y,
                     int physicsMode, bool additive) {
    if (!model.boneTable || !model.boneSelection || model.boneCount == 0)
        return false;
    int first = -1;
    int candidate = -1;
    bool changed = false;
    for (std::uint32_t i = 0; i < model.boneCount; ++i) {
        const mdl::BoneRecord& bone = model.boneTable[i];
        if (!(bone.flags & mdl::kBoneFlagVisible) ||
            !(bone.type < mdl::BoneType::InertTip || bone.type == mdl::BoneType::FixedAxis))
            continue;
        if (bone.hasRigidBody &&
            (physicsMode == 1 || (physicsMode == 2 && !bone.physicsDisabled)))
            continue;
        const int dx = bone.selState - x;
        const int dy = bone.selState2 - y;
        if (dx * dx + dy * dy >= 64)
            continue;
        if (additive) {
            model.boneSelection[i] = model.boneSelection[i] == 0 ? 1 : 0;
            if (model.boneSelection[i])
                model.selectedBone = static_cast<int>(i);
            else if (model.selectedBone == static_cast<int>(i))
                model.selectedBone = -1;
            changed = true;
        } else {
            if (first < 0) first = static_cast<int>(i);
            if (static_cast<int>(i) > model.selectedBone) {
                candidate = static_cast<int>(i);
                break;
            }
        }
    }
    if (!additive) {
        if (candidate < 0) candidate = first;
        if (candidate >= 0) {
            std::memset(model.boneSelection, 0, model.boneCount);
            model.boneSelection[candidate] = 1;
            model.selectedBone = candidate;
            changed = true;
        }
    }
    return changed;
}

void MouseInteractionBegin(MMDApp* app) {
    struct KeySlot { int key; std::int32_t MMDAppState::* state; };
    static constexpr KeySlot kKeySlots[] = {
        {VK_UP, &MMDAppState::upKeyState},
        {VK_DOWN, &MMDAppState::downKeyState},
        {VK_LEFT, &MMDAppState::leftKeyState},
        {VK_RIGHT, &MMDAppState::rightKeyState},
        {VK_SPACE, &MMDAppState::spaceKeyState},
        {VK_DELETE, &MMDAppState::deleteKeyState},
        {VK_ESCAPE, &MMDAppState::escKeyState},
        {VK_TAB, &MMDAppState::tabKeyState},
        {VK_RETURN, &MMDAppState::enterKeyState},
        {VK_MENU, &MMDAppState::menuKeyState},
        {221, &MMDAppState::keyState221},
        {226, &MMDAppState::keyState226},
    };
    for (const KeySlot& slot : kKeySlots)
        PollKey(app, app->state.*slot.state, slot.key);

    static constexpr struct { int key; int flagIndex; } kLetterKeys[] = {
        {'x', 0}, {'X', 0}, {'z', 1}, {'Z', 1},
        {'c', 2}, {'C', 2}, {'v', 3}, {'V', 3},
        {'d', 4}, {'D', 4}, {'a', 5}, {'A', 5},
        {'b', 6}, {'B', 6}, {'g', 7}, {'G', 7},
        {'s', 8}, {'S', 8}, {'i', 9}, {'I', 9},
        {'h', 10}, {'H', 10}, {'k', 11}, {'K', 11},
        {'p', 12}, {'P', 12}, {'u', 13}, {'U', 13},
        {'j', 14}, {'J', 14}, {'f', 15}, {'F', 15},
        {'r', 16}, {'R', 16}, {'l', 17}, {'L', 17},
    };
    for (const auto& slot : kLetterKeys)
        PollKey(app, app->state.dialogFlags[slot.flagIndex], slot.key);

    for (int i = 0; i < 10; ++i)
        PollKey(app, app->state.numpadKeyState[i], VK_NUMPAD0 + i);

    PollKey(app, app->ShiftModifierState(), VK_SHIFT);
    PollKey(app, app->CtrlModifierState(), VK_CONTROL);
    PollKey(app, app->LeftMouseButtonState(), VK_LBUTTON);
    PollKey(app, app->RightMouseButtonState(), VK_RBUTTON);
    PollKey(app, app->MiddleMouseButtonState(), VK_MBUTTON);

    HWND active = app->FloatingWindow();
    if (active == nullptr)
        active = app->state.hwnd;
    app->ViewportInputActive() = static_cast<std::uint8_t>(
        GetForegroundWindow() == active);
    const int operation = ViewportToolAtPoint(app);
    BeginOrEndViewportToolDrag(app, operation);
    if (app->ViewportInputActive() == 0)
        return;

    if (app->LeftMouseButtonHeld())
        UpdateBoneBoxSelection(app);

    if (directBoneUndoPending && app->LeftMouseButtonHeld() &&
        (app->MouseX() != app->PreviousMouseX() ||
         app->MouseY() != app->PreviousMouseY())) {
        PushBoneEditUndo(app);
        app->SceneModified() = 1;
        directBoneUndoPending = false;
    }

    const int dx = app->MouseX() - app->PreviousMouseX();
    const int dy = app->MouseY() - app->PreviousMouseY();

    if (app->LeftMouseButtonHeld()) {
        const ViewportToolAction viewMode = app->ViewToolDragOperation();
        if (viewMode == ViewportToolAction::CameraOrbit) {
            app->CameraDistance() -= static_cast<float>(dy) * 0.1f;
            ViewRefreshGate(app);
        } else if (viewMode == ViewportToolAction::CameraPan) {
            if (app->state.optflag[0] != 0)
                PanCameraPosition(app, dx, dy);
            else {
                app->ViewOffsetX() -= static_cast<float>(dx) * DragScale(app);
                app->ViewOffsetY() += static_cast<float>(dy) * DragScale(app);
            }
            ViewRefreshGate(app);
        }
    }

}

void MouseInteractionEnd(MMDApp* app) {
    app->PreviousMouseX() = app->MouseX();
    app->PreviousMouseY() = app->MouseY();
}

double g_AngleDegreesScale = 180.0;               
double g_AnglePiTruncated = 3.141592025756836;   

namespace {

#ifdef _WIN64
using PanelDragScalar = float;
#else
using PanelDragScalar = double;
#endif
constexpr PanelDragScalar kPositionDragCoarse = 0.5f;
constexpr PanelDragScalar kPositionDragFine = 0.005f;
constexpr PanelDragScalar kPositionDragNormal = 0.05f;
constexpr PanelDragScalar kAngleDragCoarse = 0.2f;
constexpr PanelDragScalar kAngleDragFine = 0.002f;
constexpr PanelDragScalar kAngleDragNormal = 0.02f;
constexpr PanelDragScalar kDisplayPi = 3.141592025756836f;
constexpr PanelDragScalar kHalfTurnDegrees = 180.0f;

int DyOf(MMDApp* s) {
    return s->MouseY() - s->PreviousMouseY();
}

bool SelA3(MMDApp* s) { return s->ShiftModifierActive(); }
bool SelB3(MMDApp* s) { return s->CtrlModifierActive(); }

void EchoEdit(MMDApp* s, int dlgItem, const char* text) {
    HWND window = static_cast<HWND>(s->Hwnd());
    SetWindowTextA(GetDlgItem(window, dlgItem), text);
}

}  

void ModeCameraAdjust(MMDApp* app, int axis) {
    static const int kEdit[3] = {0x1DE, 0x1DF, 0x1E0};
    if (axis < 0 || axis > 2) { ViewRefreshGate(app); return; }
    const int target = app->state.coordinateSystem;
    const int dy = DyOf(app);
    PanelDragScalar scale;
    if (SelA3(app)) scale = kPositionDragCoarse;
    else if (SelB3(app)) scale = kPositionDragFine;
    else scale = kPositionDragNormal;
    if (target == 2 &&
        app->state.optflag[0] != 0) {
        auto* accessory = app->AccessorySlot(app->state.selectedObjectSlot);
        if (accessory != nullptr) {
            float& value = accessory->position[axis];
            value = static_cast<float>(static_cast<PanelDragScalar>(value) -
                                       static_cast<PanelDragScalar>(dy) * scale);
            char buf[0x100];
            sprintf_s(buf, 0x100, "%3.4f", value);
            EchoEdit(app, kEdit[axis], buf);
        }
    } else if (app->state.cameraParentModel >= 0 || target == 1) {
        app->CameraPosition()[axis] = static_cast<float>(
            static_cast<PanelDragScalar>(app->CameraPosition()[axis]) +
            static_cast<PanelDragScalar>(dy) * scale);
    } else {
        auto& api = d3dx::Get();
        d3dx::D3DXMATRIXF rx{}, ry{}, rz{}, rotation{};
        api.rotZ(&rz, -app->CameraRoll());
        api.rotX(&rx, -app->CameraPitch());
        api.multiply(&rotation, &rz, &rx);
        api.rotY(&ry, -app->CameraYaw());
        api.multiply(&rotation, &rotation, &ry);
        float local[3]{};
        local[axis] = static_cast<float>(static_cast<PanelDragScalar>(dy) * scale);
        float world[4]{};
        api.vec3Transform(world, local, &rotation);
        app->CameraPositionX() += world[0];
        app->CameraPositionY() += world[1];
        app->CameraPositionZ() += world[2];
    }
    ViewRefreshGate(app);
}

void ModeAngleAdjust(MMDApp* app, int axis) {
    static const int kEdit[3] = {0x1E1, 0x1E2, 0x1E3};
    if (axis < 0 || axis > 2) { ViewRefreshGate(app); return; }
    const int target = app->state.coordinateSystem;
    const int dy = DyOf(app);
    PanelDragScalar scale;
    if (SelA3(app)) scale = kAngleDragCoarse;
    else if (SelB3(app)) scale = kAngleDragFine;
    else scale = kAngleDragNormal;
    if (target == 2) {
        auto* accessory = app->AccessorySlot(app->state.selectedObjectSlot);
        if (accessory != nullptr) {
            float& value = accessory->rotation[axis];
            value = static_cast<float>(static_cast<PanelDragScalar>(value) +
                                       static_cast<PanelDragScalar>(dy) * scale);
            char buf[0x100];
            sprintf_s(buf, 0x100, "%3.4f",
                      value / kDisplayPi * kHalfTurnDegrees);
            EchoEdit(app, kEdit[axis], buf);
        }
    } else {
        app->CameraRotation()[axis] = static_cast<float>(
            static_cast<PanelDragScalar>(app->CameraRotation()[axis]) +
            static_cast<PanelDragScalar>(dy) * scale);
    }
    ViewRefreshGate(app);
}

void ModeRotate(MMDApp* app, int  ) {
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ModeTranslate(MMDApp* app, int  ) {
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ModeScale(MMDApp* app) {
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ModeBoneRotate(MMDApp* app, int) {
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ModeDirectBoneRotate(MMDApp* app) {
    const ViewportDragMode mode = app->InteractionDragMode();
    if (app->MouseX() != app->PreviousMouseX()) {
        app->InteractionDragMode() = ViewportDragMode::ViewAxisRotateX;
        BoneEditModes(app);
    }
    if (app->MouseY() != app->PreviousMouseY()) {
        app->InteractionDragMode() = ViewportDragMode::ViewAxisRotateY;
        BoneEditModes(app);
    }
    app->InteractionDragMode() = mode;
    ViewRefreshGate(app);
}

void ModeDirectBoneTranslate(MMDApp* app) {
    const ViewportDragMode mode = app->InteractionDragMode();
    app->InteractionDragMode() = ViewportDragMode::BoneMoveScreenPlane;
    BoneEditModes(app);
    app->InteractionDragMode() = mode;
    ViewRefreshGate(app);
}

void ModeGizmoRotate(MMDApp* app, int axis) {
    const int pixels = TransformGizmoDragPixels(app);
    if (pixels == 0)
        return;
    const auto mode = app->InteractionDragMode();
    const int previousY = app->PreviousMouseY();
    app->InteractionDragMode() = static_cast<ViewportDragMode>(4 + axis);
    app->PreviousMouseY() = app->MouseY() + pixels;
    BoneEditModes(app);
    app->PreviousMouseY() = previousY;
    app->InteractionDragMode() = mode;
    ViewRefreshGate(app);
}

void ModeGizmoMove(MMDApp* app, int axis) {
    const int pixels = TransformGizmoDragPixels(app);
    if (pixels == 0)
        return;
    const auto mode = app->InteractionDragMode();
    const int previousY = app->PreviousMouseY();
    app->InteractionDragMode() = static_cast<ViewportDragMode>(10 + axis);
    app->PreviousMouseY() = app->MouseY() + (axis == 2 ? -pixels : pixels);
    BoneEditModes(app);
    app->PreviousMouseY() = previousY;
    app->InteractionDragMode() = mode;
    ViewRefreshGate(app);
}

void ModePhysicsBody(MMDApp* app, int) {
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ApplyCameraReferenceModeChange(MMDApp* app, int oldMode) {
    auto& api = d3dx::Get();
    d3dx::D3DXMATRIXF ry{}, rot{}, rz{};
    api.rotY(&ry, app->CameraYaw());
    api.rotX(&rot, app->CameraPitch());
    api.multiply(&rot, &ry, &rot);
    api.rotZ(&rz, app->CameraRoll());
    api.multiply(&rot, &rot, &rz);
    if (app->CameraParentModel() >= 0)
        api.multiply(&rot,
                     reinterpret_cast<d3dx::D3DXMATRIXF*>(
                         &app->CameraAttachmentBasis()),
                     &rot);

    unsigned char* model = app->SelectedModel();
    auto* bones = rinmotiondance::mdl::Bones(model);

    auto modeOrigin = [&](int modeValue, d3dx::D3DXMATRIXF* out) {
        if (modeValue == 1) {                                  
            const mdl::BoneRecord& rec =
                bones[mdl::Mdl(model)->centerBone];
            const float tx = -rec.matInit[12];
            const float ty = -rec.matInit[13];
            const float tz = -rec.matInit[14];
            api.translation(out, tx, ty, tz);
        } else if (modeValue == 2) {                           
            std::int32_t idx = rinmotiondance::mdl::Mdl(model)->selectedBone;
            if (idx < 0)
                idx = 0;
            const mdl::BoneRecord& rec = bones[idx];
            const float z = rec.matInit[2] * rec.position[0] +
                            rec.matInit[6] * rec.position[1] +
                            rec.matInit[10] * rec.position[2] +
                            rec.matInit[14];
            const float y = rec.matInit[1] * rec.position[0] +
                            rec.matInit[5] * rec.position[1] +
                            rec.matInit[9] * rec.position[2] +
                            rec.matInit[13];
            const float x = rec.matInit[0] * rec.position[0] +
                            rec.matInit[4] * rec.position[1] +
                            rec.matInit[8] * rec.position[2] +
                            rec.matInit[12];
            api.translation(out, -x, -y, -z);
        } else {
            api.translation(out, 0.0f, 0.0f, 0.0f);
        }
    };

    d3dx::D3DXMATRIXF tOld{}, tNew{};
    modeOrigin(oldMode, &tOld);                       
    modeOrigin(static_cast<int>(app->CameraReferenceMode()), &tNew);
    api.multiply(&tOld, &tOld, &rot);                 
    api.multiply(&tNew, &tNew, &rot);                 
    app->ViewOffsetX() -= tOld.m[3][0] - tNew.m[3][0];
    app->ViewOffsetY() -= tOld.m[3][1] - tNew.m[3][1];
    app->CameraDistance() -= tOld.m[3][2] - tNew.m[3][2];
}

}  
