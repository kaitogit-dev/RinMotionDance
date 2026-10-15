#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>

#include "fx_slots.hpp"
#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mme_bridge.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

using Matrix = d3dx::D3DXMATRIXF;

#ifdef RINMOTIONDANCE_DIAG
struct MaterialStateCapture {
    FILE* stream = nullptr;
    bool first = true;
    unsigned sequence = 0;
};

MaterialStateCapture g_materialCapture;

bool IsRegularCaptureFile(const char* path) {
    const DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

void WriteWideHex(FILE* stream, const wchar_t* value, std::size_t limit) {
    std::fputc('"', stream);
    if (value != nullptr) {
        for (std::size_t i = 0; i < limit && value[i] != L'\0'; ++i)
            std::fprintf(stream, "%04X", static_cast<unsigned>(value[i]));
    }
    std::fputc('"', stream);
}

bool BeginMaterialStateCapture() {
    static LONG captured = 0;
    if (InterlockedCompareExchange(&captured, 0, 0) != 0)
        return false;

    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "RINMOTIONDANCE_VB_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return false;

    char stable[2]{};
    if (GetEnvironmentVariableA("RINMOTIONDANCE_AB_STABLE_CAPTURE", stable,
                                sizeof(stable)) == 1 && stable[0] == '1') {
        char requireLine[2]{};
        const bool refreshed =
            GetEnvironmentVariableA("RINMOTIONDANCE_AB_REQUIRE_LINE", requireLine,
                                    sizeof(requireLine)) == 1 &&
            requireLine[0] == '1';
        char gate[MAX_PATH]{};
        std::snprintf(gate, sizeof(gate), "%s\\%s", directory,
                      refreshed ? "vb.capture.active" : "vb.capture.ready");
        if (!IsRegularCaptureFile(gate))
            return false;
    }
    if (InterlockedCompareExchange(&captured, 1, 0) != 0)
        return false;

    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\material_states.json", directory);
    g_materialCapture.stream = std::fopen(path, "wb");
    if (g_materialCapture.stream == nullptr)
        return false;
    g_materialCapture.first = true;
    g_materialCapture.sequence = 0;
    std::fputs("{\"schema\":1,\"renderer\":\"fixed\",\"materials\":[\n",
               g_materialCapture.stream);
    return true;
}

DWORD TextureStageState(IDirect3DDevice9* device, DWORD stage,
                        D3DTEXTURESTAGESTATETYPE type) {
    DWORD value = 0;
    device->GetTextureStageState(stage, type, &value);
    return value;
}

DWORD SamplerState(IDirect3DDevice9* device, DWORD stage,
                   D3DSAMPLERSTATETYPE type) {
    DWORD value = 0;
    device->GetSamplerState(stage, type, &value);
    return value;
}

DWORD RenderState(IDirect3DDevice9* device, D3DRENDERSTATETYPE type) {
    DWORD value = 0;
    device->GetRenderState(type, &value);
    return value;
}

void DumpMaterialState(MMDApp* app, IDirect3DDevice9* device,
                       unsigned char* model, unsigned char* material,
                       UINT materialIndex, UINT firstIndex) {
    FILE* stream = g_materialCapture.stream;
    if (stream == nullptr)
        return;

    int slot = -1;
    for (int i = 0; i < kModelSlotCount; ++i) {
        if (app->ModelSlot(i) == model) {
            slot = i;
            break;
        }
    }
    const mdl::ModelMaterialRecord& record = mdl::Material(material);
    DWORD alphaBits = 0;
    std::memcpy(&alphaBits, &record.diffuse[3], sizeof(alphaBits));
    std::fprintf(stream,
        "%s{\"sequence\":%u,\"model_slot\":%d,\"model_order\":%u,"
        "\"material\":%u,\"first_index\":%u,\"index_count\":%u,"
        "\"model_format\":%u,\"alpha_bits\":\"%08X\","
        "\"toon_index\":%d,\"edge_flag\":%u,\"pmx_flags\":%u,"
        "\"sphere_mode\":%u,\"main_path_utf16\":",
        g_materialCapture.first ? "" : ",\n", g_materialCapture.sequence++,
        slot, static_cast<unsigned>(mdl::Mdl(model)->comboSelIndex), materialIndex, firstIndex,
        record.faceVertexCount, static_cast<unsigned>(mdl::Mdl(model)->physicsMode),
        alphaBits, static_cast<int>(
            static_cast<std::int8_t>(record.toonReference)),
        static_cast<unsigned>(record.doubleSided),
        static_cast<unsigned>(record.flags),
        static_cast<unsigned>(record.sphereMode));
    WriteWideHex(stream, record.texturePath, 280);
    std::fputs(",\"sphere_path_utf16\":", stream);
    WriteWideHex(stream, record.spherePath, 280);
    std::fputs(",\"stages\":[", stream);

    for (DWORD stage = 0; stage < 3; ++stage) {
        IDirect3DBaseTexture9* texture = nullptr;
        const HRESULT textureResult = device->GetTexture(stage, &texture);
        const bool bound = SUCCEEDED(textureResult) && texture != nullptr;
        if (texture != nullptr)
            texture->Release();
        D3DMATRIX transform{};
        device->GetTransform(static_cast<D3DTRANSFORMSTATETYPE>(
                                 D3DTS_TEXTURE0 + stage), &transform);
        DWORD matrixBits[16]{};
        std::memcpy(matrixBits, &transform, sizeof(matrixBits));
        std::fprintf(stream,
            "%s{\"stage\":%u,\"role\":\"%s\",\"bound\":%s,"
            "\"color_op\":%u,\"color_arg1\":%u,\"color_arg2\":%u,"
            "\"alpha_op\":%u,\"alpha_arg1\":%u,\"alpha_arg2\":%u,"
            "\"texcoord_index\":%u,\"transform_flags\":%u,"
            "\"address_u\":%u,\"address_v\":%u,\"mag_filter\":%u,"
            "\"min_filter\":%u,\"mip_filter\":%u,\"transform_bits\":[",
            stage == 0 ? "" : ",", stage,
            stage == 0 ? "toon" : stage == 1 ? "cascade1" : "cascade2",
            bound ? "true" : "false",
            TextureStageState(device, stage, D3DTSS_COLOROP),
            TextureStageState(device, stage, D3DTSS_COLORARG1),
            TextureStageState(device, stage, D3DTSS_COLORARG2),
            TextureStageState(device, stage, D3DTSS_ALPHAOP),
            TextureStageState(device, stage, D3DTSS_ALPHAARG1),
            TextureStageState(device, stage, D3DTSS_ALPHAARG2),
            TextureStageState(device, stage, D3DTSS_TEXCOORDINDEX),
            TextureStageState(device, stage, D3DTSS_TEXTURETRANSFORMFLAGS),
            SamplerState(device, stage, D3DSAMP_ADDRESSU),
            SamplerState(device, stage, D3DSAMP_ADDRESSV),
            SamplerState(device, stage, D3DSAMP_MAGFILTER),
            SamplerState(device, stage, D3DSAMP_MINFILTER),
            SamplerState(device, stage, D3DSAMP_MIPFILTER));
        for (int word = 0; word < 16; ++word)
            std::fprintf(stream, "%s\"%08X\"", word == 0 ? "" : ",",
                         matrixBits[word]);
        std::fputs("]}", stream);
    }
    std::fprintf(stream,
        "],\"render_states\":{\"cull_mode\":%u,\"z_func\":%u,"
        "\"alpha_blend\":%u,\"alpha_test\":%u,\"src_blend\":%u,"
        "\"dest_blend\":%u,\"lighting\":%u},"
        "\"light_bits\":[",
        RenderState(device, D3DRS_CULLMODE),
        RenderState(device, D3DRS_ZFUNC),
        RenderState(device, D3DRS_ALPHABLENDENABLE),
        RenderState(device, D3DRS_ALPHATESTENABLE),
        RenderState(device, D3DRS_SRCBLEND),
        RenderState(device, D3DRS_DESTBLEND),
        RenderState(device, D3DRS_LIGHTING));
    DWORD lightBits[sizeof(D3DLIGHT9) / sizeof(DWORD)]{};
    const D3DLIGHT9& sceneLight = app->SceneLight();
    std::memcpy(lightBits, &sceneLight, sizeof(lightBits));
    for (int word = 0; word < 24; ++word) {
        std::fprintf(stream, "%s\"%08X\"", word == 0 ? "" : ",",
                     lightBits[word]);
    }
    std::fputs("],\"toon_ptrs\":[", stream);
    for (int slot = 0; slot < 11; ++slot) {
        std::fprintf(stream, "%s\"%08X\"", slot == 0 ? "" : ",",
                     static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(
                         app->ToonTexture(slot))));
    }
    std::fputs("]}", stream);
    g_materialCapture.first = false;
}

