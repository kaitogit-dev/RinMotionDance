#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <commdlg.h>
#include <cstdarg>
#include <cstdint>
#include <new>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {


constexpr std::size_t kModelBoneFlag2D8D = 0x2D8D;  
constexpr std::size_t kModelSelId2D7C = 0x2D7C;     
constexpr std::size_t kModelSelId2D7D = 0x2D7D;     


using mdl::BoneClipboardRecord;
using mdl::CameraClipboardRecord;
using mdl::DisplayClipboardRecord;
using mdl::GravityClipboardRecord;
using mdl::IkClipboardState;
using mdl::LightClipboardRecord;
using mdl::MorphClipboardRecord;
using mdl::SelectorClipboardState;
using mdl::ShadowClipboardRecord;

template <typename Record>
static void ReplaceClipboardBuffer(Record*& buffer, std::uint32_t count) {
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
    if (count == 0) {
        return;
    }
    buffer = static_cast<Record*>(malloc(count * sizeof(Record)));
    memset(buffer, 0, count * sizeof(Record));
}

#ifdef RINMOTIONDANCE_DIAG
static void TraceModelPaste(const char* format, ...) {
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_STATE_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\model_paste_trace.log", directory);
    std::FILE* stream = nullptr;
    if (fopen_s(&stream, path, "ab") != 0 || stream == nullptr)
        return;
    va_list args;
    va_start(args, format);
    std::vfprintf(stream, format, args);
    va_end(args);
    std::fputc('\n', stream);
    std::fclose(stream);
}
#else
static inline void TraceModelPaste(const char*, ...) {}
#endif


void RefreshRequest(int area);                     
void PanelPaint(MMDApp* app);                      
void SelectionReeval(MMDApp* app);                 
void CopyDirPathW(wchar_t* dest, const wchar_t* src); 
void ReloadModels(MMDApp* app);                    
int SeekModelFrame(unsigned char* model, int frame, int physicsMode);  
void SnapshotSelectedKeysForUndo(unsigned char* model, int frame);  

void UndoModelEdit(unsigned char* model, std::int32_t& frame);  
void RedoModelEdit(unsigned char* model, std::int32_t& frame);  
void ApplyCameraReferenceModeChange(MMDApp* app, int oldMode);  
void StepFrame(MMDApp* app, bool forward);      
void DeleteMarkedKeyframes(MMDApp* app);                    
void StopPlayback(MMDApp* app);             
void RegisterDisplayKeyCurrent(unsigned char* model, int frame);
void DeleteModel(unsigned char* model, int flag);  
void SeekSelectedModelToCurrentFrame(MMDApp* app);                    
void ApplyModelComboSelection(MMDApp* app);                    
INT_PTR CALLBACK SelectNavDlgProc(HWND, UINT, WPARAM, LPARAM);  
bool WaveStartPlayback(void* obj);               
void WaveSeekAndFeed(void* obj, double seconds);  

void RefreshLightPanel(MMDApp* app);                    
void RefreshSelfShadowPanel(MMDApp* app);                    
void ApplyGravityTrack(MMDApp* app);                    
void ApplyAccessoryTrack(MMDApp* app, int idx);           
void SyncAccessoryEditPanel(MMDApp* app);                    
bool RegisterBoneKey(unsigned char* model, unsigned char* rec, int frameOffset, unsigned char useSelected);  
void ResetBoneKeyCursor(unsigned char* model);           
int  PasteAccessoryKeyRecord(MMDApp* app, void* rec, int useSelectedSlot);  
void IdentityCtor(void* obj);                      
void* ConstructArrayElements(void* block, std::uint32_t elementSize,
                             std::uint32_t count,
                             void* ctor);       

int MarkKeyTrackRangeByName(unsigned char* model, std::uint32_t from,
                            std::uint32_t to,
                            const char* name);   
int RegisterCameraKey(MMDApp* app, const void* rec,
                      int overflowAdvertised);             
int RegisterLightKey(MMDApp* app, const void* rec);      
int RegisterSelfShadowKey(MMDApp* app, const void* rec);      
int RegisterGravityKey(MMDApp* app, const void* rec);      


static unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

template <typename Key, typename Source, typename Fill>
static int InsertGlobalFrame(MMDApp* app, Key* table,
                             std::uint32_t relativeFrame,
                             const Source& source, Fill fill,
                             int advertisedCapacity = 10000) {
    if (table == nullptr) {
        return 0;
    }

    const std::uint32_t frame =
        relativeFrame + static_cast<std::uint32_t>(app->CurrentFrame());
    std::uint32_t index = 0;
    std::uint32_t next = table[0].next;
    while (next != 0) {
        index = next;
        const Key& candidate = table[index];
        if (candidate.frame >= frame) {
            break;
        }
        next = candidate.next;
    }

    Key& current = table[index];
    if (current.frame == frame) {
        fill(current, source);
        return 1;
    }

    std::uint32_t freeIndex = 1;
    while (freeIndex < 10000 && table[freeIndex].frame != 0) {
        ++freeIndex;
    }
    if (freeIndex >= 10000) {
        char message[0x100];
        if (app->state.englishUI != 0) {
            sprintf_s(message, sizeof(message),
                      "You cannot regist over %dpoint.\n"
                      "Please execute 'delete unused frame'",
                      advertisedCapacity);
            MessageBoxA(app->MainWindow(), message,
                        "register frame", 0);
        } else {
            static const char kJpOverflow[] =
                "\x93\x6f\x98\x5e\x83\x7c\x83\x43\x83\x93\x83\x67"
                "\x90\x94\x82\xaa%d\x8c\xc2\x82\xf0\x89\x7a\x82\xa6"
                "\x82\xdc\x82\xb5\x82\xbd\n\x82\xb1\x82\xea\x88\xc8"
                "\x8f\xe3\x82\xcc\x93\x6f\x98\x5e\x82\xcd\x8d\x73"
                "\x82\xa6\x82\xdc\x82\xb9\x82\xf1\n\x81\x75\xcc\xda"
                "\xb0\xd1\x95\xd2\x8f\x57\x81\x76\x82\xcc\x81\x75"
                "\x95\x73\x97\x70\xcc\xda\xb0\xd1\x8d\xed\x8f\x9c"
                "\x81\x76\x82\xf0\x8e\xc0\x8d\x73\x82\xb5\x82\xc4"
                "\x89\xba\x82\xb3\x82\xa2";
            static const char kJpTitle[] =
                "\xcc\xda\xb0\xd1\x93\x6f\x98\x5e";
            sprintf_s(message, sizeof(message), kJpOverflow,
                      advertisedCapacity);
            MessageBoxA(app->MainWindow(), message,
                        kJpTitle, 0);
        }
        return 0;
    }

    Key& added = table[freeIndex];
    if (current.frame < frame) {
        current.next = freeIndex;
        added.previous = index;
        added.next = 0;
    } else {
        const std::uint32_t previous = current.previous;
        table[previous].next = freeIndex;
        added.previous = previous;
        current.previous = freeIndex;
        added.next = index;
    }
    added.frame = frame;
    fill(added, source);
    if (frame > app->state.lastRegisteredFrame) {
        app->state.lastRegisteredFrame = frame;
    }
    return 1;
}

static void FillCameraFrame(mdl::CameraKey& key,
                            const CameraClipboardRecord& source) {
    key.distance = source.distance;
    std::memcpy(key.eye, source.eye, sizeof(key.eye));
    std::memcpy(key.target, source.target, sizeof(key.target));
    std::memcpy(key.interpolation, source.interpolation,
                sizeof(key.interpolation));
    key.perspective = source.perspective;
    key.fov = source.fov;
    key.selected = 1;
    key.parentModel = source.parentModel;
    key.parentBone = source.parentBone;
}

static void FillLightFrame(mdl::LightKey& key,
                           const LightClipboardRecord& source) {
    std::memcpy(key.direction, source.direction, sizeof(key.direction));
    std::memcpy(key.color, source.color, sizeof(key.color));
    key.selected = 1;
}

static void FillShadowFrame(mdl::SelfShadowKey& key,
                            const ShadowClipboardRecord& source) {
    key.mode = source.mode;
    key.distance = source.distance;
    key.selected = 1;
}

static void FillGravityFrame(mdl::GravityKey& key,
                             const GravityClipboardRecord& source) {
    key.acceleration = source.acceleration;
    std::memcpy(key.direction, source.direction, sizeof(key.direction));
    key.noise = source.noise;
    key.noiseEnabled = source.noiseEnabled;
    key.selected = 1;
}

int RegisterCameraKey(MMDApp* app, const void* rec,
                      int overflowAdvertised) {  
    const auto& source = *static_cast<const CameraClipboardRecord*>(rec);
    return InsertGlobalFrame(app, app->CameraKeys(), source.frame, source,
                             FillCameraFrame, overflowAdvertised);
}

int RegisterLightKey(MMDApp* app, const void* rec) {  
    const auto& source = *static_cast<const LightClipboardRecord*>(rec);
    return InsertGlobalFrame(app, app->LightKeys(), source.frame, source,
                             FillLightFrame);
}

int RegisterSelfShadowKey(MMDApp* app, const void* rec) {  
    const auto& source = *static_cast<const ShadowClipboardRecord*>(rec);
    return InsertGlobalFrame(app, app->ShadowKeys(), source.frame, source,
                             FillShadowFrame);
}

int RegisterGravityKey(MMDApp* app, const void* rec) {  
    const auto& source = *static_cast<const GravityClipboardRecord*>(rec);
    return InsertGlobalFrame(app, app->GravityKeys(), source.frame, source,
                             FillGravityFrame);
}

template <typename Key>
static Key* FindGlobalFrame(Key* table, std::uint32_t frame) {
    std::uint32_t index = 0;
    for (;;) {
        Key* rec = &table[index];
        const std::uint32_t value = rec->frame;
        if (value >= frame) {
            return value == frame ? rec : nullptr;
        }
        index = rec->next;
        if (index == 0) {
            return nullptr;
        }
    }
}

void RegisterCameraState(MMDApp* app, int frame) {
    CameraClipboardRecord source{};
    const std::uint32_t absolute = static_cast<std::uint32_t>(frame);
    source.frame =
        absolute - static_cast<std::uint32_t>(app->CurrentFrame());
    std::memcpy(source.eye, app->CameraPosition(), sizeof(source.eye));
    std::memcpy(source.target, app->CameraRotation(), sizeof(source.target));
    source.fov = static_cast<std::int32_t>(
        app->state.cameraFov);
    source.perspective = app->CameraPerspective();
    source.distance = app->CameraDistance();
    source.parentModel = app->CameraParentModel();
    source.parentBone = app->CameraParentBone();

    std::uint8_t defaultInterpolation[4][6]{};
    std::memset(defaultInterpolation[0], 20, 12);
    std::memset(defaultInterpolation[2], 107, 12);
    const bool resetInterpolation =
        SendMessageA(GetDlgItem(app->MainWindow(), panel::kPhysicsFrameCheckbox),
                     BM_GETCHECK, 0, 0) == BST_CHECKED;
    const std::uint8_t (*interpolation)[6] = defaultInterpolation;
    mdl::CameraKey* existing = FindGlobalFrame(app->CameraKeys(), absolute);
    if (existing != nullptr && !resetInterpolation) {
        interpolation = existing->interpolation;
    }
    std::memcpy(source.interpolation, interpolation,
                sizeof(source.interpolation));
    RegisterCameraKey(app, &source, 600000);
}

void RegisterLightState(MMDApp* app, int frame) {
    LightClipboardRecord source{};
    source.frame =
        static_cast<std::uint32_t>(frame) -
        static_cast<std::uint32_t>(app->CurrentFrame());
    std::memcpy(source.direction, app->LightDirection(),
                sizeof(source.direction));
    std::memcpy(source.color, app->LightColor(), sizeof(source.color));
    RegisterLightKey(app, &source);
}

static void UnlinkAccessoryKey(mdl::AccessoryKey* keys,
                               std::uint32_t index) {
    mdl::AccessoryKey& key = keys[index];
    keys[key.previous].next = key.next;
    keys[key.next].previous = key.previous;
    key.frame = 0;
    key.previous = 0;
    key.next = 0;
}

template <typename Key>
static void UnlinkGlobalKey(Key* keys, std::uint32_t index) {
    Key& key = keys[index];
    keys[key.previous].next = key.next;
    keys[key.next].previous = key.previous;
    key.frame = 0;
    key.previous = 0;
    key.next = 0;
}

