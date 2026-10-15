#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/mme_bridge.hpp"
#include "pump_input.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"
#include "frame_state_dump.hpp"

namespace rinmotiondance {

namespace {

void AdvanceFrameRenderGate(MMDApp* app) {
    auto& state = app->state.messageSeen;
    if (app->PlaybackActive() != 0)
        state = 1;
    if (app->state.frameVolumeControlEnabled == 0)
        state = 1;
    if (app->FrameRangeDialog() != nullptr)
        state = 1;

    if (state == 1)
        state = 2;
    else if (state == 2)
        state = 3;
    else if (state == 3)
        state = 0;
}

#ifdef RINMOTIONDANCE_DIAG

void TraceOperationInput(MMDApp* app, const char* phase) {
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_AB_TRACE_INPUT", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;
    const int state = app->LeftMouseButtonState();
    const auto mode = app->InteractionDragMode();
    if (state == 0 && mode == ViewportDragMode::None)
        return;
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\input_trace.jsonl", directory);
    std::FILE* stream = nullptr;
    if (fopen_s(&stream, path, "ab") != 0 || stream == nullptr)
        return;
    std::uint32_t quaternion[4]{};
    const unsigned slot = app->state.slotIdx;
    auto* model = app->ModelSlot(slot);
    if (model != nullptr) {
        const int selected = rinmotiondance::mdl::Mdl(model)->selectedBone;
        auto* bones = rinmotiondance::mdl::Bones(model);
        const int count = static_cast<int>(rinmotiondance::mdl::Mdl(model)->boneCount);
        if (bones != nullptr && selected >= 0 && selected < count)
            std::memcpy(quaternion, &bones[selected].rotQuat[0],
                        sizeof(quaternion));
    }
    std::fprintf(stream,
        "{\"phase\":\"%s\",\"state\":%d,\"mode\":%u,"
        "\"mouse\":[%d,%d,%d,%d],\"quaternion\":["
        "\"%08X\",\"%08X\",\"%08X\",\"%08X\"]}\n",
        phase, state, mode,
        app->MouseX(), app->MouseY(),
        app->PreviousMouseX(), app->PreviousMouseY(),
        quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
    std::fclose(stream);
}

bool LineCaptureSwitchReady() {
    char stableCapture[2]{};
    if (GetEnvironmentVariableA("RINMOTIONDANCE_AB_STABLE_CAPTURE", stableCapture,
                                sizeof(stableCapture)) != 1 ||
        stableCapture[0] != '1') {
        return true;
    }

    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_VB_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return false;

    char readyPath[MAX_PATH]{};
    std::snprintf(readyPath, sizeof(readyPath),
                  "%s\\vb.capture.ready", directory);
    const DWORD attributes = GetFileAttributesA(readyPath);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool IsRegularFile(const char* path) {
    const DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool WriteCaptureMarker(const char* path, const char* contents) {
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    const DWORD size = static_cast<DWORD>(std::strlen(contents));
    const bool ok = WriteFile(file, contents, size, &written, nullptr) != FALSE &&
                    written == size && FlushFileBuffers(file) != FALSE;
    CloseHandle(file);
    return ok;
}

void ActivateRefreshedVbCapture() {
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_VB_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;

    char requestPath[MAX_PATH]{};
    char activePath[MAX_PATH]{};
    std::snprintf(requestPath, sizeof(requestPath),
                  "%s\\vb.capture.ready", directory);
    std::snprintf(activePath, sizeof(activePath),
                  "%s\\vb.capture.active", directory);
    if (IsRegularFile(requestPath) && !IsRegularFile(activePath))
        WriteCaptureMarker(activePath, "active after successful Present\n");
}

void HoldPresentCaptureFence() {
    char stableCapture[2]{};
    if (GetEnvironmentVariableA("RINMOTIONDANCE_AB_STABLE_CAPTURE", stableCapture,
                                sizeof(stableCapture)) != 1 ||
        stableCapture[0] != '1') {
        return;
    }

    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_VB_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;

    char requestPath[MAX_PATH]{};
    char readyPath[MAX_PATH]{};
    char releasePath[MAX_PATH]{};
    std::snprintf(requestPath, sizeof(requestPath),
                  "%s\\screen.present.request", directory);
    if (!IsRegularFile(requestPath))
        return;
    std::snprintf(readyPath, sizeof(readyPath),
                  "%s\\screen.present.ready", directory);
    std::snprintf(releasePath, sizeof(releasePath),
                  "%s\\screen.present.release", directory);
    if (!WriteCaptureMarker(readyPath, "presented\n"))
        return;

    const ULONGLONG deadline = GetTickCount64() + 30000;
    while (!IsRegularFile(releasePath) && GetTickCount64() < deadline)
        Sleep(1);
}

#endif  

}  

void TimelineAdvance(MMDApp* app) {
    D3DRenderer* r = app->Renderer();
    if (r == nullptr)
        return;  
    IDirect3DDevice9* device = r->device;  
    if (device == nullptr)
        return;

    IDirect3DSurface9* surf = nullptr;                            
    if (FAILED(device->CreateRenderTarget(
            app->RenderWidth(),
            app->RenderHeight(),
            r->backbufferFormat,  
            D3DMULTISAMPLE_NONE, 0, FALSE, &surf, nullptr))) {
        DestroyWindow(app->RecordingWindow());                    
        app->RecordingWindow() = nullptr;
        RefreshMainWindowViewport(app);                                           
        return;
    }

    IDirect3DSurface9** backBuffer = &r->backbufferSurface;  
    if (*backBuffer == nullptr)                                   
        device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, backBuffer);

    RECT rect;                                                    
    rect.left = 0;
    rect.top = 0;
    rect.right = app->RenderWidth();
    rect.bottom = app->RenderHeight();

    HRESULT hr = device->StretchRect(*backBuffer, &rect, surf, &rect,
                                     D3DTEXF_LINEAR);             
    if (FAILED(hr))
        hr = device->StretchRect(*backBuffer, &rect, surf, &rect,
                                 D3DTEXF_NONE);                   
    if (FAILED(hr)) {
        DestroyWindow(app->RecordingWindow());                    
        app->RecordingWindow() = nullptr;
        RefreshMainWindowViewport(app);                                           
        if (surf != nullptr)
            surf->Release();                                      
        return;
    }

    const wchar_t* path = app->CaptureSavePath();
    std::intptr_t format;  
    if (wcsstr(path, L".bmp") != nullptr)
        format = 0;                                               
    else if (wcsstr(path, L".jpg") != nullptr)
        format = 1;                                               
    else if (wcsstr(path, L".png") != nullptr)
        format = 3;                                               
    else if (wcsstr(path, L".dds") != nullptr)
        format = 4;                                               
    else if (wcsstr(path, L".dib") != nullptr)
        format = 6;                                               
    else if (wcsstr(path, L".pfm") != nullptr)
        format = 8;                                               
    else if (wcsstr(path, L".hdr") != nullptr)
        format = 7;                                               
    else
        format = reinterpret_cast<std::intptr_t>(surf);           
    auto& d3dxApi = d3dx::Get();
    d3dxApi.saveSurfaceToFileW(path, static_cast<int>(format),
                               surf, nullptr, &rect);            
    if (surf != nullptr) {
        surf->Release();                                          
        surf = nullptr;
    }

    if (*backBuffer == nullptr)                                   
        device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, backBuffer);
    if (r->multisampleAvailable == 0)  
        device->SetRenderTarget(0, *backBuffer);                  
    DestroyWindow(app->RecordingWindow());                        
    app->RecordingWindow() = nullptr;

    const std::int32_t winW = app->state.recRTW;
    const std::int32_t winH = app->state.recRTH;
    const bool tooWide = r->screenHeight > winH;   
    const bool tooHigh = r->screenWidth > winW;    
    if (tooWide || tooHigh) {
        r->presentParameters.BackBufferWidth = winW;   
        r->presentParameters.BackBufferHeight = winH;  
        r->screenWidth = winW;   
        r->screenHeight = winH;  
        PostDeviceReset(app);                                     
    }
    RefreshMainWindowViewport(app);                                               
}

void FrameDriver(MMDApp* app) {
#ifdef RINMOTIONDANCE_DIAG
    static char tickPath[MAX_PATH]{};
    static bool tickInit = GetEnvironmentVariableA(
        "RINMOTIONDANCE_TRACE_TICKS", tickPath, sizeof tickPath) != 0;
    if (tickInit) {
        static LONG tickCount = 0;
        if (InterlockedIncrement(&tickCount) % 60 == 0) {
            if (FILE* f = fopen(tickPath, "a"))
                std::fprintf(f, "tick %ld\n", tickCount), std::fclose(f);
        }
    }
#endif
    AdvanceFrameRenderGate(app);
#ifdef RINMOTIONDANCE_DIAG
    char freezePhysics[2]{};
    if (GetEnvironmentVariableA("RINMOTIONDANCE_AB_FREEZE_PHYSICS", freezePhysics,
                                sizeof(freezePhysics)) == 1 &&
        freezePhysics[0] == '1') {
        app->state.accessoryEditDialogOpen = 1;
    }
    char requireLine[2]{};
    const bool requireLineEnabled =
        GetEnvironmentVariableA("RINMOTIONDANCE_AB_REQUIRE_LINE", requireLine,
                                sizeof(requireLine)) == 1 &&
        requireLine[0] == '1';
    char keepPhysics[2]{};
    const bool keepPhysicsEnabled =
        GetEnvironmentVariableA("RINMOTIONDANCE_AB_KEEP_PHYSICS", keepPhysics,
                                sizeof(keepPhysics)) == 1 &&
        keepPhysics[0] == '1';
    static LONG lineModeRequested = 0;
#endif
    DumpFrameEntryState(app);
#ifdef RINMOTIONDANCE_DIAG
    if (requireLineEnabled && lineModeRequested != 0)
        DumpFrameEntryState(app, "frame_state_line.json");
#endif
    auto& s = *app;

    float& fpsElapsed = s.state.fpsOverlayElapsedSeconds;
    std::uint32_t& fpsFrames = s.state.fpsOverlayFrameCount;
    fpsElapsed += s.DeltaTime();
    if (fpsElapsed > 1.0f) {
        s.state.framesPerSecond = fpsFrames - 1;
        fpsElapsed = 0.0f;
        fpsFrames = 0;
    }
    ++fpsFrames;
    if (s.state.framesPerSecond == 29)
        s.state.framesPerSecond = 30;
    if (s.state.framesPerSecond == 59)
        s.state.framesPerSecond = 60;

    MouseInteractionBegin(app);                                  
    ConsumeRightButtonDrag(app);
    ConsumeMiddleButtonPan(app);
    ConsumeKeyboardInput(app);
#ifdef RINMOTIONDANCE_DIAG
    TraceOperationInput(app, "before");
#endif

    const ViewportDragMode mode = s.InteractionDragMode();
    if (mode != ViewportDragMode::None && s.LeftMouseButtonHeld()) {
        switch (mode) {
        case ViewportDragMode::ViewAxisRotateX:
        case ViewportDragMode::ViewAxisRotateY:
        case ViewportDragMode::ViewAxisRotateZ:
            ModeRotate(app, static_cast<int>(mode) - 1);
            break;
        case ViewportDragMode::LocalAxisTranslateX:
        case ViewportDragMode::LocalAxisTranslateY:
        case ViewportDragMode::LocalAxisTranslateZ:
            ModeTranslate(app, static_cast<int>(mode) - 4);
            break;
        case ViewportDragMode::BoneScale:
            ModeScale(app);
            break;
        case ViewportDragMode::BoneMoveVertical:
        case ViewportDragMode::BoneMoveScreenPlane:
            ModeBoneRotate(app, static_cast<int>(mode) - 8);
            break;
        case ViewportDragMode::PhysicsAxisX:
        case ViewportDragMode::PhysicsAxisY:
        case ViewportDragMode::PhysicsAxisZ:
            ModePhysicsBody(app, static_cast<int>(mode) - 10);
            break;
        case ViewportDragMode::CameraAdjustX:
        case ViewportDragMode::CameraAdjustY:
        case ViewportDragMode::CameraAdjustZ:
            ModeCameraAdjust(app, static_cast<int>(mode) - 13);
            break;
        case ViewportDragMode::AngleAdjustX:
        case ViewportDragMode::AngleAdjustY:
        case ViewportDragMode::AngleAdjustZ:
            ModeAngleAdjust(app, static_cast<int>(mode) - 16);
            break;
        default:
            break;
        }
    }
#ifdef RINMOTIONDANCE_DIAG
    TraceOperationInput(app, "after");
#endif

    MouseInteractionEnd(app);

    unsigned char selActive = 0;
    if (s.state.frameStepPlayback == 0 &&                  
        s.state.depthDeviceEnabled != 0) {                  
        using OpenNIIsTrackingFn = void(__stdcall*)(unsigned char*);
        auto cb = reinterpret_cast<OpenNIIsTrackingFn>(
            s.OpenniTrackingCallback());                         
        if (cb != nullptr)
            cb(&selActive);
        ManageKinectRecordGate(app, selActive);
    }

    RenderFrameScene(app);

    DumpRequestedFrameState(app);

#ifdef RINMOTIONDANCE_DIAG
    if (requireLineEnabled && LineCaptureSwitchReady() &&
        InterlockedCompareExchange(&lineModeRequested, 1, 0) == 0) {
        const bool alreadyModelMode =
            app->state.optflag[0] == 0 &&
            app->EditMode() == ViewportEditMode::Bone;
        if (!alreadyModelMode) {
            RebuildModelModePanel(app);
            app->state.optflag[0] = 0;
            app->EditMode() = ViewportEditMode::Bone;
            PostLanguageSweep(app);
            PostModelReload2(app);
            HandleWindowSize(app);
            InvalidateRect(static_cast<HWND>(app->Hwnd()), nullptr, FALSE);
        }
        char stableCapture[2]{};
        if (GetEnvironmentVariableA(
                "RINMOTIONDANCE_AB_STABLE_CAPTURE", stableCapture,
                sizeof(stableCapture)) == 1 && stableCapture[0] == '1') {
            app->state.fpsOverlayElapsedSeconds = 0.0f;
            app->state.fpsOverlayFrameCount = 0;
            app->state.framesPerSecond = 0;
        }
        if (!keepPhysicsEnabled)
            app->state.accessoryEditDialogOpen = 1;
    }
#endif

    if (s.state.frameStepPlayback != 0) {                   
        if (!RecordingReadbackPass(app))
            return;
    }

    PlaybackCatchup(app, selActive);
    PumpKinectSkeleton(app, selActive);
    PhysicsFrame(app, selActive);
    s.state.selectionActiveLatch = selActive;

    if (s.RecordingWindow() == nullptr)
        PrepareFrameTextOverlay(app);
    PrepareFrameLineOverlay(app);

    if (s.state.timelineAdvanceDue != 0) {
        TimelineAdvance(app);                                     
        s.state.timelineAdvanceDue = 0;
        s.state.timelineAdvanceRequested = 0;
    }
    if (s.state.timelineAdvanceRequested != 0) {
        s.state.timelineAdvanceDue = 1;
        s.state.timelineAdvanceRequested = 0;
    }

    if (s.state.modelReloadPending != 0) {
#ifdef RINMOTIONDANCE_DIAG
        if (getenv("RINMOTIONDANCE_TRACE_REC")) {
            FILE* tf = fopen(getenv("RINMOTIONDANCE_TRACE_REC"), "a");
            if (tf) { fputs("frame section8 reload path\n", tf); fclose(tf); }
        }
#endif
        s.state.cameraAttachmentTransformSuppressed = 0;
        s.ViewOffsetX() = 0.0f;
        s.ViewOffsetY() = 0.0f;
        ReloadModels(app);                                        
        PostModelReload(app);                                     
        s.state.modelReloadPending = 0;
    }

    IDirect3DDevice9* device = nullptr;
    if (s.Renderer() != nullptr)
        device = s.Renderer()->device;                     
    static HRESULT hr = D3D_OK;         
    bool presented = false;
    if (device != nullptr) {
        if (s.state.messageSeen != 0 &&
            s.AviStereoOutput() == 0) {
            presented = true;
            if (s.FullscreenMode() != 0) {
                hr = mme::Present(app, device, nullptr, nullptr,
                                  nullptr, nullptr);
            } else if (s.RecordingWindow() != nullptr) {          
                RECT r;
                r.left = 0;
                r.top = 0;
                r.right = static_cast<LONG>(s.RenderWidth());
                r.bottom = static_cast<LONG>(s.RenderHeight());
                hr = mme::Present(app, device, &r, &r,
                                  s.RecordingWindow(), nullptr);
            } else if (s.FloatingWindow() != nullptr) {
                RECT r = s.ViewportRect();                        
                hr = mme::Present(app, device, &r, &r,
                                  s.state.floatingWindow, nullptr);
            } else {
                RECT r = s.ViewportRect();                        
                hr = mme::Present(app, device, &r, &r,
                                  static_cast<HWND>(s.Hwnd()), nullptr);
            }
            if (hr == D3DERR_DEVICELOST) {
                while (s.Renderer()->device->TestCooperativeLevel() != D3DERR_DEVICENOTRESET) {
                    Sleep(1);
                    if (s.FullscreenMode() != 0) {
                        MSG recovery;
                        if (PeekMessageA(&recovery, nullptr, 0, 0, PM_REMOVE)) {
                            TranslateMessage(&recovery);
                            DispatchMessageA(&recovery);
                        }
                    }
                }
                PostDeviceReset(app);
            }
        }
    }
#ifdef RINMOTIONDANCE_DIAG
    if (presented && SUCCEEDED(hr)) {
        if (requireLineEnabled && lineModeRequested != 0)
            ActivateRefreshedVbCapture();
        HoldPresentCaptureFence();
    }
#endif

    if (s.state.recordPlaybackStartPending != 0) {       
        s.CurrentFrame() =
            s.state.recordSavedFrame;        
        s.state.frameStepPlayback = 1;          

        const std::int32_t fa = s.state.aviRecordStartFrame;
        s.state.playbackStartSeconds =
            static_cast<float>((fa < 0
                                    ? static_cast<double>(fa) + g_Wrap32
                                    : static_cast<double>(fa)) /
                               g_FrameScale);

        const std::int32_t fb = s.state.aviRecordEndFrame;
        s.state.playbackEndSeconds =
            static_cast<float>((fb < 0
                                    ? static_cast<double>(fb) + g_Wrap32
                                    : static_cast<double>(fb)) /
                               g_FrameScale);

        s.state.aviBackgroundSample = s.state.aviRecordStartFrame;
        s.PlaybackCursorSeconds() = s.PlaybackStartSeconds();

        if (s.state.aviRecordStartFrame ==
            s.CurrentFrame())
            s.state.playbackFrameChanged = 1;

        s.state.playbackActive = 1;
        UpdateBoneFrames(app);                                    
        s.state.recordedFrameCount = 0;
        EnableWindow(GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kPlayButton),
                     FALSE);
        s.state.recordPlaybackStartPending = 0;
    }

}

}  
