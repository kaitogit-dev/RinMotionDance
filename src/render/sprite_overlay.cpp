#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "vb_dump.hpp"

namespace rinmotiondance {
namespace {

struct SpriteVertex {
    float x;
    float y;
    float z;
    float rhw;
    D3DCOLOR color;
    float u;
    float v;
};

static_assert(sizeof(SpriteVertex) == 28);

struct LineVertex {
    float x;
    float y;
    float z;
    float rhw;
    D3DCOLOR color;
};

static_assert(sizeof(LineVertex) == 20);

struct BoneOverlayResult {
    int selectedX;
    int selectedY;
    float selectedClipW;
    std::uint32_t spritePrimitiveCount;
    std::uint32_t linePrimitiveCount;
};

static_assert(sizeof(BoneOverlayResult) == 20);

template <typename T>
T& At(void* base, std::size_t offset) {
    return *reinterpret_cast<T*>(static_cast<unsigned char*>(base) + offset);
}

struct ClipPoint {
    float x;
    float y;
    float z;
    float w;
};

ClipPoint TransformPoint(float x, float y, float z, const D3DMATRIX& m) {
    return {
        x * m._11 + y * m._21 + z * m._31 + m._41,
        x * m._12 + y * m._22 + z * m._32 + m._42,
        x * m._13 + y * m._23 + z * m._33 + m._43,
        x * m._14 + y * m._24 + z * m._34 + m._44,
    };
}

ClipPoint TransformPoint(const ClipPoint& p, const D3DMATRIX& m) {
    return {
        p.x * m._11 + p.y * m._21 + p.z * m._31 + p.w * m._41,
        p.x * m._12 + p.y * m._22 + p.z * m._32 + p.w * m._42,
        p.x * m._13 + p.y * m._23 + p.z * m._33 + p.w * m._43,
        p.x * m._14 + p.y * m._24 + p.z * m._34 + p.w * m._44,
    };
}

bool ProjectBonePoint(rinmotiondance::mdl::BoneRecord* bone, std::size_t pointOffset,
                      const D3DMATRIX& world, const D3DMATRIX& view,
                      const D3DMATRIX& projection, const RECT& viewport,
                      int* screenX, int* screenY, float* clipW) {
    const auto& boneMatrix =
        *reinterpret_cast<const D3DMATRIX*>(bone->matInit);
    const float* point = reinterpret_cast<const float*>(
        reinterpret_cast<const unsigned char*>(bone) + pointOffset);
    ClipPoint p = TransformPoint(point[0], point[1], point[2], boneMatrix);
    p = TransformPoint(p, world);
    p = TransformPoint(p, view);
    p = TransformPoint(p, projection);
    *clipW = p.w;
    if (p.w <= 0.0f) {
        *screenX = 393939;
        *screenY = 393939;
        return false;
    }
    const double width = static_cast<double>(viewport.right - viewport.left);
    const double height = static_cast<double>(viewport.bottom - viewport.top);
    *screenX = static_cast<int>(p.x / p.w * width * 0.5 +
                                (viewport.left + viewport.right) / 2);
    *screenY = static_cast<int>((viewport.top + viewport.bottom) / 2 -
                                p.y / p.w * height * 0.5);
    return true;
}

__declspec(noinline) float BoneVCoordinate(int numerator, int origin,
                                           float lengthValue, float scale) {
#if defined(_M_IX86)
    static const double kEight = 8.0;
    float result = 0.0f;
    __asm {
        fild dword ptr numerator
        fdiv dword ptr lengthValue
        fmul qword ptr kEight
        fmul dword ptr scale
        fiadd dword ptr origin
        fstp dword ptr result
    }
    return result;
#else
    return static_cast<float>(static_cast<double>(numerator) / lengthValue *
                              8.0 * scale + origin);
#endif
}

__declspec(noinline) void BoneMarkerBounds(int x, int y, float scale,
                                           float* left, float* top,
                                           float* right, float* bottom) {
    float l = 0.0f;
    float t = 0.0f;
    float r = 0.0f;
    float b = 0.0f;
#if defined(_M_IX86)
    static const double kHalf = 9.5;
    static const double kSize = 18.0;
    __asm {
        fld dword ptr scale
        fmul qword ptr kHalf

        fild dword ptr x
        fsub st, st(1)
        fstp dword ptr l
        fld dword ptr l
        fld dword ptr scale
        fmul qword ptr kSize
        faddp st(1), st
        fstp dword ptr r

        fild dword ptr y
        fsub st, st(1)
        fstp dword ptr t
        fld dword ptr t
        fld dword ptr scale
        fmul qword ptr kSize
        faddp st(1), st
        fstp dword ptr b

        fstp st(0)
    }
#else
    l = static_cast<float>(x - scale * 9.5);
    r = static_cast<float>(l + scale * 18.0);
    t = static_cast<float>(y - scale * 9.5);
    b = static_cast<float>(t + scale * 18.0);
#endif
    *left = l;
    *top = t;
    *right = r;
    *bottom = b;
}

void AppendQuad(SpriteVertex*& out, float left, float top,
                float right, float bottom,
                float u0, float v0, float u1, float v1) {
    constexpr D3DCOLOR white = 0xFFFFFFFFu;
    const SpriteVertex quad[6] = {
        {right, top,    0.0f, 1.0f, white, u1, v0},
        {right, bottom, 0.0f, 1.0f, white, u1, v1},
        {left,  top,    0.0f, 1.0f, white, u0, v0},
        {left,  top,    0.0f, 1.0f, white, u0, v0},
        {right, bottom, 0.0f, 1.0f, white, u1, v1},
        {left,  bottom, 0.0f, 1.0f, white, u0, v1},
    };
    for (const SpriteVertex& vertex : quad)
        *out++ = vertex;
}

bool AppendBoneV(LineVertex*& out, int startX, int startY,
                 int endX, int endY, float scale, D3DCOLOR color) {
    const int dx = endX - startX;
    const int dy = endY - startY;
    const float length = static_cast<float>(
        std::sqrt(static_cast<double>(dx * dx + dy * dy)));
    if (length == 0.0f)
        return false;

    const LineVertex vertices[4] = {
        {BoneVCoordinate(-dy, startX, length, scale),
         BoneVCoordinate( dx, startY, length, scale),
         0.0f, 1.0f, color},
        {static_cast<float>(endX), static_cast<float>(endY),
         0.0f, 1.0f, color},
        {static_cast<float>(endX), static_cast<float>(endY),
         0.0f, 1.0f, color},
        {BoneVCoordinate( dy, startX, length, scale),
         BoneVCoordinate(-dx, startY, length, scale),
         0.0f, 1.0f, color},
    };
    for (const LineVertex& vertex : vertices)
        *out++ = vertex;
    return true;
}

void AppendLine(LineVertex*& out, int startX, int startY,
                int endX, int endY, D3DCOLOR color) {
    *out++ = {static_cast<float>(startX), static_cast<float>(startY),
              0.0f, 1.0f, color};
    *out++ = {static_cast<float>(endX), static_cast<float>(endY),
              0.0f, 1.0f, color};
}


struct GizmoSegment {
    float x0;
    float y0;
    float x1;
    float y1;
    bool visible;
};

struct TransformGizmoCache {
    MMDApp* app = nullptr;
    unsigned char* model = nullptr;
    int bone = -1;
    bool move = false;
    bool valid = false;
    int count = 0;
    float dragX = 0.0f;
    float dragY = 0.0f;
    GizmoSegment segments[3][64]{};
};

TransformGizmoCache gTransformGizmo;

bool ProjectGizmoPoint(const ClipPoint& position,
                       const D3DMATRIX& world, const D3DMATRIX& view,
                       const D3DMATRIX& projection, const RECT& bounds,
                       float* x, float* y) {
    ClipPoint p = TransformPoint(position, world);
    p = TransformPoint(p, view);
    p = TransformPoint(p, projection);
    if (p.w <= 0.001f || !std::isfinite(p.w))
        return false;
    const float px = p.x / p.w;
    const float py = p.y / p.w;
    if (!std::isfinite(px) || !std::isfinite(py))
        return false;
    *x = (bounds.left + bounds.right) * 0.5f +
         px * (bounds.right - bounds.left) * 0.5f;
    *y = (bounds.top + bounds.bottom) * 0.5f -
         py * (bounds.bottom - bounds.top) * 0.5f;
    return true;
}

void PrepareTransformGizmo(MMDApp* app, unsigned char* model,
                           const mdl::BoneRecord& bone, int boneIndex,
                           const D3DMATRIX& world, const D3DMATRIX& view,
                           const D3DMATRIX& projection, const RECT& bounds,
                           bool move) {
    auto& gizmo = gTransformGizmo;
    const auto& boneMatrix =
        *reinterpret_cast<const D3DMATRIX*>(bone.matInit);
    const ClipPoint origin = TransformPoint(bone.position[0],
        bone.position[1], bone.position[2], boneMatrix);
    float cx = 0.0f;
    float cy = 0.0f;
    if (!ProjectGizmoPoint(origin, world, view, projection,
                           bounds, &cx, &cy))
        return;
    float basis[16]{};
    BoneLocalAxes(app, basis);
    ClipPoint axes[3]{};
    for (int i = 0; i < 3; ++i) {
        float direction[3]{};
        if (app->state.coordinateSystem == 1) {
            direction[i] = 1.0f;
        } else {
            for (int k = 0; k < 3; ++k)
                direction[k] = basis[i * 4 + k];
            const float transformed[3] = {
                boneMatrix._11 * direction[0] + boneMatrix._21 * direction[1] +
                    boneMatrix._31 * direction[2],
                boneMatrix._12 * direction[0] + boneMatrix._22 * direction[1] +
                    boneMatrix._32 * direction[2],
                boneMatrix._13 * direction[0] + boneMatrix._23 * direction[1] +
                    boneMatrix._33 * direction[2]};
            for (int k = 0; k < 3; ++k)
                direction[k] = transformed[k];
        }
        axes[i] = {direction[0], direction[1], direction[2], 0.0f};
    }
    float maxPixels = 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        ClipPoint point{origin.x + axes[axis].x, origin.y + axes[axis].y,
                        origin.z + axes[axis].z, 1.0f};
        float x = 0.0f;
        float y = 0.0f;
        if (!ProjectGizmoPoint(point, world, view, projection,
                               bounds, &x, &y))
            return;
        maxPixels = std::max(maxPixels, std::hypot(x - cx, y - cy));
    }
    if (maxPixels < 0.0001f)
        return;
    const float radius = std::clamp(67.0f / maxPixels, 0.001f, 50.0f);
    gizmo.app = app;
    gizmo.model = model;
    gizmo.bone = boneIndex;
    gizmo.move = move;
    gizmo.count = move ? 3 : 64;
    for (int axis = 0; axis < 3; ++axis) {
        for (int segment = 0; segment < gizmo.count; ++segment) {
            GizmoSegment& item = gizmo.segments[axis][segment];
            item.visible = false;
            float x0 = 0.0f;
            float y0 = 0.0f;
            float x1 = 0.0f;
            float y1 = 0.0f;
            if (move) {
                ClipPoint end{origin.x + axes[axis].x * radius,
                              origin.y + axes[axis].y * radius,
                              origin.z + axes[axis].z * radius, 1.0f};
                if (!ProjectGizmoPoint(end, world, view, projection,
                                       bounds, &x1, &y1))
                    continue;
                x0 = cx;
                y0 = cy;
                if (segment > 0) {
                    const float length = std::hypot(x1 - cx, y1 - cy);
                    if (length < 0.5f)
                        continue;
                    const float tx = (x1 - cx) / length;
                    const float ty = (y1 - cy) / length;
                    const float side = segment == 1 ? 1.0f : -1.0f;
                    x0 = x1 - tx * 12.0f + -ty * side * 6.0f;
                    y0 = y1 - ty * 12.0f + tx * side * 6.0f;
                }
            } else {
                const int u = (axis + 1) % 3;
                const int v = (axis + 2) % 3;
                const float t0 = segment * 6.28318530718f / 64.0f;
                const float t1 = (segment + 1) * 6.28318530718f / 64.0f;
                const ClipPoint p0{
                    origin.x + radius * (axes[u].x * std::cos(t0) +
                                         axes[v].x * std::sin(t0)),
                    origin.y + radius * (axes[u].y * std::cos(t0) +
                                         axes[v].y * std::sin(t0)),
                    origin.z + radius * (axes[u].z * std::cos(t0) +
                                         axes[v].z * std::sin(t0)), 1.0f};
                const ClipPoint p1{
                    origin.x + radius * (axes[u].x * std::cos(t1) +
                                         axes[v].x * std::sin(t1)),
                    origin.y + radius * (axes[u].y * std::cos(t1) +
                                         axes[v].y * std::sin(t1)),
                    origin.z + radius * (axes[u].z * std::cos(t1) +
                                         axes[v].z * std::sin(t1)), 1.0f};
                if (!ProjectGizmoPoint(p0, world, view, projection,
                                       bounds, &x0, &y0) ||
                    !ProjectGizmoPoint(p1, world, view, projection,
                                       bounds, &x1, &y1))
                    continue;
            }
            if (x0 < bounds.left - 100 || x0 > bounds.right + 100 ||
                x1 < bounds.left - 100 || x1 > bounds.right + 100 ||
                y0 < bounds.top - 100 || y0 > bounds.bottom + 100 ||
                y1 < bounds.top - 100 || y1 > bounds.bottom + 100)
                continue;
            item = {x0, y0, x1, y1, true};
        }
    }
    gizmo.valid = true;
}

void AppendTransformGizmo(LineVertex*& lines, std::uint32_t& count) {
    const auto& gizmo = gTransformGizmo;
    if (!gizmo.valid)
        return;
    const D3DCOLOR colors[3] = {0xFFF05C61u, 0xFF58D47Au, 0xFF608BFFu};
    const D3DCOLOR selected[3] = {0xFFFFADA9u, 0xFFB5FFC6u, 0xFFB3C7FFu};
    const int hovered = static_cast<int>(gizmo.app->ViewportToolOperation());
    const int active = static_cast<int>(gizmo.app->InteractionDragMode());
    for (int axis = 0; axis < 3; ++axis) {
        const int op = (gizmo.move ? 25 : 22) + axis;
        const int drag = (gizmo.move ? 24 : 21) + axis;
        const D3DCOLOR color =
            hovered == op || active == drag ? selected[axis] : colors[axis];
        for (int i = 0; i < gizmo.count; ++i) {
            const auto& segment = gizmo.segments[axis][i];
            if (!segment.visible || count >= 4998)
                continue;
            AppendLine(lines, static_cast<int>(segment.x0),
                       static_cast<int>(segment.y0),
                       static_cast<int>(segment.x1),
                       static_cast<int>(segment.y1), color);
            ++count;
        }
    }
}

}  

