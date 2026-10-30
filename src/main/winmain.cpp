#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <timeapi.h>   
#include <new>
#include <algorithm>
#include <cmath>

#include <cstdint>
#include <cwchar>
#include <cstdio>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "mme_host_api.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {
    using namespace rinmotiondance;

    (void)hPrevInstance;

    struct EffectRuntime {
        explicit EffectRuntime(HINSTANCE instance) { MmeHostInitializeRuntime(instance); }
        ~EffectRuntime() { MmeHostShutdownRuntime(); }
    } effectRuntime(hInstance);

    MMDApp* app = new MMDApp();
    g_Block = app;
    app->state = MMDAppState{};  
    app->InitDefaults();                                          

    if (*lpCmdLine != '\0') {
        wchar_t converted[256];  
        ConvertAnsiToWide(app->Renderer(),                  
                          lpCmdLine, converted, 0x100);           
        wcscpy_s(app->EnvFileName(), 0x100, converted);
    } else {
        swprintf_s(app->EnvFileName(), 0x100, g_SourceFormat, L"", L"");
    }

    if (!InitMainWindowAndD3D(g_Block, hInstance, nShowCmd))       
        return 0;

    MSG msg{};
    const std::uint64_t firstTick = GetTickCount64();
    std::uint32_t timeHigh = static_cast<std::uint32_t>(firstTick >> 32);
    DWORD timeLow = static_cast<DWORD>(firstTick);
    app->MilliToSec() = 0.001f;
#ifdef RINMOTIONDANCE_DIAG
    char pumpStatsPath[MAX_PATH]{};
    std::FILE* pumpStats = nullptr;
    if (GetEnvironmentVariableA("RINMOTIONDANCE_PUMP_STATS",
                                pumpStatsPath, MAX_PATH) > 0) {
        pumpStats = std::fopen(pumpStatsPath, "a");
    }
    DWORD statsT0 = timeGetTime();
    std::uint64_t statsLoops = 0, statsMsgs = 0, statsPumps = 0;
    double statsSleepMs = 0.0, statsPumpMs = 0.0;
#endif
    while (msg.message != WM_QUIT) {       
        if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
#ifdef RINMOTIONDANCE_DIAG
            if (pumpStats != nullptr) ++statsMsgs;
#endif
        } else {
            const bool recording = app->RecordingWindow() != nullptr;
            float fps = app->FpsLimit();
            if (!std::isfinite(fps) || fps <= 0.0f)
                fps = 60.0f;
            fps = std::min(fps, 120.0f);
            if (!recording) {
                const HWND window = static_cast<HWND>(app->Hwnd());
                const HWND foreground = GetForegroundWindow();
                DWORD foregroundPid = 0;
                if (foreground != nullptr)
                    GetWindowThreadProcessId(foreground, &foregroundPid);
                const bool interacting =
                    (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 ||
                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0 ||
                    (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
                if (window != nullptr && IsIconic(window))
                    fps = std::min(fps, 4.0f);
                else if (foregroundPid != GetCurrentProcessId())
                    fps = std::min(fps, 12.0f);
                else if (app->PlaybackActive() == 0 && !interacting)
                    fps = std::min(fps, 24.0f);
            }

            const std::uint64_t lastTick =
                (static_cast<std::uint64_t>(timeHigh) << 32) | timeLow;
            std::uint64_t nowTick = GetTickCount64();
            double delta = static_cast<double>(nowTick - lastTick) * 0.001;
            const double framePeriod = 1.0 / fps;
            if (!recording && delta < framePeriod) {
#ifdef RINMOTIONDANCE_DIAG
                const DWORD pre = timeGetTime();
#endif
                const DWORD waitMs = static_cast<DWORD>(
                    std::clamp(std::ceil((framePeriod - delta) * 1000.0),
                               1.0, 1000.0));
                MsgWaitForMultipleObjectsEx(0, nullptr, waitMs,
                                            QS_ALLINPUT, MWMO_INPUTAVAILABLE);
#ifdef RINMOTIONDANCE_DIAG
                if (pumpStats != nullptr)
                    statsSleepMs += timeGetTime() - pre;
#endif
                nowTick = GetTickCount64();
                delta = static_cast<double>(nowTick - lastTick) * 0.001;
                if (delta < framePeriod)
                    continue;
            }
            app->DeltaTime() = static_cast<float>(
                std::clamp(delta, 0.0, 0.25));
            app->TimeNowLow() = static_cast<std::uint32_t>(nowTick);
            app->TimeNowHigh() = static_cast<std::uint32_t>(nowTick >> 32);
            timeLow = app->TimeNowLow();
            timeHigh = app->TimeNowHigh();

#ifdef RINMOTIONDANCE_DIAG
            DWORD pumpT0 = pumpStats != nullptr ? timeGetTime() : 0;
#endif
            FrameDriver(g_Block);                            
#ifdef RINMOTIONDANCE_DIAG
            if (pumpStats != nullptr) {
                statsPumpMs += timeGetTime() - pumpT0;
                ++statsPumps;
                ++statsLoops;
                DWORD elapsed = timeGetTime() - statsT0;
                if (elapsed >= 1000) {
                    std::fprintf(pumpStats,
                        "loops=%llu msgs=%llu pump=%llu pumpMs=%.0f "
                        "sleepMs=%.0f fpsLimit=%.2f\n",
                        (unsigned long long)statsLoops,
                        (unsigned long long)statsMsgs,
                        (unsigned long long)statsPumps,
                        statsPumpMs, statsSleepMs, app->FpsLimit());
                    std::fflush(pumpStats);
                    statsT0 += elapsed;
                    statsLoops = statsMsgs = statsPumps = 0;
                    statsSleepMs = statsPumpMs = 0.0;
                }
            }
#endif
        }
    }
#ifdef RINMOTIONDANCE_DIAG
    if (pumpStats != nullptr) std::fclose(pumpStats);
#endif

    if (g_Block != nullptr) {
        MMDApp* victim = g_Block;
        ShutdownCleanup(g_Block);                            
        delete victim;
        g_Block = nullptr;
    }
    return static_cast<int>(msg.wParam);
}
