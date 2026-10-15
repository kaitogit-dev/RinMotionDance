#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <btBulletDynamicsCommon.h>
#include <cmath>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <io.h>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/d3dx_dyn.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/scene_ownership.hpp"
#include "rinmotiondance/panel_controls.hpp"

#include "pmm_io_common.hpp"

#pragma comment(lib, "avifil32.lib")

namespace rinmotiondance {

using namespace pmm_io;

namespace {

constexpr float kPhysicsIntervalDefault = 0.01125f;
static_assert(
    __builtin_bit_cast(std::uint32_t, kPhysicsIntervalDefault) == 0x3C3851ECu,
    "flt_52A1D8 bit-exact");

inline std::int32_t& M32(unsigned char* m, std::size_t o) {
    return *reinterpret_cast<std::int32_t*>(m + o);
}
inline std::uint8_t& M8(unsigned char* m, std::size_t o) { return m[o]; }
inline float& MF(unsigned char* m, std::size_t o) {
    return *reinterpret_cast<float*>(m + o);
}
inline unsigned char*& MP(unsigned char* m, std::size_t o) {
    return *reinterpret_cast<unsigned char**>(m + o);
}

inline unsigned char* AllocBytes(std::size_t n) {
    return static_cast<unsigned char*>(::operator new(n));
}



#ifdef RINMOTIONDANCE_DIAG
void LogV1Stage(const char* stage, long pos, std::int32_t a = 0,
                std::int32_t b = 0) {
    const char* dir = std::getenv("RINMOTIONDANCE_PMM_TRACE_DIR");
    if (dir == nullptr || dir[0] == '\0')
        return;
    char p[MAX_PATH];
    sprintf_s(p, "%s\\pmm_v1_load.log", dir);
    FILE* stream = nullptr;
    if (fopen_s(&stream, p, "ab") != 0 || stream == nullptr)
        return;
    fprintf(stream, "stage=%s file_pos=%ld a=%ld b=%ld\r\n", stage, pos,
            static_cast<long>(a), static_cast<long>(b));
    fclose(stream);
}
#else
inline void LogV1Stage(const char*, long, std::int32_t = 0,
                       std::int32_t = 0) {}
#endif

constexpr std::size_t kTrackCam =
    sizeof(mdl::CameraKey) * mdl::kTimelineKeyCapacity;      
constexpr std::size_t kTrackLight =
    sizeof(mdl::LightKey) * mdl::kTimelineKeyCapacity;       
constexpr std::size_t kTrackSel =
    sizeof(mdl::SelfShadowKey) * mdl::kTimelineKeyCapacity;  
constexpr std::size_t kTrackShadow =
    sizeof(mdl::GravityKey) * mdl::kTimelineKeyCapacity;     
constexpr std::size_t kTrackAcc =
    sizeof(mdl::AccessoryKey) * mdl::kTimelineKeyCapacity;   

}  

namespace {

const char kJpChainCapDisp[] = "\x83Z\x81[\x83u\x83\x66\x81[\x83^\x82\xCC\x88\xD9\x8F\xED";   
const char kJpChainFmtPhys[] =
    "\x22%s\x22\x83\x82\x83\x66\x83\x8B\x81\x41\x83{\x81[\x83"
    "\x93\x22%s\x22\x82\xCC\x83t\x83\x8C\x81[\x83\x80\x83\x66"
    "\x81[\x83^\x82\xC9\x88\xD9\x8F\xED\x82\xAA\x8C\xA9\x82\xC2"
    "\x82\xA9\x82\xE8\x82\xDC\x82\xB5\x82\xBD\x0A\x0A\x88\xD9"
    "\x8F\xED\x82\xC8\x83t\x83\x8C\x81[\x83\x80(\x83t\x83\x8C"
    "\x81[\x83\x80\x94\xD4\x8D\x86%d\x88\xC8\x8D~)\x82\xF0\x8D"
    "\xED\x8F\x9C\x82\xB5\x82\xDC\x82\xB7\x0A%d\x83t\x83\x8C\x81"
    "[\x83\x80\x88\xC8\x8D~\x82\xCC%s\x82\xCC\x83\x82\x81[\x83"
    "V\x83\x87\x83\x93\x82\xF0\x8D\xC4\x93x\x90\xDD\x92\xE8\x82"
    "\xB5\x92\xBC\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2";   
const char kJpChainFmtDisp[] =
    "\x22%s\x22\x83\x82\x83\x66\x83\x8B\x81\x41\x22\x95\x5C\x8E"
    "\xA6\xA5IK\xA5\x8AO\x90\x65\x22\x82\xCC\x83t\x83\x8C\x81"
    "[\x83\x80\x83\x66\x81[\x83^\x82\xC9\x88\xD9\x8F\xED\x82\xAA"
    "\x8C\xA9\x82\xC2\x82\xA9\x82\xE8\x82\xDC\x82\xB5\x82\xBD"
    "\x0A\x0A\x88\xD9\x8F\xED\x82\xC8\x22\x95\x5C\x8E\xA6\xA5"
    "IK\xA5\x8AO\x90\x65\x22\x83t\x83\x8C\x81[\x83\x80\x81i\x83"
    "t\x83\x8C\x81[\x83\x80\x94\xD4\x8D\x86%d\x88\xC8\x8D~\x81"
    "j\x82\xF0\x8D\xED\x8F\x9C\x82\xB5\x82\xDC\x82\xB7\x0A\x90"
    "\x5C\x82\xB5\x96\xF3\x82\xA0\x82\xE8\x82\xDC\x82\xB9\x82"
    "\xF1\x82\xAA\x81\x41%s\x83\x82\x83\x66\x83\x8B\x82\xCC%d"
    "\x83t\x83\x8C\x81[\x83\x80\x88\xC8\x8D~\x82\xCC\x22\x95\x5C"
    "\x8E\xA6\xA5IK\x22\x82\xF0\x8D\xC4\x93x\x90\xDD\x92\xE8\x82"
    "\xB5\x92\xBC\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2";   
const char kJpModelFailFmt[] =
    "%s\x82\xCC\x83\x82\x83\x66\x83\x8B\x83t\x83@\x83\x43\x83"
    "\x8B\x82\xAA\x8C\xA9\x82\xC2\x82\xA9\x82\xE8\x82\xDC\x82"
    "\xB9\x82\xF1\x0A\x0A\x83\x82\x83\x66\x83\x8B\x83t\x83@\x83"
    "\x43\x83\x8B\x82\xCC\x8F\xEA\x8F\x8A\x82\xF0\x8Ew\x92\xE8"
    "\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2\x0A\x0A(\x83|\x83"
    "\x8A\x83S\x83\x93\x92\xB8\x93_\x90\x94\x82\xE2\x83{\x81["
    "\x83\x93\x8D\x5C\x91\xA2\x81\x41\x95\x5C\x8F\xEE\x82\xCC"
    "\x8E\xED\x97\xDE\x93\x99\x91S\x82\xAD\x93\xAF\x82\xB6\x83"
    "\x82\x83\x66\x83\x8B\x83t\x83@\x83\x43\x83\x8B\x82\xC5\x82"
    "\xC8\x82\xA2\x82\xC6\x0A\x93\xC7\x82\xDD\x8D\x9E\x82\xDD"
    "\x83G\x83\x89\x81[\x82\xAA\x94\xAD\x90\xB6\x82\xB5MMD\x82"
    "\xAA\x8B\xAD\x90\xA7\x8FI\x97\xB9\x82\xB7\x82\xE9\x89\xC2"
    "\x94\x5C\x90\xAB\x82\xAA\x82\xA0\x82\xE8\x82\xDC\x82\xB7"
    "\x82\xCC\x82\xC5\x92\x8D\x88\xD3\x82\xB5\x82\xC4\x89\xBA"
    "\x82\xB3\x82\xA2)";   
const char kJpCannotOpenAcc[] =
    "\x83\x41\x83N\x83Z\x83T\x83\x8A\x83t\x83@\x83\x43\x83\x8B"
    "(%s)\x82\xAA\x8C\xA9\x82\xC2\x82\xA9\x82\xE8\x82\xDC\x82"
    "\xB9\x82\xF1\x0A\x0A%s\x82\xCC\x8F\xEA\x8F\x8A\x82\xF0\x8E"
    "w\x92\xE8\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2(\x91\xE3"
    "\x91\xD6\x89\xC2)";   

}  

namespace {

struct PmmV1LoadContext {
    MMDApp* s;
    HWND main;
    PathResolutionWorkspace* paths;
    D3DRenderer* wrap;
    unsigned char** slots;
    mdl::AccessoryRecord** accs;
    mdl::AccessoryKey** accTracks;
    char text[0x100];          
    char box[0x3E8];           
    char mbPath[0x100];        
    wchar_t widePath[0x100];   
    wchar_t wideTmp[0x100];    
    wchar_t ofnFile[0x100];    
    wchar_t ofnTitle[0x100];
    wchar_t wndText[0x100];    
    char lbText[0x100];        
    mdl::CameraKey* cameraKeys = nullptr;
    mdl::LightKey* lightKeys = nullptr;
    mdl::SelfShadowKey* selfShadowKeys = nullptr;

    explicit PmmV1LoadContext(MMDApp* app)
        : s(app),
          main(reinterpret_cast<HWND>(app->Hwnd())),
          paths(&app->PathWorkspace()),
          wrap(app->Renderer()),
          slots(app->ModelSlots()),
          accs(app->AccessorySlots()),
          accTracks(app->AccessoryKeyTracks()) {}
};

static void AbortV1Load(MMDApp* s, int fd) {
    _close(fd);                                              
    ResetAppState(s);                                        
    HandleWindowSize(s);                                     
}

static void LoadSceneV1_DisposeAndHeader(PmmV1LoadContext& ctx, int fd) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    char* const text = ctx.text;

