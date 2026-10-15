#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <io.h>

#include "rinmotiondance/charset_conv.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/mme_bridge.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

#include "pmm_io_common.hpp"

namespace rinmotiondance {

namespace {

inline void W(int fd, const void* buf, unsigned int count) {
    _write(fd, buf, static_cast<unsigned int>(count));
}

void WritePmmBoneKey(int fd, const mdl::BoneKey& key) {
    W(fd, &key.frame, 4); W(fd, &key.previous, 4); W(fd, &key.next, 4);
    for (int lane = 0; lane < 4; ++lane) {
        W(fd, &key.interpolation[lane], 1);
        W(fd, &key.interpolation[lane + 4], 1);
        W(fd, &key.interpolation[lane + 8], 1);
        W(fd, &key.interpolation[lane + 12], 1);
    }
    W(fd, key.position, sizeof key.position);
    W(fd, key.rotation, sizeof key.rotation);
    const unsigned char allocated = key.allocated != 0;
    const unsigned char physicsDisabled = key.physicsDisabled != 0;
    W(fd, &allocated, 1); W(fd, &physicsDisabled, 1);
}

void WritePmmMorphKey(int fd, const mdl::MorphKey& key) {
    W(fd, &key.frame, 4); W(fd, &key.previous, 4); W(fd, &key.next, 4);
    W(fd, &key.value, 4);
    const unsigned char allocated = key.allocated != 0;
    W(fd, &allocated, 1);
}

void WritePmmDisplayKey(int fd, const mdl::DisplayKey& key,
                        int ikCount, int rigidCount) {
    W(fd, &key.frame, 4); W(fd, &key.previous, 4); W(fd, &key.next, 4);
    const unsigned char visible = key.visible != 0;
    W(fd, &visible, 1);
    const auto* flags = mdl::IkStates(key);
    for (int i = 0; i < ikCount; ++i) {
        const unsigned char enabled = flags[i] != 0;
        W(fd, &enabled, 1);
    }
    const auto* pairs = mdl::SelectorStates(key);
    for (int i = 0; i < rigidCount; ++i)
        W(fd, &pairs[i], sizeof(pairs[i]));
    const unsigned char allocated = key.allocated != 0;
    W(fd, &allocated, 1);
}

inline unsigned char NameLen(const unsigned char* s) {
    const unsigned char* p = s;
    while (*p != 0) ++p;
    return static_cast<unsigned char>(p - s);
}

inline unsigned char ScaleQuirkByte(float scale01, unsigned char additive) {
    const int v = static_cast<int>(static_cast<double>(scale01) * 100.0);
    unsigned char b = static_cast<unsigned char>((-0x1C - v) * 2);
    if (additive != 0) b = static_cast<unsigned char>(b + 1);
    return b;
}

void WritePmmAccessoryKey(int fd, const mdl::AccessoryKey& key) {
    W(fd, &key.frame, 4);
    W(fd, &key.previous, 4);
    W(fd, &key.next, 4);
    const unsigned char visibleAndOpacity =
        ScaleQuirkByte(key.opacity, key.visible);
    W(fd, &visibleAndOpacity, 1);
    W(fd, &key.parentModel, 4);
    W(fd, &key.parentBone, 4);
    W(fd, key.position, sizeof key.position);
    W(fd, key.rotation, sizeof key.rotation);
    W(fd, &key.scale, 4);
    const unsigned char shadowEnabled = key.shadowEnabled != 0;
    const unsigned char selected = key.selected != 0;
    W(fd, &shadowEnabled, 1);
    W(fd, &selected, 1);
}


void WritePmmCameraKey(int fd, const mdl::CameraKey& key) {
    W(fd, &key.frame, 4);            
    W(fd, &key.previous, 4);         
    W(fd, &key.next, 4);             
    W(fd, &key.distance, 4);         
    W(fd, &key.eye[0], 4);           
    W(fd, &key.eye[1], 4);           
    W(fd, &key.eye[2], 4);           
    W(fd, &key.target[0], 4);        
    W(fd, &key.target[1], 4);        
    W(fd, &key.target[2], 4);        
    W(fd, &key.parentModel, 4);      
    W(fd, &key.parentBone, 4);       
    for (int j = 0; j < 6; ++j) {    
        W(fd, &key.interpolation[0][j], 1);
        W(fd, &key.interpolation[1][j], 1);
        W(fd, &key.interpolation[2][j], 1);
        W(fd, &key.interpolation[3][j], 1);
    }
    {
        const unsigned char b = key.perspective != 0;    
        W(fd, &b, 1);
    }
    W(fd, &key.fov, 4);              
    {
        const unsigned char b = key.selected != 0;       
        W(fd, &b, 1);
    }
}

void WritePmmLightKey(int fd, const mdl::LightKey& key) {
    W(fd, &key.frame, 4);            
    W(fd, &key.previous, 4);         
    W(fd, &key.next, 4);             
    W(fd, &key.color[0], 4);         
    W(fd, &key.color[1], 4);         
    W(fd, &key.color[2], 4);         
    W(fd, &key.direction[0], 4);     
    W(fd, &key.direction[1], 4);     
    W(fd, &key.direction[2], 4);     
    {
        const unsigned char b = key.selected != 0;       
        W(fd, &b, 1);
    }
}

void WritePmmGravityKey(int fd, const mdl::GravityKey& key) {
    W(fd, &key.frame, 4);            
    W(fd, &key.previous, 4);         
    W(fd, &key.next, 4);             
    {
        const unsigned char b = key.noiseEnabled != 0;   
        W(fd, &b, 1);
    }
    W(fd, &key.noise, 4);            
    W(fd, &key.acceleration, 4);     
    W(fd, &key.direction[0], 4);     
    W(fd, &key.direction[1], 4);     
    W(fd, &key.direction[2], 4);     
    {
        const unsigned char b = key.selected != 0;       
        W(fd, &b, 1);
    }
}

void WritePmmSelfShadowKey(int fd, const mdl::SelfShadowKey& key) {
    W(fd, &key.frame, 4);            
    W(fd, &key.previous, 4);         
    W(fd, &key.next, 4);             
    W(fd, &key.mode, 1);             
    W(fd, &key.distance, 4);         
    {
        const unsigned char b = key.selected != 0;       
        W(fd, &b, 1);
    }
}

template <typename TKey>
std::int32_t CountSparseTrackFrames(const TKey* track) {
    std::int32_t cnt = 0;
    const TKey* rec = track + 1;
    for (int k = 0; k < 3333; ++k, rec += 3) {
        if (rec[0].frame != 0) ++cnt;
        if (rec[1].frame != 0) ++cnt;
        if (rec[2].frame != 0) ++cnt;
    }
    return cnt;
}

const char kJpSaveFailFmt[] =         
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xaa\x95\xdb\x91\xb6\x82\xc5"
    "\x82\xab\x82\xdc\x82\xb9\x82\xf1:%d";
const char kJpSaveFailCaption[] =     
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x95\xdb\x91\xb6";
const char kJpCannotOpenText[] =      
    "\x82\xbb\x82\xcc\x83\x74\x83\x48\x83\x8b\x83\x5f\x82\xc9\x82\xcd"
    "\x83\x5a\x81\x5b\x83\x75\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1";



void WritePmmFileHeader(int fd, MMDApp* s, HWND main, char* hdr) {
    s->state.windowLayoutReady = 0;
    sprintf_s(hdr, 0x100, "Polygon Movie maker 0002");
    hdr[25] = hdr[26] = hdr[27] = hdr[28] = hdr[29] = 0;
    W(fd, hdr, 0x1E);                                          

    W(fd, &s->state.renderW, 4);      
    W(fd, &s->state.renderH, 4);      
    {
        const std::int32_t v = reinterpret_cast<const std::int32_t&>(
            s->state.floatingWindow);
        if (v == 0)
            W(fd, &s->state.sidebarWidth, 4);
        else
            W(fd, &s->state.separateWindowSidebarWidth, 4);
    }
    W(fd, &s->state.cameraFov, 4);        

    for (int f = 0; f < 7; ++f) {                              
        const unsigned char b =
            s->state.optflag[f] != 0;               
        W(fd, &b, 1);
    }
    W(fd, &s->SelectedModelSlot(), 1);                         

    {                                                          
        const unsigned char b = static_cast<unsigned char>(
            SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_GETCOUNT, 0, 0) - 1);
        W(fd, &b, 1);
    }
}


void WritePmmModelBlocks(int fd, const D3DRenderer* renderer, unsigned char** const slots,
                         char* text) {
    for (unsigned char slot = 0; slot < kModelSlotCount; ++slot) {  
        if (slots[slot] == 0) continue;
        W(fd, &slot, 1);                                       
        unsigned char* const model = slots[slot];

        {  
            const unsigned char len = NameLen(
                reinterpret_cast<const unsigned char*>(mdl::Mdl(model)->name));
            W(fd, &len, 1);                                    
            W(fd, mdl::Mdl(model)->name, len);                 
        }
        {  
            const unsigned char len = NameLen(
                reinterpret_cast<const unsigned char*>(
                    mdl::Mdl(model)->nameEn));
            W(fd, &len, 1);                                    
            W(fd, mdl::Mdl(model)->nameEn, len);               
        }

        WideToSjis(renderer, text,                                  
                       mdl::Mdl(model)->path, 0x100);
        W(fd, text, 0x100);                                    

        const mdl::ModelRecord* const record = mdl::Mdl(model);
        const std::int32_t dispCount =
            static_cast<std::int32_t>(record->boneCount);
        const std::int32_t morphCount =
            static_cast<std::int32_t>(record->morphCount);
        const std::int32_t ikCount =
            static_cast<std::int32_t>(record->ikChainCount);
        const std::int32_t rigidCount =
            static_cast<std::int32_t>(record->boneOrderCount);
        mdl::BoneRecord* const bones = mdl::Bones(model);
        mdl::MorphRecord* const morphs = mdl::Morphs(model);
        mdl::IkChain* const ikChains = mdl::IkChains(model);
        mdl::BoneOrderEntry* const rigidFrames =
            mdl::BoneOrder(model);
        mdl::BoneKey* const dispKeys = mdl::BoneKeys(model);
        mdl::MorphKey* const morphKeys = mdl::MorphKeys(model);
        mdl::DisplayKey* const physKeys = mdl::DisplayKeys(model);

        W(fd, &record->groupCount, 1);                         
        W(fd, &record->boneCount, sizeof(record->boneCount));  
        if (dispCount != 0 && dispCount > 0) {
            for (std::int32_t i = 0; i < dispCount; ++i) {
                const unsigned char len = NameLen(
                    reinterpret_cast<const unsigned char*>(bones[i].name));
                W(fd, &len, 1);                                
                W(fd, bones[i].name, len);                     
            }
        }
        W(fd, &record->morphCount, sizeof(record->morphCount)); 
        if (morphCount != 0 && morphCount > 0) {
            for (std::int32_t i = 0; i < morphCount; ++i) {
                const unsigned char len = NameLen(
                    reinterpret_cast<const unsigned char*>(morphs[i].name));
                W(fd, &len, 1);                                
                W(fd, morphs[i].name, len);                    
            }
        }
        W(fd, &record->ikChainCount, sizeof(record->ikChainCount)); 
        if (ikCount != 0 && ikCount > 0) {
            for (std::int32_t i = 0; i < ikCount; ++i)
                W(fd, &ikChains[i].boneIndex, sizeof(ikChains[i].boneIndex));
        }
        W(fd, &record->boneOrderCount,
          sizeof(record->boneOrderCount));                     
        if (rigidCount != 0 && rigidCount > 0) {
            for (std::int32_t i = 0; i < rigidCount; ++i)
                W(fd, &rigidFrames[i].boneIndex,
                  sizeof(rigidFrames[i].boneIndex));           
        }
        W(fd, &record->comboSelIndex, 1);                      
        {
            const unsigned char b = record->loadComplete != 0;
            W(fd, &b, 1);                                      
        }
        W(fd, &record->selectedBone, sizeof(record->selectedBone)); 
        for (const std::int32_t selectedMorph :
             mdl::Mdl(model)->selectedMorphs)
            W(fd, &selectedMorph, sizeof(selectedMorph));      

        const unsigned char boneCount = record->groupCount;
        W(fd, &record->groupCount, 1);                         
        if (boneCount != 0) {
            const mdl::DisplayGroup* const groups =
                mdl::DisplayGroups(model);
            for (unsigned char i = 0; i < boneCount; ++i) {    
                const unsigned char b =
                    groups[i].nameEn[0] != 0;
                W(fd, &b, 1);
            }
        }
        W(fd, &record->boneListPos, sizeof(record->boneListPos)); 
        W(fd, &record->maxFrame, sizeof(record->maxFrame));    

        if (dispCount != 0 && dispCount > 0) {
            for (std::uint16_t i = 0;
                 static_cast<std::int32_t>(i) < dispCount; ++i) {
                WritePmmBoneKey(fd, dispKeys[i]);
            }
        }
        {                                                      
            std::int32_t cnt = 0;
            if (dispCount < static_cast<std::int32_t>(mdl::kBoneKeyCapacity)) {
                for (std::int32_t i = dispCount;
                     i < static_cast<std::int32_t>(mdl::kBoneKeyCapacity); ++i)
                    if (dispKeys[i].frame != 0)
                        ++cnt;
            }
            W(fd, &cnt, 4);                                    
            if (dispCount < static_cast<std::int32_t>(mdl::kBoneKeyCapacity)) {                          
                for (std::int32_t i = dispCount;
                     i < static_cast<std::int32_t>(mdl::kBoneKeyCapacity); ++i) {
                    if (dispKeys[i].frame == 0)
                        continue;
                    W(fd, &i, 4);                              
                    WritePmmBoneKey(fd, dispKeys[i]);
                }
            }
        }

        if (morphCount != 0 && morphCount > 0) {
            for (std::int32_t i = 0; i < morphCount; ++i)
                WritePmmMorphKey(fd, morphKeys[i]);
        }
        {                                                      
            std::int32_t cnt = 0;
            if (morphCount < 20000) {
                for (std::int32_t i = morphCount; i < 20000; ++i)
                    if (morphKeys[i].frame != 0)
                        ++cnt;
            }
            W(fd, &cnt, 4);
            if (morphCount < 20000) {
                for (std::int32_t i = morphCount; i < 20000; ++i) {
                    if (morphKeys[i].frame == 0)
                        continue;
                    W(fd, &i, 4);                              
                    WritePmmMorphKey(fd, morphKeys[i]);
                }
            }
        }

        WritePmmDisplayKey(fd, physKeys[0], ikCount, rigidCount);
        {  
            std::int32_t cnt = 0;
            for (std::int32_t i = 1; i < 1000; ++i)
                if (physKeys[i].frame != 0) ++cnt;
            W(fd, &cnt, 4);
            for (std::int32_t i = 1; i < 1000; ++i) {          
                if (physKeys[i].frame == 0)
                    continue;
                W(fd, &i, 4);
                WritePmmDisplayKey(fd, physKeys[i], ikCount, rigidCount);
            }
        }

        if (dispCount != 0 && dispCount > 0) {
            unsigned char* const physicsState = record->bonePhysicsState;
            unsigned char* const selection = record->boneSelection;
            for (std::int32_t i = 0; i < dispCount; ++i) {
                W(fd, &bones[i].trans[0], 4);                   
                W(fd, &bones[i].trans[1], 4);                   
                W(fd, &bones[i].trans[2], 4);                   
                W(fd, &bones[i].rotQuat[0], 4);                 
                W(fd, &bones[i].rotQuat[1], 4);                 
                W(fd, &bones[i].rotQuat[2], 4);                 
                W(fd, &bones[i].rotQuat[3], 4);                 
                {
                    const unsigned char b = bones[i].physicsDisabled != 0;
                    W(fd, &b, 1);                              
                }
                {
                    const unsigned char b = physicsState[i] != 0; 
                    W(fd, &b, 1);
                }
                {
                    const unsigned char b = selection[i] != 0; 
                    W(fd, &b, 1);
                }
            }
        }
        if (morphCount != 0 && morphCount > 0) {
            for (std::int32_t i = 0; i < morphCount; ++i)
                W(fd, &morphs[i].value, sizeof(morphs[i].value));
        }
        if (ikCount != 0 && ikCount > 0) {
            for (std::int32_t i = 0; i < ikCount; ++i) {
                const unsigned char b = ikChains[i].enabled != 0;
                W(fd, &b, 1);                                  
            }
        }
        if (rigidCount != 0 && rigidCount > 0) {
            for (std::int32_t i = 0; i < rigidCount; ++i) {
                W(fd, &rigidFrames[i].windowStart, 4);         
                W(fd, &rigidFrames[i].windowEnd, 4);           
                W(fd, &rigidFrames[i].linkedModel, 4);         
                W(fd, &rigidFrames[i].linkedBone, 4);          
            }
        }
        {
            const unsigned char b =                            
                record->postLoadFlag2 != 0;
            W(fd, &b, 1);
        }
        W(fd, &record->edgeScale, sizeof(record->edgeScale));  
        {
            const unsigned char b =                            
                record->toonFlag != 0;
            W(fd, &b, 1);
        }
        const unsigned char displayOrder = mdl::Mdl(model)->comboSelIndex2;
        W(fd, &displayOrder, sizeof(displayOrder));             
    }
}


void WritePmmCameraSection(int fd, MMDApp* s) {
    const mdl::CameraKey* const cam = s->CameraKeys();  
    WritePmmCameraKey(fd, cam[0]);                              
    {  
        const std::int32_t cnt = CountSparseTrackFrames(cam);   
        W(fd, &cnt, 4);
        for (std::int32_t i = 1; i < 10000; ++i) {             
            if (cam[i].frame == 0)
                continue;
            W(fd, &i, 4);
            WritePmmCameraKey(fd, cam[i]);                      
        }
    }

    W(fd, &s->state.cameraPosition[0], 4);         
    W(fd, &s->state.cameraPosition[1], 4);         
    W(fd, &s->state.cameraPosition[2], 4);         
    W(fd, &s->state.viewOffsetX, 4);         
    W(fd, &s->state.viewOffsetY, 4);         
    W(fd, &s->state.cameraDistance, 4);     
    W(fd, &s->state.cameraPitch, 4);         
    W(fd, &s->state.cameraYaw, 4);         
    W(fd, &s->state.cameraRoll, 4);         
    {
        const unsigned char b =                                
            s->state.cameraPerspective != 0;
        W(fd, &b, 1);
    }
}


void WritePmmLightSection(int fd, MMDApp* s) {
    const mdl::LightKey* const light = s->LightKeys();  
    WritePmmLightKey(fd, light[0]);                             
    {  
        const std::int32_t cnt = CountSparseTrackFrames(light); 
        W(fd, &cnt, 4);
        for (std::int32_t i = 1; i < 10000; ++i) {             
            if (light[i].frame == 0)
                continue;
            W(fd, &i, 4);
            WritePmmLightKey(fd, light[i]);                     
        }
    }

    W(fd, s->LightColor() + 0, sizeof(float));
    W(fd, s->LightColor() + 1, sizeof(float));
    W(fd, s->LightColor() + 2, sizeof(float));
    W(fd, s->LightDirection() + 0, sizeof(float));
    W(fd, s->LightDirection() + 1, sizeof(float));
    W(fd, s->LightDirection() + 2, sizeof(float));
    W(fd, &s->SelectedAccessorySlot(), 1);                      
    W(fd, &s->DisplayObjectListScrollPosition(), 4);
}


void WritePmmAccessoryShadowList(int fd, HWND main, char* text) {
    {
        const unsigned char cnt = static_cast<unsigned char>(
            SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_GETCOUNT, 0, 0));
        W(fd, &cnt, 1);                                        
        for (unsigned int i = 0; i < cnt; ++i) {               
            SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_GETLBTEXT, i,
                         reinterpret_cast<LPARAM>(text));
            W(fd, text, 100);                                  
        }
    }
}


