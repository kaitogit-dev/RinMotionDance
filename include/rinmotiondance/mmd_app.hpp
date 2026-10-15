#pragma once

#include <cstddef>
#include <cstdint>

#include "rinmotiondance/app_layout.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/clipboard_layout.hpp"
#include "rinmotiondance/d3d_wrapper.hpp"
#include "rinmotiondance/dialog_order_arrays.hpp"
#include "rinmotiondance/dshow_recorder.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/physics_scene.hpp"
#include "rinmotiondance/path_workspace.hpp"
#include "rinmotiondance/wave_audio_context.hpp"

namespace rinmotiondance {

enum class UiThemeColor : int {
    WindowFill = 0,
    WindowBorder = 1,
    ControlLight = 2,
    ControlDark = 3,
    TimelineBase = 22,
    TimelineRows = 23,
    TimelineGrid = 24,
    PanelHeader = 25,
    PanelBody = 26,
    HeaderBand = 27,
    RowBand = 28,
    LabelGrid = 29,
    SelectionText = 30,
    SelectionFill = 31,
    SelectionBorder = 32,
    DisabledText = 33,
    Accent = 34,
};

enum class AccessoryRenderPass : std::int32_t {
    None = 0,
    FixedFunction = 1,
    Effect = 2,
    ProjectedGroundShadow = 3,
    ModelOutline = 4,
    ModelEffect = 5,
};

enum class ViewportEditMode : std::int32_t {
    Bone = 0,
    BoneBox = 1,
    None = 2,
    Camera = 3,
    Light = 4,
    ToolDrag = 5,
};

enum class GlobalTimelineTrack : std::uint8_t {
    Camera = 0,
    Light = 1,
    SelfShadow = 2,
    Gravity = 3,
};

enum class TimelineSelectionBand : std::uint8_t {
    Camera = 0,
    Light = 1,
    SelfShadow = 2,
    Gravity = 3,
    Accessory = 4,
    ModelIk = 5,
    ModelMorph = 6,
    ModelBone = 7,
};

enum class TimelineSelectionRow : std::uint8_t {
    None = 0,
    Bone = 1,
    Morph = 2,
};

struct TimelineSelectionRecord {
    std::int32_t words[4];
};

extern TimelineSelectionRecord* g_timelineSelectionRecords[8];

enum class ScreenCaptureMode : std::int32_t {
    Disabled = 0,
    FullFrame = 1,
    CropFourByThree = 2,
    BackgroundRefresh = 3,
};

using DepthTextureProvider = void(__stdcall*)(IDirect3DBaseTexture9**);

enum class ViewportDragMode : std::int32_t {
    None = 0,
    ViewAxisRotateX = 1,
    ViewAxisRotateY = 2,
    ViewAxisRotateZ = 3,
    LocalAxisTranslateX = 4,
    LocalAxisTranslateY = 5,
    LocalAxisTranslateZ = 6,
    BoneScale = 7,
    BoneMoveVertical = 8,
    BoneMoveScreenPlane = 9,
    PhysicsAxisX = 10,
    PhysicsAxisY = 11,
    PhysicsAxisZ = 12,
    CameraAdjustX = 13,
    CameraAdjustY = 14,
    CameraAdjustZ = 15,
    AngleAdjustX = 16,
    AngleAdjustY = 17,
    AngleAdjustZ = 18,
};

enum class ViewportToolAction : std::int32_t {
    None = 0,
    CameraOrbit = 1,
    CameraPan = 2,
    CameraGizmoHorizontal = 3,
    CameraGizmoVertical = 4,
    CameraGizmoRing = 5,
    LightGizmoHorizontal = 6,
    LightGizmoVertical = 7,
    LightGizmoCenter = 8,
    TransformAxisX = 9,
    TransformAxisY = 10,
    TransformAxisZ = 11,
    PhysicsAxisX = 12,
    PhysicsAxisY = 13,
    PhysicsAxisZ = 14,
    CameraAdjustX = 15,
    CameraAdjustY = 16,
    CameraAdjustZ = 17,
    AngleAdjustX = 18,
    AngleAdjustY = 19,
    AngleAdjustZ = 20,
    CycleCoordinateSystem = 21,
};

enum class CameraAttachmentReference : std::uint8_t {
    None = 0,
    ModelRoot = 1,
    SelectedBone = 2,
};

class MMDApp {
public:
    MMDApp();                                
    void InitDefaults();                     

    MMDAppState state;

    HWND& FrameRangeDialog() {
#if defined(_M_X64)
        return m_frameRangeDialog;
#else
        return state.frameRangeDialog;
#endif
    }

    HWND& GravitySettingDialog() {
#if defined(_M_X64)
        return m_gravitySettingDialog;
#else
        return state.gravitySettingDialog;
#endif
    }

    HWND& GroundShadowColorDialog() {
#if defined(_M_X64)
        return m_groundShadowColorDialog;
#else
        return state.groundShadowColorDialog;
#endif
    }
    WNDPROC& GroundShadowColorEditProc() {
#if defined(_M_X64)
        return m_groundShadowColorEditProc;
#else
        return state.groundShadowColorEditProc;
#endif
    }

    DialogOrderArrays& DialogOrders() { return m_dialogOrders; }

    std::int32_t& MorphFrameShift() {
#if defined(_M_X64)
        return m_morphFrameShift;
#else
        return state.morphFrameShift;
#endif
    }
    std::int32_t& BlinkStartFrame() {
#if defined(_M_X64)
        return m_blinkStartFrame;
#else
        return state.blinkStartFrame;
#endif
    }
    std::int32_t& BlinkEndFrame() {
#if defined(_M_X64)
        return m_blinkEndFrame;
#else
        return state.blinkEndFrame;
#endif
    }

    WNDPROC& EdgeThicknessEditProc() {  
#if defined(_M_X64)
        return m_edgeThicknessEditProc;
#else
        return state.edgeThicknessEditProc;
#endif
    }
    WNDPROC& ModelEdgeEditProc() {
#if defined(_M_X64)
        return m_modelEdgeEditProc;
#else
        return state.modelEdgeEditProc;
#endif
    }
    std::int32_t& ModelEdgeComboCursor(int combo) {  
#if defined(_M_X64)
        return m_modelEdgeComboCursor[combo];
#else
        return state.modelEdgeComboCursor[combo];
#endif
    }
    WNDPROC& FrameCopyEditProc() {
#if defined(_M_X64)
        return m_frameCopyEditProc;
#else
        return state.frameCopyEditProc;
#endif
    }
    WNDPROC& AccessoryFrameEditProc() {
#if defined(_M_X64)
        return m_accessoryFrameEditProc;
#else
        return state.accessoryFrameEditProc;
#endif
    }