    ReleaseSceneModels(*s);                                      
    ReleaseGlobalTimelineTracks(*s);                              
    ReleaseAccessoriesAndTracks(*s);                              
    s->state.accessoryRenderSplitOrder = 0;
    s->SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
    s->state.mainModelComboSelection = 0;
    s->state.cameraParentModel = -1;
    s->state.cameraParentBone = 0;
    s->state.cameraAttachmentBasis[14] = 0;               
    s->state.cameraAttachmentBasis[13] = 0;               
    s->state.cameraAttachmentBasis[12] = 0.0f;                    
    s->state.cameraAttachmentBasis[11] = 0.0f;                    
    s->state.cameraAttachmentBasis[9] = 0.0f;                     
    s->state.cameraAttachmentBasis[8] = 0.0f;                     
    s->state.cameraAttachmentBasis[7] = 0.0f;                     
    s->state.cameraAttachmentBasis[6] = 0.0f;                     
    s->state.cameraAttachmentBasis[4] = 0.0f;                     
    s->state.cameraAttachmentBasis[3] = 0.0f;                     
    s->state.cameraAttachmentBasis[2] = 0.0f;                     
    s->state.cameraAttachmentBasis[1] = 0.0f;                     
    s->state.cameraAttachmentBasis[15] = 1.0f;                    
    s->state.cameraAttachmentBasis[10] = 1.0f;                     
    s->state.cameraAttachmentBasis[5] = 1.0f;                 
    s->state.cameraAttachmentBasis[0] = 1.0f;                    
    s->state.followCameraEnabled = 0;                
    CheckMenuItem(GetMenu(main), 0xF7, 0);                       
    SendMessageA(GetDlgItem(main, panel::kFollowCameraCheckbox), BM_SETCHECK, 0, 0);    

    s->state.physicsInterval = kPhysicsIntervalDefault;           
    s->state.selfShadowMode = 0;                 

    if (s->AviFrameReader() != nullptr) {
        AVIStreamGetFrameClose(s->AviFrameReader());
        s->AviFrameReader() = nullptr;
    }
    if (s->AviStream() != nullptr) {
        AVIStreamRelease(s->AviStream());
        s->AviStream() = nullptr;
    }
    if (s->AviFile() != nullptr) {
        AVIFileRelease(s->AviFile());
        s->AviFile() = nullptr;
    }
    s->AviBackgroundEnabled() = 0;                               
    swprintf_s(s->AviBackgroundPath(), 0x100, L"");              

    if (s->AviBackgroundTexture() != nullptr) {                   
        s->AviBackgroundTexture()->Release();
        s->AviBackgroundTexture() = nullptr;
    }

    Rd(fd, &s->state.renderW, 4);       
    Rd(fd, &s->state.renderH, 4);       
    {
        std::int32_t editFlag = 0;
        Rd(fd, &editFlag, 4);                                    
        if (s->state.floatingWindow == 0)
            s->state.sidebarWidth = editFlag;   
    }
    Rd(fd, &s->state.cameraFov, 4);          
    for (int f = 0; f < 6; ++f) {
        unsigned char b = 0;
        Rd(fd, &b, 1);                                           
        s->state.optflag[f] = (b == 1) ? 1 : 0;       
    }
    strcpy_s(text, 0x100, "");                                   

    for (int id = 478; id <= 484; ++id) {  
        HWND item = GetDlgItem(main, id);
        const int len = GetWindowTextLengthA(item);
        SendMessageA(item, EM_SETSEL, 0, len);
        SendMessageA(item, EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(text));
    }
    SendMessageA(GetDlgItem(main, panel::kShadowCheckbox), BM_SETCHECK, 0, 0);     
    SendMessageA(GetDlgItem(main, panel::kAddBlendCheckbox), BM_SETCHECK, 0, 0);     
    SendMessageA(GetDlgItem(main, panel::kAccessoryAddBlendCheckbox), BM_SETCHECK, 0, 0);     
    CheckMenuItem(GetMenu(main), 0xFE, 0);                       
    EnableMenuItem(GetMenu(main), 0x120, 1);                     
    EnableMenuItem(GetMenu(main), 0x121, 1);                     
}


static unsigned char LoadSceneV1_ModelPrePass(PmmV1LoadContext& ctx,
                                            int fd) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    char* const text = ctx.text;

    s->state.projectedShadowBlendEnabled = 0;                   
    Rd(fd, &s->SelectedModelSlot(), 1);                            
    unsigned char modelCount = 0;
    Rd(fd, &modelCount, 1);                                      

    SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_RESETCONTENT, 0, 0); 
    if (s->EnglishUI() == 0)
        SendMessageW(GetDlgItem(main, panel::kMainComboModel), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kWCamLight));       
    else
        SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("camera/light/accessory"));
    SendMessageA(GetDlgItem(main, panel::kMainComboGround), CB_RESETCONTENT, 0, 0); 
    if (s->EnglishUI() == 0)
        SendMessageW(GetDlgItem(main, panel::kMainComboGround), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kWGround));         
    else
        SendMessageA(GetDlgItem(main, panel::kMainComboGround), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>("ground"));
    SendMessageA(GetDlgItem(main, panel::kMainComboNormal), CB_RESETCONTENT, 0, 0); 
    if (s->EnglishUI() == 0)
        SendMessageW(GetDlgItem(main, panel::kMainComboNormal), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kWNashi));          
    else
        SendMessageA(GetDlgItem(main, panel::kMainComboNormal), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(kNon));
    SendMessageA(GetDlgItem(main, panel::kBoneRegisterCombo), CB_RESETCONTENT, 0, 0); 

    for (unsigned char i = 0; i < modelCount; ++i) {
        Rd(fd, text, 0x14);                                      
        SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(text));
        SendMessageA(GetDlgItem(main, panel::kMainComboGround), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(text));
        SendMessageA(GetDlgItem(main, panel::kMainComboNormal), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(text));
    }
    return modelCount;
}


