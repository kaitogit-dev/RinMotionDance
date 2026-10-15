#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <timeapi.h>   
#include <new>

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

    MSG msg;
    std::uint32_t timeHigh = 0;         
    DWORD timeLow = timeGetTime();      
    app->MilliToSec() = 0.001f;         

    PeekMessageA(&msg, nullptr, 0, 0, 0);  
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
            std::uint32_t nowLow = timeGetTime();           
            std::uint32_t nowHigh = 0;                       
            const std::int64_t lastTick =
                (static_cast<std::int64_t>(timeHigh) << 32) | timeLow;
            float delta = static_cast<float>(
                static_cast<std::int64_t>(nowLow) - lastTick) * 0.001f;
            app->DeltaTime() = delta;                        

            float sleepSec = 1.0f / app->FpsLimit() - delta;
            if (app->RecordingWindow() == nullptr && sleepSec > 0.0f) {
#ifdef RINMOTIONDANCE_DIAG
                DWORD pre = timeGetTime();
#endif
                Sleep(static_cast<DWORD>(sleepSec * 1000.0f));
                std::uint64_t addMs = static_cast<std::uint64_t>(
                    static_cast<std::int64_t>(sleepSec / 0.001f));
                nowHigh = static_cast<std::uint32_t>(addMs >> 32);
                nowLow += static_cast<std::uint32_t>(addMs);
                app->DeltaTime() = sleepSec + app->DeltaTime();
#ifdef RINMOTIONDANCE_DIAG
                if (pumpStats != nullptr) statsSleepMs += timeGetTime() - pre;
#endif
            }
            app->TimeNowLow() = nowLow;                      
            app->TimeNowHigh() = nowHigh;                    
            timeLow = nowLow;
            timeHigh = nowHigh;

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