void EndMaterialStateCapture() {
    if (g_materialCapture.stream == nullptr)
        return;
    std::fputs("\n]}\n", g_materialCapture.stream);
    std::fclose(g_materialCapture.stream);
    g_materialCapture.stream = nullptr;
}
#else
inline bool BeginMaterialStateCapture() { return false; }
inline void DumpMaterialState(MMDApp*, IDirect3DDevice9*, unsigned char*,
                              unsigned char*, UINT, UINT) {}
inline void EndMaterialStateCapture() {}
#endif


void Identity(Matrix* out) {
    std::memset(out, 0, sizeof(*out));
    out->m[0][0] = out->m[1][1] = out->m[2][2] = out->m[3][3] = 1.0f;
}

Matrix HalfTexelMatrix() {
    Matrix m;
    Identity(&m);
    m.m[0][0] = 0.5f;
    m.m[1][1] = -0.5f;
    m.m[2][2] = 0.0f;
    m.m[3][0] = 0.5f;
    m.m[3][1] = 0.5f;
    return m;
}

void Multiply(Matrix* out, const Matrix* a, const Matrix* b) {
    d3dx::Get().multiply(out, a, b);
}

IDirect3DTexture9* FindCachedTexture(D3DRenderer* sub, const wchar_t* path) {
    if (path == nullptr || path[0] == L'\0')
        return nullptr;
    for (int i = 0; i < 10000; ++i) {
        wchar_t* name = static_cast<wchar_t*>(
            sub->resourcePool[i].heapBuffer);
        if (name == nullptr)
            return nullptr;
        if (wcscmp(name, path) == 0)
            return reinterpret_cast<IDirect3DTexture9*>(
                sub->resourcePool[i].comObject);
    }
    return nullptr;
}

void CachedTextureColor(D3DRenderer* sub, const wchar_t* path, float out[4]) {
    out[0] = out[1] = out[2] = out[3] = 1.0f;
    if (path == nullptr || path[0] == L'\0')
        return;
    for (int i = 0; i < 10000; ++i) {
        const wchar_t* name = static_cast<const wchar_t*>(
            sub->resourcePool[i].heapBuffer);
        if (name == nullptr)
            return;
        if (wcscmp(name, path) == 0) {
            const unsigned char* tag = reinterpret_cast<const unsigned char*>(
                &sub->resourcePool[i].tag);
            out[0] = tag[0] * (1.0f / 256.0f);
            out[1] = tag[1] * (1.0f / 256.0f);
            out[2] = tag[2] * (1.0f / 256.0f);
            return;
        }
    }
}

void ModelVertexFormat(unsigned char* model, DWORD* fvf, UINT* stride) {
    switch (mdl::Mdl(model)->pmxAdditionalUvCount) {
    case 1: *fvf = 524818;   *stride = 48; break;
    case 2: *fvf = 2622226;  *stride = 64; break;
    case 3: *fvf = 11011090; *stride = 80; break;
    case 4: *fvf = 44565778; *stride = 96; break;
    default: *fvf = 274;     *stride = 32; break;
    }
}

IDirect3DTexture9* ToonTexture(MMDApp* app, D3DRenderer* sub,
                               unsigned char* model,
                               unsigned char* material) {
    const int index = static_cast<std::int8_t>(
        mdl::Material(material).toonReference);
    if (index == -1)
        return app->ToonTexture(0);
    if (index >= 0 && index < 10) {
        static const char* names[10] = {
            "toon01.bmp", "toon02.bmp", "toon03.bmp", "toon04.bmp",
            "toon05.bmp", "toon06.bmp", "toon07.bmp", "toon08.bmp",
            "toon09.bmp", "toon10.bmp"};
        if (strcmp(mdl::PmdToonFileNames(model)[index],
                   names[index]) == 0)
            return app->ToonTexture(index + 1);
    }
    if (mdl::Mdl(model)->physicsMode == 2) {
        return FindCachedTexture(sub,
                                 mdl::Material(material).toonPath);
    }
    if (index >= 0 && index < 10) {
        wchar_t converted[256] = {};
        wchar_t path[256] = {};
        ConvertAnsiToWide(reinterpret_cast<D3DRenderer*>(sub), mdl::PmdToonFileNames(model)[index],
                          converted, 0x100);
        swprintf_s(path, 0x100, L"%s%s",
                   mdl::Mdl(model)->modelDirectory, converted);
        return FindCachedTexture(sub, path);
    }
    return app->ToonTexture(0);
}

bool HasSuffix(const wchar_t* value, const wchar_t* lower,
               const wchar_t* upper);

void Cross3(float out[3], const float left[3], const float right[3]) {
#if defined(_M_IX86)
    float* destination = out;
    const float* lhs = left;
    const float* rhs = right;
    __asm {
        mov eax, lhs
        mov ecx, rhs
        mov edx, destination

        fld dword ptr [eax + 4]
        fmul dword ptr [ecx + 8]
        fld dword ptr [eax + 8]
        fmul dword ptr [ecx + 4]
        fsubp st(1), st(0)
        fstp dword ptr [edx]

        fld dword ptr [eax + 8]
        fmul dword ptr [ecx]
        fld dword ptr [eax]
        fmul dword ptr [ecx + 8]
        fsubp st(1), st(0)
        fstp dword ptr [edx + 4]

        fld dword ptr [eax]
        fmul dword ptr [ecx + 4]
        fld dword ptr [eax + 4]
        fmul dword ptr [ecx]
        fsubp st(1), st(0)
        fstp dword ptr [edx + 8]
    }
#else
    out[0] = static_cast<float>(
        static_cast<double>(left[1]) * right[2] -
        static_cast<double>(left[2]) * right[1]);
    out[1] = static_cast<float>(
        static_cast<double>(left[2]) * right[0] -
        static_cast<double>(left[0]) * right[2]);
    out[2] = static_cast<float>(
        static_cast<double>(left[0]) * right[1] -
        static_cast<double>(left[1]) * right[0]);
#endif
}