static bool LoadSceneV1_ModelBlock(PmmV1LoadContext& ctx, int fd) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    PathResolutionWorkspace& paths = *ctx.paths;
    D3DRenderer* const wrap = ctx.wrap;
    unsigned char** const slots = ctx.slots;
    char* const box = ctx.box;
    char* const mbPath = ctx.mbPath;
    wchar_t* const widePath = ctx.widePath;
    wchar_t* const ofnFile = ctx.ofnFile;
    wchar_t* const ofnTitle = ctx.ofnTitle;

            unsigned char slotByte = 0;
            Rd(fd, &slotByte, 1);                                
            unsigned char* nm =
                static_cast<unsigned char*>(operator new(mdl::kSize));
            if (nm != nullptr) IdentityCtor(nm);                    
            slots[slotByte] = nm;                                
            std::memset(slots[slotByte], 0, mdl::kSize);         
            ModelInitDefaults(slots[slotByte]);                  

            char name20[0x14];
            Rd(fd, name20, 0x14);                                
            Rd(fd, mbPath, 0x100);                               
            ResolveAnsiUserFile(reinterpret_cast<unsigned char*>(wrap),
                                mbPath, widePath, 0x100,
                                paths);                           

            LogV1Stage("model-resolved", _tell(fd), slotByte);
            unsigned char* model = slots[slotByte];
            bool loaded = ModelLoadPMD(                          
                model, main, widePath, wrap,
                static_cast<int>(
                    reinterpret_cast<std::uintptr_t>(s->state.exeDir)),
                0, s->EnglishUI(), s->Physics(),
                paths);
            if (!loaded) {                                       
                sprintf_s(box, 0x3E8,
                          s->EnglishUI() != 0
                              ? "Cannot open the model file:%s"
                                "\n\nPlease select the same model"
                              : kJpModelFailFmt,
                          name20);
                MessageBoxA(main, box,
                            s->EnglishUI() != 0 ? "open file"
                                                : kJpOpenCaption,
                            0);                                  
                SetCurrentDirectoryW(reinterpret_cast<const wchar_t*>(
                    s->state.exeDir));                         
                swprintf_s(ofnFile, 0x100, L"");                 
                OPENFILENAMEW ofn;
                std::memset(&ofn, 0, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);                          
                ofn.hwndOwner =
                    s->state.floatingWindow
                        ? reinterpret_cast<HWND>(
                              s->state.floatingWindow)
                        : main;
                ofn.lpstrFilter = kWFilterModel;
                ofn.lpstrFile = ofnFile;
                ofn.nMaxFile = 0x100;
                ofn.Flags = OFN_FILEMUSTEXIST;
                ofn.lpstrInitialDir =
                    (GetMenuState(GetMenu(main), 0x12D, 0) & 8)
                        ? reinterpret_cast<LPCWSTR>(s->state.dirModel)
                        : kWUserModel;
                ofn.lpstrDefExt = L"pmd;pmx";
                ofn.nMaxFileTitle = 0x100;
                ofn.lpstrFileTitle = ofnTitle;
                ofn.lpstrTitle =
                    s->EnglishUI() != 0
                        ? L"load model"
                        : reinterpret_cast<LPCWSTR>(kWOpenFile);
                if (!GetOpenFileNameW(&ofn)) {                   
                    AbortV1Load(s, fd);                                 
                    return false;
                }
                if (GetMenuState(GetMenu(main), 0x12D, 0) & 8) {
                    ExtractDirFromPath(ofnTitle, ofnFile);       
                    wcscpy_s(reinterpret_cast<wchar_t*>(
                                 s->state.dirModel),
                             0x3E8, ofnTitle);                   
                }
                if (!ModelLoadPMD(                               
                        model, main, ofnFile, wrap,
                        static_cast<int>(reinterpret_cast<
                            std::uintptr_t>(s->state.exeDir)),
                        0, s->EnglishUI(),
                        s->Physics(),
                        paths)) {
                    MessageBoxA(
                        main,
                        s->EnglishUI() != 0 ? "Cannot open the model file"
                                           : kJpCannotOpenModel,
                        s->EnglishUI() != 0 ? "open file" : kJpOpenCaption,
                        0);
                }
            }

            LogV1Stage("model-loaded", _tell(fd), loaded ? 1 : 0);
            mdl::ModelRecord* const record = mdl::Mdl(model);
            Rd(fd, &record->comboSelIndex, 1);                   
            record->comboSelIndex2 = record->comboSelIndex;
            s->state.mainModelComboSelection =
                record->comboSelIndex;
            {
                unsigned char b = 0;
                Rd(fd, &b, 1);                                   
                rinmotiondance::mdl::Mdl(model)->loadComplete = (b == 1) ? 1 : 0;
            }
            Rd(fd, &record->selectedBone, sizeof(record->selectedBone));
            for (std::int32_t& selectedMorph :
                 mdl::Mdl(model)->selectedMorphs)
                Rd(fd, &selectedMorph, sizeof(selectedMorph));   
            unsigned char serializedGroupCount = 0;
            Rd(fd, &serializedGroupCount, 1);                    
            mdl::DisplayGroup* const displayGroups =
                mdl::DisplayGroups(model);
            const unsigned char loadedGroupCount =
                mdl::Mdl(model)->groupCount;
            for (unsigned char g = 0; g < serializedGroupCount; ++g) {
                unsigned char b = 0;
                Rd(fd, &b, 1);                                   
                if (displayGroups != nullptr && g < loadedGroupCount)
                    displayGroups[g].flags = (b == 1) ? 1 : 0;
            }
            LogV1Stage("misc-done", _tell(fd),
                        static_cast<int>(rinmotiondance::mdl::Mdl(model)->boneCount),
                        static_cast<int>(rinmotiondance::mdl::Mdl(model)->morphCount));
            Rd(fd, &record->boneListPos, sizeof(record->boneListPos));
            Rd(fd, &record->maxFrame, sizeof(record->maxFrame));

            {
                const std::int32_t boneCnt =
                    static_cast<std::int32_t>(record->boneCount);
                mdl::BoneKey* keys = mdl::BoneKeys(model);
                for (std::int32_t i = 0; i < boneCnt; ++i) {
                    ReadPmmBoneKey<PmmStream::V1>(fd, keys[i]);
                }
                std::int32_t cnt = 0;
                Rd(fd, &cnt, 4);
                for (std::int32_t i = 0; i < cnt; ++i) {
                    std::int32_t keyIndex = 0;
                    Rd(fd, &keyIndex, 4);
                    ReadPmmBoneKey<PmmStream::V1>(fd, keys[keyIndex]);
                }
            }
            {
                const std::int32_t morphCnt =
                    static_cast<std::int32_t>(record->morphCount);
                mdl::MorphKey* keys = mdl::MorphKeys(model);
                for (std::int32_t i = 0; i < morphCnt; ++i)      
                    ReadPmmMorphKey(fd, keys[i]);
                std::int32_t cnt = 0;
                Rd(fd, &cnt, 4);                                 
                for (std::int32_t i = 0; i < cnt; ++i) {
                    std::int32_t keyIndex = 0;
                    Rd(fd, &keyIndex, 4);
                    ReadPmmMorphKey(fd, keys[keyIndex]);
                }
            }

            LogV1Stage("bonekeys-done", _tell(fd));
            {
                mdl::DisplayKey* keys = mdl::DisplayKeys(model);
                Rd(fd, &keys[0].frame, 4);
                Rd(fd, &keys[0].previous, 4);
                Rd(fd, &keys[0].next, 4);
                unsigned char visible = 0;
                Rd(fd, &visible, 1);
                keys[0].visible = visible == 1;
                for (std::int32_t i = 0;
                     i < static_cast<std::int32_t>(record->ikChainCount); ++i) {
                    unsigned char b = 0;
                    Rd(fd, &b, 1);                               
                    mdl::IkStates(keys[0])[i] = b == 1;
                }
                {
                    unsigned char b = 0;
                    Rd(fd, &b, 1);                               
                    keys[0].allocated = b == 1;
                }
                std::int32_t cnt = 0;
                Rd(fd, &cnt, 4);                                 
                for (std::int32_t i = 0; i < cnt; ++i) {
                    std::int32_t keyIndex = 0;
                    Rd(fd, &keyIndex, 4);
                    mdl::DisplayKey& key = keys[keyIndex];
                    Rd(fd, &key.frame, 4);
                    Rd(fd, &key.previous, 4);
                    Rd(fd, &key.next, 4);
                    unsigned char visible = 0;
                    Rd(fd, &visible, 1);
                    key.visible = visible == 1;
                    for (std::int32_t j = 0;
                         j < static_cast<std::int32_t>(record->ikChainCount);
                         ++j) {
                        unsigned char b = 0;
                        Rd(fd, &b, 1);                           
                        mdl::IkStates(key)[j] = b == 1;
                    }
                    unsigned char b = 0;
                    Rd(fd, &b, 1);                               
                    key.allocated = b == 1;
                }
            }
            LogV1Stage("physkeys-done", _tell(fd));
            {
                const std::int32_t boneCnt =
                    static_cast<std::int32_t>(record->boneCount);
                mdl::BoneRecord* const bones = mdl::Bones(model);
                for (std::int32_t i = 0; i < boneCnt; ++i) {
                    mdl::BoneRecord& bone = bones[i];
                    Rd(fd, &bone.trans[0], sizeof(bone.trans[0]));
                    Rd(fd, &bone.trans[1], sizeof(bone.trans[1]));
                    Rd(fd, &bone.trans[2], sizeof(bone.trans[2]));
                    Rd(fd, &bone.rotQuat[0], sizeof(bone.rotQuat[0]));
                    Rd(fd, &bone.rotQuat[1], sizeof(bone.rotQuat[1]));
                    Rd(fd, &bone.rotQuat[2], sizeof(bone.rotQuat[2]));
                    Rd(fd, &bone.rotQuat[3], sizeof(bone.rotQuat[3]));
                    Rd(fd, &bone.rotQuat[0], sizeof(bone.rotQuat[0]));
                    unsigned char b = 0;
                    Rd(fd, &b, 1);                               
                    record->bonePhysicsState[i] = (b == 1) ? 1 : 0;
                    Rd(fd, &b, 1);                               
                    record->boneSelection[i] = (b == 1) ? 1 : 0;
                }
            }
            mdl::MorphRecord* const morphs = mdl::Morphs(model);
            for (std::int32_t i = 0;
                 i < static_cast<std::int32_t>(record->morphCount); ++i)
                Rd(fd, &morphs[i].value, sizeof(morphs[i].value));
            mdl::IkChain* const ikChains = mdl::IkChains(model);
            for (std::int32_t i = 0;
                 i < static_cast<std::int32_t>(record->ikChainCount); ++i) {
                unsigned char b = 0;
                Rd(fd, &b, 1);
                ikChains[i].enabled = (b == 1) ? 1 : 0;
            }
            LogV1Stage("state-done", _tell(fd));
            EnableMenuItem(GetMenu(main), 0x120, 0);             
            EnableMenuItem(GetMenu(main), 0x121, 0);             
    return true;
}


static void LoadSceneV1_RegisterCombo(PmmV1LoadContext& ctx) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    unsigned char** const slots = ctx.slots;

    SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_RESETCONTENT, 0, 0); 
    if (s->state.optflag[0] == 0) {
        if (s->EnglishUI() == 0) {
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWXMove));      
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWYMove));
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWZMove));
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWRot));
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWAll));
        } else {
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("x axis move"));
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("y axis move"));
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("z axis move"));
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("rotation"));
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("all"));        
        }
        SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_SETCURSEL,       
                     rinmotiondance::mdl::Mdl(
                         slots[s->SelectedModelSlot()])->comboSelIndex,
                     0);
        PostLoadInit(slots[s->SelectedModelSlot()]);                 
    } else {
        if (s->EnglishUI() == 0) {
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWXMove));      
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWYMove));
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWZMove));
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWRot));
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWDist));        
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWViewAng));
            SendMessageW(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kWAll));
        } else {
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("x axis move"));
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("y axis move"));
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("z axis move"));
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("rotation"));
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("distance"));   
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("view angle")); 
            SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>("all"));        
        }
        SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_SETCURSEL, 0, 0);
    }
    SendMessageA(GetDlgItem(main, panel::kInterpCurveCombo), CB_SETCURSEL, 3, 0);    

}