    float& RotationDialogTemp(int index) {
#if defined(_M_X64)
        return m_rotationDialogTemp[index];
#else
        return state.rotationDialogTemp[index];
#endif
    }

    unsigned char* CameraFrameScratch() {
#if defined(_M_X64)
        return m_cameraFrameScratch;
#else
        return state.cameraFrameScratch;
#endif
    }
    unsigned char* BoneFrameScratch() {
#if defined(_M_X64)
        return m_boneFrameScratch;
#else
        return state.boneFrameScratch;
#endif
    }

    float* GroundShadowColor() {
        return reinterpret_cast<float*>(CameraFrameScratch() + 112);
    }

    void*& AccessoryEditArray() {
#if defined(_M_X64)
        return m_accessoryEditArray;
#else
        return state.accessoryEditArray;
#endif
    }
    unsigned char& AccessoryApplyGate() {
#if defined(_M_X64)
        return m_accessoryApplyGate;
#else
        return state.accessoryApplyGate;
#endif
    }

    wchar_t* CaptureSavePath() {
#if defined(_M_X64)
        return m_captureSavePath;
#else
        return state.captureSavePath;
#endif
    }


    PathResolutionWorkspace& PathWorkspace() {
#if defined(_M_X64)
        return m_pathWorkspace;
#else
        return state.pathWorkspace;
#endif
    }
    const PathResolutionWorkspace& PathWorkspace() const {
#if defined(_M_X64)
        return m_pathWorkspace;
#else
        return state.pathWorkspace;
#endif
    }