void BuildToonTransform(MMDApp* app, Matrix* result) {
    auto& api = d3dx::Get();
    const D3DVECTOR& lightDirection = app->SceneLight().Direction;
    float direction[3] = {
        -lightDirection.x, -lightDirection.y, -lightDirection.z};
    api.vec3Normalize(direction, direction);

    const float vertical[3] = {
        0.0f, direction[0] == 0.0f && direction[2] == 0.0f ? 0.0f : 1.0f,
        direction[0] == 0.0f && direction[2] == 0.0f ? -1.0f : 0.0f};
    float horizontal[3]{};
    Cross3(horizontal, vertical, direction);
    api.vec3Normalize(horizontal, horizontal);
    float correctedVertical[3]{};
    Cross3(correctedVertical, direction, horizontal);
    api.vec3Normalize(correctedVertical, correctedVertical);

    Matrix basis{};
    for (int row = 0; row < 3; ++row) {
        basis.m[row][0] = horizontal[row];
        basis.m[row][1] = correctedVertical[row];
        basis.m[row][2] = direction[row];
    }
    basis.m[3][3] = 1.0f;

    Matrix rotation;
    Matrix scaling;
    Matrix translation;
    Matrix temporary;
    Matrix projected;
    constexpr float kRotationAngle = -(3.141592f * 0.5f);
    static_assert(__builtin_bit_cast(std::uint32_t, kRotationAngle) ==
                      0xBFC90FD8u,
                  "flt_530E00 bit-exact");
    float rotationAngle = kRotationAngle;                 
    api.rotX(&rotation, rotationAngle);
    api.scaling(&scaling, 0.5f, -0.5f, 1.0f);
    api.translation(&translation, 0.5f, 0.5f, 0.0f);
    api.multiply(&temporary, &basis, &rotation);
    api.multiply(&projected, &temporary, &scaling);
    api.multiply(result, &projected, &translation);
}

void DisableSphereTextureStage(IDirect3DDevice9* device) {
    device->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
}

void ConfigureMaterialStages(MMDApp* app, D3DRenderer* sub,
                             IDirect3DDevice9* device,
                             unsigned char* model,
                             unsigned char* material) {
    const mdl::ModelMaterialRecord& record = mdl::Material(material);
    const wchar_t* mainPath = record.texturePath;
    const wchar_t* spherePath = record.spherePath;
    DisableSphereTextureStage(device);

    if (mdl::Mdl(model)->physicsMode == 2) {
        const Matrix halfTexel = HalfTexelMatrix();
        Matrix identityTexel;
        Identity(&identityTexel);
        const auto setStageTransform = [&](DWORD stage, const Matrix& m) {
            device->SetTransform(static_cast<D3DTRANSFORMSTATETYPE>(
                                     D3DTS_TEXTURE0 + stage),
                                 reinterpret_cast<const D3DMATRIX*>(&m));
        };
        if (mainPath[0] != L'\0') {
            device->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
            device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
            device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
            device->SetTexture(1, FindCachedTexture(sub, mainPath));
            const unsigned char sphereMode = record.sphereMode;
            if (spherePath[0] != L'\0' && sphereMode != 0) {
                switch (sphereMode) {
                case 1:
                    setStageTransform(2, halfTexel);
                    device->SetTextureStageState(2, D3DTSS_COLOROP,
                                                 D3DTOP_MODULATE);
                    device->SetTextureStageState(2,
                                                 D3DTSS_TEXTURETRANSFORMFLAGS,
                                                 D3DTTFF_COUNT2);
                    device->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX,
                                                 0x10000);
                    break;
                case 2:
                    setStageTransform(2, halfTexel);
                    device->SetTextureStageState(2, D3DTSS_COLOROP,
                                                 D3DTOP_ADD);
                    device->SetTextureStageState(2,
                                                 D3DTSS_TEXTURETRANSFORMFLAGS,
                                                 D3DTTFF_COUNT2);
                    device->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX,
                                                 0x10000);
                    break;
                case 3:
                    setStageTransform(2, identityTexel);
                    device->SetTextureStageState(2, D3DTSS_COLOROP,
                                                 D3DTOP_MODULATE);
                    device->SetTextureStageState(2,
                                                 D3DTSS_TEXTURETRANSFORMFLAGS,
                                                 D3DTTFF_COUNT2);
                    device->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 1);
                    break;
                default:
                    break;  
                }
                device->SetTexture(2, FindCachedTexture(sub, spherePath));
            }
        } else if (spherePath[0] != L'\0' && record.sphereMode != 0) {
            const unsigned char sphereMode = record.sphereMode;
            switch (sphereMode) {
            case 1:
                setStageTransform(1, halfTexel);
                device->SetTextureStageState(1, D3DTSS_COLOROP,
                                             D3DTOP_MODULATE);
                device->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS,
                                             D3DTTFF_COUNT2);
                device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX,
                                             0x10000);
                break;
            case 2:
                setStageTransform(1, halfTexel);
                device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_ADD);
                device->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS,
                                             D3DTTFF_COUNT2);
                device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX,
                                             0x10000);
                break;
            case 3:
                setStageTransform(1, identityTexel);
                device->SetTextureStageState(1, D3DTSS_COLOROP,
                                             D3DTOP_MODULATE);
                device->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS,
                                             D3DTTFF_COUNT2);
                device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
                break;
            default:
                break;
            }
            device->SetTexture(1, FindCachedTexture(sub, spherePath));
        } else {
            device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        }
    } else if (mainPath[0] == L'\0') {
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    } else if (HasSuffix(mainPath, L".sph", L".SPH")) {
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        device->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS,
                                     D3DTTFF_COUNT2);
        device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0x10000);
        device->SetTexture(1, FindCachedTexture(sub, mainPath));
    } else if (HasSuffix(mainPath, L".spa", L".SPA")) {
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_ADD);
        device->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS,
                                     D3DTTFF_COUNT2);
        device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0x10000);
        device->SetTexture(1, FindCachedTexture(sub, mainPath));
    } else {
        device->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
        device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        device->SetTexture(1, FindCachedTexture(sub, mainPath));
    }

    if (mdl::Mdl(model)->physicsMode != 2) {
        if (spherePath[0] != L'\0') {
            device->SetTextureStageState(2, D3DTSS_COLOROP,
                HasSuffix(spherePath, L".sph", L".SPH") ? D3DTOP_MODULATE
                                                        : D3DTOP_ADD);
            const Matrix halfTexel = HalfTexelMatrix();
            device->SetTransform(D3DTS_TEXTURE2,
                                 reinterpret_cast<const D3DMATRIX*>(&halfTexel));
            device->SetTextureStageState(2, D3DTSS_TEXTURETRANSFORMFLAGS,
                                         D3DTTFF_COUNT2);
            device->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 0x10000);
            device->SetTexture(2, FindCachedTexture(sub, spherePath));
        }
    }

    Matrix toonTransform;
    BuildToonTransform(app, &toonTransform);
    device->SetTransform(D3DTS_TEXTURE0,
                          reinterpret_cast<const D3DMATRIX*>(&toonTransform));
    device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,
                                 D3DTTFF_COUNT2);
    device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0x10000);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetTexture(0, ToonTexture(app, sub, model, material));
}

void ResetSphereStageAfterDraw(IDirect3DDevice9* device,
                               unsigned char* material) {
    const wchar_t* mainPath = mdl::Material(material).texturePath;
    if (HasSuffix(mainPath, L".sp", L".SP")) {
        device->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
        device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
    }
}

bool HasSuffix(const wchar_t* value, const wchar_t* lower,
               const wchar_t* upper) {
    return value != nullptr &&
           (wcsstr(value, lower) != nullptr || wcsstr(value, upper) != nullptr);
}

void SetEffectColor(void* effect, const char* name, const float source[4]) {
    float value[4] = {source[0], source[1], source[2], source[3]};
    for (float& channel : value)
        channel = std::min(channel, 1.0f);
    fx::SetFloatArray(effect, name, value, 4);
}