static void LoadSceneV1_ReallocateTracks(PmmV1LoadContext& ctx) {
    auto* const s = ctx.s;
    D3DRenderer* const wrap = ctx.wrap;
    mdl::AccessoryRecord** const accs = ctx.accs;
    mdl::AccessoryKey** const accTracks = ctx.accTracks;

    ctx.cameraKeys = reinterpret_cast<mdl::CameraKey*>(
        AllocBytes(kTrackCam));                                  
    s->CameraKeys() = ctx.cameraKeys;
    std::memset(ctx.cameraKeys, 0, kTrackCam);                        
    ctx.lightKeys = reinterpret_cast<mdl::LightKey*>(
        AllocBytes(kTrackLight));                                
    s->LightKeys() = ctx.lightKeys;
    std::memset(ctx.lightKeys, 0, kTrackLight);
    ctx.selfShadowKeys = reinterpret_cast<mdl::SelfShadowKey*>(
        AllocBytes(kTrackSel));                                  
    s->ShadowKeys() = ctx.selfShadowKeys;
    std::memset(ctx.selfShadowKeys, 0, kTrackSel);
    auto* const gravityKeys = reinterpret_cast<mdl::GravityKey*>(
        AllocBytes(kTrackShadow));                               
    s->GravityKeys() = gravityKeys;
    std::memset(gravityKeys, 0, kTrackShadow);
    ctx.selfShadowKeys[0].mode = wrap->postProcessEnabled ? 1 : 0;   
    ctx.selfShadowKeys[0].distance = 0.0112500004f;                   
    s->state.selfShadowMode = 1;                 
    s->state.selfShadowEnabled = 0;
    gravityKeys[0].noise = 10;
    gravityKeys[0].acceleration = 9.8000002f;                    
    gravityKeys[0].direction[1] = -1.0f;                         
    for (std::size_t i = 0; i < mdl::kTimelineKeyCapacity; ++i)   
        ctx.cameraKeys[i].parentModel = -1;
    for (int i = 0; i < 255; ++i) {                              
        auto* const keys = reinterpret_cast<mdl::AccessoryKey*>(
            AllocBytes(kTrackAcc));
        accTracks[i] = keys;
        std::memset(keys, 0, kTrackAcc);                          
        keys[0].visible = 1;
        keys[0].parentModel = -1;
        keys[0].scale = 1.0f;
        keys[0].opacity = 1.0f;
        accs[i] = nullptr;
    }
}


static void LoadSceneV1_CameraTrack(PmmV1LoadContext& ctx, int fd) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    char* const text = ctx.text;
    mdl::CameraKey* const cameraKeys = ctx.cameraKeys;

    {
        ReadPmmCameraKey<PmmStream::V1>(fd, cameraKeys[0]);
        std::int32_t cnt = 0;
        Rd(fd, &cnt, 4);                                         
        for (std::int32_t i = 0; i < cnt; ++i) {
            std::int32_t timelineFrame = 0;
            Rd(fd, &timelineFrame, 4);
            ReadPmmCameraKey<PmmStream::V1>(fd, cameraKeys[timelineFrame]);
        }
    }
    LogV1Stage("camtrack-done", _tell(fd));
    Rd(fd, &s->state.cameraPosition[0], 4);
    Rd(fd, &s->state.cameraPosition[1], 4);
    Rd(fd, &s->state.cameraPosition[2], 4);
    Rd(fd, &s->state.viewOffsetX, 4);
    Rd(fd, &s->state.viewOffsetY, 4);
    Rd(fd, &s->state.cameraDistance, 4);      
    Rd(fd, &s->state.cameraPitch, 4);
    Rd(fd, &s->state.cameraYaw, 4);
    Rd(fd, &s->state.cameraRoll, 4);
    {
        unsigned char b = 0;
        Rd(fd, &b, 1);                                           
        s->state.cameraPerspective = (b == 1) ? 1 : 0;
    }
    {
        const int frame =
            static_cast<int>(s->state.cameraFov);    
        SendMessageA(GetDlgItem(main, panel::kFovSlider), TBM_SETPOS, 1,
                     frame);                                     
        HWND item = GetDlgItem(main, panel::kFovEdit);
        SendMessageA(item, EM_SETSEL, 0, GetWindowTextLengthA(item));
        sprintf_s(text, 0x100, "%3d", frame);                    
        SendMessageA(item, EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(text));
        SendMessageA(GetDlgItem(main, panel::kPerspectiveCheckbox), BM_SETCHECK,
                     s->state.cameraPerspective == 0 ? 1 : 0,
                     0);                                         
    }

    LogV1Stage("cammisc-done", _tell(fd));
}


static void LoadSceneV1_LightTrack(PmmV1LoadContext& ctx, int fd) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    char* const text = ctx.text;
    mdl::LightKey* const lightKeys = ctx.lightKeys;

    {
        ReadPmmLightKey(fd, lightKeys[0]);
        std::int32_t cnt = 0;
        Rd(fd, &cnt, 4);                                         
        for (std::int32_t i = 0; i < cnt; ++i) {
            std::int32_t timelineFrame = 0;
            Rd(fd, &timelineFrame, 4);
            ReadPmmLightKey(fd, lightKeys[timelineFrame]);
        }
    }
    Rd(fd, s->LightColor() + 0, sizeof(float));
    Rd(fd, s->LightColor() + 1, sizeof(float));
    Rd(fd, s->LightColor() + 2, sizeof(float));
    Rd(fd, s->LightDirection() + 0, sizeof(float));
    Rd(fd, s->LightDirection() + 1, sizeof(float));
    Rd(fd, s->LightDirection() + 2, sizeof(float));

    LogV1Stage("light-done", _tell(fd));
    {
        const int sliderIds[] = {455, 456, 457, 458, 459, 460};  
        const int editIds[] = {461, 462, 463, 464, 465, 466};    
        const float values[] = {
            s->LightColor()[0], s->LightColor()[1], s->LightColor()[2],
            s->LightDirection()[0], s->LightDirection()[1],
            s->LightDirection()[2]};
        for (int k = 0; k < 6; ++k) {
            const double scale = k < 3 ? 256.0 : 100.0;
            SendMessageA(GetDlgItem(main, sliderIds[k]),
                         TBM_SETPOS, 1,
                         static_cast<int>(values[k] * scale));    
            HWND item = GetDlgItem(main, editIds[k]);
            SendMessageA(item, EM_SETSEL, 0, GetWindowTextLengthA(item));
            if (k < 3)
                sprintf_s(text, 0x100, "%3d",
                          static_cast<int>(values[k] * 256.0));  
            else
                sprintf_s(text, 0x100, "%+3.1f", values[k]);     
            SendMessageA(item, EM_REPLACESEL, 0,
                         reinterpret_cast<LPARAM>(text));
        }
    }

}


