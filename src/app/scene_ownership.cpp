#include <cstdio>
#include <cstdlib>
#include <new>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/scene_ownership.hpp"

namespace rinmotiondance {

namespace {

#ifdef RINMOTIONDANCE_DIAG
void TraceAccessoryRelease(int slot, const char* phase, const void* track) {
    const char* directory = std::getenv("RINMOTIONDANCE_PMM_TRACE_DIR");
    if (directory == nullptr || directory[0] == '\0')
        return;
    char path[MAX_PATH];
    sprintf_s(path, "%s\\pmm_model_load.log", directory);
    FILE* stream = nullptr;
    if (fopen_s(&stream, path, "ab") != 0 || stream == nullptr)
        return;
    fprintf(stream, "stage=accessory-track-%s slot=%d track=%p\r\n", phase,
            slot, track);
    fclose(stream);
}
#else
inline void TraceAccessoryRelease(int, const char*, const void*) {}
#endif

}  

void ReleaseSceneModels(MMDApp& app) {
    for (int slot = 0; slot < kModelSlotCount; ++slot) {
        unsigned char*& model = app.ModelSlot(slot);
        if (model == nullptr)
            continue;
        ModelDispose(model);
        ::operator delete(model);
        model = nullptr;
    }
}

void ReleaseGlobalTimelineTracks(MMDApp& app) {
    if (app.CameraKeys() != nullptr) {
        ::operator delete(app.CameraKeys());
        app.CameraKeys() = nullptr;
    }
    if (app.LightKeys() != nullptr) {
        ::operator delete(app.LightKeys());
        app.LightKeys() = nullptr;
    }
    if (app.ShadowKeys() != nullptr) {
        ::operator delete(app.ShadowKeys());
        app.ShadowKeys() = nullptr;
    }
    if (app.GravityKeys() != nullptr) {
        ::operator delete(app.GravityKeys());
        app.GravityKeys() = nullptr;
    }
}

void ReleaseAccessoriesAndTracks(MMDApp& app) {
    for (int slot = 0; slot < 255; ++slot) {
        mdl::AccessoryRecord*& accessory = app.AccessorySlot(slot);
        if (accessory != nullptr) {
            DisposeAccessory(accessory);
            ::operator delete(accessory);
            accessory = nullptr;
        }
        mdl::AccessoryKey*& track = app.AccessoryKeys(slot);
        if (track != nullptr) {
            TraceAccessoryRelease(slot, "before-free", track);
            ::operator delete(track);
            TraceAccessoryRelease(slot, "after-free", nullptr);
            track = nullptr;
        }
    }
}

}  