void SelectPmxTechnique(D3DRenderer* sub, IDirect3DDevice9* device,
                        void* effect, unsigned char* material) {
    const mdl::ModelMaterialRecord& record = mdl::Material(material);
    const wchar_t* mainPath = record.texturePath;
    const wchar_t* spherePath = record.spherePath;
    IDirect3DTexture9* mainTexture = FindCachedTexture(sub, mainPath);
    const unsigned char sphereMode = record.sphereMode;

    device->SetTexture(1, mainTexture);
    device->SetTexture(2, nullptr);
    if (mainTexture != nullptr) {
        IDirect3DTexture9* sphereTexture =
            sphereMode != 0 && spherePath[0] != L'\0'
                ? FindCachedTexture(sub, spherePath)
                : nullptr;
        if (sphereTexture != nullptr) {
            device->SetTexture(2, sphereTexture);
            if (sphereMode == 1 || sphereMode == 2) {
                fx::SetTechnique(effect, "BShadowSphiaTextureTec");
                fx::SetBool(effect, "spadd", sphereMode == 2);
            } else if (sphereMode == 3) {
                fx::SetTechnique(effect, "BShadowTextureTexCd2Tec");
            } else {
                fx::SetTechnique(effect, "BShadowTextureTec");
            }
        } else {
            fx::SetTechnique(effect, "BShadowTextureTec");
        }
        return;
    }

    IDirect3DTexture9* sphereTexture =
        sphereMode != 0 && spherePath[0] != L'\0'
            ? FindCachedTexture(sub, spherePath)
            : nullptr;
    if (sphereTexture == nullptr) {
        fx::SetTechnique(effect, "BufferShadowTec");
        return;
    }
    device->SetTexture(1, sphereTexture);
    if (sphereMode == 1 || sphereMode == 2) {
        fx::SetTechnique(effect, "BShadowSphiaTec");
        fx::SetBool(effect, "spadd", sphereMode == 2);
    } else if (sphereMode == 3) {
        fx::SetTechnique(effect, "BShadowTexCd2Tec");
    } else {
        fx::SetTechnique(effect, "BufferShadowTec");
    }
}

void SelectPmdTechnique(D3DRenderer* sub, IDirect3DDevice9* device,
                        void* effect, unsigned char* material) {
    const mdl::ModelMaterialRecord& record = mdl::Material(material);
    const wchar_t* mainPath = record.texturePath;
    const wchar_t* spherePath = record.spherePath;
    if (mainPath[0] == L'\0') {
        device->SetTexture(1, nullptr);
        device->SetTexture(2, nullptr);
        fx::SetTechnique(effect, "BufferShadowTec");
        return;
    }

    if (HasSuffix(mainPath, L".sph", L".SPH") ||
        HasSuffix(mainPath, L".spa", L".SPA")) {
        IDirect3DTexture9* sphereTexture = FindCachedTexture(sub, mainPath);
        device->SetTexture(1, sphereTexture);
        device->SetTexture(2, nullptr);
        if (sphereTexture != nullptr) {
            fx::SetTechnique(effect, "BShadowSphiaTec");
            fx::SetBool(effect, "spadd",
                     HasSuffix(mainPath, L".spa", L".SPA") ? 1 : 0);
        } else {
            fx::SetTechnique(effect, "BufferShadowTec");
        }
        return;
    }

    IDirect3DTexture9* mainTexture = FindCachedTexture(sub, mainPath);
    device->SetTexture(1, mainTexture);
    device->SetTexture(2, nullptr);
    if (mainTexture == nullptr) {
        fx::SetTechnique(effect, "BufferShadowTec");
        return;
    }
    IDirect3DTexture9* sphereTexture = spherePath[0] != L'\0'
        ? FindCachedTexture(sub, spherePath) : nullptr;
    if (sphereTexture != nullptr) {
        device->SetTexture(2, sphereTexture);
        fx::SetTechnique(effect, "BShadowSphiaTextureTec");
        fx::SetBool(effect, "spadd",
                 HasSuffix(spherePath, L".spa", L".SPA") ? 1 : 0);
    } else {
        fx::SetTechnique(effect, "BShadowTextureTec");
    }
}