    mdl::AccessoryRecord*& AccessorySlot(int index) {
        return AccessorySlots()[index];
    }
    mdl::AccessoryRecord* AccessorySlot(int index) const {
        return AccessorySlots()[index];
    }
    mdl::AccessoryRecord** AccessorySlots() {
        return reinterpret_cast<mdl::AccessoryRecord**>(state.objectSlots);
    }
    mdl::AccessoryRecord* const* AccessorySlots() const {
        return reinterpret_cast<mdl::AccessoryRecord* const*>(state.objectSlots);
    }
    void*& ObjectSlot(int index) {
        return reinterpret_cast<void*&>(AccessorySlots()[index]);
    }
    const void* ObjectSlot(int index) const {
        return reinterpret_cast<void* const&>(AccessorySlots()[index]);
    }
    unsigned char*& ModelSlot(int index) {
        return ModelSlots()[index];
    }
    unsigned char* ModelSlot(int index) const {
        return ModelSlots()[index];
    }
    unsigned char** ModelSlots() {
        return reinterpret_cast<unsigned char**>(state.modelSlots);
    }
    unsigned char* const* ModelSlots() const {
        return reinterpret_cast<unsigned char* const*>(state.modelSlots);
    }
    void ClearModelSlots() {
        for (int index = 0; index < kModelSlotCount; ++index)
            ModelSlot(index) = nullptr;
    }
    std::uint8_t& SelectedModelSlot() {
        return state.slotIdx;
    }
    std::uint8_t SelectedModelSlot() const {
        return state.slotIdx;
    }
    void SetSelectedModelSlot(std::uint8_t slot) {
        SelectedModelSlot() = slot;
    }
    unsigned char*& SelectedModel() {
        return ModelSlot(SelectedModelSlot());
    }
    unsigned char* SelectedModel() const {
        return ModelSlot(SelectedModelSlot());
    }
    void*& BoneCopyRecords() { return state.boneCopyRecords; }  
    mdl::BoneClipboardRecord*& BoneClipboard() {
#if defined(_M_X64)
        return reinterpret_cast<mdl::BoneClipboardRecord*&>(
            m_boneClipboard);
#else
        return reinterpret_cast<mdl::BoneClipboardRecord*&>(
            state.boneClipboard);
#endif
    }
    mdl::MorphClipboardRecord*& MorphClipboard() {
#if defined(_M_X64)
        return reinterpret_cast<mdl::MorphClipboardRecord*&>(
            m_morphClipboard);
#else
        return reinterpret_cast<mdl::MorphClipboardRecord*&>(
            state.morphClipboard);
#endif
    }
    mdl::DisplayClipboardRecord*& DisplayClipboard() {
        return reinterpret_cast<mdl::DisplayClipboardRecord*&>(
            state.displayClipboard);
    }
    mdl::CameraClipboardRecord*& CameraClipboard() {
#if defined(_M_X64)
        return reinterpret_cast<mdl::CameraClipboardRecord*&>(
            m_cameraClipboard);
#else
        return reinterpret_cast<mdl::CameraClipboardRecord*&>(
            state.cameraClipboard);
#endif
    }
    mdl::LightClipboardRecord*& LightClipboard() {
#if defined(_M_X64)
        return reinterpret_cast<mdl::LightClipboardRecord*&>(
            m_lightClipboard);
#else
        return reinterpret_cast<mdl::LightClipboardRecord*&>(
            state.lightClipboard);
#endif
    }
    mdl::ShadowClipboardRecord*& ShadowClipboard() {
#if defined(_M_X64)
        return reinterpret_cast<mdl::ShadowClipboardRecord*&>(
            m_shadowClipboard);
#else
        return reinterpret_cast<mdl::ShadowClipboardRecord*&>(
            state.shadowClipboard);
#endif
    }
    mdl::GravityClipboardRecord*& GravityClipboard() {
#if defined(_M_X64)
        return reinterpret_cast<mdl::GravityClipboardRecord*&>(
            m_gravityClipboard);
#else
        return reinterpret_cast<mdl::GravityClipboardRecord*&>(
            state.gravityClipboard);
#endif
    }
    mdl::AccessoryClipboardKey*& AccessoryClipboard() {
#if defined(_M_X64)
        return reinterpret_cast<mdl::AccessoryClipboardKey*&>(
            m_accessoryClipboard);
#else
        return reinterpret_cast<mdl::AccessoryClipboardKey*&>(
            state.accessoryClipboard);
#endif
    }
    mdl::CameraKey*& CameraKeys() {
        return reinterpret_cast<mdl::CameraKey*&>(state.cameraKeyTrack);
    }
    mdl::LightKey*& LightKeys() {
        return reinterpret_cast<mdl::LightKey*&>(state.lightKeyTrack);
    }
    mdl::SelfShadowKey*& ShadowKeys() {
        return reinterpret_cast<mdl::SelfShadowKey*&>(state.selfShadowKeyTrack);
    }
    mdl::GravityKey*& GravityKeys() {
        return reinterpret_cast<mdl::GravityKey*&>(state.gravityKeyTrack);
    }
    mdl::AccessoryKey*& AccessoryKeys(int slot) {
        return AccessoryKeyTracks()[slot];
    }
    mdl::AccessoryKey** AccessoryKeyTracks() {
        return reinterpret_cast<mdl::AccessoryKey**>(state.accKeyTracks);
    }
    mdl::AccessoryKey* const* AccessoryKeyTracks() const {
        return reinterpret_cast<mdl::AccessoryKey* const*>(state.accKeyTracks);
    }
    std::uint8_t& GlobalTrackSelected(GlobalTimelineTrack track) {
        return state.globalTrackSelected[static_cast<std::uint8_t>(track)];
    }
    std::uint8_t GlobalTrackSelected(GlobalTimelineTrack track) const {
        return state.globalTrackSelected[static_cast<std::uint8_t>(track)];
    }
    void SelectGlobalTimelineTrack(GlobalTimelineTrack track) {
        for (std::uint8_t index = 0; index < 4; ++index)
            state.globalTrackSelected[index] =
                index == static_cast<std::uint8_t>(track) ? 1 : 0;
    }
    void ClearGlobalTimelineTrackSelection() {
        for (std::uint8_t index = 0; index < 4; ++index)
            state.globalTrackSelected[index] = 0;
    }
    std::uint8_t& TimelineSelectionChanged() {
        return state.timelineSelectionChanged;
    }
    std::uint8_t TimelineSelectionChanged() const {
        return state.timelineSelectionChanged;
    }
    std::uint8_t& SceneModified() {
        return state.sceneModified;
    }
    std::uint8_t SceneModified() const {
        return state.sceneModified;
    }
    TimelineSelectionRow& PendingTimelineSelectionRow() {
        return reinterpret_cast<TimelineSelectionRow&>(state.pendingTimelineSelectionRow);
    }
    TimelineSelectionRow PendingTimelineSelectionRow() const {
        return static_cast<TimelineSelectionRow>(state.pendingTimelineSelectionRow);
    }
    std::uint8_t& SelectionBoxDragging() { return state.selectionBoxDragging; }
    std::int32_t& MouseX() { return state.mouseX; }
    std::int32_t& MouseY() { return state.mouseY; }
    std::int32_t& PreviousMouseX() { return state.previousMouseX; }
    std::int32_t& PreviousMouseY() { return state.previousMouseY; }
    std::uint8_t& ViewportInputActive() {
        return state.viewportInputActive;
    }
    std::int32_t& ShiftModifierState() { return state.shiftModifierState; }
    std::int32_t& CtrlModifierState() { return state.ctrlModifierState; }
    bool ShiftModifierActive() const {
        return state.shiftModifierState == 3;
    }
    bool CtrlModifierActive() const {
        return state.ctrlModifierState == 3;
    }
    std::uint8_t& SidebarResizeDragging() {
        return state.sidebarResizeDragging;
    }
    std::int32_t& LeftMouseButtonState() { return state.leftMouseButtonState; }
    std::int32_t& RightMouseButtonState() { return state.rightMouseButtonState; }
    std::int32_t& MiddleMouseButtonState() { return state.middleMouseButtonState; }
    bool LeftMouseButtonHeld() const { return state.leftMouseButtonState == 3; }
    bool RightMouseButtonHeld() const { return state.rightMouseButtonState == 3; }
    bool MiddleMouseButtonHeld() const { return state.middleMouseButtonState == 3; }
    std::int32_t& SelectionBoxAnchorX() {
#if defined(_M_X64)
        return m_selectionBoxAnchorX;
#else
        return state.selectionBoxAnchorX;  
#endif
    }
    std::int32_t& SelectionBoxAnchorY() {
#if defined(_M_X64)
        return m_selectionBoxAnchorY;
#else
        return state.selectionBoxAnchorY;  
#endif
    }
    std::int32_t& TimelineRangeFirstOffset() {
#if defined(_M_X64)
        return m_timelineRangeFirstOffset;
#else
        return state.timelineRangeFirstOffset;  
#endif
    }
    std::int32_t& TimelineRangeFirstBase() {
#if defined(_M_X64)
        return m_timelineRangeFirstBase;
#else
        return state.timelineRangeFirstBase;  
#endif
    }
    std::int32_t& TimelineRangeLastOffset() {
#if defined(_M_X64)
        return m_timelineRangeLastOffset;
#else
        return state.timelineRangeLastOffset;  
#endif
    }
    std::int32_t& TimelineRangeLastBase() {
#if defined(_M_X64)
        return m_timelineRangeLastBase;
#else
        return state.timelineRangeLastBase;  
#endif
    }
    std::uint8_t& TimelineRangeApplyEnabled() {
        return state.timelineRangeApplyEnabled;
    }
    void ClearTimelineRange() {
        TimelineRangeFirstOffset() = 0;
        TimelineRangeFirstBase() = 0;
        TimelineRangeLastOffset() = 0;
        TimelineRangeLastBase() = 0;
        TimelineRangeApplyEnabled() = 0;
    }
    std::int32_t& TimelineSelectionCount(TimelineSelectionBand band) {
        const auto idx = static_cast<std::size_t>(band);
#if defined(_M_X64)
        return m_timelineSelectionSlots[2 * idx];
#else
        return state.timelineSelectionSlots[2 * idx];
#endif
    }
    void* TimelineSelectionRegion() {
#if defined(_M_X64)
        return m_timelineSelectionSlots;
#else
        return state.timelineSelectionSlots;
#endif
    }
    TimelineSelectionRecord*& TimelineSelectionRecords(
        TimelineSelectionBand band) {
        return g_timelineSelectionRecords[
            static_cast<std::size_t>(band)];
    }
    mdl::ClipboardSelectionCounts& ClipboardCounts() {
        return state.clipboardCounts;
    }
    const mdl::ClipboardSelectionCounts& ClipboardCounts() const {
        return state.clipboardCounts;
    }
    std::int32_t& CurrentFrame() {
        return reinterpret_cast<std::int32_t&>(state.currentFrame);
    }
    HWND& MainWindow() {
        return state.hwnd;
    }
    float* LightDirection() {
        return state.lightDirection;
    }
    float* LightColor() {
        return state.lightColor;
    }
    D3DLIGHT9& SceneLight() {
#if defined(_M_X64)
        return m_sceneLight;
#else
        return reinterpret_cast<D3DLIGHT9&>(state.lightDirection[3]);
#endif
    }
    const D3DLIGHT9& SceneLight() const {
#if defined(_M_X64)
        return m_sceneLight;
#else
        return reinterpret_cast<const D3DLIGHT9&>(state.lightDirection[3]);
#endif
    }
    void ApplyTimelineLightState() {
        D3DLIGHT9& light = SceneLight();
        light.Type = D3DLIGHT_DIRECTIONAL;
        light.Direction.x = LightDirection()[0];
        light.Direction.y = LightDirection()[1];
        light.Direction.z = LightDirection()[2];
        light.Specular.r = LightColor()[0];
        light.Specular.g = LightColor()[1];
        light.Specular.b = LightColor()[2];
        light.Ambient.r = LightColor()[0];
        light.Ambient.g = LightColor()[1];
        light.Ambient.b = LightColor()[2];
    }
    std::uint8_t& GroundShadowEnabled() {
        return state.groundShadowEnabled;
    }
    IDirect3DVertexBuffer9*& GroundGridVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(
            state.groundGridVertices);
    }
    IDirect3DIndexBuffer9*& GroundGridIndices() {
        return reinterpret_cast<IDirect3DIndexBuffer9*&>(
            state.groundGridIndices);
    }
    ViewportEditMode& EditMode() {
        return reinterpret_cast<ViewportEditMode&>(state.editMode);
    }
    const ViewportEditMode& EditMode() const {
        return const_cast<const ViewportEditMode&>(
            reinterpret_cast<const ViewportEditMode&>(state.editMode));
    }
    bool UsesViewportTool() const {
        return static_cast<std::int32_t>(EditMode()) >=
               static_cast<std::int32_t>(ViewportEditMode::None);
    }
    std::int32_t& ViewportToolCenterX() { return state.viewportToolCenterX; }
    std::int32_t& ViewportToolCenterY() { return state.viewportToolCenterY; }
    std::uint32_t& ViewportToolHovered() {
        return state.viewportToolHovered;
    }
    ViewportToolAction& ViewportToolOperation() {
        return reinterpret_cast<ViewportToolAction&>(state.viewportToolOperation);
    }
    ViewportToolAction& ViewToolDragOperation() {
        return reinterpret_cast<ViewportToolAction&>(
            state.viewToolDragOperation);
    }
    ViewportDragMode& InteractionDragMode() {
        return reinterpret_cast<ViewportDragMode&>(state.interactionDragMode);
    }
    std::int32_t& ViewportToolDragOriginX() {
        return state.dragOriginX;
    }
    std::int32_t& ViewportToolDragOriginY() {
        return state.dragOriginY;
    }
    std::int32_t& BoneBoxStartX() { return state.boneBoxStartX; }
    std::int32_t& BoneBoxStartY() { return state.boneBoxStartY; }
    std::int32_t& BoneBoxSelectionActive() {
        return state.boneBoxSelectionActive;
    }
    D3DMATRIX& LightViewProjection() {
#if defined(_M_X64)
        return m_lightViewProjectionMatrix;
#else
        return reinterpret_cast<D3DMATRIX&>(state.lightViewProjectionMatrix);
#endif
    }
    const D3DMATRIX& LightViewProjection() const {
#if defined(_M_X64)
        return m_lightViewProjectionMatrix;
#else
        return reinterpret_cast<const D3DMATRIX&>(
            state.lightViewProjectionMatrix);
#endif
    }
    D3DMATRIX& WorldViewProjection() {
#if defined(_M_X64)
        return m_worldViewProjectionMatrix;
#else
        return reinterpret_cast<D3DMATRIX&>(state.worldViewProjectionMatrix);
#endif
    }
    const D3DMATRIX& WorldViewProjection() const {
#if defined(_M_X64)
        return m_worldViewProjectionMatrix;
#else
        return reinterpret_cast<const D3DMATRIX&>(
            state.worldViewProjectionMatrix);
#endif
    }
    IDirect3DTexture9*& AviBackgroundTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.aviBackgroundTexture);
    }
    IDirect3DSurface9*& AviBackgroundSurface() {
        return reinterpret_cast<IDirect3DSurface9*&>(state.aviBackgroundSurface);
    }
    void*& AviDrawDib() {
        return state.drawDib;
    }
    void*& AviFile() {
        return state.aviFile;
    }
    void*& AviStream() {
        return state.aviStream;
    }
    void*& AviFrameReader() {
        return state.aviFrameReader;
    }
    HWND& FloatingWindow() {
        return state.floatingWindow;
    }
    HWND& RecordingWindow() {
        return state.recordingWindow;
    }
    IDirect3DTexture9*& ToonTexture(int index) {
        return reinterpret_cast<IDirect3DTexture9*&>(
            state.toonTextures[index]);
    }
    IDirect3DTexture9*& SceneFontTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.sceneFontTexture);
    }
    IDirect3DVertexBuffer9*& AviOverlayVertices() {
#if defined(_M_X64)
        return m_overlayVertexBuffers.avi;
#else
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.aviOverlayVertices);
#endif
    }
    IDirect3DTexture9*& PictureBackgroundTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(
            state.pictureBackgroundTexture);
    }
    IDirect3DVertexBuffer9*& PictureOverlayVertices() {
#if defined(_M_X64)
        return m_overlayVertexBuffers.picture;
#else
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.pictureOverlayVertices);
#endif
    }
    wchar_t* AviBackgroundPath() {
        return state.aviBackgroundPath;
    }
    wchar_t* PictureBackgroundPath() {
        return state.pictureBackgroundPath;
    }
    std::int32_t& AviStreamStartFrame() {
        return state.aviStreamStart;
    }
    std::int32_t& AviStreamEndFrame() {
        return state.aviStreamEnd;
    }
    std::uint8_t& AviUsesThirtyFpsTiming() {
        return state.aviUsesThirtyFpsTiming;
    }
    std::int32_t& AviOffsetX() { return state.aviOffsetX; }
    std::int32_t& AviOffsetY() { return state.aviOffsetY; }
    float& AviScale() { return state.aviScale; }
    std::int32_t& AviFrameWidth() { return state.aviFrameWidth; }
    std::int32_t& AviFrameHeight() { return state.aviFrameHeight; }
    std::uint8_t& PictureBackgroundEnabled() {
        return state.pictureBackgroundEnabled;
    }
    std::int32_t& PictureOffsetX() { return state.pictureOffsetX; }
    std::int32_t& PictureOffsetY() { return state.pictureOffsetY; }
    float& PictureScale() { return state.pictureScale; }
    std::int32_t& PictureWidth() { return state.pictureWidth; }
    std::int32_t& PictureHeight() { return state.pictureHeight; }
    std::uint32_t& AviBackgroundEnabled() {
        return state.aviBackgroundEnabled;
    }
    IDirect3DTexture9*& CaptureTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.captureTexture);
    }
    IDirect3DSurface9*& CaptureRenderTarget() {
        return reinterpret_cast<IDirect3DSurface9*&>(state.captureRenderTarget);
    }
    IDirect3DSurface9*& CaptureSystemSurface() {
        return reinterpret_cast<IDirect3DSurface9*&>(state.captureSystemSurface);
    }
    ScreenCaptureMode& CaptureMode() {
        return reinterpret_cast<ScreenCaptureMode&>(state.captureMode);
    }
    void*& CaptureReadbackPixels() {
        return state.captureReadbackPixels;
    }
    std::uint8_t*& RecordingCompletionFlag() {
        return reinterpret_cast<std::uint8_t*&>(state.recordingCompletionFlag);
    }
    IDirect3DTexture9*& OverlayTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.overlayTexture);
    }
    IDirect3DVertexBuffer9*& SpriteOverlayVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(
            state.spriteOverlayVertices);
    }
    std::uint32_t& SpriteOverlayPrimitiveCount() {
        return state.spriteOverlayPrimitiveCount;
    }
    IDirect3DTexture9*& ProjectedShadowRestoreTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(
            state.projectedShadowRestoreTexture);
    }
    std::uint8_t& ProjectedShadowBlendEnabled() {
        return state.projectedShadowBlendEnabled;
    }
    void*& ActiveRenderObject() {
        return state.activeRenderObject;
    }
    AccessoryRenderPass& ActiveRenderPass() {
        return reinterpret_cast<AccessoryRenderPass&>(state.activeRenderPass);
    }
    std::uint8_t& ModelNonDisplayMode() {
        return state.modelNonDisplayMode;
    }
    std::int32_t& ModelOutlineColorRed() {
        return state.modelOutlineColorRed;
    }
    std::int32_t& ModelOutlineColorGreen() {
        return state.modelOutlineColorGreen;
    }
    std::int32_t& ModelOutlineColorBlue() {
        return state.modelOutlineColorBlue;
    }
    std::uint8_t& WireframeRenderingEnabled() {
        return state.wireframeRenderingEnabled;
    }
    unsigned char* PanelRowFlags() {
#if defined(_M_X64)
        return m_panelRowFlags;
#else
        return state.buf656632;
#endif
    }
    std::uint32_t& GravityNoiseModeWord() {
        return *reinterpret_cast<std::uint32_t*>(&state.gravityNoiseEnabled);
    }
    std::uint8_t& RecordedFrameCountByte1() {
        return reinterpret_cast<std::uint8_t*>(&state.recordedFrameCount)[1];
    }
    std::int32_t& AccessoryRenderSplitOrder() {
        return reinterpret_cast<std::int32_t&>(state.accessoryRenderSplitOrder);
    }
    std::uint8_t& SelectedAccessorySlot() {
        return state.selectedObjectSlot;
    }
    std::uint8_t& SelectedObjectSlot() {
        return SelectedAccessorySlot();
    }
    std::uint8_t SelectedObjectSlot() const {
        return state.selectedObjectSlot;
    }
    std::int32_t& DisplayObjectListScrollPosition() {
        return state.displayObjectListScrollPosition;
    }
    std::int32_t DisplayObjectListScrollPosition() const {
        return state.displayObjectListScrollPosition;
    }
    std::int32_t& DisplayObjectListMatchCount() {
        return state.displayObjectListMatchCount;
    }
    std::int32_t DisplayObjectListMatchCount() const {
        return state.displayObjectListMatchCount;
    }
    std::uint32_t& LastRegisteredFrame() {
        return state.lastRegisteredFrame;
    }
    IDirect3DVertexBuffer9*& OverlayVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.overlayVertices);
    }
    std::uint32_t& TextOverlayPrimitiveCount() {
        return state.textOverlayPrimitiveCount;
    }
    IDirect3DTexture9*& TextOverlayTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.sceneFontTexture);
    }
    std::uint32_t& LineOverlayPrimitiveCount() {
        return state.lineOverlayPrimitiveCount;
    }
    IDirect3DVertexBuffer9*& GroundPlaneVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(
            state.groundPlaneVertices);
    }
    IDirect3DVertexBuffer9*& LeftViewportVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.leftViewportVertices);
    }
    IDirect3DVertexBuffer9*& RightViewportVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.rightViewportVertices);
    }
    std::uint8_t& SelfShadowCompositionEnabled() {
        return state.selfShadowCompositionEnabled;
    }
    std::int32_t& SelfShadowMode() {
        return state.selfShadowMode;
    }
    std::uint8_t& DepthTextureCompositionEnabled() {
        return state.depthTextureCompositionEnabled;
    }
    std::uint8_t& DepthDeviceEnabled() {
        return state.depthDeviceEnabled;
    }
    DepthTextureProvider& DepthTextureCallback() {
        return reinterpret_cast<DepthTextureProvider&>(state.depthTextureCallback);
    }
    HDC& PanelDC() { return state.hdcMainPanel; }
    HDC& TimelineDC() { return state.hdcTimeline; }
    HDC& CurveDC() { return state.hdcInterpCurve; }
    HBITMAP& PanelBitmap() { return reinterpret_cast<HBITMAP&>(state.bmpPanel); }
    HBITMAP& PanelSpareBitmap() { return reinterpret_cast<HBITMAP&>(state.bmpPanelSpare); }
    HBITMAP& TimelineBitmap() { return reinterpret_cast<HBITMAP&>(state.bmpTimelineStrip); }
    HBITMAP& CurveBitmap() { return reinterpret_cast<HBITMAP&>(state.bmpInterpCurve); }
    HBRUSH UiBrush(int index) const {
        return reinterpret_cast<HBRUSH>(
            static_cast<std::uintptr_t>(state.brushes[index]));
    }
    void SetUiBrush(int index, HBRUSH brush) {
        state.brushes[index] =
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(brush));
    }
    COLORREF& ThemeColor(UiThemeColor color) {
        return reinterpret_cast<COLORREF&>(
            state.themeColors[static_cast<int>(color)]);
    }
    const COLORREF& ThemeColor(UiThemeColor color) const {
        return reinterpret_cast<const COLORREF&>(
            state.themeColors[static_cast<int>(color)]);
    }
    COLORREF& ThemeColorAt(int index) {
        return reinterpret_cast<COLORREF&>(state.themeColors[index]);
    }
    const COLORREF& ThemeColorAt(int index) const {
        return reinterpret_cast<const COLORREF&>(state.themeColors[index]);
    }
    std::uint8_t& UiTextRed() { return state.uiTextRed; }
    std::uint8_t& UiTextGreen() { return state.uiTextGreen; }
    std::uint8_t& UiTextBlue() { return state.uiTextBlue; }
    wchar_t* WavePath() {
        return reinterpret_cast<wchar_t*>(state.wavPath);
    }
    std::uint8_t& WaveEnabled() {
        return state.waveEnabled;
    }
    wchar_t* AviOutputPath() {
#if defined(_M_X64)
        return m_aviOutputPath;
#else
        return state.aviOutputPath;
#endif
    }
    std::int32_t& AviRecordStartFrame() {
        return state.aviRecordStartFrame;
    }
    std::int32_t& AviRecordEndFrame() {
        return state.aviRecordEndFrame;
    }
    float& AviRecordFps() { return state.aviRecordFps; }
    std::uint8_t& AviIncludeWave() {
        return state.aviIncludeWave;
    }
    std::int32_t& AviCodecSelection() {
#if defined(_M_X64)
        return m_aviCodecSelection;
#else
        return state.aviCodecSelection;
#endif
    }
    std::uint8_t& AviStereoOutput() {
        return state.aviStereoOutput;
    }
    std::int32_t& AviStereoWidthMultiplier() {
        return state.aviStereoWidthMultiplier;
    }
    float& ProjectedShadowDiffuseAlpha() {
        return state.projectedShadowDiffuseAlpha;
    }
    float& ProjectedShadowAmbientIntensity() {
        return state.projectedShadowAmbientIntensity;
    }
    void SetProjectedShadowAmbientRgb(float intensity) {
        ProjectedShadowAmbientIntensity() = intensity;
        state.projectedShadowAmbientG = intensity;
        state.projectedShadowAmbientB = intensity;
    }
    void SetProjectedShadowAmbient(float intensity) {
        SetProjectedShadowAmbientRgb(intensity);
        state.projectedShadowAmbientA = intensity;
    }
    float& ProjectedShadowSpecularAlpha() {
        return state.projectedShadowSpecularAlpha;
    }
    D3DMATERIAL9 ProjectedShadowMaterial() const {
        D3DMATERIAL9 material{};
        material.Diffuse.a = state.projectedShadowDiffuseAlpha;
        material.Ambient = {state.projectedShadowAmbientIntensity,
                            state.projectedShadowAmbientG,
                            state.projectedShadowAmbientB,
                            state.projectedShadowAmbientA};
        material.Specular.a = state.projectedShadowSpecularAlpha;
        return material;
    }
    std::uint8_t& DirectSoundAvailable() {
        return state.directSoundAvailable;
    }
    std::int32_t& TimelineStartFrame() {
        return state.timelineStartFrame;
    }
    std::uint8_t& PlaybackActive() {
        return state.playbackActive;
    }
    std::uint8_t PlaybackActive() const {
        return state.playbackActive;
    }
    std::uint8_t& PlaybackLoopEnabled() {
        return state.playbackLoopEnabled;
    }
    std::uint8_t PlaybackLoopEnabled() const {
        return state.playbackLoopEnabled;
    }
    std::uint8_t& FrameStepPlayback() {
        return state.frameStepPlayback;
    }
    std::uint8_t FrameStepPlayback() const {
        return state.frameStepPlayback;
    }
    float& PlaybackStartSeconds() {
        return state.playbackStartSeconds;
    }
    float& PlaybackCursorSeconds() {
        return state.playbackCursorSeconds;
    }
    float& PlaybackEndSeconds() {
        return state.playbackEndSeconds;
    }
    std::uint32_t& PlaybackClockAnchorLow() {
        return state.playbackClockAnchorLow;
    }
    std::uint32_t& PlaybackClockAnchorHigh() {
        return state.playbackClockAnchorHigh;
    }
    std::int32_t& SavedPlaybackPhysicsMode() {
        return state.savedPlaybackPhysicsMode;
    }
    std::uint8_t& PlaybackStartsAtCurrentFrame() {
        return state.playbackStartsAtCurrentFrame;
    }
    std::uint8_t& PlaybackFrameChanged() {
        return state.playbackFrameChanged;
    }
    std::uint8_t& PhysicsResetPending() {
        return state.physicsResetPending;
    }
    std::uint8_t PhysicsResetPending() const {
        return state.physicsResetPending;
    }
    std::int32_t& PlaybackPhysicsMode() {
        return state.playbackPhysicsMode;
    }
    std::int32_t& SidebarWidth() {
        return state.sidebarWidth;
    }
    std::int32_t& RenderWidth() { return state.renderW; }
    std::int32_t& RenderHeight() { return state.renderH; }
    std::int32_t& SeparateWindowSidebarWidth() {
        return state.separateWindowSidebarWidth;
    }
    std::int32_t& SeparateWindowX() { return state.separateWindowX; }
    std::int32_t& SeparateWindowY() { return state.separateWindowY; }
    std::int32_t& SeparateWindowWidth() { return state.separateWindowWidth; }
    std::int32_t& SeparateWindowHeight() { return state.separateWindowHeight; }
    std::uint8_t& SeparateWindowMaximized() {
        return state.separateWindowMaximized;
    }
    std::uint8_t& FrameVolumeControlEnabled() {
        return state.frameVolumeControlEnabled;
    }
    float& SidebarRatio() { return state.sidebarRatio; }
    std::int32_t& FrameNormalization() {
        return state.frameNormalization;
    }
    RECT& ViewportRect() {
        return reinterpret_cast<RECT&>(state.hideRight);
    }
    const RECT& ViewportRect() const {
        return reinterpret_cast<const RECT&>(state.hideRight);
    }
    std::uint8_t& FullscreenMode() {
        return state.fullscreenMode;
    }
    std::uint32_t& MessageSeen() {
        return state.messageSeen;
    }
    std::uint8_t& WindowLayoutReady() {
        return state.windowLayoutReady;
    }
    std::uint8_t& EnhancedModelDirty() {
        return state.enhancedModelDirty;
    }
    std::uint8_t& AutoRepeatCount() {
        return state.autoRepeat;
    }
    std::uint8_t& UiOptionFlag(int index) {
        return state.optflag[index];
    }
    std::uint8_t& CameraMode() { return UiOptionFlag(0); }
    std::uint32_t& MainModelComboSelection() {
        return state.mainModelComboSelection;
    }
    std::uint8_t& GroundGridEnabled() { return state.groundGridEnabled; }
    std::uint8_t& FpsOverlayEnabled() { return state.fpsOverlayEnabled; }
    float& FpsOverlayElapsedSeconds() { return state.fpsOverlayElapsedSeconds; }
    std::int32_t& FpsOverlayFrameCount() {
        return reinterpret_cast<std::int32_t&>(state.fpsOverlayFrameCount);
    }
    std::int32_t& FramesPerSecond() {
        return reinterpret_cast<std::int32_t&>(state.framesPerSecond);
    }
    float& BoneRotationEditDegreesX() {
        return state.eulerX;
    }
    float& BoneRotationEditDegreesY() {
        return state.eulerY;
    }
    float& BoneRotationEditDegreesZ() {
        return state.eulerZ;
    }
    std::uint8_t& AudioSeekReady() {
        return state.a02B6;
    }
    std::uint8_t& AutomaticFrameAdvanceEnabled() {
        return state.automaticFrameAdvanceEnabled;
    }
    WNDPROC& OriginalEditProc() {
        return reinterpret_cast<WNDPROC&>(state.origEditProc);
    }
    WNDPROC& OriginalTrackbarProc() {
        return reinterpret_cast<WNDPROC&>(state.origTrackProc);
    }
    std::uint32_t& CameraTrackCursor() { return state.cameraTrackCursor; }
    std::uint8_t& CameraTrackActive() { return state.cameraTrackActive; }
    std::uint32_t& LightTrackCursor() { return state.lightTrackCursor; }
    std::uint8_t& LightTrackActive() { return state.lightTrackActive; }
    std::uint32_t& ShadowTrackCursor() { return state.shadowTrackCursor; }
    std::uint8_t& ShadowTrackActive() { return state.shadowTrackActive; }
    std::uint32_t& GravityTrackCursor() { return state.gravityTrackCursor; }
    std::uint8_t& GravityTrackActive() { return state.gravityTrackActive; }
    std::uint32_t& AccessoryTrackCursor(int slot) {
        return state.accessoryTrackCursor[slot];
    }
    std::uint8_t& AccessoryTrackActive(int slot) {
#if defined(_M_X64)
        return m_accessoryTrackActive[slot];
#else
        return state.accessoryTrackActive[slot];
#endif
    }
    float* CameraPosition() { return state.cameraPosition; }
    float* CameraRotation() { return &state.cameraPitch; }
    float& CameraPositionX() { return state.cameraPosition[0]; }
    float& CameraPositionY() { return state.cameraPosition[1]; }
    float& CameraPositionZ() { return state.cameraPosition[2]; }
    float& CameraPitch() { return state.cameraPitch; }
    float& CameraYaw() { return state.cameraYaw; }
    float& CameraRoll() { return state.cameraRoll; }
    float& ViewOffsetX() { return state.viewOffsetX; }
    float& ViewOffsetY() { return state.viewOffsetY; }
    float& CameraDistance() { return state.cameraDistance; }
    float& CameraFov() { return state.cameraFov; }
    std::uint8_t& CameraPerspective() {
        return state.cameraPerspective;
    }
    std::int32_t& CameraParentModel() {
        return reinterpret_cast<std::int32_t&>(state.cameraParentModel);
    }
    std::int32_t& CameraParentBone() {
        return state.cameraParentBone;
    }
    CameraAttachmentReference& CameraReferenceMode() {
        return reinterpret_cast<CameraAttachmentReference&>(
            state.cameraReferenceMode);
    }
    D3DMATRIX& CameraAttachmentBasis() {
        return reinterpret_cast<D3DMATRIX&>(state.cameraAttachmentBasis);
    }
    const D3DMATRIX& CameraAttachmentBasis() const {
        return reinterpret_cast<const D3DMATRIX&>(state.cameraAttachmentBasis);
    }
    std::uint8_t& CameraAttachmentTransformSuppressed() {
        return state.cameraAttachmentTransformSuppressed;
    }
    D3DMATRIX& ViewRotationTransform() {
#if defined(_M_X64)
        return m_viewRotationTransform;
#else
        return reinterpret_cast<D3DMATRIX&>(state.viewRotationTransform[0]);
#endif
    }
    const D3DMATRIX& ViewRotationTransform() const {
#if defined(_M_X64)
        return m_viewRotationTransform;
#else
        return reinterpret_cast<const D3DMATRIX&>(
            state.viewRotationTransform[0]);
#endif
    }
    std::int32_t& ShadowMode() {
        return state.selfShadowMode;
    }
    float& ShadowDistance() {
        return state.physicsInterval;
    }
    float* GravityDirection() { return &GravityX(); }
    std::int32_t& GravityNoise() {
        return reinterpret_cast<std::int32_t&>(state.gravityNoise);
    }
    std::uint8_t& GravityNoiseEnabled() {
        return state.gravityNoiseEnabled;
    }

    float& FpsLimit()               { return state.fpsLimit; }   
    float& DeltaTime()              { return state.deltaTime; }  
    std::uint32_t& TimeNowLow()     { return state.timeNowLow; }
    std::uint32_t& TimeNowHigh()    { return state.timeNowHigh; }
    float& MilliToSec()             { return state.milliToSec; }  

    wchar_t* EnvFileName()          { return state.envFileName; }

    void*& LocaleTablePtr()         { return state.renderer; }    

    void*& Hwnd()                   { return reinterpret_cast<void*&>(state.hwnd); }        
    void*& HInstance() {
#if defined(_M_X64)
        return m_hInstance;
#else
        return state.hInstance;  
#endif
    }
    void*& OpenniTrackingCallback() {
#if defined(_M_X64)
        return m_openniTrackingCallback;
#else
        return state.openniTrackingCallback;  
#endif
    }
    std::uint8_t& KinectCaptureStage() { return state.autoRepeat; }
    std::uint8_t KinectCaptureStage() const { return state.autoRepeat; }
    void*& OniExportSlot(int index) {
        switch (index) {
        case 0: return state.oniExportSlot0;
        case 1: return state.oniExportSlot1;
#if defined(_M_X64)
        case 2: return m_oniExportSlot2;
#else
        case 2: return reinterpret_cast<void*&>(state.oniExportSlot2);
#endif
        case 3: return state.depthTextureCallback;
#if defined(_M_X64)
        case 4: return m_oniExportSlot4;
#else
        case 4: return reinterpret_cast<void*&>(state.oniExportSlot4);
#endif
        case 5: return OpenniTrackingCallback();
        default: return state.oniExportSlot6;
        }
    }
    std::int32_t& FrameRangeStartFrame() {
#if defined(_M_X64)
        return m_frameRangeStartFrame;
#else
        return state.frameRangeStartFrame;  
#endif
    }
    WINDOWPLACEMENT& SavedPlacement() {
#if defined(_M_X64)
        return m_savedPlacement;
#else
        return state.savedPlacement;  
#endif
    }
    D3DRenderer*& Renderer()        { return reinterpret_cast<D3DRenderer*&>(state.renderer); }
    WaveAudioContext*& Audio() {
        return reinterpret_cast<WaveAudioContext*&>(state.audioContext);
    }
    DShowRecorder*& Recorder() {
        return reinterpret_cast<DShowRecorder*&>(state.recorder);
    }
    void*& AxisMeshObject()        { return state.axisMeshObject; }
    PhysicsScene*& Physics()        { return reinterpret_cast<PhysicsScene*&>(state.physicsScene); }
    wchar_t* ExeDir()               { return state.exeDir; }
    unsigned char& EnglishUI()      { return state.englishUI; }  

    wchar_t* DirModel()   { return state.dirModel; }
    wchar_t* DirUser()    { return state.dirUser; }
    wchar_t* DirAccs()    { return state.dirAccs; }
    wchar_t* DirMotion()  { return state.dirMotion; }
    wchar_t* DirPose()    { return state.dirPose; }
    wchar_t* DirWave()    { return state.dirWave; }
    wchar_t* DirBg()      { return state.dirBg; }

    float& GravityX()               { return state.gravityX; }
    float& GravityY()               { return state.gravityY; }
    float& GravityZ()               { return state.gravityZ; }
    float& GravityMagnitude()       { return state.gravityMagnitude; }
    float& PhysicsInterval()        { return state.physicsInterval; }  

    char* RecentFile(int index) {
        return index == 0 ? reinterpret_cast<char*>(state.recentFile0)
             : index == 1 ? reinterpret_cast<char*>(state.recentFile1)
                          : reinterpret_cast<char*>(state.recentFile2);
    }