static void ResetCameraRecord(mdl::CameraKey& key, bool head) {
    key.selected = 0;
    key.distance = -45.0f;
    key.eye[0] = 0.0f;
    key.eye[1] = head ? 10.0f : 0.0f;
    key.eye[2] = 0.0f;
    std::memset(key.target, 0, sizeof(key.target));
    key.parentModel = -1;
    key.parentBone = 0;
    std::memset(key.interpolation, 20, 12);
    std::memset(reinterpret_cast<unsigned char*>(key.interpolation) + 12,
                107, 12);
}

static void ResetLightRecord(mdl::LightKey& key, bool head) {
    key.selected = 0;
    if (head) {
        key.direction[0] = -0.5f;
        key.direction[1] = -1.0f;
        key.direction[2] = 0.5f;
        key.color[0] = 0.602f;  
        key.color[1] = 0.602f;
        key.color[2] = 0.602f;
    } else {
        std::memset(key.direction, 0,
                    sizeof(key.direction) + sizeof(key.color));
    }
}

static unsigned char DefaultShadowMode(MMDApp* app) {
    D3DRenderer* locale = app->Renderer();
    if (locale != nullptr && locale->postProcessEnabled != 0) {  
        return 1;
    }
    return app->state.selfShadowMode != 0 ? 1 : 0;
}

static void ResetShadowRecord(MMDApp* app, mdl::SelfShadowKey& key) {
    key.selected = 0;
    key.mode = DefaultShadowMode(app) != 0 ? 1 : 0;
    key.distance = 0.01125f;
}

static void ResetGravityRecord(mdl::GravityKey& key) {
    key.selected = 0;
    key.noiseEnabled = 0;
    key.noise = 10;
    key.acceleration = 9.8000002f;
    key.direction[0] = 0.0f;
    key.direction[1] = -1.0f;
    key.direction[2] = 0.0f;
}

static void ResetAccessoryRecord(mdl::AccessoryKey& key) {
    key.selected = 0;
    key.parentBone = 0;
    key.visible = 1;
    key.shadowEnabled = 0;
    key.parentModel = -1;
    std::memset(key.position, 0, sizeof(key.position));
    std::memset(key.rotation, 0, sizeof(key.rotation));
    key.opacity = 1.0f;
    key.scale = 1.0f;
}

void DeleteMarkedKeyframes(MMDApp* app) {
    if (app == nullptr) {
        return;
    }
    if (app->state.optflag[0] != 0) {
        auto* camera = app->CameraKeys();
        if (camera[0].selected != 0) {
            ResetCameraRecord(camera[0], true);
        }
        for (std::uint32_t i = 1; i < 10000; ++i) {
            if (camera[i].selected != 0) {
                UnlinkGlobalKey(camera, i);
                ResetCameraRecord(camera[i], false);
            }
        }

        auto* light = app->LightKeys();
        if (light[0].selected != 0) {
            ResetLightRecord(light[0], true);
            light[0].next = 0;
        }
        for (std::uint32_t i = 1; i < 10000; ++i) {
            if (light[i].selected != 0) {
                UnlinkGlobalKey(light, i);
                ResetLightRecord(light[i], false);
            }
        }

        auto* shadow = app->ShadowKeys();
        if (shadow[0].selected != 0) {
            ResetShadowRecord(app, shadow[0]);
            shadow[0].next = 0;
        }
        for (std::uint32_t i = 1; i < 10000; ++i) {
            if (shadow[i].selected != 0) {
                UnlinkGlobalKey(shadow, i);
                ResetShadowRecord(app, shadow[i]);
            }
        }

        auto* gravity = app->GravityKeys();
        if (gravity[0].selected != 0) {
            ResetGravityRecord(gravity[0]);
            gravity[0].next = 0;
        }
        for (std::uint32_t i = 1; i < 10000; ++i) {
            if (gravity[i].selected != 0) {
                UnlinkGlobalKey(gravity, i);
                ResetGravityRecord(gravity[i]);
            }
        }

        for (std::uint32_t slot = 0; slot < 255; ++slot) {
            mdl::AccessoryKey* accessory = app->AccessoryKeys(slot);
            if (accessory == nullptr) {
                continue;
            }
            if (accessory[0].selected != 0) {
                ResetAccessoryRecord(accessory[0]);
            }
            for (std::uint32_t i = 1; i < 10000; ++i) {
                if (accessory[i].selected != 0) {
                    UnlinkAccessoryKey(accessory, i);
                    ResetAccessoryRecord(accessory[i]);
                }
            }
        }
    } else {
        unsigned char* model = ActiveModel(app);
        if (model != nullptr)
            DeleteMarkedModelKeys(model,
                      app->state.currentFrame);
    }

    PanelPaint(app);
    SelectionReeval(app);
    if (app->state.optflag[0] != 0) {
        ReloadModels(app);
        RefreshLightPanel(app);
        RefreshSelfShadowPanel(app);
        ApplyGravityTrack(app);
        for (int slot = 0; slot < 255; ++slot) {
            if (app->AccessoryKeys(slot) != nullptr) {
                ApplyAccessoryTrack(app, slot);
            }
        }
        SyncAccessoryEditPanel(app);
    } else {
        unsigned char* model = ActiveModel(app);
        if (model != nullptr) {
            SeekModelFrame(model,
                      app->state.currentFrame,
                      app->PlaybackPhysicsMode());
        }
    }
}

void ViewResetRefresh(MMDApp* app) {
    PostViewRefresh(app);
    RefreshRequest(-1);
}

void ResetCameraAttachmentBasis(MMDApp* app) {
    D3DMATRIX& basis = app->CameraAttachmentBasis();
    basis = {};
    basis.m[0][0] = 1.0f;
    basis.m[1][1] = 1.0f;
    basis.m[2][2] = 1.0f;
    basis.m[3][3] = 1.0f;
}

void ResetViewVariant(MMDApp* app, float rot, bool gate30C, bool rotIn310) {
    if (app->state.optflag[0] != 0) {
        app->CameraRotation()[0] = rotIn310 ? rot : 0.0f;
        app->CameraRotation()[1] = rotIn310 ? 0.0f : rot;
        app->CameraRotation()[2] = 0.0f;
        app->CameraDistance() = -45.0f;
        ViewResetRefresh(app);
        return;
    }
    const std::int32_t a0430 = app->CameraParentModel();
    const std::uint8_t slot = app->SelectedModelSlot();
    const std::uint8_t esi = app->state.followCameraEnabled;
    const bool c = a0430 >= 0;  
    const bool b = (slot == a0430) &&
                   app->PlaybackActive() == 0 &&
                   esi != 0;  
    if (c && b) {
        ViewResetRefresh(app);  
        return;
    }
    if (c && esi != 0) {
        app->CameraAttachmentTransformSuppressed() = 1;
    }
    app->ViewOffsetX() = 0.0f;
    app->CameraDistance() = -45.0f;
    if (gate30C) {
        if (app->CameraReferenceMode() ==
            CameraAttachmentReference::SelectedBone) {
            app->ViewOffsetY() = 0.0f;
        } else {
            app->ViewOffsetY() = 10.0f;
        }
    } else {
        app->ViewOffsetY() = 0.0f;
    }
    app->CameraRotation()[0] = rotIn310 ? rot : 0.0f;
    app->CameraRotation()[1] = rotIn310 ? 0.0f : rot;
    app->CameraRotation()[2] = 0.0f;
    ResetCameraAttachmentBasis(app);
    ViewResetRefresh(app);
}

float FrameToSeconds(std::int32_t v) {
    const double d = static_cast<double>(static_cast<std::uint32_t>(v));
    return static_cast<float>(d / 30.0);  
}

std::int32_t FirstSelectedFrame374(MMDApp* app) {
    auto* keys = app->CameraKeys();
    for (std::int32_t i = 0; i < 0x2710; ++i) {
        if (keys[i].selected != 0) {
            return i;
        }
    }
    return -1;
}

template <typename Key>
void SelectGlobalFrameRange(Key* keys, std::uint32_t from,
                            std::uint32_t to) {
    for (std::uint32_t i = 0; i < 10000; ++i) {
        Key& key = keys[i];
        key.selected = 0;
        if (from <= key.frame && key.frame <= to &&
            (i == 0 || key.frame != 0)) {
            key.selected = 1;
        }
    }
}

std::int32_t FirstSelectedBoneKey(unsigned char* model) {
    mdl::BoneKey* keys = mdl::BoneKeys(model);
    for (std::int32_t i = 0;
         i < static_cast<std::int32_t>(mdl::kBoneKeyCapacity); ++i) {
        if (keys[i].allocated != 0) {
            return i;
        }
    }
    return -1;
}

static void ReadBoneCurveChannel(MMDApp* app, const mdl::BoneKey& key,
                                 std::size_t channel, std::size_t row) {
    app->state.lightA[row] = key.interpolation[channel];
    app->state.lightB[row] = key.interpolation[4 + channel];
    app->state.lightC[row] = key.interpolation[8 + channel];
    app->state.lightD[row] = key.interpolation[12 + channel];
}

static void WriteBoneCurveChannel(MMDApp* app, mdl::BoneKey& key,
                                  std::size_t channel, std::size_t row) {
    key.interpolation[channel] = app->state.lightA[row];
    key.interpolation[4 + channel] = app->state.lightB[row];
    key.interpolation[8 + channel] = app->state.lightC[row];
    key.interpolation[12 + channel] = app->state.lightD[row];
}

static void ResetBoneCurveChannel(mdl::BoneKey& key, std::size_t channel) {
    key.interpolation[channel] = 0x14;
    key.interpolation[4 + channel] = 0x14;
    key.interpolation[8 + channel] = 0x6B;
    key.interpolation[12 + channel] = 0x6B;
}

static void ReadCameraCurveChannel(MMDApp* app, const mdl::CameraKey& key,
                                   std::size_t channel, std::size_t row) {
    app->state.lightA[row] = key.interpolation[0][channel];
    app->state.lightB[row] = key.interpolation[1][channel];
    app->state.lightC[row] = key.interpolation[2][channel];
    app->state.lightD[row] = key.interpolation[3][channel];
}

static void WriteCameraCurveChannel(MMDApp* app, mdl::CameraKey& key,
                                    std::size_t channel, std::size_t row) {
    key.interpolation[0][channel] = app->state.lightA[row];
    key.interpolation[1][channel] = app->state.lightB[row];
    key.interpolation[2][channel] = app->state.lightC[row];
    key.interpolation[3][channel] = app->state.lightD[row];
}

static void ResetCameraCurveChannel(mdl::CameraKey& key,
                                    std::size_t channel) {
    key.interpolation[0][channel] = 0x14;
    key.interpolation[1][channel] = 0x14;
    key.interpolation[2][channel] = 0x6B;
    key.interpolation[3][channel] = 0x6B;
}

static bool TextEqN(const char* buf, const char* lit, std::size_t n) {
    return memcmp(buf, lit, n) == 0;
}

static const char kJpAllFrame[] = "\x91\x53\xCC\xDA\xB0\xD1";          
static const char kJpAllFacial[] =
    "\x91\x53\x95\x5C\x8F\xEE\xCC\xDA\xB0\xD1";                       
static const char kJpAllBone[] =
    "\x91\x53\x83\x7B\x81\x5B\x83\x93\xCC\xDA\xB0\xD1";               
static const char kJpSelBone[] = "\x91\x49\x91\xF0\xCE\xDE\xB0\xDD";  
static const char kJpSelFacial[] = "\x91\x49\x91\xF0\x95\x5C\x8F\xEE";  
static const char kLightNameRoot[] = "\x83\x8B\x81\x5B\x83\x67";

template <typename Key>
static void MarkFrameSelected(Key* keys, std::uint32_t start,
                              std::uint32_t frame) {
    std::uint32_t index = start;
    while (keys[index].frame < frame) {
        const std::uint32_t next = keys[index].next;
        if (next == 0) {
            break;
        }
        index = next;
    }
    if (keys[index].frame == frame) {
        keys[index].selected = 1;
    }
}

template <typename Key>
static void MarkModelFrameAllocated(Key* keys, std::uint32_t start,
                                    std::uint32_t frame) {
    std::uint32_t index = start;
    while (keys[index].frame < frame) {
        const std::uint32_t next = keys[index].next;
        if (next == 0) {
            break;
        }
        index = next;
    }
    if (keys[index].frame == frame) {
        keys[index].allocated = 1;
    }
}