static bool LoadSceneV1_AccessoryBlock(PmmV1LoadContext& ctx, int fd) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    PathResolutionWorkspace& paths = *ctx.paths;
    D3DRenderer* const wrap = ctx.wrap;
    mdl::AccessoryRecord** const accs = ctx.accs;
    mdl::AccessoryKey** const accTracks = ctx.accTracks;
    unsigned char** const slots = ctx.slots;
    char* const text = ctx.text;
    char* const mbPath = ctx.mbPath;
    wchar_t* const wideTmp = ctx.wideTmp;
    wchar_t* const ofnFile = ctx.ofnFile;
    wchar_t* const ofnTitle = ctx.ofnTitle;

    Rd(fd, &s->SelectedAccessorySlot(), 1);                       
    Rd(fd, &s->DisplayObjectListScrollPosition(), 4);             
    SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_RESETCONTENT, 0, 0);
    SendMessageA(GetDlgItem(main, panel::kAttachBoneCombo), CB_RESETCONTENT, 0, 0);
    unsigned char accCount = 0;
    Rd(fd, &accCount, 1);                                        
    EnableMenuItem(GetMenu(main), 0xF9, accCount == 0 ? 1 : 0);  
    for (unsigned char i = 0; i < accCount; ++i) {
        Rd(fd, text, 100);
        SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(text));
    }
    LogV1Stage("acc-head-done", _tell(fd), accCount);
    char accName[100];
    for (unsigned char i = 0; i < accCount; ++i) {
        unsigned char accSlot = 0;
        Rd(fd, &accSlot, 1);                                     
        auto* acc = static_cast<mdl::AccessoryRecord*>(operator new(
            sizeof(mdl::AccessoryRecord)));                     
        if (acc != nullptr) IdentityCtor(acc);                      
        accs[accSlot] = acc;
        std::memset(accs[accSlot], 0, sizeof(mdl::AccessoryRecord));
        InitAccessoryRecord(accs[accSlot]);                              
        Rd(fd, accName, 100);                                    
        Rd(fd, mbPath, 0x100);                                   
        ResolveAnsiUserFile(reinterpret_cast<unsigned char*>(wrap),
                            mbPath, wideTmp, 0x100,
                            paths);                               
        if (!LoadAccessoryObject(s, accs[accSlot], wideTmp)) {   
            sprintf_s(text, 0x100,
                      s->EnglishUI() != 0
                          ? "Cannot open file:%s.\n\nPlease select "
                            "accessory of %s."
                          : kJpCannotOpenAcc,
                      accName, accName);                         
            MessageBoxA(main, text,
                        s->EnglishUI() != 0 ? "open file" : kJpOpenCaption,
                        0);                                      
            SetCurrentDirectoryW(reinterpret_cast<const wchar_t*>(
                s->state.exeDir));                             
            swprintf_s(ofnFile, 0x100, L"");                     
            OPENFILENAMEW ofn;
            std::memset(&ofn, 0, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);                              
            ofn.hwndOwner =
                s->state.floatingWindow
                    ? reinterpret_cast<HWND>(
                          s->state.floatingWindow)
                    : main;
            ofn.lpstrFilter =
                s->EnglishUI() != 0 ? kWFilterAccEn : kWFilterAccJp;
            ofn.lpstrFile = ofnFile;
            ofn.nMaxFile = 0x100;
            ofn.Flags = OFN_FILEMUSTEXIST;
            ofn.lpstrInitialDir =
                (GetMenuState(GetMenu(main), 0x12D, 0) & 8)
                    ? reinterpret_cast<LPCWSTR>(s->state.dirAccs)
                    : kWUserAcc;
            ofn.lpstrDefExt = L"x";                              
            ofn.nMaxFileTitle = 0x100;
            ofn.lpstrFileTitle = ofnTitle;
            ofn.lpstrTitle =
                s->EnglishUI() != 0
                    ? L"open file"                               
                    : reinterpret_cast<LPCWSTR>(kWOpenFile);
            if (!GetOpenFileNameW(&ofn)) {                       
                AbortV1Load(s, fd);                                     
                return false;
            }
            if (GetMenuState(GetMenu(main), 0x12D, 0) & 8) {
                ExtractDirFromPath(ofnTitle, ofnFile);           
                wcscpy_s(reinterpret_cast<wchar_t*>(
                             s->state.dirAccs),
                         0x3E8, ofnTitle);                       
            }
            if (!LoadAccessoryObject(s, accs[accSlot], ofnFile)) {
                _close(fd);                                      
                ResetAppState(s);                                
                return false;
            }
        }

        LogV1Stage("acc-obj-done", _tell(fd), accSlot);
        mdl::AccessoryRecord& accessory = *mdl::Accessory(accs[accSlot]);
        Rd(fd, &accessory.order, 1);                              
        strcpy_s(accessory.name, sizeof accessory.name, accName);
        auto* const accessoryKeys =
            reinterpret_cast<mdl::AccessoryKey*>(accTracks[accSlot]);
        ReadPmmAccessoryKey(fd, accessoryKeys[0]);
        {
            std::int32_t cnt = 0;
            Rd(fd, &cnt, 4);                                     
            for (std::int32_t k = 0; k < cnt; ++k) {
                std::int32_t timelineFrame = 0;
                Rd(fd, &timelineFrame, 4);
                ReadPmmAccessoryKey(fd, accessoryKeys[timelineFrame]);
            }
        }
        {
            unsigned char b = 0;                                 
            Rd(fd, &b, 1);
            accessory.visible = b & 1;
            accessory.opacity =
                static_cast<float>(100 - (b >> 1)) / 100.0f;
        }
        Rd(fd, &accessory.parentModel, 4);                        
        Rd(fd, &accessory.parentBone, 4);
        Rd(fd, &accessory.rotation[0], 4);
        Rd(fd, &accessory.rotation[1], 4);
        Rd(fd, &accessory.rotation[2], 4);
        Rd(fd, &accessory.scale, 4);
        Rd(fd, &accessory.position[0], 4);                        
        Rd(fd, &accessory.position[1], 4);                        
        Rd(fd, &accessory.position[2], 4);                        
        {
            unsigned char b = 0;
            Rd(fd, &b, 1);                                       
            accessory.shadowEnabled = (b == 1) ? 1 : 0;
        }
    }
    {
        const unsigned char cur = s->SelectedAccessorySlot();
        if (accs[cur] != nullptr) {
            mdl::AccessoryRecord& accessory = *mdl::Accessory(accs[cur]);
            SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_SETCURSEL,  
                         accessory.order, 0);
            const std::int32_t parentSlot = accessory.parentModel;
            unsigned char* pm = slots[parentSlot];
            if (pm != nullptr &&
                static_cast<std::int32_t>(
                    rinmotiondance::mdl::Mdl(pm)->boneCount) > 0) {
                for (std::int32_t b = 0;
                     b < static_cast<std::int32_t>(
                             rinmotiondance::mdl::Mdl(pm)->boneCount);
                     ++b) {
                    const rinmotiondance::mdl::BoneRecord& entry =
                        rinmotiondance::mdl::Bones(pm)[b];
                    if (entry.type < rinmotiondance::mdl::BoneType::InertTip ||
                        entry.type == rinmotiondance::mdl::BoneType::FixedAxis)
                        SendMessageA(GetDlgItem(main, panel::kAttachBoneCombo),
                                     CB_ADDSTRING, 0,
                                     reinterpret_cast<LPARAM>(entry.name));
                }
            }
            SyncAccessoryEditPanel(s);                                        
        }
    }

    LogV1Stage("accessories-done", _tell(fd));
    return true;
}


