#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstdint>
#include <cstring>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

constexpr double kPiOver180 = 0.01745329238474369;   

float CameraEase(const mdl::CameraKey* keys, int ch, int idx, float t) {
    const auto& interpolation = keys[idx].interpolation;
    if (interpolation[0][ch] == interpolation[1][ch] &&
        interpolation[2][ch] == interpolation[3][ch])
        return t;                                        
#if defined(_M_IX86)
    const double x1 = static_cast<double>(
                          static_cast<signed char>(interpolation[0][ch])) / 127.0;
    const double x2 = static_cast<double>(
                          static_cast<signed char>(interpolation[2][ch])) / 127.0;
    float u = 0.5f, half = 0.25f;
    for (int i = 0; i < 12; ++i) {                       
        const float om = 1.0f - u;
        const double gx = om * 3.0 * u * u * x2 + om * (om * 3.0) * u * x1 +
                          u * u * u;
        if (static_cast<float>(gx) == t)                 
            break;
        u = static_cast<float>(gx >= static_cast<double>(t) ? u - half
                                                            : u + half);
        half *= 0.5f;
    }
    const float om = 1.0f - u;                           
    return static_cast<float>(
        3.0 * om * u * u *
            static_cast<double>(
                static_cast<signed char>(interpolation[3][ch])) / 127.0 +
        om * (om * 3.0) * u *
            static_cast<double>(
                static_cast<signed char>(interpolation[1][ch])) / 127.0 +
        u * u * u);
#else
    const float x1 = static_cast<float>(
                         static_cast<signed char>(interpolation[0][ch])) / 127.0f;
    const float x2 = static_cast<float>(
                         static_cast<signed char>(interpolation[2][ch])) / 127.0f;
    const float y1b = static_cast<float>(
        static_cast<signed char>(interpolation[1][ch]));
    const float y2b = static_cast<float>(
        static_cast<signed char>(interpolation[3][ch]));
    float u = 0.5f, half = 0.25f;
    for (int i = 0; i < 12; ++i) {                       
        const float v = 1.0f - u;
        const float v3 = v * 3.0f;
        const float gx = (v3 * v * u) * x1 + (v3 * u * u) * x2 + u * u * u;
        if (gx == t)                                     
            break;
        u = gx >= t ? u - half : u + half;               
        half *= 0.5f;
    }
    const float v = 1.0f - u;                            
    const float v3 = v * 3.0f;
    return (v3 * v * u) * y1b / 127.0f + (v3 * u * u) * y2b / 127.0f +
           u * u * u;
#endif
}

void CopyCameraKey(MMDApp* app, const mdl::CameraKey& key) {
    auto& s = *app;
    std::memcpy(s.CameraPosition(), key.eye, sizeof(key.eye));
    std::memcpy(s.CameraRotation(), key.target, sizeof(key.target));
    s.CameraDistance() = key.distance;
    s.CameraPerspective() = key.perspective;
    s.CameraFov() = static_cast<float>(key.fov);
}

void CopyLightKey(MMDApp* app, const mdl::LightKey& key, bool apply) {
    auto& s = *app;
    std::memcpy(s.LightDirection(), key.direction, sizeof(key.direction));
    std::memcpy(s.LightColor(), key.color, sizeof(key.color));
    s.ApplyTimelineLightState();
    if (!apply)
        return;
    D3DRenderer* wrapper = s.Renderer();
    IDirect3DDevice9* dev = wrapper->device;               
    dev->SetLight(0, &s.SceneLight());                       
}

}  

