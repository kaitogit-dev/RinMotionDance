#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "rinmotiondance/clipboard_layout.hpp"
#include "rinmotiondance/bone_binding.hpp"
#include "rinmotiondance/path_workspace.hpp"
#include "rinmotiondance/raw_pad.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>


namespace rinmotiondance {

constexpr std::size_t kAppObjectSize =
    sizeof(void*) == 8 ? 0xA55E0 : 0xA4530;

constexpr int kModelSlotCount = sizeof(void*) == 8 ? 255 : 100;

struct MMDAppState {
#if defined(_M_X64)
    RawPad<4> pad0inst;  
#else
    void* hInstance;
#endif
    std::int32_t mouseX;               
    std::int32_t mouseY;               
    std::int32_t previousMouseX;       
    std::int32_t previousMouseY;       
    std::int32_t upKeyState;           
    std::int32_t downKeyState;         
    std::int32_t leftKeyState;         
    std::int32_t rightKeyState;        
    std::int32_t shiftModifierState;   
    std::int32_t spaceKeyState;        
    std::int32_t escKeyState;          
    std::int32_t dialogFlags[18];
    std::int32_t keyState221;               
    std::int32_t keyState226;               
    std::int32_t tabKeyState;               
    std::int32_t leftMouseButtonState;      
    std::int32_t rightMouseButtonState;     
    std::int32_t middleMouseButtonState;    
    std::int32_t numpadKeyState[10];        
    std::int32_t deleteKeyState;            
    std::int32_t enterKeyState;            
    std::int32_t ctrlModifierState;         
    std::int32_t menuKeyState;              
    unsigned char sidebarResizeDragging;
#if defined(_M_X64)
    RawPad<7> pad2;
#else
    RawPad<3> pad2;
#endif
    void* audioContext;  
    wchar_t wavPath[256];
#if defined(_M_X64)
    RawPad<3> pad4;
#endif
    unsigned char directSoundAvailable;
#if defined(_M_X64)
    RawPad<4> pad5;
#else
    RawPad<3> pad5;
#endif
    HDC hdcMainPanel;  
    void* bmpPanel;
    void* bmpPanelSpare;
    HDC hdcTimeline;
    void* bmpTimelineStrip;
    HDC hdcInterpCurve;
    void* bmpInterpCurve;
    void* bmpRes101;  
    void* bmpRes119;
    unsigned char optflag[7];  
    RawPad<1> pad21;
    void* groundGridVertices;
    void* groundGridIndices;
    float viewOffsetX;
    float viewOffsetY;
    float cameraPitch;
    float cameraYaw;
    float cameraRoll;
    unsigned char cameraPerspective;
    unsigned char groundGridEnabled;
    unsigned char fpsOverlayEnabled;
    RawPad<1> pad31;
    float fpsOverlayElapsedSeconds;
    std::uint32_t fpsOverlayFrameCount;
    std::uint32_t framesPerSecond;
    std::uint32_t v32c;
    unsigned char playbackActive;
    RawPad<3> pad36;
    float cameraPosition[3];
    unsigned char cameraReferenceMode;
    unsigned char playbackLoopEnabled;
    unsigned char playbackReturnsToStartFrame;
    RawPad<1> pad42;
    std::uint32_t viewportToolHovered;   
    std::uint32_t viewToolDragOperation;   
    std::uint32_t interactionDragMode;   
    void* boneCopyRecords;    
#if defined(_M_X64)
    RawPad<24> pad46;
#else
    void* boneClipboard;    
    void* morphClipboard;   
#endif
    void* displayClipboard;  
#if defined(_M_X64)
    RawPad<32> pad47;
#else
    void* cameraClipboard;      
    void* lightClipboard;       
    void* shadowClipboard;      
    void* gravityClipboard;     
    void* accessoryClipboard;   
#endif
    void* cameraKeyTrack;  
    void* lightKeyTrack;  
    void* selfShadowKeyTrack;  
    void* gravityKeyTrack;  
    void* accKeyTracks[255];  
#if defined(_M_X64)
    void* modelSlots[255];  
#else
    void* modelSlots[100];  
#endif
    unsigned char slotIdx;  
    RawPad<3> pad54;
    std::int32_t editMode;
    unsigned char groundShadowEnabled;
    RawPad<3> pad56;
    std::uint32_t aviBackgroundEnabled;
    std::int32_t viewportToolCenterX;   
    std::int32_t viewportToolCenterY;   
    float selectedClipW;                
    std::uint32_t viewportToolOperation;
    std::int32_t dragOriginX;
    std::int32_t dragOriginY;
    std::int32_t boneBoxStartX;         
    std::int32_t boneBoxStartY;         
    std::int32_t boneBoxSelectionActive;
    std::int32_t scrollCbSize;
    std::int32_t scrollFMask;
    std::int32_t scrollNMin;
    std::int32_t scrollNMax;
    std::int32_t scrollNPage;
    std::int32_t scrollNPos;
    RawPad<20> pad67;
    std::int32_t timelineScrollNPage;  
    std::int32_t timelineScrollNPos;   
    RawPad<4> pad67b;
    std::int32_t timelineStartFrame;
    std::int32_t currentFrame;
    std::int32_t rowHitBone[40000];   
    std::int32_t rowHitMorph[40000];  
    std::int32_t rowHitIk[40000];     
    std::int32_t rowHitBand0[200];  
    std::int32_t rowHitBand1[200];  
    std::int32_t rowHitBand2[200];  
    std::int32_t rowHitBand3[200];  
    std::int32_t rowHitAcc[40000];  
    unsigned char interpCurveUniformFound;     
    unsigned char interpCurveControlCache[4];  
    unsigned char pendingTimelineSelectionRow;
    unsigned char lightA[6];
    unsigned char lightB[6];
    unsigned char lightC[6];
    unsigned char lightD[6];
    RawPad<2> pad100;
    std::int32_t copiedBoneCount;               
    mdl::ClipboardSelectionCounts clipboardCounts;  
    std::int32_t displayObjectListScrollPosition;
    std::int32_t displayObjectListMatchCount;
    std::int32_t jointLineMap[200];  
    void* objectSlots[255];
    std::uint32_t lastRegisteredFrame;  
    unsigned char selectedObjectSlot;
    RawPad<3> pad115;
    float lightDirection[3];
    RawPad<36> pad118;
    float lightColor[3];
    RawPad<28> pad121;
    std::uint32_t sceneLightRange;  
    RawPad<24> pad122;
    float cameraFov;  
    wchar_t aviBackgroundPath[256];  
#if defined(_M_X64)
    RawPad<8> pad126;  
#endif
    void* drawDib;
    void* aviBackgroundTexture;
    void* aviBackgroundSurface;
#if !defined(_M_X64)
    std::uint32_t aviOverlayVertices;
#endif
    void* aviFile;
    void* aviStream;  
    void* aviFrameReader;
    std::int32_t aviStreamStart;
    std::int32_t aviStreamEnd;
    unsigned char aviUsesThirtyFpsTiming;
    RawPad<3> pad134;
    std::int32_t aviOffsetX;
    std::int32_t aviOffsetY;
    float aviScale;
    std::int32_t aviFrameWidth;
    std::int32_t aviFrameHeight;
    unsigned char pictureBackgroundEnabled;
#if defined(_M_X64)
    RawPad<7> pad140;
#else
    RawPad<3> pad140;
#endif
    void* pictureBackgroundTexture;
#if defined(_M_X64)
    RawPad<8> pad129;
#else
    std::uint32_t pictureOverlayVertices;
#endif
    std::int32_t pictureOffsetX;
    std::int32_t pictureOffsetY;
    float pictureScale;
    std::int32_t pictureWidth;
    std::int32_t pictureHeight;
    wchar_t pictureBackgroundPath[256];
    std::int32_t aviBackgroundSample;
    float playbackCursorSeconds;
    float keyRepeatTimer;
    float playbackStartSeconds;
    float playbackEndSeconds;
    std::uint32_t cameraTrackCursor;      
    std::uint8_t cameraTrackActive;       
    RawPad<3> pad153a;
    std::uint32_t lightTrackCursor;       
    std::uint8_t lightTrackActive;        
    RawPad<3> pad153b;
    std::uint32_t shadowTrackCursor;      
    std::uint8_t shadowTrackActive;       
    RawPad<3> pad153c;
    std::uint32_t gravityTrackCursor;     
    std::uint8_t gravityTrackActive;      
    RawPad<3> pad153d;
    std::uint32_t accessoryTrackCursor[55];  
    RawPad<800> pad153e;                     
    std::uint8_t accessoryTrackActive[55];   
#if defined(_M_X64)
    RawPad<199> pad153;                      
#else
    RawPad<200> pad153;                      
#endif
    unsigned char playbackEnabledSnapshot[7];  
    unsigned char characterTransparentMode;    
    unsigned char blinkPhase;                  
#if defined(_M_X64)
    RawPad<5> pad155;  
#endif
    void* captureTexture;  
    std::uint32_t captureMode;
#if defined(_M_X64)
    RawPad<4> pad157;
#endif
    void* captureRenderTarget;
    void* captureSystemSurface;
#if defined(_M_X64)
    RawPad<512> pad159;
#else
    wchar_t aviOutputPath[256];
#endif
    unsigned char frameStepPlayback;  
    RawPad<3> pad160;
    std::int32_t recordedFrameCount;
    unsigned char followCameraEnabled;
    unsigned char playbackStartsAtCurrentFrame;   
    unsigned char projectedShadowBlendEnabled;   
    RawPad<1> pad164;
    std::int32_t coordinateSystem;  
    void* axisMeshObject;  
#if !defined(_M_X64)
    RawPad<4> pad166;
#endif
    std::uint32_t playbackClockAnchorLow;   
    std::uint32_t playbackClockAnchorHigh;   
    void* physicsScene;
    unsigned char physicsEditorJointPage;
    unsigned char physicsResetPending;
    unsigned char playbackFrameChanged;   
    RawPad<1> pad172;
    float gravityX;
    float gravityY;
    float gravityZ;
    float gravityMagnitude;
    std::uint32_t gravityNoise;
    float gravityNoiseTimer;
    unsigned char timelineAdvanceDue;   
    unsigned char viewportInputActive;   
    RawPad<2> pad180;
    void* recordingCompletionFlag;
    unsigned char recordPlaybackStartPending;   
    RawPad<3> pad182;
    std::int32_t recordSavedFrame;
    void* toonTextures[11];
    void* spriteOverlayVertices;
    std::uint32_t spriteOverlayPrimitiveCount;
#if defined(_M_X64)
    RawPad<4> pad186;
#endif
    void* overlayTexture;
    void* overlayVertices;
    std::uint32_t textOverlayPrimitiveCount;
#if defined(_M_X64)
    RawPad<4> pad189;
#endif
    void* sceneFontTexture; 
    unsigned char recentFile0[256];
    unsigned char recentFile1[256];
    unsigned char recentFile2[256];
    std::uint32_t lineOverlayPrimitiveCount;
#if defined(_M_X64)
    RawPad<4> pad194;
#endif
    void* groundPlaneVertices;
    unsigned char separateWindowMouseSeen;
#if defined(_M_X64)
    RawPad<7> pad196;
#else
    RawPad<3> pad196;
#endif
    void* projectedShadowRestoreTexture;
#if defined(_M_X64)
    RawPad<512> pad197;
#else
    wchar_t captureSavePath[256];
#endif
    void* captureReadbackPixels;
#if defined(_M_X64)
    RawPad<3536> pad199;  
#else
    PathResolutionWorkspace pathWorkspace;
#endif
    void* leftViewportVertices;
    void* rightViewportVertices;
    float toonEdgeTable[30];
    unsigned char selfShadowEnabled;
    unsigned char selectionBoxDragging;
    RawPad<2> pad204;
#if defined(_M_X64)
    RawPad<8> pad205;
#else
    std::int32_t selectionBoxAnchorX;   
    std::int32_t selectionBoxAnchorY;   
#endif
    unsigned char blackBackgroundEnabled;  
    unsigned char modelNonDisplayMode;
    unsigned char wavPlaysOnFrameMove;     
    unsigned char floorVisible;            
    std::int32_t modelOutlineColorRed;
    std::int32_t modelOutlineColorGreen;
    std::int32_t modelOutlineColorBlue;
    unsigned char customColorTable[48];
    RawPad<16> pad209;  
    unsigned char wireframeRenderingEnabled;  
#if defined(_M_X64)
    RawPad<131> pad213;
#else
    RawPad<3> pad213a;
    float lightViewProjectionMatrix[16];   
    float worldViewProjectionMatrix[16];   
#endif
    void* activeRenderObject;      
    std::int32_t activeRenderPass;  
#if !defined(_M_X64)
    std::int32_t renderPassCount;
#endif
    unsigned char fullscreenMode;   
    RawPad<3> pad217;
    HMENU savedMenu;  
#if defined(_M_X64)
    std::int32_t renderPassCount;  
    RawPad<40> pad219;
#else
    WINDOWPLACEMENT savedPlacement;
#endif
    unsigned char fullscreenFlagsSaved;  
    RawPad<3> pad220;
    std::int32_t recRTW;
    std::int32_t recRTH;
    unsigned char recordFullscreenActive;
    unsigned char stereoActivated;
    unsigned char a02B6;
    unsigned char sjisOut[256];
    unsigned char timelineAdvanceRequested;   
    unsigned char depthDeviceEnabled;
    RawPad<3> pad228;
    HMODULE oniModule;
    void* oniExportSlot0;
    void* oniExportSlot1;
    std::uint32_t oniExportSlot2;
#if defined(_M_X64)
    RawPad<4> pad232;
#endif
    void* depthTextureCallback;
    std::uint32_t oniExportSlot4;
#if defined(_M_X64)
    RawPad<12> pad231;
#else
    void* openniTrackingCallback;  
#endif
    void* oniExportSlot6;
    unsigned char kinectMirrorEnabled;
    unsigned char kinectInitLostBone;
    unsigned char depthTextureCompositionEnabled;
    unsigned char kinectCaptureActive;
    float fpsLimitSaved;  
    unsigned char globalTrackSelected[4];
    unsigned char selectionActiveLatch;
    unsigned char automaticFrameAdvanceEnabled;
    unsigned char openniVersion;
    unsigned char timelineSelectionChanged;
    std::int32_t timelineSelectionSlots[16];  
#if defined(_M_X64)
    RawPad<64> pad248;  
#endif
    std::uint32_t mainModelComboSelection;
    std::int32_t cameraParentModel;
    std::int32_t cameraParentBone;
    float cameraAttachmentBasis[16];
    unsigned char cameraAttachmentTransformSuppressed;   
    RawPad<3> pad269;
    unsigned char logFont[60];  
    unsigned char modelReloadPending;   
    RawPad<3> pad271;
    HFONT hFontUI;  
    float eulerX;
    float eulerY;
    float eulerZ;
    std::uint32_t brushes[11];
#if defined(_M_X64)
    RawPad<48> pad276;
#endif
    unsigned char buf656632[200];  
#if defined(_M_X64)
    RawPad<16> pad277;  
#else
    std::int32_t timelineRangeFirstOffset;   
    std::int32_t timelineRangeFirstBase;     
    std::int32_t timelineRangeLastOffset;    
    std::int32_t timelineRangeLastBase;      
#endif
    unsigned char timelineRangeApplyEnabled;
    unsigned char viewDirty;
    unsigned char b6568482;
    unsigned char mouseJumped;
    unsigned char uiTextRed;
    unsigned char uiTextGreen;
    unsigned char uiTextBlue;
    RawPad<1> pad284;
    std::uint32_t themeColors[35];
#if defined(_M_X64)
    RawPad<1> pad319;  
#else
    unsigned char accessoryApplyGate;
#endif
    unsigned char accessoryEditDialogOpen;  
    RawPad<2> pad320;
    mdl::BoneOrderEntry* selectNavRecords;  
    unsigned char physicsBodiesMoved;
    unsigned char playbackAlwaysOnOffMode;
    RawPad<2> pad323;
    std::int32_t savedPlaybackPhysicsMode;
#if defined(_M_X64)
    RawPad<64> pad324;
#else
    float viewRotationTransform[16];
#endif
    unsigned char a06B4;
    unsigned char a06B5;
    unsigned char a06B6;
#if defined(_M_X64)
    RawPad<5> pad327;
#else
    RawPad<1> pad327;
#endif
    HWND hwnd;
    float deltaTime;
#if defined(_M_X64)
    RawPad<4> pad329;
#endif
    void* recorder;  
    void* renderer;  
    std::int32_t sidebarWidth;
    unsigned char waveEnabled;
    RawPad<1> pad333;
    wchar_t exeDir[256];
    RawPad<2> pad334;
    void* origEditProc;
    std::int32_t renderW;
    std::int32_t renderH;
    float cameraDistance;  
    float fpsLimit;  
    float modelOffsetX;
    float modelOffsetY;
    float modelOffsetZ;
#if !defined(_M_X64)
    std::int32_t frameRangeStartFrame;  
    std::int32_t morphFrameShift;  
    std::int32_t blinkStartFrame;  
    std::int32_t blinkEndFrame;    
#else
    RawPad<16> pad289;  
#endif
    wchar_t envFileName[256];
    std::int32_t aviRecordStartFrame;
    std::int32_t aviRecordEndFrame;
    float aviRecordFps;
    unsigned char aviIncludeWave;
    unsigned char sceneModified;
    RawPad<2> pad348;
    std::uint32_t a0B10;
#if defined(_M_X64)
    RawPad<24> pad349;
#else
    HWND groundShadowColorDialog;  
    WNDPROC groundShadowColorEditProc;  
    RawPad<4> accessoryOrderArray;  
#endif
    std::int32_t accessoryRenderSplitOrder;  
#if defined(_M_X64)
    RawPad<40> pad357;
#else
    void* accessoryEditArray;
    float rotationDialogTemp[7];
#endif
    HWND edgeThicknessDialog;  
#if defined(_M_X64)
    RawPad<8> pad354;  
#else
    WNDPROC edgeThicknessEditProc;  
#endif
    unsigned char englishUI;  
#if defined(_M_X64)
    RawPad<7> pad353;  
#else
    RawPad<3> pad353;
#endif
#if defined(_M_X64)
    RawPad<8> pad356;
    RawPad<20> pad358;
#else
    HWND frameRangeDialog;
    WNDPROC modelEdgeEditProc;      
    std::int32_t modelEdgeComboCursor[3];  
#endif
    unsigned char enhancedModelDirty;  
    RawPad<3> pad355;
    std::uint32_t timeNowLow;
    std::uint32_t timeNowHigh;
    float milliToSec;
    HWND frameCopyDialog;  
#if defined(_M_X64)
    RawPad<8> pad361;  
#else
    WNDPROC frameCopyEditProc;  
#endif
    void* rigidScratchArray;
#if defined(_M_X64)
    RawPad<192> pad360;
#else
    unsigned char cameraFrameScratch[172];
#endif
    std::int32_t selectedRigidIndex;  
    void* jointScratchArray;  
#if defined(_M_X64)
    RawPad<152> pad364;
#else
    unsigned char boneFrameScratch[140];
#endif
    std::int32_t selectedJointIndex;  
    std::int32_t playbackPhysicsMode;  
    unsigned char rigidBodyDisplayEnabled;  
#if defined(_M_X64)
    RawPad<23> pad367;
#else
    RawPad<3> pad367;
    HWND gravitySettingDialog;  
    WNDPROC accessoryFrameEditProc;  
#endif
    unsigned char gravityNoiseEnabled;
#if defined(_M_X64)
    RawPad<7> pad368;
#else
    RawPad<3> pad368;
    std::int32_t aviCodecSelection;
#endif
    void* origTrackProc;
    unsigned char aviSettings;
#if defined(_M_X64)
    RawPad<3> pad370;
#else
    RawPad<11> pad370;
#endif
    float projectedShadowDiffuseAlpha;
    float projectedShadowAmbientIntensity;   
    float projectedShadowAmbientG;
    float projectedShadowAmbientB;
    float projectedShadowAmbientA;
    RawPad<12> pad375;
    float projectedShadowSpecularAlpha;
#if defined(_M_X64)
    RawPad<30> pad376;  
#else
    RawPad<20> pad376;
#endif
    HWND recordingWindow;  
    unsigned char selfShadowCompositionEnabled;  
    RawPad<3> pad378;
    float physicsInterval;  
    std::int32_t selfShadowMode;
#if defined(_M_X64)
    RawPad<12> pad380;  
#else
    RawPad<4> pad380;
#endif
    HWND floatingWindow;   
    std::int32_t separateWindowSidebarWidth;
    std::int32_t hideRight;
    std::int32_t hideTop;
    std::int32_t hideLeft;
    std::int32_t hideBottom;
    std::int32_t separateWindowX;
    std::int32_t separateWindowY;
    std::int32_t separateWindowWidth;
    std::int32_t separateWindowHeight;
    unsigned char separateWindowMaximized;
    unsigned char aviStereoOutput;   
    RawPad<2> pad392;
    std::int32_t aviStereoWidthMultiplier;  
    unsigned char autoRepeat;
    RawPad<3> pad394;
    std::uint32_t messageSeen;   
    wchar_t dirModel[1000];      
    wchar_t dirUser[1000];
    wchar_t dirAccs[1000];
    wchar_t dirMotion[1000];
    wchar_t dirPose[1000];
    wchar_t dirWave[1000];
    wchar_t dirBg[1000];             
    unsigned char frameVolumeControlEnabled;  
    RawPad<3> pad403;
    std::int32_t frameNormalization;  
    float sidebarRatio;
    unsigned char windowLayoutReady;  
    char statusText[256];  
#if defined(_M_X64)
    RawPad<7> padTail;
#else
    RawPad<3> padTail;
#endif
};

static_assert(sizeof(MMDAppState) == kAppObjectSize,
              "app state size must match the original binary");


#include "rinmotiondance/layout_pins.hpp"

}  