template <typename Key>
static void ClearModelKeyMarks(Key* keys, std::size_t capacity) {
    for (std::size_t i = 0; i < capacity; ++i) {
        keys[i].allocated = 0;
    }
}

template <typename Key>
static void SelectModelFrameRange(Key* keys, std::size_t capacity,
                                  std::uint32_t activeHeadCount,
                                  std::uint32_t from, std::uint32_t to) {
    for (std::size_t i = 0; i < capacity; ++i) {
        Key& key = keys[i];
        key.allocated = 0;
        if (from <= key.frame && key.frame <= to &&
            (i < activeHeadCount || key.frame != 0)) {
            key.allocated = 1;
        }
    }
}

template <typename Key>
static std::uint32_t CountMarkedModelKeys(const Key* keys,
                                          std::size_t capacity,
                                          std::uint32_t& minFrame) {
    std::uint32_t count = 0;
    for (std::size_t i = 0; i < capacity; ++i) {
        const Key& key = keys[i];
        if (key.allocated == 0) {
            continue;
        }
        ++count;
        if (key.frame < minFrame) {
            minFrame = key.frame;
        }
    }
    return count;
}

template <typename Key>
static std::uint32_t CountSelectedKeys(const Key* keys, std::size_t capacity,
                                       std::uint32_t& minFrame) {
    std::uint32_t count = 0;
    for (std::size_t i = 0; i < capacity; ++i) {
        const Key& key = keys[i];
        if (key.selected == 0) {
            continue;
        }
        ++count;
        if (key.frame < minFrame) {
            minFrame = key.frame;
        }
    }
    return count;
}

template <typename Key>
static void ClearSelectedKeys(Key* keys, std::size_t capacity) {
    for (std::size_t i = 0; i < capacity; ++i) {
        keys[i].selected = 0;
    }
}

static const char kMsgDelModelJp[] =
    "\x83\x82\x83\x66\x83\x8b\x81\x46"
    "%s"
    "\x82\xF0\x8D\xED\x8F\x9C\x82\xB5\x82\xDC\x82\xB7\n"
    "\x83\x82\x83\x66\x83\x8b\x82\xCC\x83\x74\x83\x8C\x81\x5B\x83\x80"
    "\x83\x66\x81\x5B\x83\x5E\x82\xE0\x91\x53\x82\xC4\x8D\xED\x8F\x9C"
    "\x82\xB3\x82\xEA\x82\xDC\x82\xB7\n"
    "\x82\xDC\x82\xBD\x83\x41\x83\x4E\x83\x5A\x83\x54\x83\x8A\x82\xC5"
    "\x82\xB1\x82\xCC\x83\x82\x83\x66\x83\x8B\x82\xC9\x92\xC7\x8F\x5D"
    "\x82\xB5\x82\xE9\x82\xE0\x82\xCC\x82\xCD\x92\x6E\x96\xCA\x82\xC9"
    "\x92\xC7\x8F\x5D\x82\xB5\x82\xE9\x82\xE6\x82\xA4\x82\xC9\x95\xCF"
    "\x8D\x58\x82\xB3\x82\xEA\x82\xDC\x82\xB7\n"
    "\x28\x82\xB1\x82\xCC\x91\x80\x8D\xEC\x82\xCD\x8C\xB3\x82\xC9"
    "\x96\xDF\x82\xB7\x8E\x96\x82\xCD\x82\xC5\x82\xAB\x82\xDC\x82\xB9"
    "\x82\xF1\x29\n\n"
    "\x8D\xED\x8F\x9C\x82\xB5\x82\xC4\x82\xE0\x82\xE6\x82\xEB\x82\xB5"
    "\x82\xA2\x82\xC5\x82\xB7\x82\xA9\x81\x48";
static const char kCaptionDelModelJp[] =
    "\x83\x82\x83\x66\x83\x8b\x8d\xed\x8f\x9c";
static const char kMsgDelModelEn[] =
    "Trying to delete Model(%s). \n"
    "All flame data about this model will be deleted too.\n"
    "(This operation cannot undo!!)\n\nAre you OK?";

static const wchar_t kFilterModel[] =
    L"All Model files(*.pmd,*.pmx)\x00*.pmd;*.pmx\x00";
static const wchar_t kInitDirModel[] = L"UserFile\\Model";
static const wchar_t kDefExtPmd[] = L"pmd;pmx";
static const wchar_t kTitleLoadModel[] = L"load model";
static const wchar_t kTitleOpenFileJp[] =
    L"\x30D5\x30A1\x30A4\x30EB\x3092\x958B\x304F";