void ConfigureEffectMaterial(MMDApp* app, D3DRenderer* sub,
                             IDirect3DDevice9* device, void* effect,
                             unsigned char* model,
                             unsigned char* material, UINT materialIndex) {
    const mdl::ModelMaterialRecord& record = mdl::Material(material);
    const float alpha = record.diffuse[3];
    const float diffuse[4] = {record.diffuse[0], record.diffuse[1],
                              record.diffuse[2], alpha};
    const float ambient[4] = {record.diffuseMirror[0], record.diffuseMirror[1],
                              record.diffuseMirror[2], alpha};
    const float emissive[4] = {record.ambient[0], record.ambient[1],
                               record.ambient[2], alpha};
    const float specular[4] = {record.specular[0], record.specular[1],
                               record.specular[2], alpha};
    const D3DCOLORVALUE& lightAmbient = app->SceneLight().Ambient;
    const float edge[4] = {
        ambient[0] * lightAmbient.r + emissive[0],
        ambient[1] * lightAmbient.g + emissive[1],
        ambient[2] * lightAmbient.b + emissive[2], alpha};
    SetEffectColor(effect, "EgColor", edge);
    SetEffectColor(effect, "MatDifColor", diffuse);
    SetEffectColor(effect, "MatAmbColor", ambient);
    SetEffectColor(effect, "MatEmsColor", emissive);
    SetEffectColor(effect, "MatSpcColor", specular);

    float toon[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const int toonIndex = static_cast<std::int8_t>(record.toonReference);
    if (toonIndex != -1) {
        bool standard = false;
        if (toonIndex >= 0 && toonIndex < 10) {
            static const char* names[10] = {
                "toon01.bmp", "toon02.bmp", "toon03.bmp", "toon04.bmp",
                "toon05.bmp", "toon06.bmp", "toon07.bmp", "toon08.bmp",
                "toon09.bmp", "toon10.bmp"};
            standard = strcmp(mdl::PmdToonFileNames(model)[toonIndex],
                              names[toonIndex]) == 0;
            if (standard) {
                const float* table = app->state.toonEdgeTable;
                toon[0] = table[3 * toonIndex + 0];
                toon[1] = table[3 * toonIndex + 1];
                toon[2] = table[3 * toonIndex + 2];
            }
        }
        if (!standard) {
            if (mdl::Mdl(model)->physicsMode == 2) {
                CachedTextureColor(sub, record.toonPath, toon);
            } else if (toonIndex >= 0 && toonIndex < 10) {
                wchar_t converted[256] = {};
                wchar_t path[256] = {};
                ConvertAnsiToWide(reinterpret_cast<D3DRenderer*>(sub), mdl::PmdToonFileNames(model)[toonIndex],
                                  converted, 0x100);
                swprintf_s(path, 0x100, L"%s%s",
                    mdl::Mdl(model)->modelDirectory,
                    converted);
                CachedTextureColor(sub, path, toon);
            }
        }
    }
    if (mdl::Mdl(model)->physicsMode == 2) {
        const mdl::MaterialMorphChannels& add =
            mdl::MaterialMorphAdd(model)[materialIndex].channels;
        const mdl::MaterialMorphChannels& mul =
            mdl::MaterialMorphMul(model)[materialIndex].channels;
        for (int i = 0; i < 3; ++i)
            toon[i] = toon[i] * mul.toonTint[i] + add.toonTint[i];
        toon[3] = add.toonTint[3] + mul.toonTint[3];
    }
    fx::SetFloatArray(effect, "ToonColor", toon, 4);

    const D3DCOLORVALUE& lightSpecular = app->SceneLight().Specular;
    float litSpecular[4] = {
        specular[0] * lightSpecular.r, specular[1] * lightSpecular.g,
        specular[2] * lightSpecular.b, record.specularPower};
    if (litSpecular[3] == 0.0f)
        litSpecular[3] = 0.1f;
    fx::SetFloatArray(effect, "SpcColor", litSpecular, 4);

    const float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (mdl::Mdl(model)->physicsMode == 2) {
        const mdl::MaterialMorphChannels& add =
            mdl::MaterialMorphAdd(model)[materialIndex].channels;
        const mdl::MaterialMorphChannels& mul =
            mdl::MaterialMorphMul(model)[materialIndex].channels;
        fx::SetFloatArray(effect, "TexCAdd",
                        add.textureTint, 4);
        fx::SetFloatArray(effect, "TexCMul",
                        mul.textureTint, 4);
        fx::SetFloatArray(effect, "SphCAdd",
                        add.sphereTint, 4);
        fx::SetFloatArray(effect, "SphCMul",
                        mul.sphereTint, 4);
        SelectPmxTechnique(sub, device, effect, material);
    } else {
        fx::SetFloatArray(effect, "TexCAdd", zero, 4);
        fx::SetFloatArray(effect, "TexCMul", one, 4);
        fx::SetFloatArray(effect, "SphCAdd", zero, 4);
        fx::SetFloatArray(effect, "SphCMul", one, 4);
        SelectPmdTechnique(sub, device, effect, material);
    }
}

void DrawModelMaterials(MMDApp* app, unsigned char* model, bool effectPass,
                        bool shadowOnly) {
    if (model == nullptr || mdl::Mdl(model)->loadComplete == 0)
        return;
    mdl::ModelRecord& state = *mdl::Mdl(model);
    D3DRenderer* sub = app->Renderer();
    auto* device = sub->device;
    auto* effect = sub->effect;
    auto* materials = mdl::Materials(model);
    auto* vertices = mdl::ResourceAs<IDirect3DVertexBuffer9>(
        mdl::Mdl(model)->vertexBuffer);
    auto* indices = mdl::ResourceAs<IDirect3DIndexBuffer9>(
        mdl::Mdl(model)->indexBuffer);
    if (device == nullptr || materials == nullptr || vertices == nullptr ||
        indices == nullptr)
        return;

    DWORD fvf;
    UINT stride;
    ModelVertexFormat(model, &fvf, &stride);
    if (!effectPass && !shadowOnly) {
        device->SetRenderState(D3DRS_LIGHTING, TRUE);
        const Matrix halfTexel = HalfTexelMatrix();
        device->SetTransform(D3DTS_TEXTURE1,
                             reinterpret_cast<const D3DMATRIX*>(&halfTexel));
    }
    UINT firstIndex = 0;
    state.toonShared = static_cast<std::uint32_t>(-1);
    for (UINT i = 0; i < state.materialCount; ++i) {
        unsigned char* material = reinterpret_cast<unsigned char*>(&materials[i]);
        const mdl::ModelMaterialRecord& record = materials[i];
        const UINT indexCount = static_cast<UINT>(record.faceVertexCount);
        ++state.toonShared;
        if (indexCount == 0)
            continue;

        bool draw = true;
        if (shadowOnly) {
            if (mdl::Mdl(model)->physicsMode == 2 && record.edgeColor[3] <= 0.0f)
                draw = false;
            if (record.diffuse[3] == 0.9800000190734863f)
                draw = false;
            if (mdl::Mdl(model)->physicsMode == 2 && (record.flags >> 2 & 1) == 0)
                draw = false;
        }
        if (draw) {
            D3DMATERIAL9 d3dMaterial{};
            std::memcpy(&d3dMaterial, material, sizeof(d3dMaterial));
            if (state.displayState != 0)
                d3dMaterial.Diffuse.a = 0.5f;

            if (!effectPass && !shadowOnly) {
                device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                device->SetRenderState(D3DRS_DESTBLEND,
                                       D3DBLEND_INVSRCALPHA);
                ConfigureMaterialStages(app, sub, device, model, material);
                device->SetFVF(fvf);
                bool stateDumped = false;
                const auto issueDraw = [&]() {
                    device->SetStreamSource(0, vertices, 0, stride);
                    device->SetIndices(indices);
                    if (!stateDumped) {
                        DumpMaterialState(app, device, model, material, i,
                                          firstIndex);
                        stateDumped = true;
                    }
                    mme::DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0, 0,
                                              state.vertexCount,
                                              firstIndex, indexCount / 3);
                };
                const float alpha = d3dMaterial.Diffuse.a;
                if (state.postLoadFlag2 != 0) {
                    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
                    device->SetMaterial(&d3dMaterial);
                    if (alpha < 1.0f) {
                        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
                        issueDraw();
                        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
                    }
                    issueDraw();
                    device->SetRenderState(D3DRS_DESTBLEND,
                                           D3DBLEND_INVSRCALPHA);
                } else if (alpha < 1.0f ||
                           (mdl::Mdl(model)->physicsMode == 2 &&
                            (record.flags & 1) != 0)) {
                    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                    device->SetRenderState(D3DRS_SRCBLEND,
                                           D3DBLEND_SRCALPHA);
                    device->SetRenderState(D3DRS_DESTBLEND,
                                           D3DBLEND_INVSRCALPHA);
                    device->SetMaterial(&d3dMaterial);
                    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
                    issueDraw();
                    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
                } else {
                    device->SetMaterial(&d3dMaterial);
                    issueDraw();
                }
                ResetSphereStageAfterDraw(device, material);
            } else {
                if (shadowOnly) {
                    device->SetRenderState(
                        D3DRS_CULLMODE,
                        record.doubleSided == 0 ? D3DCULL_CCW
                                                : D3DCULL_NONE);
                    if (effectPass && effect != nullptr)
                        fx::BeginPass(effect, 0);
                } else {
                }
                if (effectPass && effect != nullptr && !shadowOnly) {
                    device->SetRenderState(D3DRS_CULLMODE,
                        record.diffuse[3] >= 1.0f ||
                                mdl::Mdl(model)->physicsMode == 2
                            ? D3DCULL_CCW : D3DCULL_NONE);
                    device->SetRenderState(D3DRS_DESTBLEND,
                        state.postLoadFlag2 != 0 ? D3DBLEND_ONE
                                          : D3DBLEND_INVSRCALPHA);
                    if (mdl::Mdl(model)->physicsMode == 2 && (record.flags & 1) != 0)
                        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
                    ConfigureEffectMaterial(app, sub, device, effect,
                                            model, material, i);
                    UINT passes = 0;
                    fx::Begin(effect, &passes);
                    device->SetTexture(0,
                        mdl::Mdl(model)->physicsMode == 2 && (record.flags & 8) == 0
                            ? sub->spriteTexture
                            : sub->hdrTexture);
                    fx::BeginPass(effect, 0);
                }
                device->SetFVF(fvf);
                device->SetStreamSource(0, vertices, 0, stride);
                device->SetIndices(indices);
                mme::DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0, 0,
                                          state.vertexCount, firstIndex,
                                          indexCount / 3);
                if (effectPass && effect != nullptr) {
                    fx::EndPass(effect);
                    if (!shadowOnly) {
                        fx::End(effect);
                        if (mdl::Mdl(model)->physicsMode == 2 && (record.flags & 1) != 0)
                            device->SetRenderState(D3DRS_CULLMODE,
                                                   D3DCULL_CCW);
                    }
                }
            }
        }
        firstIndex += indexCount;
    }
    state.toonShared = static_cast<std::uint32_t>(-1);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
}

