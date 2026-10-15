#pragma once

#include <cstdint>
#include <io.h>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/model.hpp"

namespace rinmotiondance::pmm_io {

enum class PmmStream { V1, V2 };

inline int Rd(int fd, void* buf, unsigned int count) {
    return _read(fd, buf, count);
}


template <PmmStream S>
inline void ReadPmmBoneKey(int fd, mdl::BoneKey& key) {
    Rd(fd, &key.frame, 4); Rd(fd, &key.previous, 4); Rd(fd, &key.next, 4);
    for (int lane = 0; lane < 4; ++lane) {
        Rd(fd, &key.interpolation[lane], 1);
        Rd(fd, &key.interpolation[lane + 4], 1);
        Rd(fd, &key.interpolation[lane + 8], 1);
        Rd(fd, &key.interpolation[lane + 12], 1);
    }
    Rd(fd, key.position, sizeof key.position);
    Rd(fd, key.rotation, sizeof key.rotation);
    Rd(fd, &key.allocated, 1);
    if constexpr (S == PmmStream::V2)
        Rd(fd, &key.physicsDisabled, 1);
}

inline void ReadPmmMorphKey(int fd, mdl::MorphKey& key) {
    Rd(fd, &key.frame, 4); Rd(fd, &key.previous, 4); Rd(fd, &key.next, 4);
    Rd(fd, &key.value, 4);
    Rd(fd, &key.allocated, 1);
}

template <PmmStream S>
inline void ReadPmmCameraKey(int fd, mdl::CameraKey& key) {
    Rd(fd, &key, 0x28);
    if constexpr (S == PmmStream::V2) {
        Rd(fd, &key.parentModel, 4);
        Rd(fd, &key.parentBone, 4);
    }
    for (int k = 0; k < 6; ++k) {
        Rd(fd, &key.interpolation[0][k], 1);
        Rd(fd, &key.interpolation[1][k], 1);
        Rd(fd, &key.interpolation[2][k], 1);
        Rd(fd, &key.interpolation[3][k], 1);
    }
    unsigned char b = 0;
    Rd(fd, &b, 1);
    key.perspective = (b == 1) ? 1 : 0;
    Rd(fd, &key.fov, 4);
    Rd(fd, &b, 1);
    key.selected = (b == 1) ? 1 : 0;
}

inline void ReadPmmLightKey(int fd, mdl::LightKey& key) {
    Rd(fd, &key.frame, 4);
    Rd(fd, &key.previous, 4);
    Rd(fd, &key.next, 4);
    Rd(fd, &key.color[0], 4);
    Rd(fd, &key.color[1], 4);
    Rd(fd, &key.color[2], 4);
    Rd(fd, &key.direction[0], 4);
    Rd(fd, &key.direction[1], 4);
    Rd(fd, &key.direction[2], 4);
    unsigned char b = 0;
    Rd(fd, &b, 1);
    key.selected = (b == 1) ? 1 : 0;
}

inline void ReadPmmSelfShadowKey(int fd, mdl::SelfShadowKey& key) {
    Rd(fd, &key.frame, 4);
    Rd(fd, &key.previous, 4);
    Rd(fd, &key.next, 4);
    Rd(fd, &key.mode, 1);
    Rd(fd, &key.distance, 4);
    unsigned char b = 0;
    Rd(fd, &b, 1);
    key.selected = (b == 1) ? 1 : 0;
}

inline void ReadPmmGravityKey(int fd, mdl::GravityKey& key) {
    Rd(fd, &key.frame, 4);
    Rd(fd, &key.previous, 4);
    Rd(fd, &key.next, 4);
    unsigned char b = 0;
    Rd(fd, &b, 1);
    key.noiseEnabled = (b == 1) ? 1 : 0;
    Rd(fd, &key.noise, 4);
    Rd(fd, &key.acceleration, 4);
    Rd(fd, &key.direction[0], 4);
    Rd(fd, &key.direction[1], 4);
    Rd(fd, &key.direction[2], 4);
    Rd(fd, &b, 1);
    key.selected = (b == 1) ? 1 : 0;
}

inline void ReadPmmAccessoryKey(int fd, mdl::AccessoryKey& key) {
    unsigned char b = 0;
    Rd(fd, &key.frame, 4);
    Rd(fd, &key.previous, 4);
    Rd(fd, &key.next, 4);
    Rd(fd, &b, 1);
    key.visible = b & 1;
    key.opacity =
        static_cast<float>(100 - (b >> 1)) / 100.0f;
    Rd(fd, &key.parentModel, 4);
    Rd(fd, &key.parentBone, 4);
    Rd(fd, key.position, sizeof key.position);
    Rd(fd, key.rotation, sizeof key.rotation);
    Rd(fd, &key.scale, 4);
    Rd(fd, &b, 1);
    key.shadowEnabled = (b == 1) ? 1 : 0;
    Rd(fd, &b, 1);
    key.selected = (b == 1) ? 1 : 0;
}

inline void DiscardPmmBoneKey(int fd) {
    mdl::BoneKey ignored{};
    ReadPmmBoneKey<PmmStream::V2>(fd, ignored);
}

inline void DiscardPmmMorphKey(int fd) {
    mdl::MorphKey ignored{};
    ReadPmmMorphKey(fd, ignored);
}

inline void DiscardPmmDisplayKey(int fd, std::int32_t ikCount,
                                 std::int32_t rigidBodyCount) {
    std::uint32_t ignoredLink = 0;
    std::uint8_t ignoredFlag = 0;
    Rd(fd, &ignoredLink, 4);
    Rd(fd, &ignoredLink, 4);
    Rd(fd, &ignoredLink, 4);
    Rd(fd, &ignoredFlag, 1);
    for (std::int32_t i = 0; i < ikCount; ++i)
        Rd(fd, &ignoredFlag, 1);
    for (std::int32_t i = 0; i < rigidBodyCount; ++i) {
        Rd(fd, &ignoredLink, 4);
        Rd(fd, &ignoredLink, 4);
    }
    Rd(fd, &ignoredFlag, 1);
}

extern "C" {
__declspec(dllimport) std::int32_t __stdcall AVIStreamGetFrameClose(void* pg);
__declspec(dllimport) std::int32_t __stdcall AVIStreamRelease(void* pavi);
__declspec(dllimport) std::int32_t __stdcall AVIFileRelease(void* pfile);
}

inline const wchar_t kWNashi[]    = L"\x306A\x3057";                    
inline const wchar_t kWGround[]   = L"\x5730\x9762";                    
inline const wchar_t kWCamLight[] =
    L"\xFF76\xFF92\xFF97\xFF65\x7167\x660E\xFF65"
      L"\xFF71\xFF78\xFF7E\xFF7B\xFF98";                         
inline const wchar_t kWLight[]    = L"\x7167\x660E";                    
inline const wchar_t kWSelfSh[]   = L"\x30BB\x30EB\x30D5\x5F71";        
inline const wchar_t kWGravity[]  = L"\x91CD\x529B";                    
inline const wchar_t kWAll[]      = L"\x3059\x3079\x3066";              
inline const wchar_t kWViewAng[]  = L"\x8996\x91CE\x89D2";              
inline const wchar_t kWDist[]     = L"\x8DDD\x3000\x96E2";              
inline const wchar_t kWZMove[]    = L"\xFF5A\x79FB\x52D5";              
inline const wchar_t kWYMove[]    = L"\xFF59\x79FB\x52D5";              
inline const wchar_t kWXMove[]    = L"\xFF58\x79FB\x52D5";              
inline const wchar_t kWRot[]      = L"\x56DE\x3000\x8EE2";              
inline const wchar_t kWCamera[]   = L"\x30AB\x30E1\x30E9";              
inline const wchar_t kWOpenFile[] = L"\x30D5\x30A1\x30A4\x30EB\x3092"
                                    L"\x958B\x304F";                    
inline const wchar_t kWUserModel[] = L"UserFile\\Model";
inline const wchar_t kWUserAcc[]   = L"UserFile\\Accessory";
inline const wchar_t kWFilterModel[] =
    L"All Model files(*.pmd,*.pmx)\0*.pmd;*.pmx\0";              
inline const wchar_t kWFilterAccJp[] =
    L"\x8AAD\x8FBC\x53EF\x80FD\x30D5\x30A1\x30A4\x30EB"
      L"(*.x,*.vac)\0*.x;*.vac\0";                               
inline const wchar_t kWFilterAccEn[] =
    L"accessory file(*.x,*.vac)\0*.x;*.vac\0";
inline const char kNon[] = "non";                                       

inline const char kJpChainCapPhys[] =
    "\x95\x5C\x8E\xA6\xA5\x49\x4B\xA5\x8A\x4F\x90\x65\x83\x66\x81\x5B"
    "\x83\x5E\x88\xD9\x8F\xED";                                  
inline const char kJpCannotOpenModel[] =
    "\x83\x82\x83\x66\x83\x8B\x83\x74\x83\x40\x83\x43\x83\x8B\x93\xC7"
    "\x82\xDD\x8D\x9E\x82\xDD\x8E\xB8\x94\x73";                  
inline const char kJpOpenCaption[] =
    "\x83\x74\x83\x40\x83\x43\x83\x8B\x93\xC7\x8D\x9E";          

inline const wchar_t kAppTitleFormat[] = L"RinMotionDance [%s]";

}  