private:

    DialogOrderArrays m_dialogOrders;

#if defined(_M_X64)
    D3DLIGHT9 m_sceneLight{};

    void* m_hInstance = nullptr;

    void* m_openniTrackingCallback = nullptr;

    std::int32_t m_selectionBoxAnchorX = 0;
    std::int32_t m_selectionBoxAnchorY = 0;
    std::int32_t m_timelineRangeFirstOffset = 0;
    std::int32_t m_timelineRangeFirstBase = 0;
    std::int32_t m_timelineRangeLastOffset = 0;
    std::int32_t m_timelineRangeLastBase = 0;
    D3DMATRIX m_lightViewProjectionMatrix{};
    D3DMATRIX m_worldViewProjectionMatrix{};
    std::uint8_t m_accessoryTrackActive[55] = {};
    void* m_oniExportSlot2 = nullptr;
    void* m_oniExportSlot4 = nullptr;
    WINDOWPLACEMENT m_savedPlacement{};
    std::int32_t m_frameRangeStartFrame = 0;
    D3DMATRIX m_viewRotationTransform{};
    void* m_boneClipboard = nullptr;
    void* m_morphClipboard = nullptr;
    void* m_cameraClipboard = nullptr;
    void* m_lightClipboard = nullptr;
    void* m_shadowClipboard = nullptr;
    void* m_gravityClipboard = nullptr;
    void* m_accessoryClipboard = nullptr;
    std::int32_t m_timelineSelectionSlots[16] = {};
    unsigned char m_panelRowFlags[204] = {};

    PathResolutionWorkspace m_pathWorkspace{};

    struct OverlayVertexBuffers {
        IDirect3DVertexBuffer9* avi = nullptr;
        IDirect3DVertexBuffer9* picture = nullptr;
    } m_overlayVertexBuffers;

    wchar_t m_aviOutputPath[256]{};
    std::int32_t m_aviCodecSelection{};
    HWND m_gravitySettingDialog = nullptr;
    HWND m_frameRangeDialog = nullptr;
    HWND m_groundShadowColorDialog = nullptr;
    WNDPROC m_groundShadowColorEditProc = nullptr;
    std::int32_t m_morphFrameShift = 0;
    std::int32_t m_blinkStartFrame = 0;
    std::int32_t m_blinkEndFrame = 0;
    WNDPROC m_edgeThicknessEditProc = nullptr;
    WNDPROC m_modelEdgeEditProc = nullptr;
    std::int32_t m_modelEdgeComboCursor[3] = {};
    WNDPROC m_frameCopyEditProc = nullptr;
    WNDPROC m_accessoryFrameEditProc = nullptr;
    float m_rotationDialogTemp[7] = {};
    unsigned char m_cameraFrameScratch[172] = {};
    void* m_accessoryEditArray = nullptr;
    unsigned char m_accessoryApplyGate = 0;
    wchar_t m_captureSavePath[256] = {};
    unsigned char m_boneFrameScratch[140] = {};
#endif
};

static_assert(sizeof(MMDApp) >= sizeof(MMDAppState),
              "MMDApp must retain the complete compatibility state");

extern MMDApp* g_Block;

}  