void WritePmmAccessoryBlocks(
    int fd, const D3DRenderer* renderer, mdl::AccessoryRecord** const accessories,
    mdl::AccessoryKey** const accTracks, char* text) {
    for (unsigned char slot = 0; slot != 0xFF; ++slot) {       
        if (accessories[slot] == 0) continue;
        W(fd, &slot, 1);
        const mdl::AccessoryRecord& accessory =
            *mdl::Accessory(accessories[slot]);
        const auto* const track = reinterpret_cast<const mdl::AccessoryKey*>(
            accTracks[slot]);

        W(fd, accessory.name, sizeof accessory.name);          
        WideToSjis(renderer, text,                                   
                       accessory.sourcePath, 0x100);
        W(fd, text, 0x100);                                    
        W(fd, &accessory.order, 1);                             

        WritePmmAccessoryKey(fd, track[0]);
        {  
            std::int32_t cnt = 0;
            for (std::int32_t i = 1; i < 10000; ++i)
                if (track[i].frame != 0) ++cnt;
            W(fd, &cnt, 4);
            for (std::int32_t i = 1; i < 10000; ++i) {         
                if (track[i].frame == 0)
                    continue;
                W(fd, &i, 4);
                WritePmmAccessoryKey(fd, track[i]);
            }
        }
        {
            const unsigned char b = ScaleQuirkByte(            
                accessory.opacity, accessory.visible);
            W(fd, &b, 1);
        }
        W(fd, &accessory.parentModel, 4);                       
        W(fd, &accessory.parentBone, 4);                        
        W(fd, &accessory.rotation[0], 4);                       
        W(fd, &accessory.rotation[1], 4);                       
        W(fd, &accessory.rotation[2], 4);                       
        W(fd, &accessory.scale, 4);                             
        W(fd, &accessory.position[0], 4);                       
        W(fd, &accessory.position[1], 4);                       
        W(fd, &accessory.position[2], 4);                       
        {
            const unsigned char b = accessory.shadowEnabled != 0;
            W(fd, &b, 1);
        }
        {
            const unsigned char b = accessory.additiveBlend != 0;
            W(fd, &b, 1);
        }
    }
}