static void Cmd400_Undo(MMDApp* app, HWND hwnd) {
    if (app->state.optflag[0] != 0) {
        return;  
    }
    app->SceneModified() = 1;
    unsigned char* model = ActiveModel(app);
    mdl::ModelRecord& record = *mdl::Mdl(model);
    UndoModelEdit(model, app->state.currentFrame);
    const std::int32_t cnt = record.undoState[0];
    if (record.undoRings[0].slots[cnt].operation == 0) {
        EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), FALSE);
        record.undoDirty = 0;
        SetFocus(hwnd);
    }
    if (record.undoState[0] == record.undoState[1]) {
        EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), FALSE);
        record.undoDirty = 0;
        SetFocus(hwnd);
    }
    EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), TRUE);
    record.redoDirty = 1;
    PanelPaint(app);
    SelectionReeval(app);
    SetFocus(hwnd);
    app->PhysicsResetPending() = 1;
}
static void Cmd400_Redo(MMDApp* app, HWND hwnd) {
    if (app->state.optflag[0] != 0) {
        return;  
    }
    app->SceneModified() = 1;
    unsigned char* model = ActiveModel(app);
    mdl::ModelRecord& record = *mdl::Mdl(model);
    RedoModelEdit(model, app->state.currentFrame);
    if (record.undoState[0] == record.undoState[1]) {
        EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);
        record.redoDirty = 0;
        SetFocus(hwnd);
    }
    EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);
    record.undoDirty = 1;
    PanelPaint(app);
    SelectionReeval(app);
    const std::int32_t cnt = record.undoState[0];
    if (record.undoRings[0].slots[cnt].operation != 1) {
        SeekModelFrame(model,
                  app->state.currentFrame,
                  app->PlaybackPhysicsMode());
    }
    SetFocus(hwnd);
    app->PhysicsResetPending() = 1;
}
static void Cmd400_ViewResetReload(MMDApp* app) {
    if (app->state.optflag[0] != 0) {
        app->CameraRotation()[0] = 1.5707964f;
        app->CameraRotation()[1] = 0.0f;
        app->CameraRotation()[2] = 0.0f;
        app->CameraDistance() = -45.0f;
        ViewResetRefresh(app);
        return;  
    }
    const std::int32_t a0430 = app->CameraParentModel();
    const std::uint8_t slot = app->SelectedModelSlot();
    const bool hit = (slot == a0430) &&
                     app->PlaybackActive() == 0 &&
                     app->state.followCameraEnabled != 0 &&
                     a0430 >= 0;
    if (!hit) {
        app->CameraAttachmentTransformSuppressed() = 0;
        app->ViewOffsetX() = 0.0f;
        app->ViewOffsetY() = 0.0f;
        ReloadModels(app);  
        PostModelReload(app);
    }
    ViewResetRefresh(app);
}
static void Cmd400_PlayRangeEdits(MMDApp* app, HWND hwnd) {
    if (app->PlaybackActive() != 0) {
        app->PlaybackActive() = 0;
        StopPlayback(app);
        if (app->WaveEnabled() != 0) {
            CloseDataFile(app->Audio());
            WaveStartPlayback(app->Audio());
        }
        return;  
    }
    char text[0x100];
    GetWindowTextA(GetDlgItem(hwnd, panel::kPlayStartFrameEdit), text, 8);
    const std::int32_t start = atol(text);  
    GetWindowTextA(GetDlgItem(hwnd, panel::kPlayStopFrameEdit), text, 8);
    std::int32_t end = atol(text);  
    if (app->PlaybackStartsAtCurrentFrame() != 0) {
        app->PlaybackStartSeconds() = FrameToSeconds(
            app->state.currentFrame);
        app->PlaybackFrameChanged() = 1;
    } else {
        app->PlaybackStartSeconds() = FrameToSeconds(start);
        if (start == app->state.currentFrame) {
            app->PlaybackFrameChanged() = 1;
        }
    }
    if (end == 0) {
        end = app->state.lastRegisteredFrame;
    }
    app->PlaybackEndSeconds() = FrameToSeconds(end);
    app->PlaybackCursorSeconds() = app->PlaybackStartSeconds();
    app->state.playbackEnabledSnapshot[0] =
        IsWindowEnabled(GetDlgItem(hwnd, panel::kBonePasteButton)) ? 1 : 0;
    app->state.playbackEnabledSnapshot[1] =
        IsWindowEnabled(GetDlgItem(hwnd, panel::kBoneReversePasteButton)) ? 1 : 0;
    app->state.playbackEnabledSnapshot[4] =
        IsWindowEnabled(GetDlgItem(hwnd, panel::kCurvePasteButton)) ? 1 : 0;
    app->state.playbackEnabledSnapshot[2] =
        IsWindowEnabled(GetDlgItem(hwnd, panel::kPasteButton)) ? 1 : 0;
    app->state.playbackEnabledSnapshot[3] =
        IsWindowEnabled(GetDlgItem(hwnd, panel::kReversePasteButton)) ? 1 : 0;
    app->state.playbackEnabledSnapshot[5] =
        IsWindowEnabled(GetDlgItem(hwnd, panel::kUndoButton)) ? 1 : 0;
    app->state.playbackEnabledSnapshot[6] =
        IsWindowEnabled(GetDlgItem(hwnd, panel::kRedoButton)) ? 1 : 0;
    app->PlaybackActive() = 1;
    UpdateBoneFrames(app);  
    if (app->WaveEnabled() != 0) {
        if (app->AudioSeekReady() != 0) {
            app->AudioSeekReady() = 0;
            KillTimer(hwnd, 0x64);
            CloseDataFile(app->Audio());
            WaveStartPlayback(app->Audio());
        }
        if (app->AutomaticFrameAdvanceEnabled() == 0) {
            SetFrameNormalized(
                app->FrameNormalization());
            WaveSeekAndFeed(app->Audio(),
                      static_cast<double>(app->PlaybackStartSeconds()));
        }
    }
    app->PlaybackClockAnchorLow() = app->TimeNowLow();
    app->PlaybackClockAnchorHigh() = app->TimeNowHigh();
}
static void Cmd400_FrameRangeSelect(MMDApp* app, HWND hwnd) {
    char text[0x100];
    GetWindowTextA(GetDlgItem(hwnd, panel::kRangeStartEdit), text, 8);
    const std::int32_t from = atol(text);   
    GetWindowTextA(GetDlgItem(hwnd, panel::kRangeStopEdit), text, 8);
    const std::int32_t to = atol(text);     
    GetWindowTextA(GetDlgItem(hwnd, panel::kRegisterScopeCombo), text, 20);
    if (app->state.optflag[0] == 0) {
        if (TextEqN(text, kJpAllFrame, 7) || TextEqN(text, "All frame", 10)) {
            {
                unsigned char* model = ActiveModel(app);
                SelectModelFrameRange(
                    mdl::BoneKeys(model), mdl::kBoneKeyCapacity,
                    mdl::Mdl(model)->boneCount,
                    static_cast<std::uint32_t>(from),
                    static_cast<std::uint32_t>(to));
            }
            {
                unsigned char* model = ActiveModel(app);
                const std::uint32_t morphCount =
                    mdl::Mdl(model)->morphCount;
                if (morphCount > 0) {
                    SelectModelFrameRange(
                        mdl::MorphKeys(model), 20000, morphCount,
                        static_cast<std::uint32_t>(from),
                        static_cast<std::uint32_t>(to));
                }
            }
            {
                unsigned char* model = ActiveModel(app);
                SelectModelFrameRange(
                    mdl::DisplayKeys(model), 1000, 1,
                    static_cast<std::uint32_t>(from),
                    static_cast<std::uint32_t>(to));
            }
        } else if (TextEqN(text, kJpAllFacial, 11) ||
                   TextEqN(text, "All facial", 11)) {
            unsigned char* model = ActiveModel(app);
            const std::uint32_t morphCount = mdl::Mdl(model)->morphCount;
            if (morphCount > 0) {
                SelectModelFrameRange(
                    mdl::MorphKeys(model), 20000, morphCount,
                    static_cast<std::uint32_t>(from),
                    static_cast<std::uint32_t>(to));
            }
        } else if (TextEqN(text, kJpAllBone, 13) ||
                   TextEqN(text, "All bone", 9)) {
            unsigned char* model = ActiveModel(app);
            SelectModelFrameRange(
                mdl::BoneKeys(model), mdl::kBoneKeyCapacity,
                mdl::Mdl(model)->boneCount,
                static_cast<std::uint32_t>(from),
                static_cast<std::uint32_t>(to));
        } else if (TextEqN(text, kJpSelBone, 9) ||
                   TextEqN(text, "Sel Bone", 9)) {
            unsigned char* model = ActiveModel(app);
            const std::int32_t boneCount = mdl::Mdl(model)->boneCount;
            if (boneCount > 0) {
                const mdl::ModelRecord& record = *mdl::Mdl(model);
                unsigned char* sel = record.boneSelection;
                const mdl::BoneRecord* bones = record.boneTable;
                for (std::int32_t i = 0; i < boneCount; ++i) {
                    if (sel[i] != 0) {
                        MarkKeyTrackRangeByName(
                            model, static_cast<std::uint32_t>(from),
                            static_cast<std::uint32_t>(to),
                            bones[i].name);
                    }
                }
            }
        } else if (TextEqN(text, kJpSelFacial, 9) ||
                   TextEqN(text, "Sel facial", 11)) {
            unsigned char* model = ActiveModel(app);
            const mdl::ModelRecord& record = *mdl::Mdl(model);
            const std::uint8_t morphCount = record.facialFrameCount;
            if (morphCount != 0) {
                mdl::FrameGroup* groups = record.displayFrames;
                for (std::uint8_t i = 0; i < morphCount; ++i) {
                    mdl::FrameGroup& group = groups[i];
                    if (group.selected != 0) {
                        MarkKeyTrackRangeByName(
                            model, static_cast<std::uint32_t>(from),
                            static_cast<std::uint32_t>(to), group.name);
                    }
                }
            }
        } else {
            unsigned char* model = ActiveModel(app);
            MarkKeyTrackRangeByName(model, static_cast<std::uint32_t>(from),
                                    static_cast<std::uint32_t>(to), text);
        }
        PanelPaint(app);
        SelectionReeval(app);
        SetFocus(hwnd);
    } else {
        const LRESULT sel =
            SendMessageA(GetDlgItem(hwnd, panel::kRegisterScopeCombo), CB_GETCURSEL,
                         0, 0);
        if (sel == 0) {
            SelectGlobalFrameRange(
                app->CameraKeys(),
                static_cast<std::uint32_t>(from),
                static_cast<std::uint32_t>(to));
        } else if (sel == 1) {
            SelectGlobalFrameRange(
                app->LightKeys(),
                static_cast<std::uint32_t>(from),
                static_cast<std::uint32_t>(to));
        } else if (sel == 2) {
            SelectGlobalFrameRange(
                app->ShadowKeys(),
                static_cast<std::uint32_t>(from),
                static_cast<std::uint32_t>(to));
        } else if (sel == 3) {
            SelectGlobalFrameRange(
                app->GravityKeys(),
                static_cast<std::uint32_t>(from),
                static_cast<std::uint32_t>(to));
        } else {
            for (std::int32_t slot = 0; slot < 0xFF; ++slot) {
                mdl::AccessoryRecord* acc = app->AccessorySlot(slot);
                if (acc != nullptr &&
                    static_cast<unsigned int>(acc->order) ==
                        static_cast<unsigned int>(sel - 4)) {
                    auto* keys = app->AccessoryKeys(slot);
                    SelectGlobalFrameRange(
                        keys, static_cast<std::uint32_t>(from),
                        static_cast<std::uint32_t>(to));
                    break;
                }
            }
        }
        PanelPaint(app);
        SelectionReeval(app);
        SetFocus(hwnd);
    }
}
static void Cmd400_SelectionMarkSync(MMDApp* app) {
    if (app->state.optflag[0] == 0) {
        unsigned char* model = ActiveModel(app);
        mdl::ModelRecord* modelRecord = mdl::Mdl(model);
        mdl::BoneKey* boneKeys = mdl::BoneKeys(model);
        mdl::MorphKey* morphKeys = mdl::MorphKeys(model);
        mdl::DisplayKey* displayKeys = mdl::DisplayKeys(model);

        for (std::uint32_t keyIndex = 0; keyIndex < static_cast<std::uint32_t>(mdl::kBoneKeyCapacity); ++keyIndex) {
            if (boneKeys[keyIndex].allocated != 0) {
                const std::uint32_t frame = boneKeys[keyIndex].frame;
                if (modelRecord->boneCount > 0) {
                    for (std::uint32_t i = 0; i < modelRecord->boneCount;
                         ++i) {
                        MarkModelFrameAllocated(boneKeys, i, frame);
                    }
                }
                if (modelRecord->morphCount > 0) {
                    for (std::uint32_t i = 0; i < modelRecord->morphCount;
                         ++i) {
                        MarkModelFrameAllocated(morphKeys, i, frame);
                    }
                }
                MarkModelFrameAllocated(displayKeys, 0, frame);
            }
        }
        for (std::uint32_t keyIndex = 0; keyIndex < 20000; ++keyIndex) {
            if (morphKeys[keyIndex].allocated != 0) {
                const std::uint32_t frame = morphKeys[keyIndex].frame;
                if (modelRecord->boneCount > 0) {
                    for (std::uint32_t i = 0; i < modelRecord->boneCount;
                         ++i) {
                        MarkModelFrameAllocated(boneKeys, i, frame);
                    }
                }
                if (modelRecord->morphCount > 0) {
                    for (std::uint32_t i = 0; i < modelRecord->morphCount;
                         ++i) {
                        MarkModelFrameAllocated(morphKeys, i, frame);
                    }
                }
                MarkModelFrameAllocated(displayKeys, 0, frame);
            }
        }
        for (std::uint32_t keyIndex = 0; keyIndex < 1000; ++keyIndex) {
            if (displayKeys[keyIndex].allocated != 0) {
                const std::uint32_t frame = displayKeys[keyIndex].frame;
                if (modelRecord->boneCount > 0) {
                    for (std::uint32_t i = 0; i < modelRecord->boneCount;
                         ++i) {
                        MarkModelFrameAllocated(boneKeys, i, frame);
                    }
                }
                if (modelRecord->morphCount > 0) {
                    for (std::uint32_t i = 0; i < modelRecord->morphCount;
                         ++i) {
                        MarkModelFrameAllocated(morphKeys, i, frame);
                    }
                }
            }
        }
        PanelPaint(app);
        SelectionReeval(app);
    } else {
        auto* cameraKeys = app->CameraKeys();
        auto* lightKeys = app->LightKeys();
        auto* shadowKeys = app->ShadowKeys();
        auto* gravityKeys = app->GravityKeys();

        const auto markAccessories =
            [app](std::uint32_t frame, std::int32_t excludedSlot) {
                for (std::int32_t slot = 0; slot < 0xFF; ++slot) {
                    if (slot == excludedSlot ||
                        app->AccessorySlot(slot) == nullptr) {
                        continue;
                    }
                    auto* keys = app->AccessoryKeys(slot);
                    MarkFrameSelected(keys, 0, frame);
                }
            };

        for (std::int32_t i = 0; i < 0x2710; ++i) {
            if (cameraKeys[i].selected != 0) {
                const std::uint32_t frame = cameraKeys[i].frame;
                MarkFrameSelected(lightKeys, 0, frame);
                MarkFrameSelected(shadowKeys, 0, frame);
                MarkFrameSelected(gravityKeys, 0, frame);
                markAccessories(frame, -1);
            }
            if (lightKeys[i].selected != 0) {
                const std::uint32_t frame = lightKeys[i].frame;
                MarkFrameSelected(cameraKeys, 0, frame);
                MarkFrameSelected(shadowKeys, 0, frame);
                MarkFrameSelected(gravityKeys, 0, frame);
                markAccessories(frame, -1);
            }
            if (shadowKeys[i].selected != 0) {
                const std::uint32_t frame = shadowKeys[i].frame;
                MarkFrameSelected(cameraKeys, 0, frame);
                MarkFrameSelected(lightKeys, 0, frame);
                MarkFrameSelected(gravityKeys, 0, frame);
                markAccessories(frame, -1);
            }
            if (gravityKeys[i].selected != 0) {
                const std::uint32_t frame = gravityKeys[i].frame;
                MarkFrameSelected(cameraKeys, 0, frame);
                MarkFrameSelected(lightKeys, 0, frame);
                MarkFrameSelected(shadowKeys, 0, frame);
                markAccessories(frame, -1);
            }
        }
        for (std::int32_t slot = 0; slot < 0xFF; ++slot) {
            if (app->AccessorySlot(slot) == nullptr) {
                continue;
            }
            auto* keys = app->AccessoryKeys(slot);
            for (std::int32_t i = 0; i < 0x2710; ++i) {
                if (keys[i].selected == 0) {
                    continue;
                }
                const std::uint32_t frame = keys[i].frame;
                MarkFrameSelected(cameraKeys, 0, frame);
                MarkFrameSelected(lightKeys, 0, frame);
                MarkFrameSelected(shadowKeys, 0, frame);
                MarkFrameSelected(gravityKeys, 0, frame);
                markAccessories(frame, slot);
            }
        }
        PanelPaint(app);
        SelectionReeval(app);
    }
}
static void Cmd400_FrameCopy(MMDApp* app, HWND hwnd) {
    std::uint32_t minFrame = 0xFFFFFFFFu;  
    if (app->state.optflag[0] == 0) {
        if (app->DisplayClipboard() != nullptr &&
            app->ClipboardCounts().displays != 0) {
            auto* records = app->DisplayClipboard();
            const std::uint32_t camCount =
                app->ClipboardCounts().displays;
            for (std::uint32_t i = 0; i < camCount; ++i) {
                if (records[i].ikStates != nullptr) {
                    free(records[i].ikStates);
                    records[i].ikStates = nullptr;
                }
                if (records[i].selectorStates != nullptr) {
                    free(records[i].selectorStates);
                    records[i].selectorStates = nullptr;
                }
            }
        }
        auto& clipboardCounts = app->ClipboardCounts();
        clipboardCounts.bones = 0;
        clipboardCounts.morphs = 0;
        clipboardCounts.displays = 0;
        {
            unsigned char* model = ActiveModel(app);
            clipboardCounts.bones = CountMarkedModelKeys(
                mdl::BoneKeys(model), mdl::kBoneKeyCapacity, minFrame);
            clipboardCounts.morphs = CountMarkedModelKeys(
                mdl::MorphKeys(model), 20000, minFrame);
            clipboardCounts.displays = CountMarkedModelKeys(
                mdl::DisplayKeys(model), 1000, minFrame);
        }
        const std::uint32_t boneSel = clipboardCounts.bones;
        const std::uint32_t morphSel = clipboardCounts.morphs;
        const std::uint32_t camSel = clipboardCounts.displays;
        if (boneSel == 0 && morphSel == 0 && camSel == 0) {
            EnableWindow(GetDlgItem(hwnd, panel::kPasteButton), FALSE);
            EnableWindow(GetDlgItem(hwnd, panel::kReversePasteButton), FALSE);
            EnableMenuItem(GetMenu(hwnd), 0xFA, TRUE);
            return;  
        }
        EnableWindow(GetDlgItem(hwnd, panel::kPasteButton), TRUE);
        EnableMenuItem(GetMenu(hwnd), 0xFA, FALSE);
        EnableWindow(GetDlgItem(hwnd, panel::kReversePasteButton),
                     boneSel != 0 ? TRUE : FALSE);
        if (app->BoneClipboard() != nullptr) {
            free(app->BoneClipboard());
            app->BoneClipboard() = nullptr;
        }
        if (boneSel != 0) {
            void* p = malloc(boneSel * sizeof(BoneClipboardRecord));
            if (p != nullptr) {
                ConstructArrayElements(p, sizeof(BoneClipboardRecord), boneSel,
                                       reinterpret_cast<void*>(&IdentityCtor));
            }
            app->BoneClipboard() = static_cast<BoneClipboardRecord*>(p);
            memset(p, 0, boneSel * sizeof(BoneClipboardRecord));
        }
        if (app->MorphClipboard() != nullptr) {
            free(app->MorphClipboard());
            app->MorphClipboard() = nullptr;
        }
        if (morphSel != 0) {
            void* p = malloc(morphSel * sizeof(MorphClipboardRecord));
            app->MorphClipboard() = static_cast<MorphClipboardRecord*>(p);
            memset(p, 0, morphSel * sizeof(MorphClipboardRecord));
        }
        if (app->DisplayClipboard() != nullptr) {
            free(app->DisplayClipboard());
            app->DisplayClipboard() = nullptr;
        }
        if (camSel != 0) {
            unsigned char* model = ActiveModel(app);
            void* p = malloc(camSel * sizeof(DisplayClipboardRecord));
            app->DisplayClipboard() =
                static_cast<DisplayClipboardRecord*>(p);
            memset(p, 0, camSel * sizeof(DisplayClipboardRecord));
            if (mdl::Mdl(model)->ikChainCount > 0 &&
                camSel != 0) {
                auto* records = app->DisplayClipboard();
                const std::uint32_t cnt = mdl::Mdl(model)->ikChainCount;
                for (std::uint32_t i = 0; i < camSel; ++i) {
                    records[i].ikStates = static_cast<IkClipboardState*>(
                        malloc(cnt * sizeof(IkClipboardState)));
                }
            }
            if (mdl::Mdl(model)->boneOrderCount > 0 &&
                camSel != 0) {
                auto* records = app->DisplayClipboard();
                const std::uint32_t cnt =
                    mdl::Mdl(model)->boneOrderCount;
                for (std::uint32_t i = 0; i < camSel; ++i) {
                    records[i].selectorStates =
                        static_cast<SelectorClipboardState*>(
                            malloc(cnt * sizeof(SelectorClipboardState)));
                }
            }
        }
        {
            unsigned char* model = ActiveModel(app);
            mdl::BoneKey* boneKeys = mdl::BoneKeys(model);
            const mdl::BoneRecord* bones = mdl::Mdl(model)->boneTable;
            const std::uint32_t boneCount = mdl::Mdl(model)->boneCount;
            auto* dst = app->BoneClipboard();
            std::uint32_t cnt = 0;
            for (std::uint32_t recordIndex = 0; recordIndex < static_cast<std::uint32_t>(mdl::kBoneKeyCapacity);
                 ++recordIndex) {
                const mdl::BoneKey& key = boneKeys[recordIndex];
                if (key.allocated == 0) {
                    continue;
                }
                std::uint32_t idx = recordIndex;
                while (idx >= boneCount) {
                    idx = boneKeys[idx].previous;
                }
                BoneClipboardRecord& rec = dst[cnt];
                strcpy_s(rec.name, sizeof(rec.name), bones[idx].name);
                rec.frame = key.frame - minFrame;
                std::memcpy(rec.rotation, key.rotation,
                            sizeof(rec.rotation));
                std::memcpy(rec.position, key.position,
                            sizeof(rec.position));
                rec.physicsDisabled = key.physicsDisabled;
                for (std::int32_t i = 0; i < 4; ++i) {
                    rec.interpolation[i] = key.interpolation[i];
                    rec.interpolation[4 + i] = key.interpolation[4 + i];
                    rec.interpolation[8 + i] = key.interpolation[8 + i];
                    rec.interpolation[12 + i] = key.interpolation[12 + i];
                }
                ++cnt;
            }
        }
        if (mdl::Mdl(ActiveModel(app))->morphCount > 0) {
            unsigned char* model = ActiveModel(app);
            mdl::MorphKey* morphKeys = mdl::MorphKeys(model);
            const mdl::MorphRecord* morphs = mdl::Mdl(model)->morphs;
            const std::uint32_t morphCount = mdl::Mdl(model)->morphCount;
            auto* dst = app->MorphClipboard();
            std::uint32_t cnt = 0;
            for (std::uint32_t recordIndex = 0; recordIndex < 20000;
                 ++recordIndex) {
                const mdl::MorphKey& key = morphKeys[recordIndex];
                if (key.allocated == 0) {
                    continue;
                }
                std::uint32_t idx = recordIndex;
                while (idx >= morphCount) {
                    idx = morphKeys[idx].previous;
                }
                MorphClipboardRecord& rec = dst[cnt];
                strcpy_s(rec.name, sizeof(rec.name), morphs[idx].name);
                rec.frame = key.frame - minFrame;
                rec.value = key.value;
                ++cnt;
            }
        }
        {
            unsigned char* model = ActiveModel(app);
            mdl::DisplayKey* displayKeys = mdl::DisplayKeys(model);
            const mdl::BoneRecord* bones = mdl::Mdl(model)->boneTable;
            auto* records = app->DisplayClipboard();
            const std::int32_t boneIdxCount =
                mdl::Mdl(model)->ikChainCount;
            const std::int32_t lightCount =
                mdl::Mdl(model)->boneOrderCount;
            std::uint32_t cnt = 0;
            for (std::uint32_t keyIndex = 0; keyIndex < 1000;
                 ++keyIndex) {
                const mdl::DisplayKey& key = displayKeys[keyIndex];
                if (key.allocated == 0) {
                    continue;
                }
                DisplayClipboardRecord& rec = records[cnt];
                rec.frame = key.frame - minFrame;
                rec.visible = key.visible;
                rec.ikCount = boneIdxCount;
                if (boneIdxCount > 0) {
                    const mdl::IkChain* chains =
                        mdl::Mdl(model)->ikChains;
                    const auto* pose = mdl::IkStates(key);
                    for (std::int32_t i = 0; i < boneIdxCount; ++i) {
                        const std::uint32_t boneIdx =
                            chains[i].boneIndex;
                        strcpy_s(rec.ikStates[i].boneName,
                                 sizeof(rec.ikStates[i].boneName),
                                 bones[boneIdx].name);
                        rec.ikStates[i].enabled = pose[i];
                    }
                }
                rec.selectorCount = lightCount;
                const auto* lightData = mdl::SelectorStates(key);
                strcpy_s(rec.selectorStates[0].boneName,
                         sizeof(rec.selectorStates[0].boneName),
                         kLightNameRoot);
                rec.selectorStates[0].modelIndex = lightData[0].modelIndex;
                rec.selectorStates[0].boneIndex = lightData[0].boneIndex;
                if (lightCount > 1) {
                    const auto* order = mdl::BoneOrder(model);
                    for (std::int32_t i = 1; i < lightCount; ++i) {
                        const std::uint32_t boneIdx = order[i].boneIndex;
                        strcpy_s(rec.selectorStates[i].boneName,
                                 sizeof(rec.selectorStates[i].boneName),
                                 bones[boneIdx].name);
                        rec.selectorStates[i].modelIndex =
                            lightData[i].modelIndex;
                        rec.selectorStates[i].boneIndex =
                            lightData[i].boneIndex;
                    }
                }
                ++cnt;
            }
        }
    } else {
        auto& clipboardCounts = app->ClipboardCounts();
        clipboardCounts.cameras = 0;
        clipboardCounts.lights = 0;
        clipboardCounts.shadows = 0;
        clipboardCounts.gravity = 0;
        clipboardCounts.accessories = 0;
        clipboardCounts.cameras =
            CountSelectedKeys(app->CameraKeys(), 10000, minFrame);
        clipboardCounts.lights =
            CountSelectedKeys(app->LightKeys(), 10000, minFrame);
        clipboardCounts.shadows =
            CountSelectedKeys(app->ShadowKeys(), 10000, minFrame);
        clipboardCounts.gravity =
            CountSelectedKeys(app->GravityKeys(), 10000, minFrame);
        {
            std::uint32_t accessoryCount = 0;
            for (std::size_t keyIndex = 0; keyIndex < 10000;
                 ++keyIndex) {
                for (std::int32_t slot = 0; slot < 0xFF; ++slot) {
                    auto* keys = app->AccessoryKeys(slot);
                    const mdl::AccessoryKey& key = keys[keyIndex];
                    if (key.selected != 0) {
                        ++accessoryCount;
                        if (key.frame < minFrame) {
                            minFrame = key.frame;
                        }
                    }
                }
            }
            clipboardCounts.accessories = accessoryCount;
        }
        const std::uint32_t cameraCount = clipboardCounts.cameras;
        const std::uint32_t lightCount = clipboardCounts.lights;
        const std::uint32_t shadowCount = clipboardCounts.shadows;
        const std::uint32_t gravityCount = clipboardCounts.gravity;
        const std::uint32_t accessoryCount = clipboardCounts.accessories;
        if (cameraCount == 0 && lightCount == 0 && shadowCount == 0 &&
            gravityCount == 0 && accessoryCount == 0) {
            EnableWindow(GetDlgItem(hwnd, panel::kPasteButton), FALSE);
            EnableWindow(GetDlgItem(hwnd, panel::kReversePasteButton), FALSE);
            EnableMenuItem(GetMenu(hwnd), 0xFA, TRUE);
            return;  
        }
        EnableWindow(GetDlgItem(hwnd, panel::kPasteButton), TRUE);
        EnableWindow(GetDlgItem(hwnd, panel::kReversePasteButton), FALSE);
        EnableMenuItem(GetMenu(hwnd), 0xFA, FALSE);
        ReplaceClipboardBuffer(app->CameraClipboard(), cameraCount);
        ReplaceClipboardBuffer(app->LightClipboard(), lightCount);
        ReplaceClipboardBuffer(app->ShadowClipboard(), shadowCount);
        ReplaceClipboardBuffer(app->GravityClipboard(), gravityCount);
        ReplaceClipboardBuffer(app->AccessoryClipboard(), accessoryCount);
        {
            auto* keys = app->CameraKeys();
            auto* dst = app->CameraClipboard();
            std::uint32_t cnt = 0;
            for (std::size_t i = 0; i < 10000; ++i) {
                const mdl::CameraKey& key = keys[i];
                if (key.selected == 0) {
                    continue;
                }
                CameraClipboardRecord& rec = dst[cnt];
                rec.frame = key.frame - minFrame;
                std::memcpy(rec.eye, key.eye, sizeof(rec.eye));
                std::memcpy(rec.target, key.target, sizeof(rec.target));
                rec.fov = key.fov;
                rec.perspective = key.perspective;
                std::memcpy(rec.interpolation, key.interpolation,
                            sizeof(rec.interpolation));
                rec.distance = key.distance;
                rec.parentModel = key.parentModel;
                rec.parentBone = key.parentBone;
                ++cnt;
            }
        }
        {
            auto* keys = app->LightKeys();
            auto* dst = app->LightClipboard();
            std::uint32_t cnt = 0;
            for (std::size_t i = 0; i < 10000; ++i) {
                const mdl::LightKey& key = keys[i];
                if (key.selected == 0) {
                    continue;
                }
                LightClipboardRecord& rec = dst[cnt];
                rec.frame = key.frame - minFrame;
                std::memcpy(rec.direction, key.direction,
                            sizeof(rec.direction));
                std::memcpy(rec.color, key.color, sizeof(rec.color));
                ++cnt;
            }
        }
        {
            auto* keys = app->ShadowKeys();
            auto* dst = app->ShadowClipboard();
            std::uint32_t cnt = 0;
            for (std::size_t i = 0; i < 10000; ++i) {
                const mdl::SelfShadowKey& key = keys[i];
                if (key.selected == 0) {
                    continue;
                }
                ShadowClipboardRecord& rec = dst[cnt];
                rec.frame = key.frame - minFrame;
                rec.mode = key.mode;
                rec.distance = key.distance;
                ++cnt;
            }
        }
        {
            auto* keys = app->GravityKeys();
            auto* dst = app->GravityClipboard();
            std::uint32_t cnt = 0;
            for (std::size_t i = 0; i < 10000; ++i) {
                const mdl::GravityKey& key = keys[i];
                if (key.selected == 0) {
                    continue;
                }
                GravityClipboardRecord& rec = dst[cnt];
                rec.frame = key.frame - minFrame;
                rec.acceleration = key.acceleration;
                std::memcpy(rec.direction, key.direction,
                            sizeof(rec.direction));
                rec.noise = key.noise;
                rec.noiseEnabled = key.noiseEnabled;
                ++cnt;
            }
        }
        {
            auto* dst = app->AccessoryClipboard();
            std::uint32_t cnt = 0;
            for (std::size_t keyIndex = 0; keyIndex < 10000;
                 ++keyIndex) {
                for (std::int32_t slot = 0; slot < 0xFF; ++slot) {
                    auto* keys = app->AccessoryKeys(slot);
                    const mdl::AccessoryKey& key = keys[keyIndex];
                    if (key.selected == 0) {
                        continue;
                    }
                    mdl::AccessoryClipboardKey& rec = dst[cnt];
                    rec.frameOffset = key.frame - minFrame;
                    rec.slot = static_cast<std::uint8_t>(slot);
                    rec.visible = key.visible;
                    rec.shadowEnabled = key.shadowEnabled;
                    rec.parentModel = key.parentModel;
                    rec.parentBone = key.parentBone;
                    std::memcpy(rec.position, key.position,
                                sizeof(rec.position));
                    std::memcpy(rec.rotation, key.rotation,
                                sizeof(rec.rotation));
                    rec.scale = key.scale;
                    rec.opacity = key.opacity;
                    ++cnt;
                }
            }
        }
    }
}
static void Cmd400_FramePaste(MMDApp* app, HWND hwnd) {
    if (app->state.optflag[0] == 0) {
        const auto& clipboardCounts = app->ClipboardCounts();
        const std::uint32_t boneSel = clipboardCounts.bones;
        const std::uint32_t morphSel = clipboardCounts.morphs;
        const std::uint32_t camSel = clipboardCounts.displays;
        TraceModelPaste("enter bone=%u morph=%u display=%u", boneSel,
                        morphSel, camSel);
        if (boneSel == 0 && morphSel == 0 && camSel == 0) {
            return;  
        }
        app->SceneModified() = 1;
        const std::int32_t frame =
            app->state.currentFrame;
        unsigned char* model = ActiveModel(app);
        TraceModelPaste("active model=%p frame=%d", model, frame);
        ClearModelKeyMarks(mdl::BoneKeys(model), mdl::kBoneKeyCapacity);
        ClearModelKeyMarks(mdl::MorphKeys(model), 20000);
        ClearModelKeyMarks(mdl::DisplayKeys(model), 1000);
        if (boneSel != 0) {
            EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);
            EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);
            mdl::Mdl(model)->undoDirty = 1;
            mdl::Mdl(model)->redoDirty = 0;
            mdl::ModelRecord& record = *mdl::Mdl(model);
            const std::int32_t next = record.undoState[0] + 1;
            record.undoState[0] = next >= 0x1E ? 0 : next;
            const std::int32_t cur = record.undoState[0];
            record.undoState[1] = cur;
            auto& undo = rinmotiondance::mdl::Mdl(model)->undoRings[0].slots[cur];
            undo.operation = 2;
            undo.frame = frame;
            void* p = undo.bonePose;
            if (p != nullptr) {
                ::operator delete(p);
                undo.bonePose = nullptr;
            }
            const std::int32_t boneCount = mdl::Mdl(model)->boneCount;
            auto* undoBone = static_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
                ::operator new(static_cast<std::uint32_t>(boneCount) *
                       sizeof(rinmotiondance::mdl::BonePoseSnapshot)));
            if (undoBone != nullptr) {
                ConstructArrayElements(undoBone,
                                       sizeof(rinmotiondance::mdl::BonePoseSnapshot),
                                       static_cast<std::uint32_t>(boneCount),
                                       reinterpret_cast<void*>(&IdentityCtor));
            }
            undo.bonePose = undoBone;
            memset(undoBone, 0,
                   static_cast<std::uint32_t>(boneCount) *
                       sizeof(rinmotiondance::mdl::BonePoseSnapshot));
            if (boneCount > 0) {
                const mdl::ModelRecord& record = *mdl::Mdl(model);
                const mdl::BoneRecord* bones = record.boneTable;
                unsigned char* boneState = record.bonePhysicsState;
                for (std::int32_t i = 0; i < boneCount; ++i) {
                    auto& rec = undoBone[i];
                    const auto& src = bones[i];
                    rec.boneIndex = i;
                    memcpy(rec.position, src.trans, sizeof rec.position);
                    memcpy(rec.rotation, src.rotQuat, sizeof rec.rotation);
                    rec.physicsDisabled = boneState[i];
                }
            }
            undo.dirty = 0;
            p = undo.auxiliaryPose;
            if (p != nullptr) {
                ::operator delete(p);
                undo.auxiliaryPose = nullptr;
            }
            unsigned char* undoLight = static_cast<unsigned char*>(
                ::operator new(boneSel * 3 * 0x40));
            if (undoLight != nullptr) {
                ConstructArrayElements(undoLight, 0x40, boneSel * 3,
                                       reinterpret_cast<void*>(&IdentityCtor));
            }
            undo.auxiliaryPose = undoLight;
            memset(undoLight, 0, boneSel * 3 * 0x40);
        }
        std::memset(mdl::Mdl(model)->keyVisitMap, 0,
                    sizeof(mdl::Mdl(model)->keyVisitMap));
        ResetBoneKeyCursor(model);
        TraceModelPaste("undo-ready free=%d",
                        mdl::Mdl(model)->searchCursor);
        if (boneSel != 0) {
            const auto* src = app->BoneClipboard();
            std::uint32_t i = 0;
            do {
                BoneClipboardRecord rec = src[i];
                if (RegisterBoneKey(model,
                              reinterpret_cast<unsigned char*>(&rec),
                              frame, 0) == 0) {
                    break;
                }
                ++i;
            } while (i < boneSel);
        }
        ResetMorphKeyCursor(model);
        TraceModelPaste("bone-done free=%d",
                        mdl::Mdl(model)->searchCursor);
        if (morphSel != 0) {
            const auto* src = app->MorphClipboard();
            std::uint32_t i = 0;
            do {
                const MorphClipboardRecord rec = src[i];
                if (RegisterMorphKeyFromRecord(model,
                              reinterpret_cast<const unsigned char*>(&rec),
                              frame) == 0) {
                    break;
                }
                ++i;
            } while (i < morphSel);
        }
        ResetDisplayKeyCursor(model);
        TraceModelPaste("morph-done display-free=%d",
                        mdl::Mdl(model)->searchCursor);
        if (camSel != 0) {
            auto* records = app->DisplayClipboard();
            std::uint32_t i = 0;
            do {
                DisplayClipboardRecord& rec = records[i];
                const std::int32_t lightCount = rec.selectorCount;
                TraceModelPaste(
                    "display[%u] rec=%p rel=%d relptr=%p ik=%d ikptr=%p",
                    i, &rec, lightCount, rec.selectorStates,
                    rec.ikCount, rec.ikStates);
                if (lightCount > 0) {
                    for (std::int32_t j = 0; j < lightCount; ++j) {
                        SelectorClipboardState& state =
                            rec.selectorStates[j];
                        const std::int32_t slotIdx = state.modelIndex;
                        TraceModelPaste(
                            "display[%u] relation[%d] slot=%d bone=%d",
                            i, j, slotIdx, state.boneIndex);
                        if (slotIdx < 0) {
                            continue;
                        }
                        unsigned char* slotModel =
                            app->ModelSlot(slotIdx);
                        if (slotModel == nullptr) {
                            state.modelIndex = -1;
                            state.boneIndex = 0;
                        } else if (static_cast<std::int32_t>(
                                       mdl::Mdl(slotModel)->boneCount) <
                                   state.boneIndex) {
                            state.boneIndex = 0;
                        }
                    }
                }
                TraceModelPaste("display[%u] fixup-done", i);
                if (RegisterDisplayKeyFromRecord(model,
                              rec.frame, rec.visible, rec.ikCount,
                              reinterpret_cast<unsigned char*>(rec.ikStates),
                              rec.selectorCount,
                              reinterpret_cast<unsigned char*>(
                                  rec.selectorStates),
                              frame) == 0) {
                    break;
                }
                TraceModelPaste("display[%u] registrar-done", i);
                ++i;
            } while (i < camSel);
        }
        app->PhysicsResetPending() = 1;
        TraceModelPaste("model-paste loops done");
    } else {
        const auto& clipboardCounts = app->ClipboardCounts();
        const std::uint32_t cameraCount = clipboardCounts.cameras;
        const std::uint32_t lightCount = clipboardCounts.lights;
        const std::uint32_t shadowCount = clipboardCounts.shadows;
        const std::uint32_t gravityCount = clipboardCounts.gravity;
        const std::uint32_t accessoryCount = clipboardCounts.accessories;
        if (cameraCount == 0 && lightCount == 0 && shadowCount == 0 &&
            gravityCount == 0 && accessoryCount == 0) {
            return;  
        }
        app->SceneModified() = 1;
        ClearSelectedKeys(app->CameraKeys(), 10000);
        ClearSelectedKeys(app->LightKeys(), 10000);
        ClearSelectedKeys(app->ShadowKeys(), 10000);
        ClearSelectedKeys(app->GravityKeys(), 10000);
        for (std::int32_t slot = 0; slot < 0xFF; ++slot) {
            auto* keys = app->AccessoryKeys(slot);
            ClearSelectedKeys(keys, 10000);
        }
        const std::int32_t frame =
            app->state.currentFrame;
        if (cameraCount != 0) {
            const auto* src = app->CameraClipboard();
            std::uint32_t i = 0;
            do {
                const CameraClipboardRecord rec = src[i];
                if (RegisterCameraKey(app, &rec) == 0) {
                    break;
                }
                ++i;
            } while (i < cameraCount);
        }
        if (lightCount != 0) {
            const auto* src = app->LightClipboard();
            std::uint32_t i = 0;
            do {
                const LightClipboardRecord rec = src[i];
                if (RegisterLightKey(app, &rec) == 0) {
                    break;
                }
                ++i;
            } while (i < lightCount);
        }
        if (shadowCount != 0) {
            const auto* src = app->ShadowClipboard();
            std::uint32_t i = 0;
            do {
                const ShadowClipboardRecord rec = src[i];
                if (RegisterSelfShadowKey(app, &rec) == 0) {
                    break;
                }
                ++i;
            } while (i < shadowCount);
        }
        if (gravityCount != 0) {
            const auto* src = app->GravityClipboard();
            std::uint32_t i = 0;
            do {
                const GravityClipboardRecord rec = src[i];
                if (RegisterGravityKey(app, &rec) == 0) {
                    break;
                }
                ++i;
            } while (i < gravityCount);
        }
        if (accessoryCount != 0) {
            auto* src = app->AccessoryClipboard();
            std::uint32_t i = 0;
            do {
                mdl::AccessoryClipboardKey rec = src[i];
                if (PasteAccessoryKeyRecord(app, &rec, 0) == 0) {
                    break;
                }
                ++i;
            } while (i < accessoryCount);
        }
        app->PhysicsResetPending() = 1;
    }
    PanelPaint(app);
    SelectionReeval(app);
    unsigned char* model = ActiveModel(app);
    if (model != nullptr) {
        SeekModelFrame(model, app->state.currentFrame,
                  app->PlaybackPhysicsMode());
    }
    if (app->state.optflag[0] != 0) {
        ReloadModels(app);
        RefreshLightPanel(app);
        RefreshSelfShadowPanel(app);
        ApplyGravityTrack(app);
        for (std::int32_t i = 0; i < 0xFF; ++i) {
            if (app->AccessorySlot(i) != nullptr) {
                ApplyAccessoryTrack(app, i);
            }
        }
        SyncAccessoryEditPanel(app);
    }
}
static void Cmd400_FrameReversePaste(MMDApp* app, HWND hwnd) {
    if (app->state.optflag[0] != 0) {
        return;  
    }
    const std::uint32_t boneSel = app->ClipboardCounts().bones;
    if (boneSel == 0) {
        return;  
    }
    app->SceneModified() = 1;
    const std::int32_t frame =
        app->state.currentFrame;
    unsigned char* model = ActiveModel(app);
    ClearModelKeyMarks(mdl::BoneKeys(model), mdl::kBoneKeyCapacity);
    ClearModelKeyMarks(mdl::MorphKeys(model), 20000);
    ClearModelKeyMarks(mdl::DisplayKeys(model), 1000);
    EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);
    EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);
    mdl::Mdl(model)->undoDirty = 1;
    mdl::Mdl(model)->redoDirty = 0;
    mdl::ModelRecord& record = *mdl::Mdl(model);
    const std::int32_t next = record.undoState[0] + 1;
    record.undoState[0] = next >= 0x1E ? 0 : next;
    const std::int32_t cur = record.undoState[0];
    record.undoState[1] = cur;
    auto& undo = rinmotiondance::mdl::Mdl(model)->undoRings[0].slots[cur];
    undo.operation = 2;
    undo.frame = frame;
    void* p = undo.bonePose;
    if (p != nullptr) {
        ::operator delete(p);
        undo.bonePose = nullptr;
    }
    const std::int32_t boneCount = mdl::Mdl(model)->boneCount;
    auto* undoBone = static_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
        ::operator new(static_cast<std::uint32_t>(boneCount) *
                       sizeof(rinmotiondance::mdl::BonePoseSnapshot)));
    if (undoBone != nullptr) {
        ConstructArrayElements(undoBone,
                               sizeof(rinmotiondance::mdl::BonePoseSnapshot),
                               static_cast<std::uint32_t>(boneCount),
                               reinterpret_cast<void*>(&IdentityCtor));
    }
    undo.bonePose = undoBone;
    memset(undoBone, 0, static_cast<std::uint32_t>(boneCount) *
                            sizeof(rinmotiondance::mdl::BonePoseSnapshot));
    if (boneCount > 0) {
        const mdl::BoneRecord* bones = record.boneTable;
        unsigned char* boneState = record.bonePhysicsState;
        for (std::int32_t i = 0; i < boneCount; ++i) {
            auto& rec = undoBone[i];
            const auto& src = bones[i];
            rec.boneIndex = i;
            memcpy(rec.position, src.trans, sizeof rec.position);
            memcpy(rec.rotation, src.rotQuat, sizeof rec.rotation);
            rec.physicsDisabled = boneState[i];
        }
    }
    undo.dirty = 0;
    p = undo.auxiliaryPose;
    if (p != nullptr) {
        ::operator delete(p);
        undo.auxiliaryPose = nullptr;
    }
    unsigned char* undoLight =
        static_cast<unsigned char*>(::operator new(boneSel * 3 * 0x40));
    if (undoLight != nullptr) {
        ConstructArrayElements(undoLight, 0x40, boneSel * 3,
                               reinterpret_cast<void*>(&IdentityCtor));
    }
    undo.auxiliaryPose = undoLight;
    memset(undoLight, 0, boneSel * 3 * 0x40);
    std::memset(record.keyVisitMap, 0, sizeof(record.keyVisitMap));
    ResetBoneKeyCursor(model);
    if (boneSel != 0) {
        const auto* src = app->BoneClipboard();
        std::uint32_t i = 0;
        do {
            BoneClipboardRecord rec = src[i];
            if (RegisterMirroredBoneKey(model,
                          reinterpret_cast<unsigned char*>(&rec),
                          frame) == 0) {
                break;
            }
            ++i;
        } while (i < boneSel);
    }
    PanelPaint(app);
    SelectionReeval(app);
    SeekModelFrame(model, frame,
              app->PlaybackPhysicsMode());
    app->PhysicsResetPending() = 1;
}
static void Cmd400_CurveCopyFromFrame(MMDApp* app, HWND hwnd) {
    const LRESULT sel =
        SendMessageA(GetDlgItem(hwnd, panel::kInterpCurveCombo), CB_GETCURSEL, 0, 0);
    if (app->state.optflag[0] != 0) {
        const std::int32_t idx = FirstSelectedFrame374(app);
        if (idx < 0) {
            SetFocus(hwnd);
            return;  
        }
        EnableWindow(GetDlgItem(hwnd, panel::kCurvePasteButton), TRUE);
        auto* keys = app->CameraKeys();
        for (std::int32_t i = 0; i < 6; ++i) {
            const std::int32_t row = (sel < 6) ? sel : i;
            ReadCameraCurveChannel(app, keys[idx], row, i);
        }
        SetFocus(hwnd);
    } else {
        unsigned char* model = ActiveModel(app);
        const std::int32_t idx = FirstSelectedBoneKey(model);
        if (idx < 0) {
            SetFocus(hwnd);
            return;  
        }
        EnableWindow(GetDlgItem(hwnd, panel::kCurvePasteButton), TRUE);
        mdl::BoneKey* keys = mdl::BoneKeys(model);
        if (sel < 4) {
            for (std::int32_t i = 0; i < 6; ++i) {
                ReadBoneCurveChannel(app, keys[idx],
                                     static_cast<std::size_t>(sel), i);
            }
        } else {
            for (std::int32_t i = 0; i < 4; ++i) {
                ReadBoneCurveChannel(app, keys[idx], i, i);
            }
        }
        SetFocus(hwnd);
    }
}
static void Cmd400_CurvePasteToFrames(MMDApp* app, HWND hwnd) {
    app->SceneModified() = 1;
    const LRESULT sel =
        SendMessageA(GetDlgItem(hwnd, panel::kInterpCurveCombo), CB_GETCURSEL, 0, 0);
    if (app->state.optflag[0] != 0) {
        auto* keys = app->CameraKeys();
        for (std::int32_t i = 0; i < 10000; ++i) {
            mdl::CameraKey& key = keys[i];
            if (key.selected == 0) {
                continue;
            }
            if (sel < 6) {
                WriteCameraCurveChannel(
                    app, key, static_cast<std::size_t>(sel), sel);
            } else {
                for (std::int32_t channel = 0; channel < 6; ++channel) {
                    WriteCameraCurveChannel(app, key, channel, channel);
                }
            }
        }
        SelectionReeval(app);
        SetFocus(hwnd);
    } else {
        unsigned char* model = ActiveModel(app);
        SnapshotSelectedKeysForUndo(model, app->state.currentFrame);
        mdl::BoneKey* keys = mdl::BoneKeys(model);
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(mdl::kBoneKeyCapacity); ++i) {
            mdl::BoneKey& key = keys[i];
            if (key.allocated == 0) {
                continue;
            }
            if (sel < 4) {
                WriteBoneCurveChannel(app, key,
                                      static_cast<std::size_t>(sel), sel);
            } else {
                for (std::int32_t channel = 0; channel < 4; ++channel) {
                    WriteBoneCurveChannel(app, key, channel, channel);
                }
            }
        }
        SelectionReeval(app);
        SetFocus(hwnd);
    }
}
static void Cmd400_CurveResetRows(MMDApp* app, HWND hwnd) {
    app->SceneModified() = 1;
    const LRESULT sel =
        SendMessageA(GetDlgItem(hwnd, panel::kInterpCurveCombo), CB_GETCURSEL, 0, 0);
    if (app->state.optflag[0] != 0) {
        auto* keys = app->CameraKeys();
        for (std::int32_t i = 0; i < 10000; ++i) {
            mdl::CameraKey& key = keys[i];
            if (key.selected == 0) {
                continue;
            }
            if (sel < 6) {
                ResetCameraCurveChannel(key,
                                        static_cast<std::size_t>(sel));
            } else {
                for (std::int32_t channel = 0; channel < 6; ++channel) {
                    ResetCameraCurveChannel(key, channel);
                }
            }
        }
        SelectionReeval(app);
        SetFocus(hwnd);
    } else {
        unsigned char* model = ActiveModel(app);
        SnapshotSelectedKeysForUndo(model, app->state.currentFrame);
        mdl::BoneKey* keys = mdl::BoneKeys(model);
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(mdl::kBoneKeyCapacity); ++i) {
            mdl::BoneKey& key = keys[i];
            if (key.allocated == 0) {
                continue;
            }
            if (sel < 4) {
                ResetBoneCurveChannel(key, static_cast<std::size_t>(sel));
            } else {
                for (std::int32_t channel = 0; channel < 4; ++channel) {
                    ResetBoneCurveChannel(key, channel);
                }
            }
        }
        SelectionReeval(app);
        SetFocus(hwnd);
    }
}
static void Cmd400_LoadModel(MMDApp* app, HWND hwnd) {
    SetCurrentDirectoryW(app->ExeDir());
    app->state.enterKeyState = 1;
    wchar_t fileBuf[0x100];
    memset(fileBuf, 0, sizeof(fileBuf));
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app->state.floatingWindow != 0
                        ? reinterpret_cast<HWND>(
                              app->state.floatingWindow)
                        : hwnd;
    ofn.lpstrFilter = kFilterModel;
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = 0x100;
    ofn.Flags = OFN_FILEMUSTEXIST;
    ofn.lpstrInitialDir =
        (GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0
            ? app->DirModel()
            : kInitDirModel;
    ofn.lpstrDefExt = kDefExtPmd;
    ofn.nMaxFileTitle = 0x100;
    ofn.lpstrFileTitle = nullptr;
    ofn.lpstrTitle = app->state.englishUI != 0
                         ? kTitleLoadModel
                         : kTitleOpenFileJp;
    if (GetOpenFileNameW(&ofn) != 0) {
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            wchar_t* dir = ExtractDirFromPath(
                app->PathWorkspace().projectDirectory,
                fileBuf);
            CopyDirPathW(app->DirModel(), dir);
        }
        LoadModelFile(app, fileBuf);
    }
    RefreshRequest(-1);
}
static void Cmd400_DeleteModel(MMDApp* app, HWND hwnd) {
    if (app->FrameRangeDialog() != nullptr) {
        return;  
    }
    const LRESULT sel =
        SendMessageA(GetDlgItem(hwnd, panel::kMainComboModel), CB_GETCURSEL, 0, 0);
    app->state.enterKeyState = 1;
    std::int32_t found = -1;
    for (std::int32_t i = 0; i < kModelSlotCount; ++i) {
        unsigned char* m = app->ModelSlot(i);
        if (m != nullptr && mdl::Mdl(m)->comboSelIndex == sel) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        return;  
    }
    app->state.enterKeyState = 1;
    unsigned char* model = app->ModelSlot(found);
    const mdl::ModelRecord& record = *mdl::Mdl(model);
    char text[0x100];
    if (app->state.englishUI != 0) {
        sprintf_s(text, 0x100, kMsgDelModelEn, record.nameEn);
    } else {
        sprintf_s(text, 0x100, kMsgDelModelJp, record.name);
    }
    const std::uint32_t flags =
        app->state.floatingWindow != 0 ? (MB_OKCANCEL | MB_TOPMOST) : MB_OKCANCEL;
    if (MessageBoxA(hwnd, text, kCaptionDelModelJp, flags) != 1) {
        return;  
    }
    if (app->state.frameCopyDialog != 0) {
        SeekSelectedModelToCurrentFrame(app);
        if (app->state.rigidScratchArray != nullptr) {
            free(app->state.rigidScratchArray);
            app->state.rigidScratchArray = nullptr;
        }
        if (app->state.jointScratchArray != nullptr) {
            free(app->state.jointScratchArray);
            app->state.jointScratchArray = nullptr;
        }
        DestroyWindow(reinterpret_cast<HWND>(
            app->state.frameCopyDialog));
        app->state.frameCopyDialog = nullptr;
        EnableWindow(GetDlgItem(hwnd, panel::kMainComboModel), TRUE);
        EnableWindow(GetDlgItem(hwnd, panel::kPlayButton), TRUE);
    }
    const std::int32_t oldSel = record.comboSelIndex;
    const std::int32_t oldIdx = record.comboSelIndex2;
    if (model != nullptr) {
        DeleteModel(model, 1);  
    }
    app->ModelSlot(found) = nullptr;
    app->SceneModified() = 1;
    app->state.mainModelComboSelection = 0;
    for (std::int32_t i = 0; i < kModelSlotCount; ++i) {
        if (app->ModelSlot(i) != nullptr) {
            app->state.mainModelComboSelection = 1;
        }
    }
    SendMessageA(GetDlgItem(hwnd, panel::kMainComboModel), CB_DELETESTRING,
                 static_cast<WPARAM>(sel), 0);
    SendMessageA(GetDlgItem(hwnd, panel::kMainComboGround), CB_DELETESTRING,
                 static_cast<WPARAM>(sel), 0);
    SendMessageA(GetDlgItem(hwnd, panel::kMainComboNormal), CB_DELETESTRING,
                 static_cast<WPARAM>(sel), 0);
    SendMessageA(GetDlgItem(hwnd, panel::kMainComboModel), CB_SETCURSEL, 0, 0);
    const LRESULT accCount =
        SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo), CB_GETCOUNT, 0, 0);
    for (std::int32_t i = 0; i < accCount; ++i) {
        char buf[0x100];
        SendMessageA(GetDlgItem(hwnd, panel::kAccessoryCombo), CB_GETLBTEXT,
                     static_cast<WPARAM>(i),
                     reinterpret_cast<LPARAM>(buf));
        SendMessageA(GetDlgItem(hwnd, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(buf));
    }
    EnableMenuItem(GetMenu(hwnd), 0x120, 1);
    EnableMenuItem(GetMenu(hwnd), 0x121, 1);
    for (std::int32_t j = 0; j < kModelSlotCount; ++j) {
        unsigned char* m = app->ModelSlot(j);
        if (m == nullptr) {
            continue;
        }
        EnableMenuItem(GetMenu(hwnd), 0x120, 0);
        EnableMenuItem(GetMenu(hwnd), 0x121, 0);
        if (mdl::Mdl(m)->comboSelIndex > oldSel) {
            --mdl::Mdl(m)->comboSelIndex;
        }
        if (mdl::Mdl(m)->comboSelIndex2 > oldIdx) {
            --mdl::Mdl(m)->comboSelIndex2;
        }
        mdl::ModelRecord& record = *mdl::Mdl(m);
        mdl::DetachBoneBindings(record.boneOrderTable, record.boneOrderCount,
                                found);
        mdl::DisplayKey* displayKeys = mdl::DisplayKeys(m);
        for (std::int32_t keyIndex = 0; keyIndex < 1000; ++keyIndex) {
            auto* states = mdl::SelectorStates(displayKeys[keyIndex]);
            const std::int32_t cnt = record.boneOrderCount;
            for (std::int32_t k = 0; k < cnt; ++k) {
                if (states[k].modelIndex == found) {
                    states[k].modelIndex = -1;
                    states[k].boneIndex = 0;
                }
            }
        }
    }
    if (app->CameraParentModel() == found) {
        app->CameraParentModel() = -1;
        app->CameraParentBone() = 0;
        SendMessageA(GetDlgItem(hwnd, panel::kMainComboNormal), CB_SETCURSEL, 0, 0);
        SendMessageA(GetDlgItem(hwnd, panel::kBoneRegisterCombo), CB_RESETCONTENT, 0,
                     0);
    }
    {
        auto* keys = app->CameraKeys();
        for (std::int32_t i = 0; i < 10000; ++i) {
            if (keys[i].parentModel == found) {
                keys[i].parentModel = -1;
                keys[i].parentBone = 0;
            }
        }
    }
    for (std::int32_t i = 0; i < 0xFF; ++i) {
        mdl::AccessoryRecord* acc = app->AccessorySlot(i);
        if (acc == nullptr) {
            continue;
        }
        if (acc->parentModel == found) {
            acc->parentModel = -1;
            acc->parentBone = 0;
        }
        auto* keys = app->AccessoryKeys(i);
        if (keys != nullptr) {
            for (std::int32_t keyIndex = 0; keyIndex < 10000;
                 ++keyIndex) {
                mdl::AccessoryKey& key = keys[keyIndex];
                if (key.parentModel == found) {
                    key.parentModel = -1;
                    key.visible = 0;
                }
            }
        }
    }
    ApplyModelComboSelection(app);
}
static void Cmd400_AccessoryEditDialog(MMDApp* app, HWND hwnd) {
    app->state.enterKeyState = 1;
    app->state.accessoryEditDialogOpen = 1;
    const std::intptr_t result =
        DialogBoxParamA(static_cast<HINSTANCE>(app->HInstance()),
                        app->state.englishUI != 0
                            ? reinterpret_cast<LPCSTR>(0x329)
                            : reinterpret_cast<LPCSTR>(0x328),
                        hwnd, &SelectNavDlgProc, 0);
    if (result == 2) {
        return;  
    }
    app->SceneModified() = 1;
    app->state.accessoryEditDialogOpen = 0;
    if (app->DialogOrders().modelIndices != nullptr) {
        app->DialogOrders().modelIndices.reset();
    }
    if (app->AccessoryEditArray() != nullptr) {
        free(app->AccessoryEditArray());
        app->AccessoryEditArray() = nullptr;
    }
    if (app->state.selectNavRecords != nullptr) {
        delete[] app->state.selectNavRecords;
        app->state.selectNavRecords = nullptr;
    }
    if (app->AccessoryApplyGate() == 0) {
        return;  
    }
    unsigned char* model = ActiveModel(app);
    SeekModelFrame(model,
              app->state.currentFrame,
              app->PlaybackPhysicsMode());
    PostLanguageSweep(app);
    SelectionReeval(app);
}