void DrawModelEdgeGeometry(MMDApp* app, unsigned char* model,
                           bool projectedShadow) {
    D3DRenderer* sub = app->Renderer();
    auto* device = sub->device;
    void* effect = sub->effect;
    if (model == nullptr || mdl::Mdl(model)->loadComplete == 0 ||
        mdl::Mdl(model)->displayState != 0)
        return;
    mdl::ModelRecord& state = *mdl::Mdl(model);
    auto* vb = mdl::ResourceAs<IDirect3DVertexBuffer9>(
        mdl::Mdl(model)->vertexBuffer2);
    auto* ib = mdl::ResourceAs<IDirect3DIndexBuffer9>(
        mdl::Mdl(model)->indexBuffer);
    auto* materials = mdl::Materials(model);
    if (vb == nullptr || ib == nullptr || materials == nullptr)
        return;

    UINT firstIndex = 0;
    state.toonShared = static_cast<std::uint32_t>(-1);
    for (UINT i = 0; i < state.materialCount; ++i) {
        auto* material = reinterpret_cast<unsigned char*>(&materials[i]);
        const mdl::ModelMaterialRecord& record = materials[i];
        ++state.toonShared;
        const UINT count = static_cast<UINT>(record.faceVertexCount);
        bool draw = count != 0;
        if (!projectedShadow && record.doubleSided == 0)
            draw = false;
        const bool pmx = mdl::Mdl(model)->physicsMode == 2;
        if (pmx && projectedShadow && (record.flags & 2) == 0)
            draw = false;
        if (pmx && projectedShadow && sub->postProcessEnabled != 0 &&
            record.edgeColor[3] <= 0.0f)
            draw = false;
        if (draw) {
            if (pmx && !projectedShadow && sub->postProcessEnabled != 0) {
                fx::EndPass(effect);
                fx::SetFloatArray(effect, "EgColor", record.edgeColor, 4);
                fx::BeginPass(effect, 0);
            }
            device->SetFVF(66);
            device->SetStreamSource(0, vb, 0, 16);
            device->SetIndices(ib);
            mme::DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0, 0,
                state.vertexCount, firstIndex, count / 3);
        }
        firstIndex += count;
    }
    state.toonShared = static_cast<std::uint32_t>(-1);
}

void DrawModelsProjectedShadow(MMDApp* app) {
    for (int order = 0; order < kModelSlotCount; ++order) {
        for (int slot = 0; slot < kModelSlotCount; ++slot) {
            auto* model = app->ModelSlot(slot);
            if (model == nullptr || mdl::Mdl(model)->comboSelIndex != order)
                continue;
            app->ActiveRenderObject() = model;
            DrawModelEdgeGeometry(app, model, true);
            app->ActiveRenderObject() = nullptr;
            break;
        }
    }
}

bool BeginProjectedGroundShadow(MMDApp* app, IDirect3DDevice9* device,
                                Matrix* base) {
    const bool cameraGate = app->PlaybackActive() != 0 ||
        app->UsesViewportTool();
    if (app->GroundShadowEnabled() == 0 || !cameraGate ||
        app->LightDirection()[1] >= 0.0f)
        return false;
    Matrix projection{};
    Matrix projected;
    device->GetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(base));
    projection.m[0][0] = 1.0f;
    projection.m[1][0] = -app->LightDirection()[0] /
                          app->LightDirection()[1];
    projection.m[1][2] = -app->LightDirection()[2] /
                          app->LightDirection()[1];
    projection.m[2][2] = 1.0f;
    projection.m[3][1] = 0.10000000149011612f;
    projection.m[3][3] = 1.0f;
    Multiply(&projected, &projection, base);
    device->SetTransform(D3DTS_WORLD,
                         reinterpret_cast<const D3DMATRIX*>(&projected));
    app->ActiveRenderPass() = AccessoryRenderPass::ProjectedGroundShadow;
    return true;
}

void RenderProjectedGroundShadowPass(MMDApp* app,
                                     IDirect3DDevice9* device) {
    Matrix base;
    if (!BeginProjectedGroundShadow(app, device, &base))
        return;
    RenderAccessoriesProjectedGroundShadowGeometry(app);
    DrawModelsProjectedShadow(app);
    device->SetTransform(D3DTS_WORLD,
                         reinterpret_cast<const D3DMATRIX*>(&base));
}

void SetProjectedStencil(D3DRenderer* sub, IDirect3DDevice9* device,
                         bool projectedPass) {
    if (sub->d3dInitialized == 0)
        return;
    device->SetRenderState(D3DRS_STENCILFUNC,
        projectedPass ? D3DCMP_GREATER : D3DCMP_ALWAYS);
    device->SetRenderState(D3DRS_STENCILREF, projectedPass ? 2 : 1);
    device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
}

void BeginProjectedShadowPass(D3DRenderer* sub, IDirect3DDevice9* device) {
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    SetProjectedStencil(sub, device, true);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
    device->SetTexture(0, nullptr);
}

void RestoreModelMaterialPass(MMDApp* app, D3DRenderer* sub,
                              IDirect3DDevice9* device) {
    SetProjectedStencil(sub, device, false);
    device->SetRenderState(D3DRS_FILLMODE,
        app->WireframeRenderingEnabled() == 0 ? D3DFILL_SOLID
                                               : D3DFILL_WIREFRAME);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
}

void DrawModelOutlines(MMDApp* app, bool effectEdge) {
    D3DRenderer* sub = app->Renderer();
    auto* device = sub->device;
    void* effect = sub->effect;
    UINT passes = 0;
    if (effectEdge) {
        if (effect == nullptr)
            return;
        fx::SetTechnique(effect, "ColorRenderTec");
        fx::Begin(effect, &passes);
    }
    const bool suppressed = effectEdge &&
        app->ModelNonDisplayMode() != 0;
    for (int order = 0; order < kModelSlotCount; ++order) {
        unsigned char* model = nullptr;
        for (int slot = 0; slot < kModelSlotCount; ++slot) {
            auto* candidate = app->ModelSlot(slot);
            if (candidate != nullptr &&
                mdl::Mdl(candidate)->comboSelIndex == order) {
                model = candidate;
                break;
            }
        }
        if (model == nullptr)
            continue;
        const mdl::ModelRecord& state = *mdl::Mdl(model);
        if (state.edgeScale <= 0.0f || state.postLoadFlag2 != 0)
            continue;
        const bool cameraGate = app->PlaybackActive() != 0 ||
            app->UsesViewportTool();
        if (!cameraGate)
            continue;
        if (suppressed)
            continue;
        app->ActiveRenderObject() = model;
        if (effectEdge) {
            const float edge[4] = {
                app->ModelOutlineColorRed() * (1.0f / 256.0f),
                app->ModelOutlineColorGreen() * (1.0f / 256.0f),
                app->ModelOutlineColorBlue() * (1.0f / 256.0f), 1.0f};
            fx::SetFloatArray(effect, "EgColor", edge, 4);
            fx::BeginPass(effect, 0);
        }
        DrawModelEdgeGeometry(app, model, false);
        if (effectEdge)
            fx::EndPass(effect);
        app->ActiveRenderObject() = nullptr;
    }
    if (effectEdge) {
        fx::End(effect);
        device->SetVertexShader(nullptr);
        device->SetPixelShader(nullptr);
    }
}

void DrawGroundGeometry(MMDApp* app, IDirect3DDevice9* device) {
    if (app->GroundGridEnabled() == 0)
        return;
    auto* vb = app->GroundGridVertices();
    auto* ib = app->GroundGridIndices();
    if (vb != nullptr && ib != nullptr) {
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetTexture(0, nullptr);
        device->SetFVF(66);
        device->SetStreamSource(0, vb, 0, 16);
        device->SetIndices(ib);
        mme::DrawIndexedPrimitive(device, D3DPT_LINELIST, 0, 0, 90, 0, 45);
    }

    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    D3DRenderer* sub = app->Renderer();
    const bool stencil = sub != nullptr && sub->d3dInitialized != 0;
    if (stencil)
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    auto* plane = app->GroundPlaneVertices();
    if (plane != nullptr) {
        device->SetStreamSource(0, plane, 0, 16);
        device->SetFVF(66);
        mme::DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 2);
    }
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    if (stencil)
        device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
}