void WritePmmConfigBlock(int fd, MMDApp* s, HWND main, char* text) {
    W(fd, &s->state.currentFrame, 4);
    W(fd, &s->state.timelineStartFrame, 4);
    W(fd, &s->state.lastRegisteredFrame, 4);
    const std::int32_t savedEditMode = static_cast<std::int32_t>(s->EditMode());
    W(fd, &savedEditMode, 4);
    W(fd, &s->state.cameraReferenceMode, 1);           
    {
        const unsigned char b = s->state.playbackLoopEnabled != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->state.playbackReturnsToStartFrame != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->state.playbackStartsAtCurrentFrame != 0;
        W(fd, &b, 1);
    }
    {  
        GetWindowTextA(GetDlgItem(main, panel::kPlayStartFrameEdit), text, 8);
        const std::int32_t playStartFrame = atol(text);
        GetWindowTextA(GetDlgItem(main, panel::kPlayStopFrameEdit), text, 8);
        const std::int32_t playStopFrame = atol(text);
        W(fd, &playStartFrame, 4);
        W(fd, &playStopFrame, 4);
    }
    {
        const unsigned char b = s->state.waveEnabled != 0;
        W(fd, &b, 1);
    }
    std::memset(text, 0, 0x100);
    WideToSjis(s->Renderer(), text,                                       
                   reinterpret_cast<const wchar_t*>(
                       &s->state.wavPath),
                   0x100);
    W(fd, text, 0x100);                                        
    if (s->AviStream() == nullptr) {                            
        swprintf_s(s->AviBackgroundPath(), 0x100, L"%s",
                   s->AviBackgroundPath());
    }
    W(fd, &s->AviOffsetX(), 4);
    W(fd, &s->AviOffsetY(), 4);
    W(fd, &s->AviScale(), 4);
    std::memset(text, 0, 0x100);
    WideToSjis(s->Renderer(), text, s->AviBackgroundPath(), 0x100);        
    W(fd, text, 0x100);                                        
    W(fd, &s->AviBackgroundEnabled(), 4);
    if (s->PictureBackgroundTexture() == nullptr) {             
        swprintf_s(s->PictureBackgroundPath(), 0x100, L"%s",
                   s->PictureBackgroundPath());
    }
    W(fd, &s->PictureOffsetX(), 4);
    W(fd, &s->PictureOffsetY(), 4);
    W(fd, &s->PictureScale(), 4);
    std::memset(text, 0, 0x100);
    WideToSjis(s->Renderer(), text, s->PictureBackgroundPath(), 0x100);    
    W(fd, text, 0x100);                                        
    {
        const unsigned char b = s->PictureBackgroundEnabled() != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->state.fpsOverlayEnabled != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->state.groundGridEnabled != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->state.groundShadowEnabled != 0;
        W(fd, &b, 1);
    }
    W(fd, &s->state.fpsLimit, 4);     
    W(fd, &reinterpret_cast<ScreenCaptureMode&>(s->state.captureMode), 4);
    W(fd, &s->state.accessoryRenderSplitOrder, 4);
    W(fd, &s->ProjectedShadowAmbientIntensity(), 4);
    {
        const unsigned char b = s->state.projectedShadowBlendEnabled != 0;
        W(fd, &b, 1);
    }
    W(fd, reinterpret_cast<const unsigned char*>(
              &s->PlaybackPhysicsMode()), 1);                  
    W(fd, &s->state.gravityMagnitude, 4);      
    W(fd, &s->state.gravityNoise, 4);
    W(fd, &s->state.gravityX, 4);        
    W(fd, &s->state.gravityY, 4);        
    W(fd, &s->state.gravityZ, 4);        
    {
        const unsigned char b = s->state.gravityNoiseEnabled != 0;
        W(fd, &b, 1);
    }
}