static void LoadSceneV1_ConfigBlock(PmmV1LoadContext& ctx, int fd) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    PathResolutionWorkspace& paths = *ctx.paths;
    D3DRenderer* const wrap = ctx.wrap;
    char* const text = ctx.text;
    char* const mbPath = ctx.mbPath;

    Rd(fd, &s->state.currentFrame, 4);
    Rd(fd, &s->state.timelineStartFrame, 4);
    Rd(fd, &s->state.lastRegisteredFrame, 4);
    {
        HWND item = GetDlgItem(main, panel::kCurrentFrameEdit);
        SendMessageA(item, EM_SETSEL, 0, GetWindowTextLengthA(item));
        sprintf_s(text, 0x100, "%d",
                  s->state.currentFrame);         
        SendMessageA(item, EM_REPLACESEL, 0,
                     reinterpret_cast<LPARAM>(text));
    }
    PostModelReload2(s);                                         
    PostLanguageSweep(s);                                        
    HandleWindowSize(s);                                         
    ApplyModelComboSelection(s);                                                

    std::int32_t savedEditMode = 0;
    Rd(fd, &savedEditMode, 4);                                  
    s->EditMode() = static_cast<ViewportEditMode>(savedEditMode);
    {
        const ViewportEditMode mode = s->EditMode();
        const int check4 = mode == ViewportEditMode::Camera ? 1 : 0;
        const int check2 = mode == ViewportEditMode::Light ? 1 : 0;
        SendMessageA(GetDlgItem(main, panel::kBoneSelectRadio), BM_SETCHECK,
                     mode == ViewportEditMode::Bone ? 1 : 0, 0);
        SendMessageA(GetDlgItem(main, panel::kBoxSelectRadio), BM_SETCHECK,
                     mode == ViewportEditMode::BoneBox ? 1 : 0, 0);
        SendMessageA(GetDlgItem(main, panel::kBoneMoveRadio), BM_SETCHECK, check2, 0);
        SendMessageA(GetDlgItem(main, panel::kBoneRotateRadio), BM_SETCHECK, check4, 0);
    }
    {
        unsigned char b = 0;
        Rd(fd, &b, 1);                                           
        s->state.cameraReferenceMode = b;
        SendMessageA(GetDlgItem(main, panel::kCameraRefModelCheckbox), BM_SETCHECK,
                     b == 1 ? 1 : 0, 0);
        SendMessageA(GetDlgItem(main, panel::kCameraRefBoneCheckbox), BM_SETCHECK,
                     b == 2 ? 1 : 0, 0);
        Rd(fd, &b, 1);                                           
        s->state.playbackLoopEnabled = b ? 1 : 0;
        SendMessageA(GetDlgItem(main, 411), BM_SETCHECK, b ? 1 : 0, 0);
        Rd(fd, &b, 1);                                           
        s->state.playbackReturnsToStartFrame = b ? 1 : 0;
        SendMessageA(GetDlgItem(main, 413), BM_SETCHECK, b ? 1 : 0, 0);
        Rd(fd, &b, 1);                                           
        s->state.playbackStartsAtCurrentFrame = b ? 1 : 0;
        SendMessageA(GetDlgItem(main, 414), BM_SETCHECK, b ? 1 : 0, 0);
        std::int32_t playStartFrame = 0, playStopFrame = 0;
        Rd(fd, &playStartFrame, 4);                                          
        {
            HWND item = GetDlgItem(main, panel::kPlayStartFrameEdit);
            SendMessageA(item, EM_SETSEL, 0, GetWindowTextLengthA(item));
            sprintf_s(text, 0x100, "%d", playStartFrame);                    
            SendMessageA(item, EM_REPLACESEL, 0,
                         reinterpret_cast<LPARAM>(text));
        }
        Rd(fd, &playStopFrame, 4);                                          
        {
            HWND item = GetDlgItem(main, panel::kPlayStopFrameEdit);
            SendMessageA(item, EM_SETSEL, 0, GetWindowTextLengthA(item));
            sprintf_s(text, 0x100, "%d", playStopFrame);                    
            SendMessageA(item, EM_REPLACESEL, 0,
                         reinterpret_cast<LPARAM>(text));
        }
        ClearTimelineAndCurveDCs(s);                                            
        Rd(fd, &b, 1);                                           
        s->state.waveEnabled = b ? 1 : 0;
        Rd(fd, mbPath, 0x100);                                   
        ResolveAnsiUserFile(reinterpret_cast<unsigned char*>(wrap),
                            mbPath,
                            reinterpret_cast<wchar_t*>(s->state.wavPath),
                            0x100, paths);                        
        if (s->state.waveEnabled != 0)
            LoadWaveFile(s);                                     
        std::int32_t w1 = 0, w2 = 0, w3 = 0;
        Rd(fd, &w1, 4);                                          
        Rd(fd, &w2, 4);                                          
        Rd(fd, &w3, 4);                                          
        Rd(fd, mbPath, 0x100);                                   
        ResolveAnsiUserFile(reinterpret_cast<unsigned char*>(wrap),
                            mbPath,
                            s->AviBackgroundPath(),
                            0x100, paths);                        
        if (mbPath[0] != 0)
            LoadAviFile(s);                                      

        if (s->AviBackgroundEnabled() == 1) {
            Rd(fd, &s->AviBackgroundEnabled(), 4);               
            s->AviOffsetX() = w1;
            s->AviOffsetY() = w2;
            std::memcpy(&s->AviScale(), &w3, sizeof w3);
            if (s->AviBackgroundEnabled() == 1) {
                CheckMenuItem(GetMenu(main), 0xD8, 8);
                AviBgOverlayRefresh(s);                                    
            } else {
                CheckMenuItem(GetMenu(main), 0xD8, 0);
            }
        } else {
            Rd(fd, &s->AviBackgroundEnabled(), 4);               
            s->AviBackgroundEnabled() = 0;
            CheckMenuItem(GetMenu(main), 0xD8, 0);
            if (s->AviFrameReader() != nullptr) {
                AVIStreamGetFrameClose(s->AviFrameReader());
                s->AviFrameReader() = nullptr;
            }
            if (s->AviStream() != nullptr) {
                AVIStreamRelease(s->AviStream());
                s->AviStream() = nullptr;
            }
            if (s->AviFile() != nullptr) {
                AVIFileRelease(s->AviFile());
                s->AviFile() = nullptr;
            }
        }
        std::int32_t p1 = 0, p2 = 0, p3 = 0;
        Rd(fd, &p1, 4);                                          
        Rd(fd, &p2, 4);                                          
        Rd(fd, &p3, 4);                                          
        Rd(fd, mbPath, 0x100);                                   
        ResolveAnsiUserFile(reinterpret_cast<unsigned char*>(wrap),
                            mbPath,
                            s->PictureBackgroundPath(),
                            0x100, paths);                        
        s->PictureBackgroundEnabled() = 0;
        if (mbPath[0] != 0)
            LoadBackgroundPicture(s);                            
        if (s->PictureBackgroundEnabled() == 0) {
            Rd(fd, &b, 1);                                       
            s->PictureBackgroundEnabled() = 0;
            CheckMenuItem(GetMenu(main), 0xE9, 0);               
        } else {
            Rd(fd, &b, 1);                                       
            s->PictureOffsetX() = p1;
            s->PictureOffsetY() = p2;
            std::memcpy(&s->PictureScale(), &p3, sizeof p3);
            s->PictureBackgroundEnabled() = b ? 1 : 0;
            if (b != 0) {
                CheckMenuItem(GetMenu(main), 0xE9, 8);
                PicBgOverlayRefresh(s);                                    
            } else {
                CheckMenuItem(GetMenu(main), 0xE9, 0);           
            }
        }
    }
    {
        const HWND owner =
            s->state.floatingWindow
                ? reinterpret_cast<HWND>(s->state.floatingWindow)
                : main;
        unsigned char b = 0;
        Rd(fd, &b, 1);                                           
        if (b != 0) {
            s->state.fpsOverlayEnabled = 1;
            CheckMenuItem(GetMenu(main), 0xD3, 8);
            SendMessageA(GetDlgItem(owner, panel::kInfoCheckbox), BM_SETCHECK, 1, 0);
        } else {
            s->state.fpsOverlayEnabled = 0;
            CheckMenuItem(GetMenu(main), 0xD3, 0);
            SendMessageA(GetDlgItem(owner, panel::kInfoCheckbox), BM_SETCHECK, 0, 0);
        }
        Rd(fd, &b, 1);                                           
        if (b != 0) {
            s->state.groundGridEnabled = 1;
            CheckMenuItem(GetMenu(main), 0xD7, 8);
            SendMessageA(GetDlgItem(owner, panel::kCoordAxisCheckbox), BM_SETCHECK, 1, 0);
        } else {
            s->state.groundGridEnabled = 0;
            CheckMenuItem(GetMenu(main), 0xD7, 0);
            SendMessageA(GetDlgItem(owner, panel::kCoordAxisCheckbox), BM_SETCHECK, 0, 0);
        }
        Rd(fd, &b, 1);                                           
        s->state.groundShadowEnabled = b ? 1 : 0;
        CheckMenuItem(GetMenu(main), 0xDD, b ? 8 : 0);           
    }

    LogV1Stage("config-done", _tell(fd));
    Rd(fd, &s->state.fpsLimit, 4);      
    {
        const float v = s->state.fpsLimit;
        if (v == 1000.0f) {
            CheckMenuItem(GetMenu(main), 0xEB, 0);
            CheckMenuItem(GetMenu(main), 0xEC, 0);
            CheckMenuItem(GetMenu(main), 0xEA, 8);
        } else if (v == 30.0f) {
            CheckMenuItem(GetMenu(main), 0xEA, 0);
            CheckMenuItem(GetMenu(main), 0xEC, 0);
            CheckMenuItem(GetMenu(main), 0xEB, 8);
        } else {
            CheckMenuItem(GetMenu(main), 0xEB, 0);
            CheckMenuItem(GetMenu(main), 0xEA, 0);
            CheckMenuItem(GetMenu(main), 0xEC, 8);
        }
    }
    Rd(fd, &reinterpret_cast<ScreenCaptureMode&>(s->state.captureMode), 4);         
    switch (static_cast<std::int32_t>(s->state.captureMode)) {
        case 0:
            CheckMenuItem(GetMenu(main), 0xF3, 8);
            CheckMenuItem(GetMenu(main), 0xF4, 0);
            CheckMenuItem(GetMenu(main), 0xF5, 0);
            CheckMenuItem(GetMenu(main), 0xF6, 0);
            break;
        case 1:
            CheckMenuItem(GetMenu(main), 0xF3, 0);
            CheckMenuItem(GetMenu(main), 0xF4, 8);
            CheckMenuItem(GetMenu(main), 0xF5, 0);
            CheckMenuItem(GetMenu(main), 0xF6, 0);
            break;
        case 2:
            CheckMenuItem(GetMenu(main), 0xF3, 0);
            CheckMenuItem(GetMenu(main), 0xF4, 0);
            CheckMenuItem(GetMenu(main), 0xF5, 8);
            CheckMenuItem(GetMenu(main), 0xF6, 0);
            break;
        default:
            CheckMenuItem(GetMenu(main), 0xF3, 0);
            CheckMenuItem(GetMenu(main), 0xF4, 0);
            CheckMenuItem(GetMenu(main), 0xF5, 0);
            CheckMenuItem(GetMenu(main), 0xF6, 8);
            break;
    }
    s->state.gravityMagnitude = 9.8000002f;              
    s->state.gravityNoiseTimer = 0.0f;
    s->state.gravityX = 0.0f;
    s->state.gravityY = -1.0f;                     
    s->state.gravityNoiseEnabled = 0;
    s->state.gravityNoise = 10;
    s->state.gravityZ = 0.0f;
    s->state.rigidBodyDisplayEnabled = 0;
    s->state.modelOutlineColorRed = 0;
    s->state.modelOutlineColorGreen = 0;
    s->state.modelOutlineColorBlue = 0;
    SendMessageA(GetDlgItem(main, panel::kMainComboNormal), CB_SETCURSEL, 0, 0);   
    SendMessageA(GetDlgItem(main, panel::kBoneRegisterCombo), CB_SETCURSEL, 0, 0);   

    LogV1Stage("physics-defaults-done", _tell(fd));
}


