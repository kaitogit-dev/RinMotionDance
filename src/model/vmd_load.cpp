#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <float.h>
#include <io.h>
#include <sys/stat.h>

#include "rinmotiondance/globals.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
void SelectionReeval(MMDApp* app);                   
void RefreshLightPanel(MMDApp* app);                         
void RefreshSelfShadowPanel(MMDApp* app);                         
void ApplyGravityTrack(MMDApp* app);                        

namespace {

const char kCameraLightName[] =  
    "\x83\x4a\x83\x81\x83\x89\x81\x45\x8f\xc6\x96\xbe";
const char kJpTitleMotionLoad[] =  
    "\x83\x82\x81\x5b\x83\x56\x83\x87\x83\x93\x83\x66\x81\x5b\x83\x5e"
    "\x93\xc7\x8d\x9e";
const char kJpNotVmd[] =  
    "\x82\xb1\x82\xcc\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xcd\x20Vocaloid "
    "Motion Data file \x82\xc5\x82\xcd\x82\xa0\x82\xe8\x82\xdc\x82\xb9"
    "\x82\xf1";
const char kJpCannotOpen[] =  
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xaa\x93\xc7\x82\xdd\x8d\x9e"
    "\x82\xdf\x82\xdc\x82\xb9\x82\xf1:%d";
const char kJpNoCamLight[] =  
    "\x82\xb1\x82\xcc\x83\x82\x81\x5b\x83\x56\x83\x87\x83\x93\x83\x66"
    "\x81\x5b\x83\x5e\x82\xc9\x82\xcd\x83\x4a\x83\x81\x83\x89\x81\x45"
    "\x8f\xc6\x96\xbe\x82\xcc\x83\x66\x81\x5b\x83\x5e\x82\xcd\x82\xa0"
    "\x82\xe8\x82\xdc\x82\xb9\x82\xf1";
const char kJpIsCamLight[] =  
    "\x82\xb1\x82\xcc\x83\x82\x81\x5b\x83\x56\x83\x87\x83\x93\x83\x66"
    "\x81\x5b\x83\x5e\x82\xcd\x83\x4a\x83\x81\x83\x89\x81\x45\x8f\xc6"
    "\x96\xbe\x97p\x82\xcc\x83\x66\x81\x5b\x83\x5e\x82\xc5\x82\xb7\n\n"
    "\x83\x4a\x83\x81\x83\x89\x95\xd2\x8f\x57\x83\x82\x81\x5b\x83\x68"
    "\x82\xc5\x93\xc7\x82\xdd\x8d\x9e\x82\xf1\x82\xc5\x89\xba\x82\xb3"
    "\x82\xa2";
const char kJpNoCamLightShadow[] =  
    "\x82\xb1\x82\xcc\x83\x82\x81\x5b\x83\x56\x83\x87\x83\x93\x83\x66"
    "\x81\x5b\x83\x5e\x82\xc9\x82\xcd\x81\x41\x83\x4a\x83\x81\x83\x89"
    "\x81\x45\x8f\xc6\x96\xbe\x81\x45\x83\x5a\x83\x8b\x83\x74\x89\x65"
    "\x82\xcc\x83\x66\x81\x5b\x83\x5e\x82\xcd\x8a\xdc\x82\xdc\x82\xea"
    "\x82\xdc\x82\xb9\x82\xf1";
const char kJpModelMismatch[] =  
    "\x82\xb1\x82\xcc\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xcd\x20%s\x97"
    "p\x82\xcc\x83\x82\x81\x5b\x83\x56\x83\x87\x83\x93\x83\x66\x81\x5b"
    "\x83\x5e\x82\xc5\x82\xb7\n\x93\xaf\x82\xb6\x83\x7b\x81\x5b\x83\x93"
    "\x96\xbc\x82\xcc\x83\x82\x81\x5b\x83\x56\x83\x87\x83\x93\x82\xcc"
    "\x82\xdd\x82\xf0\x8e\xe6\x82\xe8\x8d\x9e\x82\xde\x8e\x96\x82\xc9"
    "\x82\xc8\x82\xe8\x82\xdc\x82\xb7\n\x91\xb1\x8d\x73\x82\xb5\x82\xdc"
    "\x82\xb7\x82\xa9\x81\x48";

}  