void DrawPreModelQuad(IDirect3DDevice9* device,
                      IDirect3DTexture9* texture,
                      IDirect3DVertexBuffer9* vertices) {
    if (texture == nullptr || vertices == nullptr)
        return;
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetTexture(0, texture);
    device->SetStreamSource(0, vertices, 0, 28);
    device->SetFVF(324);
    mme::DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 2);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
}

void DrawPreModelQuads(MMDApp* app, IDirect3DDevice9* device) {
    if (app->PictureBackgroundEnabled() != 0) {
        DrawPreModelQuad(device,
            app->PictureBackgroundTexture(),
            app->PictureOverlayVertices());
    }
    if (app->AviBackgroundEnabled() == 1) {
        DrawPreModelQuad(device,
            app->AviBackgroundTexture(),
            app->AviOverlayVertices());
    }
}

bool EffectRenderEnabled(const MMDApp* app) {
    const bool cameraGate = app->PlaybackActive() != 0 ||
        app->UsesViewportTool();
    return cameraGate && app->state.selfShadowMode > 0 &&    
           app->state.selfShadowEnabled != 0;            
}

}  

bool UseEffectModelRenderer(MMDApp* app) {
    return EffectRenderEnabled(app);
}

void RenderModelsFixed(MMDApp* app) {                         
    D3DRenderer* sub = app->Renderer();
    auto* device = sub->device;
    device->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 0x10000);
    if (sub->d3dInitialized != 0) {
        device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
        device->SetRenderState(D3DRS_STENCILREF, 1);
        device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
    }
    DrawPreModelQuads(app, device);
    DrawGroundGeometry(app, device);
    device->SetRenderState(D3DRS_FILLMODE,
        app->WireframeRenderingEnabled() == 0 ? D3DFILL_SOLID : D3DFILL_WIREFRAME);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);

    const int accessorySplit = std::max(0, std::min(
        app->AccessoryRenderSplitOrder(), 255));
    RenderAccessoriesFixedRange(app, 0, accessorySplit);
    BeginProjectedShadowPass(sub, device);
    RenderProjectedGroundShadowPass(app, device);
    RestoreModelMaterialPass(app, sub, device);

    const bool materialCapture = BeginMaterialStateCapture();
    for (int order = 0; order < kModelSlotCount; ++order) {
        for (int slot = 0; slot < kModelSlotCount; ++slot) {
            auto* model = app->ModelSlot(slot);
            if (model == nullptr || mdl::Mdl(model)->comboSelIndex != order)
                continue;
            app->ActiveRenderObject() = model;
            app->ActiveRenderPass() = AccessoryRenderPass::FixedFunction;
            DrawModelMaterials(app, model, false, false);
            app->ActiveRenderObject() = nullptr;
            break;
        }
    }
    if (materialCapture)
        EndMaterialStateCapture();

    if (app->WireframeRenderingEnabled() == 0) {
        app->ActiveRenderPass() = AccessoryRenderPass::ModelOutline;
        device->SetTexture(0, nullptr);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
        device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
        DrawModelOutlines(app, false);
    }
    app->ActiveRenderPass() = AccessoryRenderPass::FixedFunction;
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
    device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    RenderAccessoriesFixedRange(app, accessorySplit, 255);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    app->ActiveRenderPass() = AccessoryRenderPass::None;
}

void RenderModelsEffect(MMDApp* app, const float frameMatrix[16]) { 
    D3DRenderer* sub = app->Renderer();
    auto* device = sub->device;
    void* effect = sub->effect;
    if (effect == nullptr) {
        RenderModelsFixed(app);
        return;
    }

    Matrix view;
    Matrix projection;
    Matrix worldView;
    Matrix wvp;
    const Matrix frame = *reinterpret_cast<const Matrix*>(frameMatrix);
    device->GetTransform(D3DTS_VIEW,
                         reinterpret_cast<D3DMATRIX*>(&view));
    device->GetTransform(D3DTS_PROJECTION,
                         reinterpret_cast<D3DMATRIX*>(&projection));
    Multiply(&worldView, &frame, &view);
    Multiply(&wvp, &worldView, &projection);
    std::memcpy(&app->WorldViewProjection(), &wvp, sizeof(wvp));
    DrawPreModelQuads(app, device);
    DrawGroundGeometry(app, device);

    float light[4] = {app->LightDirection()[0], app->LightDirection()[1],
                      app->LightDirection()[2], 1.0f};
    d3dx::Get().vec3Normalize(light, light);
    light[3] = 1.0f;
    Matrix inverse;
    float target[3] = {app->ViewOffsetX(), app->ViewOffsetY(),
                       app->CameraDistance()};
    float place[4] = {};
    d3dx::Get().inverse(&inverse, nullptr, &frame);
    d3dx::Get().vec3Transform(place, target, &inverse);
    fx::SetFloatArray(effect, "LightDir", light, 4);
    fx::SetFloatArray(effect, "Place", place, 4);
    fx::SetMatrix(effect, "matWorldViewProj",
                reinterpret_cast<const Matrix*>(&app->WorldViewProjection()));
    fx::SetMatrix(effect, "matLightViewProj",
                reinterpret_cast<const Matrix*>(&app->LightViewProjection()));
    fx::SetMatrix(effect, "matRotate",
                reinterpret_cast<const Matrix*>(&app->ViewRotationTransform()));
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_FILLMODE,
        app->WireframeRenderingEnabled() == 0
            ? D3DFILL_SOLID : D3DFILL_WIREFRAME);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);

    const int accessorySplit = std::max(0, std::min(
        app->AccessoryRenderSplitOrder(), 255));
    RenderAccessoriesEffectRange(app, 0, accessorySplit);
    BeginProjectedShadowPass(sub, device);
    RenderProjectedGroundShadowPass(app, device);
    SetProjectedStencil(sub, device, false);
    device->SetRenderState(D3DRS_FILLMODE,
        app->WireframeRenderingEnabled() == 0 ? D3DFILL_SOLID
                                              : D3DFILL_WIREFRAME);
    device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
    device->SetTexture(0, sub->hdrTexture);
    fx::SetMatrix(effect, "matWorldViewProj",
                reinterpret_cast<const Matrix*>(&app->WorldViewProjection()));
    fx::SetMatrix(effect, "matLightViewProj",
                reinterpret_cast<const Matrix*>(&app->LightViewProjection()));
    fx::SetBool(effect, "transp", app->state.characterTransparentMode != 0);
    device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

    for (int order = 0; order < kModelSlotCount; ++order) {
        for (int slot = 0; slot < kModelSlotCount; ++slot) {
            auto* model = app->ModelSlot(slot);
            if (model == nullptr || mdl::Mdl(model)->comboSelIndex != order)
                continue;
            if (app->ModelNonDisplayMode() != 0)
                break;
            app->ActiveRenderObject() = model;
            if (mdl::Mdl(model)->toonFlag != 0) {
                app->ActiveRenderPass() = AccessoryRenderPass::Effect;
                device->SetTexture(0, sub->hdrTexture);
                DrawModelMaterials(app, model, true, false);
            } else {
                app->ActiveRenderPass() = AccessoryRenderPass::FixedFunction;
                DrawModelMaterials(app, model, false, false);
            }
            app->ActiveRenderObject() = nullptr;
            break;
        }
    }
    fx::SetBool(effect, "transp", 0);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    if (app->WireframeRenderingEnabled() == 0) {
        app->ActiveRenderPass() = AccessoryRenderPass::ModelOutline;
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
        DrawModelOutlines(app, true);
    }
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
    device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    RenderAccessoriesEffectRange(app, accessorySplit, 255);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    app->ActiveRenderPass() = AccessoryRenderPass::None;
}