void ReloadModels(MMDApp* app) {
    if (app == nullptr)
        return;
    auto& s = *app;
    mdl::CameraKey* const keys = s.CameraKeys();
    if (keys == nullptr)
        return;
    const std::uint32_t frame =
        s.state.currentFrame;
    std::uint32_t index = 0;
    while (keys[index].frame < frame) {
        const std::uint32_t next = keys[index].next;
        if (next == 0)
            break;
        index = next;
    }

    const mdl::CameraKey& current = keys[index];
    const std::uint32_t currentFrame = current.frame;
    if (currentFrame <= frame) {
        CopyCameraKey(app, current);
        s.state.viewOffsetX = 0.0f;
        s.state.viewOffsetY = 0.0f;
        if (current.parentModel != s.CameraParentModel())
            RefillBoneRegisterCombo(app, current.parentModel);
        s.CameraParentModel() = current.parentModel;
        s.CameraParentBone() = current.parentBone;
    } else {
        const mdl::CameraKey& previous = keys[current.previous];
        const std::uint32_t previousFrame = previous.frame;
        const float t = static_cast<float>(
            static_cast<double>(frame - previousFrame) /
            static_cast<double>(currentFrame - previousFrame));
        s.CameraPerspective() = previous.perspective;
        float* output[6] = {
            &s.CameraPosition()[0], &s.CameraPosition()[1],
            &s.CameraPosition()[2], &s.CameraRotation()[0],
            &s.CameraDistance(), &s.CameraFov()};
        for (int channel = 0; channel < 6; ++channel) {
            const float eased = CameraEase(keys, channel,
                                            static_cast<int>(index), t);
            const double a = channel < 3 ? previous.eye[channel]
                : channel == 3 ? previous.target[0]
                : channel == 4 ? previous.distance
                               : static_cast<double>(previous.fov);
            const double b = channel < 3 ? current.eye[channel]
                : channel == 3 ? current.target[0]
                : channel == 4 ? current.distance
                               : static_cast<double>(current.fov);
            *output[channel] = static_cast<float>(eased * (b - a) + a);
        }
        const float rotationEase = CameraEase(
            keys, 3, static_cast<int>(index), t);
        for (int axis = 0; axis < 3; ++axis) {
            const float a = previous.target[axis];
            const float b = current.target[axis];
            s.CameraRotation()[axis] = static_cast<float>(
                static_cast<double>(rotationEase) *
                    (static_cast<double>(b) - static_cast<double>(a)) +
                static_cast<double>(a));
        }
        s.state.viewOffsetX = 0.0f;
        s.state.viewOffsetY = 0.0f;
        if (previous.parentModel != s.CameraParentModel())
            RefillBoneRegisterCombo(app, previous.parentModel);
        s.CameraParentModel() = previous.parentModel;
        s.CameraParentBone() = previous.parentBone;
    }

    D3DRenderer* wrapper = s.Renderer();
    if (wrapper != nullptr) {
        IDirect3DDevice9* device = wrapper->device;    
        if (device != nullptr) {
            d3dx::D3DXMATRIXF projection{};
            auto& api = d3dx::Get();
            api.perspectiveFovLH(
                &projection,
                static_cast<float>(
                    static_cast<double>(s.CameraFov()) * kPiOver180),
                wrapper->aspectRatio,  
                1.0f, 100000.0f);
            device->SetTransform(
                D3DTS_PROJECTION,
                reinterpret_cast<const D3DMATRIX*>(&projection));
        }
    }
}