int PickTransformGizmo(MMDApp* app, int x, int y) {
    auto& gizmo = gTransformGizmo;
    if (!gizmo.valid || gizmo.app != app ||
        gizmo.model != app->SelectedModel() ||
        app->PlaybackActive() != 0 ||
        app->InteractionDragMode() != ViewportDragMode::None)
        return 0;
    auto* model = mdl::Mdl(gizmo.model);
    if (model == nullptr || model->selectedBone != gizmo.bone)
        return 0;
    const int mode = static_cast<int>(app->EditMode());
    if (!(mode == 0 || mode == 3 || mode == 4))
        return 0;
    const bool moving = mode == 4 || (mode == 0 &&
        (model->boneTable[gizmo.bone].type == mdl::BoneType::Move ||
         model->boneTable[gizmo.bone].type == mdl::BoneType::Ik));
    if (moving != gizmo.move)
        return 0;
    float best = 64.0f;
    int bestAxis = -1;
    float bestX = 0.0f;
    float bestY = 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        for (int i = 0; i < gizmo.count; ++i) {
            const GizmoSegment& s = gizmo.segments[axis][i];
            if (!s.visible)
                continue;
            const float dx = s.x1 - s.x0;
            const float dy = s.y1 - s.y0;
            const float len2 = dx * dx + dy * dy;
            if (len2 < 4.0f)
                continue;
            const float t = std::clamp(((x - s.x0) * dx +
                (y - s.y0) * dy) / len2, 0.0f, 1.0f);
            const float px = s.x0 + t * dx - x;
            const float py = s.y0 + t * dy - y;
            const float dist2 = px * px + py * py;
            if (dist2 < best) {
                best = dist2;
                bestAxis = axis;
                const float invLen = 1.0f / std::sqrt(len2);
                bestX = dx * invLen;
                bestY = dy * invLen;
            }
        }
    }
    if (bestAxis < 0)
        return 0;
    gizmo.dragX = bestX;
    gizmo.dragY = bestY;
    return (gizmo.move ? 25 : 22) + bestAxis;
}