void RenderShadowMap(MMDApp* app, const float frameMatrix[16]) {   
    D3DRenderer* sub = app->Renderer();
    auto* device = sub->device;
    void* effect = sub->effect;
    auto& api = d3dx::Get();
    if (device == nullptr || effect == nullptr)
        return;

    auto*& texture = sub->hdrTexture;
    auto*& surface = sub->shadowSurface;
    auto*& depth = sub->shadowDepthSurface;
    const UINT width = static_cast<UINT>(sub->renderTargetWidth);
    const UINT height = static_cast<UINT>(sub->renderTargetHeight);
    if (texture == nullptr &&
        SUCCEEDED(api.createTexture(device, width, height, 1,
            D3DUSAGE_RENDERTARGET, static_cast<D3DFORMAT>(114),
            D3DPOOL_DEFAULT, &texture)))
        texture->GetSurfaceLevel(0, &surface);
    if (depth == nullptr)
        device->CreateDepthStencilSurface(width, height,
            static_cast<D3DFORMAT>(77), D3DMULTISAMPLE_NONE, 0, FALSE,
            &depth, nullptr);
    if (sub->backbufferSurface == nullptr) {
        device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
            &sub->backbufferSurface);
        device->GetDepthStencilSurface(&sub->depthStencilSurface);
    }
    if (surface == nullptr || depth == nullptr)
        return;

    D3DVIEWPORT9 oldViewport;
    Matrix view;
    Matrix projection;
    Matrix worldView;
    device->GetTransform(D3DTS_VIEW,
                         reinterpret_cast<D3DMATRIX*>(&view));
    device->GetViewport(&oldViewport);
    device->GetTransform(D3DTS_PROJECTION,
                         reinterpret_cast<D3DMATRIX*>(&projection));
    fx::SetTechnique(effect, "ZValuePlotTec");
    device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
    device->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 2);
    device->SetRenderTarget(0, surface);
    device->SetDepthStencilSurface(depth);
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                  0xFFFFFFFFu, 1.0f, 0);
    device->SetTexture(0, nullptr);
    D3DVIEWPORT9 shadowViewport{0, 0, width, height, 0.0f, 1.0f};
    device->SetViewport(&shadowViewport);

    float target[3] = {app->ViewOffsetX(), app->ViewOffsetY(),
                       app->CameraDistance()};
    const D3DVECTOR& lightDirection = app->SceneLight().Direction;
    float eye[3] = {
        target[0] - lightDirection.x * 50.0f,
        target[1] - lightDirection.y * 50.0f,
        target[2] - lightDirection.z * 50.0f};
    const float up[3] = {0.0f, 0.0f, 1.0f};
    Matrix lightView;
    api.lookAtLH(&lightView, eye, target, up);

    float direction[3] = {lightDirection.x, lightDirection.y,
                          lightDirection.z};
    api.vec3Normalize(direction, direction);
    const float slope = -direction[2];
    const float edge = 1.0f - std::fabs(slope);
    const float base = app->ShadowDistance();
    const bool modeOne = app->ShadowMode() == 1;
    float scale;
    int part;
    if (modeOne) {
        part = 0;
        if (slope < -0.9f || slope > 0.9f)
            scale = base;
        else if (slope < -0.8f)
            scale = ((slope - 0.1f) * 10.0f + 10.0f) * base + base;
        else if (slope > 0.8f)
            scale = (10.0f - (slope + 0.1f) * 10.0f) * base + base;
        else
            scale = base + base;
    } else {
        part = 1;
        if (slope < -0.9f || slope > 0.9f)
            scale = base;
        else if (slope < -0.8f)
            scale = (slope * 20.0f + 19.0f) * base;
        else if (slope > 0.8f)
            scale = (19.0f - slope * 20.0f) * base;
        else
            scale = base * 3.0f;
    }
    fx::SetBool(effect, "parthf", part);

    Matrix shadowProjection{};
    shadowProjection.m[0][0] = scale;
    shadowProjection.m[1][1] = scale;
    shadowProjection.m[2][2] = base * 0.1500000059604645f;
    shadowProjection.m[3][3] = 1.0f;
    Matrix shadowMatrix;
    Matrix temp;
    Multiply(&temp, reinterpret_cast<const Matrix*>(frameMatrix), &lightView);
    Multiply(&shadowMatrix, &temp, &shadowProjection);
    Matrix transform;
    api.scaling(&transform, 1.0f, 1.0f, 2.0f);
    Multiply(&shadowMatrix, &shadowMatrix, &transform);
    api.translation(&transform, 0.0f, 0.0f, -1.0f);
    Multiply(&shadowMatrix, &shadowMatrix, &transform);

    Matrix orientation;
    Identity(&orientation);
    float positive;
    float negative;
    if (slope < -0.9f || slope > 0.9f) {
        positive = edge;
        negative = -edge;
    } else if (slope < -0.8f) {
        positive = modeOne ? 4.0f * slope + 3.7f
                           : 9.0f * slope + 8.2f;
        negative = -9.0f * slope - 8.2f;
    } else if (slope > 0.8f) {
        positive = modeOne ? 3.7f - 4.0f * slope
                           : 8.2f - 9.0f * slope;
        negative = 9.0f * slope - 8.2f;
    } else {
        positive = modeOne ? 0.5f : 1.0f;
        negative = -1.0f;
    }
    orientation.m[1][3] = positive;
    orientation.m[3][1] = negative;
    Multiply(&shadowMatrix, &shadowMatrix, &orientation);
    api.translation(&transform, 0.0f, 0.0f, 1.0f);
    Multiply(&shadowMatrix, &shadowMatrix, &transform);
    api.scaling(&transform, 1.0f, 1.0f, 0.5f);
    Multiply(&shadowMatrix, &shadowMatrix, &transform);
    std::memcpy(&app->LightViewProjection(), &shadowMatrix,
                sizeof(shadowMatrix));

    UINT passes = 0;
    app->ActiveRenderPass() = AccessoryRenderPass::ModelEffect;
    if (SUCCEEDED(fx::Begin(effect, &passes))) {
        RenderAccessoriesShadow(app);
        fx::SetMatrix(effect, "matLightViewProj", &shadowMatrix);
        Matrix wvp;
        Multiply(&worldView, reinterpret_cast<const Matrix*>(frameMatrix), &view);
        Multiply(&wvp, &worldView, &projection);
        std::memcpy(&app->WorldViewProjection(), &wvp, sizeof(wvp));
        fx::SetMatrix(effect, "matWorldViewProj", &wvp);
        for (int slot = 0; slot < kModelSlotCount; ++slot) {
            auto* model = app->ModelSlot(slot);
            if (model == nullptr || mdl::Mdl(model)->toonFlag == 0)
                continue;
            app->ActiveRenderObject() = model;
            DrawModelMaterials(app, model, true, true);
            app->ActiveRenderObject() = nullptr;
        }
        fx::End(effect);
    }
    app->ActiveRenderPass() = AccessoryRenderPass::None;
    device->SetVertexShader(nullptr);
    device->SetPixelShader(nullptr);
    IDirect3DSurface9* targetSurface =
        sub->multisampleAvailable == 0 &&
        app->RecordingWindow() != nullptr
            ? sub->captureSurface
            : sub->backbufferSurface;
    device->SetRenderTarget(0, targetSurface);
    device->SetDepthStencilSurface(sub->depthStencilSurface);
    device->SetViewport(&oldViewport);
    device->SetTransform(D3DTS_PROJECTION,
                         reinterpret_cast<const D3DMATRIX*>(&projection));
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

}  
