#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/mmd_app.hpp"

namespace rinmotiondance {
namespace {

unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

const char kCameraLightName[] =  
    "\x83\x4a\x83\x81\x83\x89\x81\x45\x8f\xc6\x96\xbe";
const char kJpCannotSave[] =  
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xaa\x95\xdb\x91\xb6\x82\xc5\x82"
    "\xab\x82\xdc\x82\xb9\x82\xf1:%d";

int Wr(int fd, const void* p, unsigned int n) { return _write(fd, p, n); }

}  

void SaveVmdFile(const wchar_t* path) {
    MMDApp* app = g_Block;
    if (app == nullptr) return;
    auto& s = *app;

    const bool cameraMode = s.state.optflag[0] != 0;
    unsigned char* const model = ActiveModel(app);

    std::uint32_t minFrame = 0xFFFFFFFF;
    unsigned int boneCnt = 0, morphCnt = 0, ikCnt = 0;
    unsigned int camCnt = 0, lightCnt = 0, shadowCnt = 0;

    if (!cameraMode) {
        const mdl::BoneKey* keys = mdl::BoneKeys(model);
        for (unsigned int i = 0; i < static_cast<unsigned int>(mdl::kBoneKeyCapacity); ++i) {
            if (keys[i].allocated != 0) {
                ++boneCnt;
                const std::uint32_t f = keys[i].frame;
                if (f < minFrame) minFrame = f;
            }
        }
        const mdl::MorphKey* mkeys = mdl::MorphKeys(model);
        for (unsigned int i = 0; i < 20000; ++i) {
            if (mkeys[i].allocated != 0) {
                ++morphCnt;
                const std::uint32_t f = mkeys[i].frame;
                if (f < minFrame) minFrame = f;
            }
        }
        const mdl::DisplayKey* ikeys = mdl::DisplayKeys(model);
        for (unsigned int i = 0; i < 1000; ++i) {
            if (ikeys[i].allocated != 0) {
                ++ikCnt;
                const std::uint32_t f = ikeys[i].frame;
                if (f < minFrame) minFrame = f;
            }
        }
    } else {
        const mdl::CameraKey* cam = app->CameraKeys();
        for (unsigned int i = 0; i < 10000; ++i) {
            if (cam[i].selected != 0) {
                ++camCnt;
                const std::uint32_t f = cam[i].frame;
                if (f < minFrame) minFrame = f;
            }
        }
        const mdl::LightKey* light = app->LightKeys();
        for (unsigned int i = 0; i < 10000; ++i) {
            if (light[i].selected != 0) {
                ++lightCnt;
                const std::uint32_t f = light[i].frame;
                if (f < minFrame) minFrame = f;
            }
        }
        const mdl::SelfShadowKey* shadow = app->ShadowKeys();
        for (unsigned int i = 0; i < 10000; ++i) {
            if (shadow[i].selected != 0) {
                ++shadowCnt;
                const std::uint32_t f = shadow[i].frame;
                if (f < minFrame) minFrame = f;
            }
        }
    }

    int fd;
    const errno_t openError = _wsopen_s(&fd, path, 0x8301, 0x40, 0x80);
    if (openError != 0) {
        char text[256];
        if (s.state.englishUI != 0)
            sprintf_s(text, 0x100, "Cannot save file:%d", openError);
        else
            sprintf_s(text, 0x100, kJpCannotSave, openError);
        MessageBoxA(static_cast<HWND>(s.Hwnd()), text, "", 0);
        return;
    }

    char buf[256];
    sprintf_s(buf, 0x100, "%s", "Vocaloid Motion Data 0002");
    Wr(fd, buf, 30);                                             
    if (!cameraMode) {
        Wr(fd, mdl::Mdl(model)->name, 20);                       
    } else {
        strcpy_s(buf, 0x100, kCameraLightName);
        Wr(fd, buf, 20);                                         
    }

    int boneCount = 0, morphCount = 0, ikChainCount = 0;
    rinmotiondance::mdl::BoneRecord* bones = nullptr;
    rinmotiondance::mdl::MorphRecord* morphs = nullptr;
    rinmotiondance::mdl::IkChain* ikChains = nullptr;
    if (!cameraMode) {
        auto* const rec = mdl::Mdl(model);
        boneCount = rec->boneCount;
        morphCount = rec->morphCount;
        ikChainCount = rec->ikChainCount;
        bones = rinmotiondance::mdl::Bones(model);
        morphs = rinmotiondance::mdl::Morphs(model);
        ikChains = rinmotiondance::mdl::IkChains(model);
    }

    std::uint32_t outDword;
    outDword = cameraMode ? 0 : boneCnt;
    Wr(fd, &outDword, 4);                                             
    if (!cameraMode) {
        const mdl::BoneKey* keys = mdl::BoneKeys(model);
        for (unsigned int idx = 0; idx < static_cast<unsigned int>(mdl::kBoneKeyCapacity); ++idx) {
            const mdl::BoneKey& rec = keys[idx];
            if (rec.allocated == 0) continue;
            unsigned int boneIdx = idx;                          
            while (boneIdx >= static_cast<unsigned int>(boneCount))
                boneIdx = keys[boneIdx].previous;
            Wr(fd, bones[boneIdx].name, 15);                    
            outDword = rec.frame - minFrame;
            Wr(fd, &outDword, 4);
            Wr(fd, rec.position, sizeof rec.position);
            Wr(fd, rec.rotation, sizeof rec.rotation);
            for (int ch = 0; ch < 4; ++ch) {
                if (ch == 0) {                                   
                    Wr(fd, rec.interpolation, 2);
                    const unsigned short marker =
                        rec.physicsDisabled != 0 ? 0x0F63 : 0;
                    Wr(fd, &marker, 2);
                } else {                                         
                    Wr(fd, rec.interpolation + ch, 4);
                }
                Wr(fd, rec.interpolation + 4 + ch, 4);
                Wr(fd, rec.interpolation + 8 + ch, 4);
                Wr(fd, rec.interpolation + 12 + ch, 4);
            }
        }
    }

    outDword = cameraMode ? 0 : morphCnt;
    Wr(fd, &outDword, 4);                                             
    if (!cameraMode) {
        const mdl::MorphKey* mkeys = mdl::MorphKeys(model);
        for (unsigned int idx = 0; idx < 20000; ++idx) {
            const mdl::MorphKey& rec = mkeys[idx];
            if (rec.allocated == 0) continue;
            unsigned int morphIdx = idx;                         
            while (morphIdx >= static_cast<unsigned int>(morphCount))
                morphIdx = mkeys[morphIdx].previous;
            Wr(fd, morphs[morphIdx].name, 15);                  
            outDword = rec.frame - minFrame;
            Wr(fd, &outDword, 4);
            Wr(fd, &rec.value, sizeof rec.value);
        }
    }

    outDword = camCnt;
    Wr(fd, &outDword, 4);                                             
    if (camCnt != 0) {
        const mdl::CameraKey* cam = app->CameraKeys();
        for (unsigned int i = 0; i < 10000; ++i) {
            const mdl::CameraKey& rec = cam[i];
            if (rec.selected == 0) continue;
            outDword = rec.frame - minFrame;
            Wr(fd, &outDword, 4);
            Wr(fd, &rec.distance, sizeof rec.distance);
            Wr(fd, rec.eye, sizeof rec.eye);
            Wr(fd, rec.target, sizeof rec.target);
            for (int j = 0; j < 6; ++j) {                        
                Wr(fd, &rec.interpolation[0][j], 1);
                Wr(fd, &rec.interpolation[2][j], 1);
                Wr(fd, &rec.interpolation[1][j], 1);
                Wr(fd, &rec.interpolation[3][j], 1);
            }
            Wr(fd, &rec.fov, sizeof rec.fov);
            const unsigned char view = rec.perspective != 0 ? 1 : 0;
            Wr(fd, &view, 1);
        }
    }

    outDword = lightCnt;
    Wr(fd, &outDword, 4);                                             
    if (lightCnt != 0) {
        const mdl::LightKey* light = app->LightKeys();
        for (unsigned int i = 0; i < 10000; ++i) {
            const mdl::LightKey& rec = light[i];
            if (rec.selected == 0) continue;
            outDword = rec.frame - minFrame;
            Wr(fd, &outDword, 4);
            Wr(fd, rec.color, sizeof rec.color);
            Wr(fd, rec.direction, sizeof rec.direction);
        }
    }

    outDword = shadowCnt;
    Wr(fd, &outDword, 4);                                             
    if (shadowCnt != 0) {
        const mdl::SelfShadowKey* shadow = app->ShadowKeys();
        for (unsigned int i = 0; i < 10000; ++i) {
            const mdl::SelfShadowKey& rec = shadow[i];
            if (rec.selected == 0) continue;
            outDword = rec.frame - minFrame;
            Wr(fd, &outDword, 4);
            Wr(fd, &rec.mode, sizeof rec.mode);
            Wr(fd, &rec.distance, sizeof rec.distance);
        }
    }

    outDword = cameraMode ? 0 : ikCnt;
    Wr(fd, &outDword, 4);                                             
    if (!cameraMode && outDword != 0) {
        const mdl::DisplayKey* ikeys = mdl::DisplayKeys(model);
        for (unsigned int i = 0; i < 1000; ++i) {
            const mdl::DisplayKey& rec = ikeys[i];
            if (rec.allocated == 0) continue;
            outDword = rec.frame - minFrame;
            Wr(fd, &outDword, 4);
            const unsigned char visible = rec.visible != 0 ? 1 : 0;
            Wr(fd, &visible, 1);
            outDword = static_cast<std::uint32_t>(ikChainCount);
            Wr(fd, &outDword, 4);
            for (int j = 0; j < ikChainCount; ++j) {             
                const int boneIdx = ikChains[j].boneIndex;
                Wr(fd, bones[boneIdx].name, 20);
                const unsigned char on =
                    mdl::IkStates(rec)[j] != 0
                        ? 1 : 0;
                Wr(fd, &on, 1);
            }
        }
    }

    _close(fd);                                                  
}

}  