void CmdControl400(MMDApp* app, HWND hwnd, std::uint16_t id,
                   std::uint16_t notify) {
    switch (id) {
    case 400:  
        Cmd400_Undo(app, hwnd);
        break;

    case 401:  
        Cmd400_Redo(app, hwnd);
        break;

    case 402:
        ResetViewVariant(app, 0.0f, true, false);
        break;
    case 403:
        ResetViewVariant(app, 3.1415927f, true, false);  
        break;
    case 404:
        ResetViewVariant(app, -1.5707964f, false, true);  
        break;
    case 405:
        ResetViewVariant(app, 1.5707964f, true, false);  
        break;
    case 406:
        ResetViewVariant(app, -1.5707964f, true, false);  
        break;

    case 407:  
        Cmd400_ViewResetReload(app);
        break;

    case 408:  
        Cmd400_PlayRangeEdits(app, hwnd);
        break;

    case 411:
        app->PlaybackLoopEnabled() =
            app->PlaybackLoopEnabled() == 0 ? 1 : 0;
        break;
    case 413:
        app->state.playbackReturnsToStartFrame =
            app->state.playbackReturnsToStartFrame == 0 ? 1 : 0;
        break;
    case 414:
        app->PlaybackStartsAtCurrentFrame() =
            app->PlaybackStartsAtCurrentFrame() == 0 ? 1 : 0;
        break;

    case 412: {
        const LRESULT checked =
            SendMessageA(GetDlgItem(hwnd, panel::kCameraRefModelCheckbox), BM_GETCHECK, 0, 0);
        const int prev = static_cast<int>(app->CameraReferenceMode());
        if (checked == 1) {
            app->CameraReferenceMode() =
                CameraAttachmentReference::ModelRoot;
            SendMessageA(GetDlgItem(hwnd, panel::kCameraRefBoneCheckbox), BM_SETCHECK, 0, 0);
            if (app->state.optflag[0] != 0) {
                break;
            }
            ApplyCameraReferenceModeChange(app, prev);
        } else {
            app->CameraReferenceMode() = CameraAttachmentReference::None;
            if (app->state.optflag[0] != 0) {
                break;
            }
            ApplyCameraReferenceModeChange(app, prev);
        }
        break;
    }

    case 418:
        StepFrame(app, false);
        break;
    case 419:
        StepFrame(app, true);
        break;

    case 429: {
        const std::int32_t sidebar =
            app->SidebarWidth();
        const std::int32_t count = (sidebar - 0x54) / 26;
        const std::int32_t frame =
            app->state.currentFrame;
        if (frame > count) {
            app->state.timelineStartFrame = frame - count;
        } else {
            app->state.timelineStartFrame = 0;
        }
        PanelPaint(app);
        if (app->state.waveEnabled != 0) {
            TimelineDrawTicks(app->state.timelineStartFrame,
                              sidebar);
            RECT rc;
            rc.left = 6;
            rc.top = 0x5F;
            rc.right = sidebar - 3;
            rc.bottom = 0x92;
            InvalidateRect(hwnd, &rc, FALSE);
        }
        break;
    }

    case 430:  
        Cmd400_CurveCopyFromFrame(app, hwnd);
        break;

    case 431:  
        Cmd400_CurvePasteToFrames(app, hwnd);
        break;

    case 432:  
        Cmd400_CurveResetRows(app, hwnd);
        break;

    case 435:  
        Cmd400_LoadModel(app, hwnd);
        break;

    case 436:
        if (notify == CBN_SELCHANGE &&
            app->FrameRangeDialog() == nullptr) {
            ApplyModelComboSelection(app);
        }
        break;

    case 437:  
        Cmd400_DeleteModel(app, hwnd);
        break;

    case 438: {
        app->SceneModified() = 1;
        unsigned char* model = ActiveModel(app);
        ClearModelKeyMarks(mdl::BoneKeys(model), mdl::kBoneKeyCapacity);
        ClearModelKeyMarks(mdl::MorphKeys(model), 20000);
        ClearModelKeyMarks(mdl::DisplayKeys(model), 1000);
        RegisterDisplayKeyCurrent(model, app->state.currentFrame);
        const std::int32_t v =
            static_cast<std::int32_t>(mdl::Mdl(model)->maxFrame);
        if (app->state.lastRegisteredFrame < v) {
            app->state.lastRegisteredFrame = v;
        }
        PanelPaint(app);
        break;
    }

    case 439: {
        mdl::Mdl(ActiveModel(app))->loadComplete =
            IsDlgButtonChecked(hwnd, panel::kModelVisibleCheckbox) == 1 ? 1
                                                                        : 0;
        break;
    }

    case 440: {
        unsigned char* model = ActiveModel(app);
        if (model != nullptr) {
            mdl::ModelRecord& record = *mdl::Mdl(model);
            record.toonFlag = record.toonFlag == 0 ? 1 : 0;
        }
        break;
    }
    case 441: {
        unsigned char* model = ActiveModel(app);
        if (model != nullptr) {
            mdl::ModelRecord& record = *mdl::Mdl(model);
            record.postLoadFlag2 = record.postLoadFlag2 == 0 ? 1 : 0;
        }
        break;
    }

    case 444: {
        const LRESULT sel =
            SendMessageA(GetDlgItem(hwnd, panel::kIkChainCombo), CB_GETCURSEL, 0, 0);
        unsigned char* model = ActiveModel(app);
        mdl::IkChain* chains = mdl::Mdl(model)->ikChains;
        if (chains != nullptr) {
            chains[sel].enabled = 1;
        }
        break;
    }
    case 445: {
        const LRESULT sel =
            SendMessageA(GetDlgItem(hwnd, panel::kIkChainCombo), CB_GETCURSEL, 0, 0);
        unsigned char* model = ActiveModel(app);
        mdl::IkChain* chains = mdl::Mdl(model)->ikChains;
        if (chains != nullptr) {
            chains[sel].enabled = 0;
        }
        break;
    }

    case 446: {
        if (app->CameraPerspective() != 0) {
            app->CameraPerspective() = 0;
            d3dx::D3DXMATRIXF mat{};
            auto* d3dx = &d3dx::Get();
            D3DRenderer* locale = app->Renderer();
            const float fov = static_cast<float>(
                static_cast<double>(app->CameraFov()) *
                0.0174532925199433);  
            const float aspect = locale->aspectRatio;  
            {
                d3dx->perspectiveFovLH(&mat, fov, aspect, 1.0f, 100000.0f);
            }
            IDirect3DDevice9* device = locale->device;
            device->SetTransform(
                D3DTS_PROJECTION,
                reinterpret_cast<const D3DMATRIX*>(&mat));
            RefreshRequest(-1);
        } else {
            app->CameraPerspective() = 1;
            RefreshRequest(-1);
        }
        break;
    }

    case 423:
        DeleteMarkedKeyframes(app);
        app->SceneModified() = 1;
        break;

    case 442:  
        Cmd400_AccessoryEditDialog(app, hwnd);
        break;

    case 415:  
        Cmd400_FrameRangeSelect(app, hwnd);
        break;

    case 416:  
        Cmd400_SelectionMarkSync(app);
        break;

    case 420:  
        Cmd400_FrameCopy(app, hwnd);
        break;

    case 421:  
        Cmd400_FramePaste(app, hwnd);
        break;

    case 422:  
        Cmd400_FrameReversePaste(app, hwnd);
        break;

    case 424: {
        if (app->state.optflag[0] != 0) {
            break;
        }
        app->state.enterKeyState = 1;
        if (app->state.englishUI != 0) {
            DialogBoxParamA(static_cast<HINSTANCE>(app->HInstance()),
                            reinterpret_cast<LPCSTR>(0x28C), hwnd,
                            &FrameRangeDlgProc, 0);
        } else {
            DialogBoxParamA(static_cast<HINSTANCE>(app->HInstance()),
                            reinterpret_cast<LPCSTR>(0x25E), hwnd,
                            &FrameRangeDlgProc, 0);
        }
        break;
    }

    default:
        break;
    }
}

}  
