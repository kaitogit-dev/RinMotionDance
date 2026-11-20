#include <cstdio>
#include <cstring>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"

namespace rinmotiondance {

void MMDApp::InitDefaults() {
    auto& s = *this;

    s.ViewOffsetX() = 0.0f;
    s.CameraDistance() = -45.0f;
    s.ViewOffsetY() = 0.0f;
    state.sidebarResizeDragging = 0;
    s.CameraRotation()[0] = 0.0f;
    s.WaveEnabled() = 0;
    s.CameraRotation()[1] = 0.0f;
    s.DirectSoundAvailable() = 1;
    s.CameraRotation()[2] = 0.0f;
    s.UiOptionFlag(0) = 1;
    s.CameraPosition()[0] = 0.0f;
    s.UiOptionFlag(1) = 1;
    s.UiOptionFlag(2) = 1;
    s.CameraPosition()[1] = 10.0f;
    s.UiOptionFlag(3) = 1;
    s.UiOptionFlag(4) = 1;
    s.CameraPosition()[2] = 0.0f;
    s.UiOptionFlag(5) = 1;
    s.UiOptionFlag(6) = 1;
    s.CameraPerspective() = 0;
    s.GroundGridEnabled() = 1;
    s.ViewportInputActive() = 1;
    s.FpsOverlayEnabled() = 0;
    state.v32c = 0;
    s.PlaybackActive() = 0;
    s.CameraReferenceMode() = CameraAttachmentReference::None;
    s.PlaybackLoopEnabled() = 0;
    state.playbackReturnsToStartFrame = 0;
    state.viewportToolHovered = 0;
    state.viewToolDragOperation = 0;
    state.interactionDragMode = 0;
    s.ClearModelSlots();
    s.EditMode() = ViewportEditMode::None;
    state.groundShadowEnabled = 1;
    state.aviBackgroundEnabled = 0;
    s.CaptureMode() = ScreenCaptureMode::Disabled;
    s.ViewportToolOperation() = ViewportToolAction::None;
    s.BoneBoxSelectionActive() = 0;
    s.TimelineStartFrame() = 0;
    s.CurrentFrame() = 0;
    s.PendingTimelineSelectionRow() = TimelineSelectionRow::None;

    for (int i = 0; i < 6; ++i) {
        state.lightA[i] = static_cast<unsigned char>(-64);
        state.lightB[i] = 0;
        state.lightC[i] = 64;
        state.lightD[i] = 127;
    }

    state.boneCopyRecords = nullptr;
    state.copiedBoneCount = 0;
    state.clipboardCounts = mdl::ClipboardSelectionCounts{};
    s.CameraKeys() = nullptr;
    s.LightKeys() = nullptr;
    s.ShadowKeys() = nullptr;
    s.GravityKeys() = nullptr;
    for (int i = 0; i < 255; ++i) {
        s.AccessoryKeys(i) = nullptr;
        s.AccessorySlot(i) = nullptr;
    }
    state.keyRepeatTimer = 0.0f;
    s.FpsLimit() = 60.0f;
    state.modelOffsetX = 0.0f;
    s.LastRegisteredFrame() = 0;
    state.modelOffsetY = 0.0f;
    s.AviBackgroundTexture() = nullptr;
    state.modelOffsetZ = 0.0f;
    s.AviBackgroundSurface() = nullptr;
    s.PictureBackgroundTexture() = nullptr;
    s.AviOverlayVertices() = nullptr;
    s.AviFile() = nullptr;
    s.AviStream() = nullptr;
    s.AviFrameReader() = nullptr;
    s.AviUsesThirtyFpsTiming() = 0;
    s.PictureBackgroundEnabled() = 0;
    state.characterTransparentMode = 0;
    state.blinkPhase = 0;
    s.DisplayObjectListMatchCount() = 0;
    s.DisplayObjectListScrollPosition() = 0;
    s.CaptureTexture() = nullptr;
    state.groundGridVertices = nullptr;
    s.CaptureRenderTarget() = nullptr;
    s.FrameStepPlayback() = 0;
    s.SceneModified() = 0;
    state.followCameraEnabled = 0;
    s.PlaybackStartsAtCurrentFrame() = 0;
    state.a0B10 = 0;
    s.GroundShadowColorDialog() = nullptr;
    state.edgeThicknessDialog = nullptr;
    state.projectedShadowBlendEnabled = 1;
    state.coordinateSystem = 0;
    state.englishUI = 1;
    FrameRangeDialog() = nullptr;
    s.EnhancedModelDirty() = 0;
    state.frameCopyDialog = nullptr;
    state.rigidScratchArray = nullptr;
    s.PlaybackPhysicsMode() = 2;
    state.rigidBodyDisplayEnabled = 0;
    s.PhysicsResetPending() = 0;
    s.PlaybackFrameChanged() = 0;
    state.gravityNoiseEnabled = 0;
    state.gravityNoise = 10;
    s.AccessoryRenderSplitOrder() = 1;
    s.GravityMagnitude() = 9.8000002f;
    state.gravityNoiseTimer = 0.0f;
    s.GravityX() = 0.0f;
    s.GravityY() = -1.0f;
    s.GravityZ() = 0.0f;
    state.timelineAdvanceDue = 0;
    s.RecordingCompletionFlag() = nullptr;
    state.recordPlaybackStartPending = 0;
    state.groundGridIndices = nullptr;
    state.spriteOverlayPrimitiveCount = 0;
    state.sceneFontTexture = nullptr;      
    state.textOverlayPrimitiveCount = 0;   
    state.overlayVertices = nullptr;       
    sprintf_s(RecentFile(0), 0x100, "%s", g_Locale);
    sprintf_s(RecentFile(1), 0x100, "%s", g_Locale);
    sprintf_s(RecentFile(2), 0x100, "%s", g_Locale);
    state.overlayTexture = nullptr;        
    state.spriteOverlayPrimitiveCount = 0; 
    state.spriteOverlayVertices = nullptr;        
    state.lineOverlayPrimitiveCount = 0;
    state.groundPlaneVertices = nullptr;
    s.AviBackgroundTexture() = nullptr;                    
    s.PictureOverlayVertices() = nullptr;
    state.separateWindowMouseSeen = 0;
    s.RecordingWindow() = nullptr;
    s.CaptureReadbackPixels() = nullptr;
    state.projectedShadowRestoreTexture = nullptr;
    s.LeftViewportVertices() = nullptr;
    s.RightViewportVertices() = nullptr;
    state.selfShadowCompositionEnabled = 0;
    s.PhysicsInterval() = 0.01125f;
    s.FloatingWindow() = nullptr;
    state.selectionBoxDragging = 0;
    state.blackBackgroundEnabled = 0;
    state.modelNonDisplayMode = 0;
    state.wavPlaysOnFrameMove = 0;
    state.floorVisible = 1;
    state.modelOutlineColorRed = 0;
    state.modelOutlineColorGreen = 0;
    state.modelOutlineColorBlue = 0;
    std::memset(state.customColorTable, 0, 0x40);
    state.wireframeRenderingEnabled = 0;
    state.activeRenderObject = nullptr;
    state.activeRenderPass = 0;
    state.renderPassCount = 0;
    s.FullscreenMode() = 0;
    s.SavedPlacement().length = sizeof(WINDOWPLACEMENT);  
    state.fullscreenFlagsSaved = 0;
    state.stereoActivated = 0;
    state.recordFullscreenActive = 0;
    s.AudioSeekReady() = 0;
    s.AviStereoOutput() = 0;
    state.timelineAdvanceRequested = 0;
    s.AviStereoWidthMultiplier() = 2;
    state.depthDeviceEnabled = 0;
    state.oniModule = nullptr;
    state.oniExportSlot0 = nullptr;
    state.oniExportSlot1 = nullptr;
    state.oniExportSlot2 = 0;
    state.depthTextureCallback = nullptr;
    state.oniExportSlot4 = 0;
    s.OpenniTrackingCallback() = nullptr;
    state.kinectMirrorEnabled = 1;
    state.kinectInitLostBone = 0;
    state.depthTextureCompositionEnabled = 1;
    state.kinectCaptureActive = 0;
    s.SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
    state.globalTrackSelected[1] = 0;
    state.globalTrackSelected[2] = 0;
    state.globalTrackSelected[3] = 0;
    state.selectionActiveLatch = 0;   
    state.automaticFrameAdvanceEnabled = 0;
    state.messageSeen = 1;
    s.TimelineSelectionChanged() = 0;
    s.MainModelComboSelection() = 0;
    s.CameraParentModel() = -1;
    s.CameraParentBone() = 0;
    state.displayClipboard = nullptr;
    s.FrameVolumeControlEnabled() = 0;
    s.FrameNormalization() = 100;
    state.axisMeshObject = nullptr;

    for (float& v : state.cameraAttachmentBasis)
        v = 0.0f;
    state.cameraAttachmentBasis[15] = 1.0f;
    state.cameraAttachmentBasis[10] = 1.0f;
    state.cameraAttachmentBasis[5] = 1.0f;
    state.cameraAttachmentBasis[0] = 1.0f;

    s.CameraAttachmentTransformSuppressed() = 0;
    state.modelReloadPending = 0;
    s.WindowLayoutReady() = 1;
    std::memset(state.buf656632, 0, 0xC8);
    state.timelineRangeApplyEnabled = 0;
    state.viewDirty = 0;
    state.b6568482 = 0;
    state.mouseJumped = 0;
    state.selectNavRecords = 0;
    state.accessoryEditDialogOpen = 0;
    state.physicsBodiesMoved = 0;
    state.playbackAlwaysOnOffMode = 0;
    state.a06B4 = 0;
    state.a06B5 = 0;
    state.a06B6 = 0;
}

}  