int LoadVmdMotion(MMDApp* app, const wchar_t* fileName) {  
    auto& s = *app;
    unsigned char* const model = app->SelectedModel();

    int fileHandle;
    unsigned int count = 0;  
    char text[256], buffer[256];
    unsigned char rec[96];
    unsigned int scratch;    
    unsigned short word;     

    const HWND hwnd = s.state.hwnd;
    const bool english = s.state.englishUI != 0;

    const errno_t openErr =
        _wsopen_s(&fileHandle, fileName,
                  0x8000  , 0x40  , 0x80  );
    if (openErr != 0) {
        if (english)
            sprintf_s(text, 0x100, "Cannot open file:%d", openErr);
        else
            sprintf_s(text, 0x100, kJpCannotOpen, openErr);
        MessageBoxA(hwnd, text, "", 0);
        return 0;
    }

    _read(fileHandle, text, 30);
    int ver = 0;
    unsigned int flavor = 0;
    int nameLen;
    if (std::strcmp(text, "Vocaloid Motion Data file") == 0) {
        ver = 1; flavor = 1; nameLen = 10;
    } else if (std::strcmp(text, "Vocaloid Motion Data 0002") == 0) {
        ver = 2; flavor = 2; nameLen = 20;
    } else {
        MessageBoxA(hwnd,
                    english ? "This is not Vocaloid Motion Data file !!"
                            : kJpNotVmd,
                    english ? "load motion file" : kJpTitleMotionLoad, 0);
        _close(fileHandle);
        return 0;
    }
    _read(fileHandle, text, nameLen);

    if (s.state.optflag[0] != 0) {
        if (ver == 2 && std::memcmp(text, kCameraLightName, 13) != 0) {
            MessageBoxA(hwnd,
                        english
                            ? "This motion file has not cammera,light,shadow data."
                            : kJpNoCamLightShadow,
                        english ? "load motion data" : kJpTitleMotionLoad, 0);
            _close(fileHandle);
            return 0;
        }
        _read(fileHandle, &count, 4);
        for (unsigned int i = 0; i < count; ++i) {
            _read(fileHandle, text, 15);
            for (int w = 0; w < 24; ++w) _read(fileHandle, &scratch, 4);
        }
        _read(fileHandle, &count, 4);
        for (unsigned int j = 0; j < count; ++j) {
            _read(fileHandle, text, 15);
            _read(fileHandle, &scratch, 4);
            _read(fileHandle, &scratch, 4);
        }
        if (_read(fileHandle, &count, 4) == 0) {
            MessageBoxA(hwnd,
                        english
                            ? "This motion data has not camera/light data !!"
                            : kJpNoCamLight,
                        english ? "load motion data" : kJpTitleMotionLoad, 0);
            _close(fileHandle);
            return 0;
        }
        if (ver == 1) {
            unsigned char ok = 1;
            for (unsigned int k = 0; k < count; ++k) {
                std::memset(rec, 0, sizeof rec);
                *reinterpret_cast<int*>(rec + 64) = -1;
                *reinterpret_cast<int*>(rec + 68) = 0;
                _read(fileHandle, rec + 0, 4);    
                _read(fileHandle, rec + 60, 4);   
                _read(fileHandle, rec + 4, 4);    
                _read(fileHandle, rec + 8, 4);
                _read(fileHandle, rec + 12, 4);
                _read(fileHandle, rec + 16, 4);   
                _read(fileHandle, rec + 20, 4);
                _read(fileHandle, rec + 24, 4);
                _read(fileHandle, rec + 33, 1);   
                _read(fileHandle, rec + 45, 1);
                _read(fileHandle, rec + 39, 1);
                _read(fileHandle, rec + 51, 1);
                for (int c = 1; c <= 5; ++c) {
                    rec[33 + c] = rec[33]; rec[45 + c] = rec[45];
                    rec[39 + c] = rec[39]; rec[51 + c] = rec[51];
                }
                *reinterpret_cast<int*>(rec + 28) = 45;  
                rec[32] = 0;                             
                if (ok && !RegisterCameraKey(app, rec)) ok = 0;
            }
        } else {
            unsigned char ok = 1;
            for (unsigned int k = 0; k < count; ++k) {
                std::memset(rec, 0, sizeof rec);
                *reinterpret_cast<int*>(rec + 64) = -1;
                *reinterpret_cast<int*>(rec + 68) = 0;
                _read(fileHandle, rec + 0, 4);
                _read(fileHandle, rec + 60, 4);
                _read(fileHandle, rec + 4, 4);
                _read(fileHandle, rec + 8, 4);
                _read(fileHandle, rec + 12, 4);
                _read(fileHandle, rec + 16, 4);
                _read(fileHandle, rec + 20, 4);
                _read(fileHandle, rec + 24, 4);
                for (int c = 0; c < 6; ++c) {     
                    _read(fileHandle, rec + c + 33, 1);
                    _read(fileHandle, rec + c + 45, 1);
                    _read(fileHandle, rec + c + 39, 1);
                    _read(fileHandle, rec + c + 51, 1);
                }
                _read(fileHandle, rec + 28, 4);   
                _read(fileHandle, &word, 1);
                rec[32] = static_cast<unsigned char>(word != 0);
                if (ok && !RegisterCameraKey(app, rec)) ok = 0;
            }
        }
        _read(fileHandle, &count, 4);
        unsigned char lightOk = 1;
        for (unsigned int k = 0; k < count; ++k) {
            std::memset(rec, 0, sizeof rec);
            _read(fileHandle, rec + 0, 4);    
            _read(fileHandle, rec + 16, 4);   
            _read(fileHandle, rec + 20, 4);
            _read(fileHandle, rec + 24, 4);
            _read(fileHandle, rec + 4, 4);    
            _read(fileHandle, rec + 8, 4);
            _read(fileHandle, rec + 12, 4);
            if (lightOk && !RegisterLightKey(app, rec)) lightOk = 0;
        }
        unsigned char shadowOk = 1;
        if (flavor == 2 && _read(fileHandle, &count, 4) > 0) {
            for (unsigned int m = 0; m < count; ++m) {
                std::memset(rec, 0, sizeof rec);
                _read(fileHandle, rec + 0, 4);
                _read(fileHandle, rec + 4, 1);
                _read(fileHandle, rec + 8, 4);
                if (shadowOk && !RegisterSelfShadowKey(app, rec)) shadowOk = 0;
            }
        }
        _close(fileHandle);
        PanelPaint(app);
        SelectionReeval(app);
        ReloadModels(app);                              
        RefreshLightPanel(app);
        RefreshSelfShadowPanel(app);
        ApplyGravityTrack(app);
        return 0;
    }

    if (std::strcmp(text, mdl::Mdl(model)->name) != 0) {
        if (std::memcmp(text, kCameraLightName, 13) == 0) {
            MessageBoxA(hwnd,
                        english
                            ? "This motion data if camera/light data !!"
                            : kJpIsCamLight,
                        english ? "load motion data" : kJpTitleMotionLoad, 0);
            _close(fileHandle);
            return 0;
        }
        if (english)
            sprintf_s(buffer, 0x100,
                      "This motion file is the data for '%s'.\n"
                      "You can regist the motion only same bone name.\n"
                      "Are you OK?", text);
        else
            sprintf_s(buffer, 0x100, kJpModelMismatch, text);
        if (MessageBoxA(hwnd, buffer,
                        english ? "load motion data" : kJpTitleMotionLoad,
                        1  ) != 1  ) {
            _close(fileHandle);
            return 0;
        }
    }
    _read(fileHandle, &count, 4);
    const unsigned int boneKeyCount = count;

    rinmotiondance::mdl::BoneKey* const bkeys = rinmotiondance::mdl::BoneKeys(model);
    rinmotiondance::mdl::MorphKey* const mkeys = rinmotiondance::mdl::MorphKeys(model);
    rinmotiondance::mdl::DisplayKey* const ikeys = rinmotiondance::mdl::DisplayKeys(model);
    for (int i = 0; i < static_cast<int>(mdl::kBoneKeyCapacity); ++i) bkeys[i].allocated = 0;
    for (int i = 0; i < 20000; ++i) mkeys[i].allocated = 0;
    for (int i = 0; i < 1000; ++i) ikeys[i].allocated = 0;

    EnableWindow(GetDlgItem(hwnd, panel::kUndoButton), TRUE);
    EnableWindow(GetDlgItem(hwnd, panel::kRedoButton), FALSE);
    mdl::ModelRecord& record = *mdl::Mdl(model);
    record.undoDirty = 1;
    record.redoDirty = 0;
    std::uint32_t& ringIdx = record.undoState[0];
    if (++ringIdx >= 30) ringIdx = 0;
    record.undoState[1] = ringIdx;

    auto& undo = rinmotiondance::mdl::Mdl(model)->undoRings[0].slots[ringIdx];
    undo.operation = 2;
    undo.dirty = 0;
    undo.frame = s.state.currentFrame;
    if (undo.bonePose != nullptr)
        operator delete(undo.bonePose);  
    const int boneCnt = static_cast<int>(record.boneCount);
    auto* const snap = static_cast<rinmotiondance::mdl::BonePoseSnapshot*>(
        operator new(boneCnt * sizeof(rinmotiondance::mdl::BonePoseSnapshot)));
    undo.bonePose = snap;
    std::memset(snap, 0,
                static_cast<std::size_t>(boneCnt) *
                    sizeof(rinmotiondance::mdl::BonePoseSnapshot));
    if (boneCnt > 0) {
        rinmotiondance::mdl::BoneRecord* const bones =
            rinmotiondance::mdl::Bones(model);
        const unsigned char* const flags =
            rinmotiondance::mdl::Mdl(model)->bonePhysicsState;
        for (int b = 0; b < boneCnt; ++b) {
            rinmotiondance::mdl::BonePoseSnapshot& slot = snap[b];
            slot.boneIndex = b;
            std::memcpy(slot.position, bones[b].trans, sizeof slot.position);
            std::memcpy(slot.rotation, bones[b].rotQuat, sizeof slot.rotation);
            slot.physicsDisabled = flags[b];
        }
    }
    if (undo.auxiliaryPose != nullptr)
        operator delete(undo.auxiliaryPose);  
    unsigned char* const rawKeys = static_cast<unsigned char*>(
        operator new(192 * boneKeyCount));
    undo.auxiliaryPose = rawKeys;
    std::memset(rawKeys, 0, 192 * boneKeyCount);
    std::memset(mdl::Mdl(model)->keyVisitMap, 0,
                sizeof(mdl::Mdl(model)->keyVisitMap));
    ResetBoneKeyCursor(model);
    (void)rawKeys;  

    unsigned char boneOk = 1;
    for (unsigned int k = 0; k < boneKeyCount; ++k) {
        _read(fileHandle, text, 15);
        if (char* nl = std::strchr(text, 10)) *nl = 0;
        strcpy_s(reinterpret_cast<char*>(rec), 30, text);
        _read(fileHandle, rec + 32, 4);   
        _read(fileHandle, rec + 52, 4);   
        _read(fileHandle, rec + 56, 4);
        _read(fileHandle, rec + 60, 4);
        _read(fileHandle, rec + 36, 4);   
        _read(fileHandle, rec + 40, 4);
        _read(fileHandle, rec + 44, 4);
        _read(fileHandle, rec + 48, 4);
        for (int ch = 0; ch < 4; ++ch) {
            if (ch != 0) {
                _read(fileHandle, &scratch, 4);
                rec[ch + 65] = static_cast<unsigned char>(scratch);
            } else {
                _read(fileHandle, &word, 2);
                rec[65] = static_cast<unsigned char>(word);
                _read(fileHandle, &word, 2);
                rec[64] = static_cast<unsigned char>(word == 3939);
            }
            _read(fileHandle, &scratch, 4);
            rec[ch + 69] = static_cast<unsigned char>(scratch);
            _read(fileHandle, &scratch, 4);
            rec[ch + 73] = static_cast<unsigned char>(scratch);
            _read(fileHandle, &scratch, 4);
            rec[ch + 77] = static_cast<unsigned char>(scratch);
        }
        if (boneOk) {
            if (!RegisterBoneKey(model, rec,
                           s.state.currentFrame, 0))
                boneOk = 0;
        }
    }
    ResetMorphKeyCursor(model);

    _read(fileHandle, &count, 4);
    unsigned char morphOk = 1;
    for (unsigned int k = 0; k < count; ++k) {
        _read(fileHandle, text, 15);
        text[15] = 0;
        if (char* nl = std::strchr(text, 10)) *nl = 0;
        strcpy_s(reinterpret_cast<char*>(rec), 30, text);
        _read(fileHandle, rec + 32, 4);   
        _read(fileHandle, rec + 36, 4);   
        if (!_finite(*reinterpret_cast<float*>(rec + 36)))
            *reinterpret_cast<float*>(rec + 36) = 0.0f;
        if (morphOk) {
            if (!RegisterMorphKeyFromRecord(model, rec,
                           s.state.currentFrame))
                morphOk = 0;
        }
    }
    unsigned int modelMax = record.maxFrame;
    if (s.LastRegisteredFrame() < modelMax)
        s.LastRegisteredFrame() = modelMax;

    _read(fileHandle, &count, 4);
    _read(fileHandle, &count, 4);
    _read(fileHandle, &count, 4);
    if (_read(fileHandle, &count, 4) > 0) {
        ResetDisplayKeyCursor(model);
        unsigned char ikOk = 1;
        for (unsigned int k = 0; k < count; ++k) {
            _read(fileHandle, rec + 0, 4);   
            _read(fileHandle, &word, 1);     
            rec[4] = static_cast<unsigned char>(word != 0);
            _read(fileHandle, rec + 8, 4);   
            int n = *reinterpret_cast<int*>(rec + 8);
            if (*reinterpret_cast<unsigned int*>(rec + 8) > 0x2710u) break;
            unsigned char* entries = nullptr;
            if (n > 0)
                entries = static_cast<unsigned char*>(
                    operator new(21 * n));  
            for (int e = 0; e < n; ++e) {
                _read(fileHandle, entries + 21 * e, 20);
                _read(fileHandle, &word, 1);
                entries[21 * e + 20] = static_cast<unsigned char>(word != 0);
            }
            *reinterpret_cast<int*>(rec + 16) = 0;
            if (ikOk) {
                if (!RegisterDisplayKeyFromRecord(model, *reinterpret_cast<int*>(rec), rec[4],
                               n, entries, 0, nullptr,
                               s.state.currentFrame))
                    ikOk = 0;
            }
            if (entries) operator delete(entries);
        }
        modelMax = record.maxFrame;
        if (s.LastRegisteredFrame() < modelMax)
            s.LastRegisteredFrame() = modelMax;
    }

    _close(fileHandle);
    PanelPaint(app);                                    
    SelectionReeval(app);                               
    return SeekModelFrame(model, s.state.currentFrame,
                     s.PlaybackPhysicsMode());
}

void LoadVmdFile(const wchar_t* path) {
    if (g_Block == nullptr) return;
    LoadVmdMotion(g_Block, path);
}

}  