void WritePmmSelectionSection(int fd, MMDApp* s) {
    const mdl::GravityKey* const sel = s->GravityKeys();  
    WritePmmGravityKey(fd, sel[0]);                             
    {  
        const std::int32_t cnt = CountSparseTrackFrames(sel);   
        W(fd, &cnt, 4);
        for (std::int32_t i = 1; i < 10000; ++i) {             
            if (sel[i].frame == 0)
                continue;
            W(fd, &i, 4);
            WritePmmGravityKey(fd, sel[i]);                     
        }
    }

    {
        const unsigned char b = s->state.selfShadowEnabled != 0;
        W(fd, &b, 1);                                          
    }
    W(fd, &s->state.physicsInterval, 4);        
}


void WritePmmShadowAndConfig2(int fd, MMDApp* s, HWND main, char* text) {
    const mdl::SelfShadowKey* const shadow = s->ShadowKeys();  
    WritePmmSelfShadowKey(fd, shadow[0]);                       
    {  
        const std::int32_t cnt = CountSparseTrackFrames(shadow);
        W(fd, &cnt, 4);
        for (std::int32_t i = 1; i < 10000; ++i) {             
            if (shadow[i].frame == 0)
                continue;
            W(fd, &i, 4);
            WritePmmSelfShadowKey(fd, shadow[i]);               
        }
    }

    W(fd, &s->state.modelOutlineColorRed, 4);
    W(fd, &s->state.modelOutlineColorGreen, 4);
    W(fd, &s->state.modelOutlineColorBlue, 4);
    {
        const unsigned char b = s->state.blackBackgroundEnabled != 0;
        W(fd, &b, 1);
    }
    W(fd, &s->state.cameraParentModel, 4);
    W(fd, &s->state.cameraParentBone, 4);
    W(fd, &s->state.cameraAttachmentBasis[0], 4);   
    for (int i = 0; i < 15; ++i)                               
        W(fd, &s->state.cameraAttachmentBasis[1 + i], 4);
    {
        const unsigned char b = s->state.followCameraEnabled != 0;
        W(fd, &b, 1);                                          
    }
    {
        const unsigned char b = s->state.cameraAttachmentTransformSuppressed != 0;
        W(fd, &b, 1);                                          
    }
    {
        const unsigned char b = s->state.floorVisible != 0;
        W(fd, &b, 1);
    }
    {  
        const HWND alt = s->state.floatingWindow;
        GetWindowTextA(GetDlgItem(alt != nullptr ? alt : main, panel::kGotoFrameEdit), text, 10);
        std::int32_t v = atol(text);
        if (v < 0) v = 0;
        W(fd, &v, 4);                                          
    }
    {
        const unsigned char one = 1;                           
        W(fd, &one, 1);
    }
}