int TransformGizmoDragPixels(MMDApp* app) {
    if (!gTransformGizmo.valid || gTransformGizmo.app != app)
        return 0;
    const float amount =
        (app->MouseX() - app->PreviousMouseX()) * gTransformGizmo.dragX +
        (app->MouseY() - app->PreviousMouseY()) * gTransformGizmo.dragY;
    return static_cast<int>(std::round(amount));
}

void PrepareFrameSpriteOverlay(MMDApp* app) {
    gTransformGizmo.valid = false;
    if (app->RecordingWindow() != nullptr)
        return;

    auto* vb = app->SpriteOverlayVertices();
    if (vb == nullptr)
        return;

    SpriteVertex* out = nullptr;
    if (FAILED(vb->Lock(0, 0x445C0, reinterpret_cast<void**>(&out), 0)))
        return;
    SpriteVertex* const spriteBegin = out;
    app->SpriteOverlayPrimitiveCount() = 0;

    const auto append = [&](float left, float top, float right, float bottom,
                            float u0, float v0, float u1, float v1) {
        AppendQuad(out, left, top, right, bottom, u0, v0, u1, v1);
        app->SpriteOverlayPrimitiveCount() += 2;
    };

    D3DRenderer* sub = app->Renderer();
    const float scale = sub != nullptr
        ? sub->viewScale
        : 1.0f;
    const RECT view = app->ViewportRect();
    const float right = static_cast<float>(view.right);
    const float bottom = static_cast<float>(view.bottom);

    if (app->state.optflag[0] == 0 &&
        app->PlaybackActive() == 0 && sub != nullptr) {
        auto* model = app->SelectedModel();
        auto* device = sub->device;
        if (model != nullptr && device != nullptr) {
            const std::uint32_t spriteCountBeforeBone =
                app->SpriteOverlayPrimitiveCount();
            BoneOverlayResult boneResult{393939, 393939, 0.0f, 0,
                app->LineOverlayPrimitiveCount()};
            auto* bones = rinmotiondance::mdl::Bones(model);
            const int boneCount = mdl::Mdl(model)->boneCount;
            const int selected = mdl::Mdl(model)->selectedBone;
            auto* active = mdl::Mdl(model)->boneSelection;
            auto* secondary = mdl::Mdl(model)->bonePhysicsState;
            const mdl::IkChain* relationships = mdl::IkChains(model);
            const int relationshipCount =
                static_cast<int>(mdl::Mdl(model)->ikChainCount);
            const int cursorKind = static_cast<int>(app->EditMode());
            const int physMode = app->PlaybackPhysicsMode();
            D3DMATRIX world{};
            D3DMATRIX viewMatrix{};
            D3DMATRIX projection{};
            device->GetTransform(D3DTS_WORLD, &world);
            device->GetTransform(D3DTS_VIEW, &viewMatrix);
            device->GetTransform(D3DTS_PROJECTION, &projection);

            for (int index = 0; bones != nullptr && index < boneCount;
                 ++index) {
                auto* bone = &bones[index];
                float w = 0.0f;
                ProjectBonePoint(bone, offsetof(mdl::BoneRecord, position),
                                 world, viewMatrix, projection,
                                 view, &bone->selState,
                                 &bone->selState2, &w);
                if (index == selected) {
                    boneResult.selectedX = bone->selState;
                    boneResult.selectedY = bone->selState2;
                    boneResult.selectedClipW = w;
                }
                if (mdl::Mdl(model)->physicsMode == 2 &&
                    (bone->flags & mdl::kBoneFlagTailIsBone) == 0) {
                    float secondaryW = 0.0f;
                    ProjectBonePoint(bone, offsetof(mdl::BoneRecord, tailOffset),
                                     world, viewMatrix, projection,
                                     view, &bone->tailScreenX,
                                     &bone->tailScreenY, &secondaryW);
                }
            }

            if (bones != nullptr) {
                auto* lineVb = sub->lineVertexBuffer;
                const std::uint32_t incomingLineCount =
                    boneResult.linePrimitiveCount;
                LineVertex* lines = nullptr;
                if (lineVb != nullptr && incomingLineCount < 5000 &&
                    SUCCEEDED(lineVb->Lock(40 * incomingLineCount,
                        40 * (5000 - incomingLineCount),
                        reinterpret_cast<void**>(&lines), 0))) {
                    const auto filtered = [&](rinmotiondance::mdl::BoneRecord* bone) -> bool {
                        return (physMode == 1 && bone->hasRigidBody) ||
                            (physMode == 2 && bone->hasRigidBody &&
                             bone->physicsDisabled == 0);
                    };
                    const auto endpoint = [&](rinmotiondance::mdl::BoneRecord* bone,
                                              int* x, int* y) -> bool {
                        const std::uint16_t flags =
                            bone->flags;
                        if (mdl::Mdl(model)->physicsMode == 2 &&
                            (flags & mdl::kBoneFlagTailIsBone) == 0) {
                            *x = bone->tailScreenX;
                            *y = bone->tailScreenY;
                            return true;
                        }
                        const int linked = bone->tailBone;
                        if (linked <= 0 || linked >= boneCount)
                            return false;
                        *x = bones[linked].selState;
                        *y = bones[linked].selState2;
                        return true;
                    };
                    const auto emitBone = [&](rinmotiondance::mdl::BoneRecord* bone,
                                              D3DCOLOR color) -> bool {
                        int endX = 393939;
                        int endY = 393939;
                        if (bone->selState == 393939 ||
                            bone->selState2 == 393939 ||
                            !endpoint(bone, &endX, &endY) ||
                            endX == 393939 || endY == 393939)
                            return false;
                        return AppendBoneV(lines, bone->selState,
                            bone->selState2, endX, endY, scale, color);
                    };

                    if (cursorKind < 2) {
                        for (int index = 0; index < boneCount; ++index) {
                            auto* bone = &bones[index];
                            const std::uint16_t flags =
                                bone->flags;
                            const mdl::BoneType type =
                                bone->type;
                            if ((flags & mdl::kBoneFlagVisible) == 0 ||
                                type >= mdl::BoneType::FixedAxis ||
                                filtered(bone) ||
                                (bone->tailBone <= 0 &&
                                 !(mdl::Mdl(model)->physicsMode == 2 &&
                                   (flags & mdl::kBoneFlagTailIsBone) == 0)))
                                continue;

                            bool inActiveRelationship = false;
                            for (int relation = 0;
                                 relationships != nullptr &&
                                 relation < relationshipCount; ++relation) {
                                const mdl::IkChain& record =
                                    relationships[relation];
                                if (record.enabled == 0)
                                    continue;
                                const std::uint8_t memberCount = record.linkCount;
                                const std::uint16_t* members = record.links;
                                for (int member = 0;
                                     members != nullptr &&
                                     member < memberCount; ++member) {
                                    if (members[member] != index)
                                        continue;
                                    inActiveRelationship = true;
                                    const D3DCOLOR color =
                                        active != nullptr && active[index]
                                        ? (index == selected ? 0xFFFF0000u
                                                             : 0xFFC80000u)
                                        : (secondary != nullptr &&
                                                   secondary[index]
                                               ? 0xFF64FF64u
                                               : 0xFFFF9B00u);
                                    if (emitBone(bone, color))
                                        boneResult.linePrimitiveCount += 2;
                                }
                            }
                            if (!inActiveRelationship) {
                                const D3DCOLOR color =
                                    active != nullptr && active[index]
                                    ? (index == selected ? 0xFFFF0000u
                                                         : 0xFFC80000u)
                                    : (secondary != nullptr &&
                                               secondary[index]
                                           ? 0xFF64FF64u
                                           : 0xFF6464E6u);
                                if (emitBone(bone, color))
                                    boneResult.linePrimitiveCount += 2;
                            }
                        }

                        for (int relation = 0;
                             relationships != nullptr &&
                             relation < relationshipCount; ++relation) {
                            const mdl::IkChain& record = relationships[relation];
                            if (record.enabled == 0)
                                continue;
                            const int first = record.boneIndex;
                            const int second = record.targetBone;
                            if (first < 0 || first >= boneCount ||
                                second < 0 || second >= boneCount)
                                continue;
                            auto* a = &bones[first];
                            auto* b = &bones[second];
                            if ((a->flags & mdl::kBoneFlagVisible) == 0 ||
                                (b->flags & mdl::kBoneFlagVisible) == 0 ||
                                a->selState == 393939 ||
                                a->selState2 == 393939 ||
                                b->selState == 393939 ||
                                b->selState2 == 393939)
                                continue;
                            AppendLine(lines, a->selState,
                                       a->selState2, b->selState,
                                       b->selState2, 0xFF000000u);
                            ++boneResult.linePrimitiveCount;
                        }
                    }

                    if (cursorKind != 2 && selected >= 0 &&
                        selected < boneCount) {
                        auto* bone = &bones[selected];
                        const std::uint16_t flags =
                            bone->flags;
                        const int linked = bone->tailBone;
                        if ((flags & mdl::kBoneFlagVisible) != 0 &&
                            bone->type < mdl::BoneType::FixedAxis &&
                            !filtered(bone) && linked > 0 &&
                            linked < boneCount &&
                            bone->selState != 393939 &&
                            bone->selState2 != 393939 &&
                            bones[linked].selState != 393939 &&
                            bones[linked].selState2 != 393939 &&
                            AppendBoneV(lines, bone->selState,
                                bone->selState2,
                                bones[linked].selState,
                                bones[linked].selState2, scale,
                                0xFFFF3232u)) {
                            boneResult.linePrimitiveCount += 2;
                        }
                    }
                    if (selected >= 0 && selected < boneCount &&
                        app->state.frameCopyDialog == nullptr &&
                        cursorKind != 1 && cursorKind != 2 &&
                        !(bones[selected].type == mdl::BoneType::FixedAxis)) {
                        const mdl::BoneType type = bones[selected].type;
                        const bool moving = cursorKind == 4 || (cursorKind == 0 &&
                            (type == mdl::BoneType::Move || type == mdl::BoneType::Ik));
                        PrepareTransformGizmo(app, model, bones[selected], selected,
                                               world, viewMatrix, projection, view, moving);
                        AppendTransformGizmo(lines, boneResult.linePrimitiveCount);
                    }
                    LineVertex* const lineBegin = lines -
                        2 * boneResult.linePrimitiveCount;
                    DumpVertexBatch("line", lineBegin,
                        boneResult.linePrimitiveCount, 40);
                    lineVb->Unlock();
                    app->LineOverlayPrimitiveCount() +=
                        boneResult.linePrimitiveCount;
                }
            }

            if (bones != nullptr) {
                for (int index = 0; index < boneCount; ++index) {
                    auto* bone = &bones[index];
                    const std::uint16_t flags = bone->flags;
                    const mdl::BoneType type = bone->type;
                    if ((flags & mdl::kBoneFlagVisible) == 0)
                        continue;

                    float v0 = 0.0f;
                    float v1 = 0.0f;
                    const bool isActive = active != nullptr && active[index] != 0;
                    const bool isSecondary = secondary != nullptr && secondary[index] != 0;
                    bool emit = false;
                    if (cursorKind > 2) {
                        if (index != selected && isActive &&
                            (type == mdl::BoneType::Move ||
                             type == mdl::BoneType::Ik)) {
                            v0 = type == mdl::BoneType::Move ? 0.034456f
                                                             : 0.104768f;
                            v1 = type == mdl::BoneType::Move
                                     ? 0.069058999f
                                     : 0.13937099f;
                            emit = true;
                        }
                    } else if (cursorKind < 2 &&
                               !((physMode == 1 && bone->hasRigidBody) ||
                                 (physMode == 2 && bone->hasRigidBody &&
                                  bone->physicsDisabled == 0))) {
                        const int state = isActive ? 0 : (isSecondary ? 1 : 2);
                        switch (type) {
                        case mdl::BoneType::RotateMove:
                        case mdl::BoneType::RotateGrant:
                        case mdl::BoneType::Effector: {
                            const float starts[] = {0.0f, 0.17508f, 0.52664f};
                            const float ends[] = {0.033902999f, 0.209683f, 0.561243f};
                            v0 = starts[state]; v1 = ends[state]; emit = true; break;
                        }
                        case mdl::BoneType::Move: {
                            const float starts[] = {0.034456f, 0.210236f, 0.56179601f};
                            const float ends[] = {0.069058999f, 0.244839f, 0.59639901f};
                            v0 = starts[state]; v1 = ends[state]; emit = true; break;
                        }
                        case mdl::BoneType::Ik: {
                            const float starts[] = {0.104768f, 0.28054801f, 0.63210797f};
                            const float ends[] = {0.13937099f, 0.31515101f, 0.66671097f};
                            v0 = starts[state]; v1 = ends[state]; emit = true; break;
                        }
                        case mdl::BoneType::UnderIk: {
                            const bool ik =
                                (flags & mdl::kBoneFlagFixedAxis) != 0;
                            const float startsIk[] = {0.77328497f, 0.73812902f, 0.70297301f};
                            const float endsIk[] = {0.80788797f, 0.77273202f, 0.73757601f};
                            const float starts[] = {0.069611996f, 0.24539199f, 0.59695202f};
                            const float ends[] = {0.104215f, 0.27999499f, 0.63155502f};
                            v0 = ik ? startsIk[state] : starts[state];
                            v1 = ik ? endsIk[state] : ends[state]; emit = true; break;
                        }
                        case mdl::BoneType::FixedAxis: {
                            const float starts[] = {0.139924f, 0.31570399f, 0.66726398f};
                            const float ends[] = {0.174527f, 0.35030699f, 0.70186698f};
                            v0 = starts[state]; v1 = ends[state]; emit = true; break;
                        }
                        default:
                            break;
                        }
                    }
                    if (emit) {
                        float left = 0.0f;
                        float top = 0.0f;
                        float right = 0.0f;
                        float bottom = 0.0f;
                        BoneMarkerBounds(bone->selState,
                                         bone->selState2, scale,
                                         &left, &top, &right, &bottom);
                        append(left, top, right, bottom,
                               0.46414399f, v0, 0.49874699f, v1);
                    }
                }

                if (cursorKind != 2 && selected >= 0 &&
                    selected < boneCount) {
                    auto* bone = &bones[selected];
                    const std::uint16_t flags =
                        bone->flags;
                    const bool filteredInMode2 =
                        physMode == 2 && bone->hasRigidBody &&
                        bone->physicsDisabled == 0;
                    float v0 = 0.0f;
                    float v1 = 0.0f;
                    bool emit = false;
                    if ((flags & mdl::kBoneFlagVisible) != 0 &&
                        !filteredInMode2) {
                        switch (bone->type) {
                        case mdl::BoneType::RotateMove:
                            v0 = 0.35086f; v1 = 0.385463f; emit = true;
                            break;
                        case mdl::BoneType::Move:
                            v0 = 0.38601601f; v1 = 0.42061901f; emit = true;
                            break;
                        case mdl::BoneType::Ik:
                        case mdl::BoneType::Unused3:
                            v0 = 0.456328f; v1 = 0.490931f; emit = true;
                            break;
                        case mdl::BoneType::UnderIk:
                            if ((flags & mdl::kBoneFlagFixedAxis) != 0) {
                                v0 = 0.77328497f;
                                v1 = 0.80788797f;
                            } else {
                                v0 = 0.42117199f;
                                v1 = 0.45877501f;
                            }
                            emit = true;
                            break;
                        case mdl::BoneType::RotateGrant:
                        case mdl::BoneType::Effector:
                            v0 = 0.0f; v1 = 0.033902999f; emit = true;
                            break;
                        case mdl::BoneType::FixedAxis:
                            v0 = 0.49148399f; v1 = 0.52608699f; emit = true;
                            break;
                        default:
                            break;
                        }
                    }
                    if (emit) {
                        float left = 0.0f;
                        float top = 0.0f;
                        float right = 0.0f;
                        float bottom = 0.0f;
                        BoneMarkerBounds(boneResult.selectedX,
                                         boneResult.selectedY, scale,
                                         &left, &top, &right, &bottom);
                        append(left, top, right, bottom,
                               0.46414399f, v0, 0.49874699f, v1);
                    }
                }
            }

            boneResult.spritePrimitiveCount =
                app->SpriteOverlayPrimitiveCount() -
                spriteCountBeforeBone;
            app->ViewportToolCenterX() = boneResult.selectedX;
            app->ViewportToolCenterY() = boneResult.selectedY;
            app->state.selectedClipW = boneResult.selectedClipW;
        }
    }

    if (app->state.optflag[0] != 0 &&
        app->PlaybackActive() == 0 &&
        app->CameraParentModel() < 0) {
        const float cx = static_cast<float>((view.left + view.right) / 2);
        const float cy = static_cast<float>((view.top + view.bottom) / 2);
        append(cx - scale * 9.5f, cy - scale * 9.5f,
               cx + scale * 8.5f, cy + scale * 8.5f,
               0.46414399f, 0.35086f, 0.49874699f, 0.385463f);
    }

    const bool playback = app->PlaybackActive() != 0;
    if (!playback) {
        float targetV0 = 0.470705f;
        float targetV1 = 0.52734601f;
        const int target = app->state.coordinateSystem;
        if (target == 1) {
            targetV0 = 0.52929902f;
            targetV1 = 0.58594f;
        } else if (target == 2) {
            targetV0 = 0.58789301f;
            targetV1 = 0.64453399f;
        }
        append(right - scale * 130.0f, bottom - scale * 115.0f,
               right - scale * 10.0f, bottom - scale * 85.0f,
               0.001953f, targetV0, 0.23242199f, targetV1);
    }

    if (!playback) {
        if (app->state.optflag[0] != 0) {
            append(right - scale * 130.0f, bottom - scale * 80.0f,
                   right - scale * 10.0f, bottom - scale * 50.0f,
                   0.001953f, 0.41210899f, 0.23242199f, 0.46875f);
            append(right - scale * 130.0f, bottom - scale * 40.0f,
                   right - scale * 10.0f, bottom - scale * 10.0f,
                   0.001953f, 0.177734f, 0.23242199f, 0.234375f);
        } else {
            const int boneMode = static_cast<std::int32_t>(app->state.v32c);
            if (boneMode >= 0 && boneMode <= 3) {
                const bool alternate = (boneMode & 1) != 0;
                const bool lowerPair = boneMode >= 2;
                append(right - scale * 130.0f, bottom - scale * 80.0f,
                       right - scale * 10.0f, bottom - scale * 50.0f,
                       0.001953f,
                       lowerPair ? 0.35351601f : 0.29492199f,
                       0.23242199f,
                       lowerPair ? 0.41015601f : 0.35156301f);
                append(right - scale * 130.0f, bottom - scale * 40.0f,
                       right - scale * 10.0f, bottom - scale * 10.0f,
                       0.001953f,
                       alternate ? 0.119141f : 0.060547002f,
                       0.23242199f,
                       alternate ? 0.175781f : 0.117188f);
            }
        }

        const int operation = static_cast<int>(app->ViewportToolOperation());
        const auto highlight = [&](int first, int second, int column,
                                   int row) {
            if (operation != first && operation != second)
                return;
            const float x0 = right - scale * (130.0f - 40.0f * column);
            const float y0 = bottom - scale * (80.0f - 40.0f * row);
            const float u0 = 0.001953f + 0.078125f * column;
            const float v0 = row == 0 ? 0.23632801f : 0.001953f;
            append(x0, y0, x0 + scale * 40.0f, y0 + scale * 30.0f,
                   u0, v0,
                   column == 2 ? 0.23242199f : u0 + 0.078125f,
                   row == 0 ? 0.29296899f : 0.058594f);
        };
        highlight(9, 18, 0, 0);
        highlight(10, 19, 1, 0);
        highlight(11, 20, 2, 0);
        highlight(12, 15, 0, 1);
        highlight(13, 16, 1, 1);
        highlight(14, 17, 2, 1);

        if (app->state.optflag[0] == 0) {
            const float x = static_cast<float>(app->ViewportToolCenterX());
            const float y = static_cast<float>(app->ViewportToolCenterY());
            const int cursorKind = static_cast<int>(app->EditMode());
            if (cursorKind == 3) {
                float u0 = 0.5f;
                float v0 = 0.5f;
                if (operation == 3) {
                    v0 = 0.75f;
                } else if (operation == 4) {
                    u0 = 0.75f;
                } else if (operation == 5) {
                    u0 = 0.75f;
                    v0 = 0.75f;
                }
                append(x - 45.0f, y - 45.0f, x + 45.0f, y + 45.0f,
                       u0, v0, u0 + 0.25f, v0 + 0.25f);
            } else if (cursorKind == 4) {
                float u0 = 0.5f;
                float v0 = 0.0f;
                if (operation == 6) {
                    v0 = 0.25f;
                } else if (operation == 7) {
                    u0 = 0.75f;
                } else if (operation == 8) {
                    u0 = 0.75f;
                    v0 = 0.25f;
                }
                append(x - 13.0f, y - 67.0f, x + 62.0f, y + 8.0f,
                       u0, v0, u0 + 0.25f, v0 + 0.25f);
            }
        }
    }

    DumpVertexBatch("sprite", spriteBegin,
        app->SpriteOverlayPrimitiveCount(), 84);
    vb->Unlock();
}

}  