void PlaybackPoseAdvance(MMDApp* app, int advance) {
    auto& s = *app;

    unsigned char** models = s.ModelSlots();
    for (int j = 0; j < kModelSlotCount; ++j)
        if (models[j] != nullptr)
            AdvanceModelKeyframes(models[j], s.PlaybackCursorSeconds(),
                      s.PlaybackPhysicsMode());

    if (advance != 0)
        return;                                          

    double frame = static_cast<double>(s.PlaybackCursorSeconds() *
                                       30.0f);
    {
        const double scaled = frame * 1000.0;
        if (scaled - static_cast<double>(static_cast<int>(scaled)) >= 0.5)
            frame = static_cast<double>(static_cast<int>(scaled) + 1) / 1000.0;
    }

    const bool editGate = s.state.optflag[0] != 0 ||
                          s.state.followCameraEnabled != 0;

    if (editGate && s.CameraTrackActive() != 0) {
        mdl::CameraKey* keys = s.CameraKeys();
        s.state.viewOffsetX = 0.0f;                        
        s.state.viewOffsetY = 0.0f;
        while (true) {
            const mdl::CameraKey& key = keys[s.CameraTrackCursor()];
            if (!(static_cast<double>(key.frame) < frame &&
                  s.CameraTrackActive() != 0))
                break;                                    
            const std::uint32_t next = key.next;
            if (next != 0) {
                s.CameraTrackCursor() = next;
            } else {
                s.CameraTrackActive() = 0;
                CopyCameraKey(app, key);
                s.CameraParentModel() = key.parentModel;
                s.CameraParentBone() = key.parentBone;
                break;
            }
        }
        if (s.CameraTrackActive() != 0) {
            const int cur = static_cast<int>(s.CameraTrackCursor());
            const mdl::CameraKey& current = keys[cur];
            const std::uint32_t frameCur = current.frame;
            if (frame == static_cast<double>(frameCur)) {
                CopyCameraKey(app, current);              
                s.CameraParentModel() = current.parentModel;
                s.CameraParentBone() = current.parentBone;
            } else {
                const mdl::CameraKey& previous = keys[current.previous];
                if (frameCur - 1 == previous.frame) {
                    CopyCameraKey(app, previous);         
                } else {
                    const std::uint32_t framePrev = previous.frame;
                    const float t =
                        (static_cast<float>(frame) -
                         static_cast<float>(framePrev)) /
                        static_cast<float>(frameCur - framePrev);
                    s.CameraPerspective() = previous.perspective;
                    float* output[6] = {
                        &s.CameraPosition()[0], &s.CameraPosition()[1],
                        &s.CameraPosition()[2], &s.CameraRotation()[0],
                        &s.CameraDistance(), &s.CameraFov()};
                    for (int ch = 0; ch < 6; ++ch) {
                        const float e = CameraEase(keys, ch, cur, t);
                        const float curV = ch < 3 ? current.eye[ch]
                            : ch == 3 ? current.target[0]
                            : ch == 4 ? current.distance
                                      : static_cast<float>(current.fov);
                        const float prevV = ch < 3 ? previous.eye[ch]
                            : ch == 3 ? previous.target[0]
                            : ch == 4 ? previous.distance
                                      : static_cast<float>(previous.fov);
                        *output[ch] = e * (curV - prevV) + prevV;
                    }
                    const float rotationEase = CameraEase(keys, 3, cur, t);
                    for (int axis = 0; axis < 3; ++axis) {
                        const float curV = current.target[axis];
                        const float prevV = previous.target[axis];
                        s.CameraRotation()[axis] =
                            rotationEase * (curV - prevV) + prevV;
                    }
                }
                s.CameraParentModel() = previous.parentModel;
                s.CameraParentBone() = previous.parentBone;
            }
        }
        D3DRenderer* wrapper = s.Renderer();
        const float fovRad = static_cast<float>(
            static_cast<double>(s.CameraFov()) * kPiOver180);
        d3dx::D3DXMATRIXF mat{};
        auto* d3dx = &d3dx::Get();
        d3dx->perspectiveFovLH(&mat, fovRad,
                               wrapper->aspectRatio,  
                               1.0f, 100000.0f);
        IDirect3DDevice9* dev = wrapper->device;   
        dev->SetTransform(D3DTS_PROJECTION,
                          reinterpret_cast<const D3DMATRIX*>(&mat));
    }

    if (editGate && s.LightTrackActive() != 0) {
        mdl::LightKey* keys = s.LightKeys();
        while (true) {
            const mdl::LightKey& key = keys[s.LightTrackCursor()];
            if (!(static_cast<double>(key.frame) < frame &&
                  s.LightTrackActive() != 0))
                break;
            const std::uint32_t next = key.next;
            if (next != 0) {
                s.LightTrackCursor() = next;
            } else {
                s.LightTrackActive() = 0;
                CopyLightKey(app, key, true);
                break;
            }
        }
        if (s.LightTrackActive() != 0) {
            const mdl::LightKey& current = keys[s.LightTrackCursor()];
            const std::uint32_t frameCur = current.frame;
            if (frame == static_cast<double>(frameCur)) {
                CopyLightKey(app, current, true);          
            } else {
                const mdl::LightKey& previous = keys[current.previous];
                const std::uint32_t framePrev = previous.frame;
                const float t =
                    (static_cast<float>(frame) -
                     static_cast<float>(framePrev)) /
                    static_cast<float>(frameCur - framePrev);
                for (int c = 0; c < 3; ++c) {             
                    const float curV = current.direction[c];
                    const float prevV = previous.direction[c];
                    s.LightDirection()[c] =
                        t * (curV - prevV) + prevV;
                }
                for (int c = 0; c < 3; ++c) {             
                    const float curV = current.color[c];
                    const float prevV = previous.color[c];
                    s.LightColor()[c] =
                        t * (curV - prevV) + prevV;
                }
                s.ApplyTimelineLightState();
                D3DRenderer* wrapper = s.Renderer();
                IDirect3DDevice9* dev = wrapper->device;  
                dev->SetLight(0, &s.SceneLight());
            }
        }
    }

    if (editGate && s.ShadowTrackActive() != 0) {
        mdl::SelfShadowKey* keys = s.ShadowKeys();
        while (true) {
            const mdl::SelfShadowKey& key = keys[s.ShadowTrackCursor()];
            if (!(static_cast<double>(key.frame) < frame &&
                  s.ShadowTrackActive() != 0))
                break;
            const std::uint32_t next = key.next;
            if (next != 0) {
                s.ShadowTrackCursor() = next;
            } else {
                s.ShadowTrackActive() = 0;
                s.ShadowMode() =
                    static_cast<signed char>(key.mode);
                s.ShadowDistance() = key.distance;
                break;
            }
        }
        if (s.ShadowTrackActive() != 0) {
            const mdl::SelfShadowKey& current = keys[s.ShadowTrackCursor()];
            const std::uint32_t frameCur = current.frame;
            if (frame == static_cast<double>(frameCur)) {
                s.ShadowMode() =
                    static_cast<signed char>(current.mode);
                s.ShadowDistance() = current.distance;
            } else {
                const mdl::SelfShadowKey& previous = keys[current.previous];
                s.ShadowMode() =
                    static_cast<signed char>(previous.mode);
                s.ShadowDistance() = previous.distance;
            }
        }
    }

    if (editGate && s.GravityTrackActive() != 0) {
        mdl::GravityKey* keys = s.GravityKeys();
        while (true) {
            const mdl::GravityKey& key = keys[s.GravityTrackCursor()];
            if (!(static_cast<double>(key.frame) < frame &&
                  s.GravityTrackActive() != 0))
                break;
            const std::uint32_t next = key.next;
            if (next != 0) {
                s.GravityTrackCursor() = next;
            } else {
                s.GravityTrackActive() = 0;
                s.GravityNoiseEnabled() = key.noiseEnabled;
                s.GravityNoise() = key.noise;
                s.GravityMagnitude() = key.acceleration;
                std::memcpy(s.GravityDirection(), key.direction,
                            sizeof(key.direction));
                break;
            }
        }
        if (s.GravityTrackActive() != 0) {
            const mdl::GravityKey& current = keys[s.GravityTrackCursor()];
            const std::uint32_t frameCur = current.frame;
            if (frame == static_cast<double>(frameCur)) {
                s.GravityNoiseEnabled() = current.noiseEnabled;
                s.GravityNoise() = current.noise;
                s.GravityMagnitude() = current.acceleration;
                std::memcpy(s.GravityDirection(), current.direction,
                            sizeof(current.direction));
            } else {
                const mdl::GravityKey& previous = keys[current.previous];
                const std::uint32_t framePrev = previous.frame;
                const float t =
                    (static_cast<float>(frame) -
                     static_cast<float>(framePrev)) /
                    static_cast<float>(frameCur - framePrev);
                s.GravityNoiseEnabled() = previous.noiseEnabled;
                s.GravityNoise() = previous.noise +
                    static_cast<int>(static_cast<float>(
                        current.noise - previous.noise) * t);
                s.GravityMagnitude() =
                    (current.acceleration - previous.acceleration) * t +
                    previous.acceleration;
                for (int axis = 0; axis < 3; ++axis) {
                    s.GravityDirection()[axis] =
                        (current.direction[axis] -
                         previous.direction[axis]) * t +
                        previous.direction[axis];
                }
            }
        }
    }

    for (int i = 0; i < 0xFF; ++i) {
        if (s.AccessoryTrackActive(i) == 0)
            continue;
        auto& cursor = s.AccessoryTrackCursor(i);
        mdl::AccessoryKey* keys = s.AccessoryKeys(i);
        mdl::AccessoryRecord* const accessorySlot = s.AccessorySlot(i);
        if (keys == nullptr || accessorySlot == nullptr) {
            s.AccessoryTrackActive(i) = 0;
            cursor = 0;
            continue;
        }
        mdl::AccessoryRecord& accessory = *accessorySlot;
        const auto applyDiscreteState = [&](const mdl::AccessoryKey& key) {
            accessory.visible = key.visible;
            accessory.shadowEnabled = key.shadowEnabled;
            accessory.parentModel = key.parentModel;
            accessory.parentBone = key.parentBone;
            std::memcpy(accessory.rotation, key.rotation,
                        sizeof accessory.rotation);
            std::memcpy(accessory.position, key.position,
                        sizeof accessory.position);
        };
        while (true) {
            const mdl::AccessoryKey& rec = keys[cursor];
            if (!(static_cast<double>(
                      rec.frame) < frame &&
                  s.AccessoryTrackActive(i) != 0))
                break;
            const std::uint32_t next = rec.next;
            if (next != 0) {
                cursor = next;
            } else {
                s.AccessoryTrackActive(i) = 0;
                applyDiscreteState(rec);
                accessory.scale = rec.scale;
                accessory.opacity = rec.opacity;
                break;
            }
        }
        if (s.AccessoryTrackActive(i) == 0)
            continue;
        const mdl::AccessoryKey& recCur = keys[cursor];
        const std::uint32_t frameCur = recCur.frame;
        if (frame == static_cast<double>(frameCur)) {
            applyDiscreteState(recCur);
            accessory.scale = recCur.scale;
            accessory.opacity = recCur.opacity;
        } else {
            const mdl::AccessoryKey& recPrev = keys[recCur.previous];
            const std::uint32_t framePrev = recPrev.frame;
            const float t = static_cast<float>(
                (frame - static_cast<double>(framePrev)) /
                static_cast<double>(frameCur - framePrev));
            accessory.scale =
                t * (recCur.scale - recPrev.scale) + recPrev.scale;
            accessory.opacity =
                t * (recCur.opacity - recPrev.opacity) + recPrev.opacity;
            applyDiscreteState(recPrev);
        }
    }
}

}  