void WritePmmModelTail(int fd, unsigned char** const slots) {
    for (unsigned char slot = 0; slot < kModelSlotCount; ++slot) {  
        if (slots[slot] == 0) continue;
        W(fd, &slot, 1);
        const std::int32_t registration =
            mdl::Mdl(slots[slot])->frameRegistrationSelection;
        W(fd, &registration, 4);                               
    }
}


}  

void SaveSceneFile(MMDApp* app) {
    auto* s = app;

    char text[0x100] = {};
    char hdr[0x100];    
    wchar_t title[0x100];

    unsigned char** const slots = s->ModelSlots();
    mdl::AccessoryRecord** const accessories = s->AccessorySlots();
    mdl::AccessoryKey** const accTracks = s->AccessoryKeyTracks();
    HWND const main = reinterpret_cast<HWND>(s->Hwnd());

    if (s->EnvFileName()[0] == L'\\') {                        
        MessageBoxA(main,
                    s->EnglishUI() != 0 ? "Cannot open save file"
                                        : kJpCannotOpenText,
                    "save", 0);
        return;
    }

    s->SceneModified() = 0;                                    
    int fd = -1;
    const errno_t err =
        _wsopen_s(&fd, s->EnvFileName(), 0x8301, 0x40, 0x80);  
    if (err != 0) {
        if (s->EnglishUI() == 0)
            sprintf_s(hdr, 0x100, kJpSaveFailFmt, err);
        else
            sprintf_s(hdr, 0x100, "Cannot save file:%d", err);
        MessageBoxA(main, hdr,
                    s->EnglishUI() == 0 ? kJpSaveFailCaption : "save file",
                    0);
        return;
    }

    WritePmmFileHeader(fd, s, main, hdr);                   
    WritePmmModelBlocks(fd, s->Renderer(), slots, text);                   
    WritePmmCameraSection(fd, s);                           
    WritePmmLightSection(fd, s);                            
    WritePmmAccessoryShadowList(fd, main, text);            
    WritePmmAccessoryBlocks(fd, s->Renderer(), accessories, accTracks, text);  
    WritePmmConfigBlock(fd, s, main, text);                 
    WritePmmSelectionSection(fd, s);                        
    WritePmmShadowAndConfig2(fd, s, main, text);            
    WritePmmModelTail(fd, slots);                           


    _close(fd);
    swprintf_s(title, 0x100, pmm_io::kAppTitleFormat, s->EnvFileName());
    SetWindowTextW(main, title);
    MessageBeep(0x40);
    s->state.windowLayoutReady = 1;

    mme::NotifyPmmSaved(s);
}

}  