static void LoadSceneV1_ReadGatedTail(PmmV1LoadContext& ctx, int fd) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    unsigned char** const slots = ctx.slots;
    mdl::AccessoryRecord** const accs = ctx.accs;
    char* const lbText = ctx.lbText;
    mdl::CameraKey* const cameraKeys = ctx.cameraKeys;
    mdl::SelfShadowKey* const selfShadowKeys = ctx.selfShadowKeys;

    if (Rd(fd, &s->state.accessoryRenderSplitOrder, 4) > 0) {  
        Rd(fd, &s->ProjectedShadowAmbientIntensity(), 4);       
        s->SetProjectedShadowAmbient(s->ProjectedShadowAmbientIntensity());
        for (int i = 0; i < kModelSlotCount; ++i) {
            if (slots[i] != nullptr) {
                unsigned char b = 0;
                Rd(fd, &b, 1);                                   
                rinmotiondance::mdl::Mdl(slots[i])->postLoadFlag2 =
                    (b == 1) ? 1 : 0;
            }
        }
        if (slots[s->SelectedModelSlot()] != nullptr &&
            rinmotiondance::mdl::Mdl(
                slots[s->SelectedModelSlot()])->postLoadFlag2 != 0 &&
            s->state.optflag[0] == 0)
            SendMessageA(GetDlgItem(main, panel::kAddBlendCheckbox), BM_SETCHECK, 1,
                         0);                                     
        for (int i = 0; i < 255; ++i) {
            if (accs[i] != nullptr) {
                unsigned char b = 0;
                Rd(fd, &b, 1);                                   
                mdl::Accessory(accs[i])->additiveBlend = (b == 1) ? 1 : 0;
            }
        }
        {
            const unsigned char cur = s->SelectedAccessorySlot();
            if (accs[cur] != nullptr &&
                mdl::Accessory(accs[cur])->additiveBlend != 0)
                SendMessageA(GetDlgItem(main, panel::kAccessoryAddBlendCheckbox), BM_SETCHECK, 1,
                             0);                                 
        }
        std::int32_t v31C0 = 0;
        if (Rd(fd, &v31C0, 4) > 0) {                             
            float edgeScaleBits;
            std::memcpy(&edgeScaleBits, &v31C0, sizeof edgeScaleBits);
            for (int i = 0; i < kModelSlotCount; ++i)
                if (slots[i] != nullptr)
                    rinmotiondance::mdl::Mdl(slots[i])->edgeScale =
                        edgeScaleBits;
            unsigned char b = 0;
            Rd(fd, &b, 1);                                       
            if (b == 1) {
                s->state.projectedShadowBlendEnabled = 1;
                CheckMenuItem(GetMenu(main), 0xFE, 8);           
            } else {
                s->state.projectedShadowBlendEnabled = 0;
            }
            const int got48F = Rd(fd, &b, 1);                    
            if (b == 1 && got48F > 0) {
                for (int i = 0; i < kModelSlotCount; ++i) {
                    if (slots[i] != nullptr) {
                        std::int32_t edge = 0;
                        Rd(fd, &edge, 4);                        
                        std::memcpy(&rinmotiondance::mdl::Mdl(slots[i])->edgeScale,
                                    &edge, sizeof edge);
                    }
                }
                unsigned char b509 = 0;
                const int got509 = Rd(fd, &b509, 1);             
                if (b509 == 1 && got509 > 0) {
                Rd(fd, reinterpret_cast<unsigned char*>(
                           &s->PlaybackPhysicsMode()),
                   1);                                           
                switch (s->PlaybackPhysicsMode()) {
                    case 0:
                        CheckMenuItem(GetMenu(main), 0x10D, 0);
                        CheckMenuItem(GetMenu(main), 0x109, 0);
                        CheckMenuItem(GetMenu(main), 0x10E, 0);
                        CheckMenuItem(GetMenu(main), 0x110, 8);
                        break;
                    case 1:
                        CheckMenuItem(GetMenu(main), 0x10D, 0);
                        CheckMenuItem(GetMenu(main), 0x109, 8);
                        CheckMenuItem(GetMenu(main), 0x10E, 0);
                        CheckMenuItem(GetMenu(main), 0x110, 0);
                        break;
                    case 2:
                        CheckMenuItem(GetMenu(main), 0x10D, 8);
                        CheckMenuItem(GetMenu(main), 0x109, 0);
                        CheckMenuItem(GetMenu(main), 0x10E, 0);
                        CheckMenuItem(GetMenu(main), 0x110, 0);
                        break;
                    default:
                        break;
                }

                Rd(fd, &s->state.gravityMagnitude, 4);
                Rd(fd, &s->state.gravityNoise, 4);
                Rd(fd, &s->state.gravityX, 4);
                Rd(fd, &s->state.gravityY, 4);
                Rd(fd, &s->state.gravityZ, 4);
                unsigned char bc = 0;
                Rd(fd, &bc, 1);                                  
                s->state.gravityNoiseEnabled =
                    (bc == 1) ? 1 : 0;
                {
                    unsigned char bd = 0;
                    const int got716 = Rd(fd, &bd, 1);           
                    if (bd == 1 && got716 > 0) {
                        unsigned char be = 0;
                        Rd(fd, &be, 1);                          
                        s->state.selfShadowMode = be;
                        s->state.selfShadowEnabled =
                            (be != 0) ? 1 : 0;
                        Rd(fd, &s->state.physicsInterval,
                           4);                                   
                        selfShadowKeys[0].mode =
                            static_cast<unsigned char>(
                                s->state.selfShadowMode);
                        selfShadowKeys[0].distance =
                            s->state.physicsInterval;
                        for (int i = 0; i < kModelSlotCount; ++i) {
                            if (slots[i] != nullptr) {
                                unsigned char b = 0;
                                Rd(fd, &b, 1);                   
                                rinmotiondance::mdl::Mdl(slots[i])->toonFlag =
                                    (b == 1) ? 1 : 0;
                            }
                        }
                        unsigned char bs = 0;
                        const int got7FE = Rd(fd, &bs, 1);       
                        if (bs == 1 && got7FE > 0) {
                            ReadPmmSelfShadowKey(fd, selfShadowKeys[0]);
                            std::int32_t cnt = 0;
                            Rd(fd, &cnt, 4);                     
                            for (std::int32_t i = 0; i < cnt; ++i) {
                                std::int32_t timelineFrame = 0;
                                Rd(fd, &timelineFrame, 4);
                                ReadPmmSelfShadowKey(fd,
                                                     selfShadowKeys[
                                                         timelineFrame]);
                            }
                            unsigned char bq = 0;
                            const int got9EC = Rd(fd, &bq, 1);   
                            if (bq == 1 && got9EC > 0) {
                                Rd(fd, &s->state.modelOutlineColorRed,
                                   4);
                                Rd(fd, &s->state.modelOutlineColorGreen,
                                   4);
                                Rd(fd, &s->state.modelOutlineColorBlue,
                                   4);
                                if (s->state.modelOutlineColorRed != 0 ||
                                    s->state.modelOutlineColorGreen != 0 ||
                                    s->state.modelOutlineColorBlue != 0) {
                                    for (int i = 0; i < kModelSlotCount; ++i)
                                        if (slots[i] != nullptr)
                                            SetModelColor(
                                                reinterpret_cast<MMDApp*>(
                                                    slots[i]),
                                                s->state.modelOutlineColorRed,
                                                s->state.modelOutlineColorGreen,
                                                s->state.modelOutlineColorBlue);
                                }

                                unsigned char br = 0;
                                const int gotAB4 = Rd(fd, &br, 1); 
                                if (br == 1 && gotAB4 > 0) {
                                    unsigned char b194 = 0;
                                    Rd(fd, &b194, 1);            
                                    s->state.blackBackgroundEnabled = (b194 != 0) ? 1 : 0;
                                    CheckMenuItem(GetMenu(main), 0x11A,
                                                  b194 ? 8 : 0);
                                    unsigned char bcam = 0;
                                    const int gotB35 =
                                        Rd(fd, &bcam, 1);        
                                    if (bcam == 1 && gotB35 > 0) {
                                        Rd(fd, &cameraKeys[0].parentModel, 4);
                                        Rd(fd, &cameraKeys[0].parentBone, 4);
                                        const std::int32_t nFrame =
                                            cameraKeys[0].parentBone;
                                        const std::int32_t nParent =
                                            cameraKeys[0].parentModel;
                                        if (nFrame >= 0 &&
                                            nFrame < 10000 &&
                                            nParent > -1 &&
                                            nParent < 1000) {

                                            std::int32_t cnt = 0;
                                            Rd(fd, &cnt, 4);     
                                            for (std::int32_t i = 0;
                                                 i < cnt; ++i) {
                                                std::int32_t timelineFrame = 0;
                                                Rd(fd, &timelineFrame, 4);
                                                Rd(fd, &cameraKeys[timelineFrame].parentModel,
                                                   4);           
                                                Rd(fd, &cameraKeys[timelineFrame].parentBone,
                                                   4);           
                                            }
                                            Rd(fd, &s->state.cameraParentModel, 4);              
                                            Rd(fd, &s->state.cameraParentBone, 4);              
                                            const std::int32_t sel0 = s->state.cameraParentModel;
                                            const std::int32_t sel1 = s->state.cameraParentBone;
                                            if (sel0 >= 0) {
                                                SendMessageA(
                                                    GetDlgItem(main,
                                                               panel::kMainComboNormal),
                                                    CB_SETCURSEL,
                                                    rinmotiondance::mdl::Mdl(
                                                        slots[sel0])->comboSelIndex,
                                                    0);        
                                            }
                                            RefillBoneRegisterCombo(s, sel0); 

                                            if (sel0 >= 0) {
                                                const LRESULT n =
                                                    SendMessageA(
                                                        GetDlgItem(
                                                            main, panel::kBoneRegisterCombo),
                                                        CB_GETCOUNT, 0,
                                                        0);     
                                                for (LRESULT i = 0;
                                                     i < n; ++i) {
                                                    SendMessageA(
                                                        GetDlgItem(
                                                            main, panel::kBoneRegisterCombo),
                                                        CB_GETLBTEXT,
                                                        static_cast<
                                                            WPARAM>(i),
                                                        reinterpret_cast<
                                                            LPARAM>(
                                                            lbText));
                                                    mdl::BoneRecord*
                                                        entry =
                                                        mdl::Bones(slots[sel0]) +
                                                        static_cast<
                                                            std::size_t>(
                                                                sel1);
                                                    if (strcmp(
                                                            lbText,
                                                            entry->name) ==
                                                        0)
                                                        SendMessageA(
                                                            GetDlgItem(
                                                                main,
                                                                panel::kBoneRegisterCombo),
                                                            CB_SETCURSEL,
                                                            static_cast<
                                                                WPARAM>(i),
                                                            0);  
                                                }
                                            }
                                        }
                                        unsigned char bdd = 0;
                                        const int gotDDFC =
                                            Rd(fd, &bdd, 1);    
                                        if (bdd == 1 && gotDDFC > 0) {
                                        Rd(fd, &s->state.cameraAttachmentBasis[0],
                                           4);
                                        for (int i = 0; i < 15; ++i)
                                            Rd(fd, &s->state.cameraAttachmentBasis[1 + i], 4);
                                        unsigned char bf7 = 0;
                                        Rd(fd, &bf7, 1);        
                                        if (bf7 == 1) {
                                            s->state.followCameraEnabled = 1;
                                            CheckMenuItem(
                                                GetMenu(main), 0xF7, 8);
                                            SendMessageA(
                                                GetDlgItem(main, panel::kFollowCameraCheckbox),
                                                BM_SETCHECK, 1, 0);
                                        } else {
                                            s->state.followCameraEnabled = 0;
                                            CheckMenuItem(
                                                GetMenu(main), 0xF7, 0);
                                            SendMessageA(
                                                GetDlgItem(main, panel::kFollowCameraCheckbox),
                                                BM_SETCHECK, 0, 0);
                                        }

                                        unsigned char b478 = 0;
                                        Rd(fd, &b478, 1);        
                                        s->state.cameraAttachmentTransformSuppressed =
                                            (b478 == 1) ? 1 : 0;
                                        unsigned char b11d = 0;
                                        const int got00E =
                                            Rd(fd, &b11d, 1);    
                                        if (b11d == 1 && got00E > 0) {
                                            unsigned char bd2 = 0;
                                            Rd(fd, &bd2, 1);    
                                            if (bd2 != 0) {
                                                CheckMenuItem(
                                                    GetMenu(main),
                                                    0x11D, 8);
                                                s->state.floorVisible = 1;
                                                s->Physics()->groundBody
                                                    ->setDeactivationTime(1.0f);
                                            } else {
                                                CheckMenuItem(
                                                    GetMenu(main),
                                                    0x11D, 0);
                                                s->state.floorVisible = 0;
                                                s->Physics()->groundBody
                                                    ->setDeactivationTime(-1.0f);
                                            }
                                            unsigned char bfin = 0;
                                            const int got0BC =
                                                Rd(fd, &bfin, 1); 
                                            if (bfin == 1 && got0BC > 0) {
                                                for (int i = 0;
                                                     i < kModelSlotCount;
                                                     ++i)
                                                    if (slots[i] !=
                                                        nullptr)
                                                        Rd(fd,
                                                           &mdl::Mdl(
                                                               slots[i])
                                                                ->comboSelIndex2,
                                                           1);  
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    }
    }

}


static void LoadSceneV1_SuccessTail(PmmV1LoadContext& ctx) {
    auto* const s = ctx.s;
    HWND const main = ctx.main;
    D3DRenderer* const wrap = ctx.wrap;
    unsigned char** const slots = ctx.slots;
    char* const text = ctx.text;
    char* const lbText = ctx.lbText;
    wchar_t* const wndText = ctx.wndText;

    if (wrap->postProcessEnabled != 0) {
        RefreshSelfShadowPanel(s);                                             
    } else {
        s->state.selfShadowEnabled = 0;
        s->state.selfShadowMode = 0;
    }
    CheckMenuItem(GetMenu(main), 0x117,
                  s->state.selfShadowEnabled != 0 ? 8 : 0);
    if (slots[s->SelectedModelSlot()] != nullptr &&
        rinmotiondance::mdl::Mdl(
            slots[s->SelectedModelSlot()])->toonFlag != 0 &&
        s->state.optflag[0] == 0)
        SendMessageA(GetDlgItem(main, panel::kShadowCheckbox), BM_SETCHECK, 1, 0); 
    {  
        float dir[3] = {s->state.gravityX,
                        s->state.gravityY,
                        s->state.gravityZ};
        auto& d3dxApi = d3dx::Get();
        d3dxApi.vec3Normalize(dir, dir);
        const float mag = s->state.gravityMagnitude;
        PhysicsScene* const physics = s->Physics();
        if (physics != nullptr && physics->world != nullptr) {
            physics->world->setGravity(
                btVector3(dir[0] * mag * 10.0f,
                          dir[1] * mag * 10.0f,
                          dir[2] * mag * 10.0f));
        }
    }
    swprintf_s(wndText, 0x100, kAppTitleFormat,                   
               reinterpret_cast<const wchar_t*>(s->state.envFileName));
    SetWindowTextW(main, wndText);

    for (int mi = 0; mi < kModelSlotCount; ++mi) {
        unsigned char* m = slots[mi];
        if (m == nullptr) continue;
        mdl::DisplayKey* keys = mdl::DisplayKeys(m);
        keys[0].previous = 0;
        std::int32_t cur = keys[0].next;
        if (cur == 0) continue;
        std::int32_t prev = 0;
        for (;;) {
                if (static_cast<std::int32_t>(keys[cur].previous) != prev) {
                    sprintf_s(text, 0x100, kJpChainFmtDisp,
                              mdl::Mdl(m)->name,
                              keys[prev].frame, keys[prev].frame,
                              keys[prev].frame);
                MessageBoxA(main, text, kJpChainCapPhys, 0);
                keys[prev].next = 0;
                break;
            }
            prev = cur;
            cur = keys[cur].next;
            if (cur == 0) break;
        }
    }
    for (int mi = 0; mi < kModelSlotCount; ++mi) {
        unsigned char* m = slots[mi];
        if (m == nullptr) continue;
        if (static_cast<std::int32_t>(rinmotiondance::mdl::Mdl(m)->boneCount) <= 0)
            continue;
        mdl::BoneKey* keys = mdl::BoneKeys(m);
        for (std::int32_t b = 0;
             b < static_cast<std::int32_t>(rinmotiondance::mdl::Mdl(m)->boneCount);
             ++b) {
            keys[b].previous = 0;
            std::int32_t cur = keys[b].next;
            if (cur == 0) continue;
            std::int32_t prev = b;
            for (;;) {
                if (static_cast<std::int32_t>(keys[cur].previous) != prev) {
                    unsigned char* frameEntry =
                        mdl::BoneBytes(mdl::Bones(m),
                                       static_cast<std::size_t>(b));
                    sprintf_s(text, 0x100, kJpChainFmtPhys,
                              mdl::Mdl(m)->name,
                              reinterpret_cast<char*>(frameEntry),
                              keys[prev].frame,
                              keys[prev].frame);
                    MessageBoxA(main, text, kJpChainCapDisp, 0);
                    keys[prev].next = 0;
                    break;
                }
                prev = cur;
                cur = keys[cur].next;
                if (cur == 0) break;
            }
        }
    }

    {
        const LRESULT sel =
            SendMessageA(GetDlgItem(main, panel::kMainComboModel), CB_GETCURSEL, 0, 0);
        SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_RESETCONTENT, 0, 0);
        if (sel == 0) {
            if (s->EnglishUI() == 0) {
                SendMessageW(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(kWCamera));
                SendMessageW(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(kWLight));  
                SendMessageW(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(kWSelfSh));
                SendMessageW(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(kWGravity));
            } else {
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>("camera")); 
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>("light"));
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>("s shadow"));
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>("gravity"));
            }
            const LRESULT n =
                SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_GETCOUNT, 0, 0);
            for (LRESULT i = 0; i < n; ++i) {                    
                SendMessageA(GetDlgItem(main, panel::kAccessoryCombo), CB_GETLBTEXT,
                             static_cast<WPARAM>(i),
                             reinterpret_cast<LPARAM>(lbText));
                SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(lbText));
            }
            SendMessageA(GetDlgItem(main, panel::kRegisterScopeCombo), CB_SETCURSEL, 0, 0);
        } else {
            int found = 0;                                       
            while (found < kModelSlotCount &&
                   (slots[found] == nullptr ||
                    rinmotiondance::mdl::Mdl(slots[found])->comboSelIndex !=
                        static_cast<unsigned char>(sel)))
                ++found;
            if (found < kModelSlotCount) {
                s->SetSelectedModelSlot(static_cast<unsigned char>(found));
                PostLoadInit(slots[found]);
            }
        }
    }
    if (s->state.floatingWindow != 0) {
        RefreshSeparateWindowViewport(s);                                             
        InvalidateRect(
            reinterpret_cast<HWND>(s->state.floatingWindow),
            nullptr, FALSE);
    }
    if (s->state.optflag[0] != 0) {
        RefreshLightPanel(s);                                             
        PanelPaint(s);                                            
    }
    InvalidateRect(main, nullptr, FALSE);                        
    RelayoutSidebarControls(s);                                                 
    s->PhysicsResetPending() = 1;
    s->state.windowLayoutReady = 1;
    s->ApplyTimelineLightState();
    TraceSceneLightState(s, "pmm-v1-load-tail");
    PostViewRefresh(s);                                           
}


}  

void LoadSceneV1(MMDApp* app, int fd) {
    auto* s = app;
    PmmV1LoadContext ctx(app);

    LoadSceneV1_DisposeAndHeader(ctx, fd);            
    const unsigned char modelCount = LoadSceneV1_ModelPrePass(ctx, fd);
    LogV1Stage("pre-pass-done", _tell(fd), modelCount);
    if (modelCount != 0) {
        unsigned char modelIdx = 0;
        for (;;) {
            if (!LoadSceneV1_ModelBlock(ctx, fd))
                return;  
            if (++modelIdx >= modelCount) break;                 
        }
    }

    LoadSceneV1_RegisterCombo(ctx);                   
    LoadSceneV1_ReallocateTracks(ctx);                
    LogV1Stage("regcombo-done", _tell(fd));
    LoadSceneV1_CameraTrack(ctx, fd);                 
    LoadSceneV1_LightTrack(ctx, fd);                  
    if (!LoadSceneV1_AccessoryBlock(ctx, fd)) return; 
    LogV1Stage("accessories-done", _tell(fd));
    LoadSceneV1_ConfigBlock(ctx, fd);                 
    LoadSceneV1_ReadGatedTail(ctx, fd);               
    LogV1Stage("gated-tail-done", _tell(fd));
    _close(fd);                                       
    LoadSceneV1_SuccessTail(ctx);                     
}

}  
