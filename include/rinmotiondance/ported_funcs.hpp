#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>

#include "rinmotiondance/charset_conv.hpp"

#include <cstdint>
#include <cstdio>

namespace rinmotiondance {

class MMDApp;
struct PathResolutionWorkspace;
struct DShowRecorder;
class D3DRenderer;  
class PhysicsScene;  

bool InitMainWindowAndD3D(MMDApp* app, void* hInstance, int nShowCmd);

void FrameDriver(MMDApp* app);
void PrepareFrameSpriteOverlay(MMDApp* app);  
void PrepareFrameTextOverlay(MMDApp* app);    
void PrepareFrameLineOverlay(MMDApp* app);    

void ShutdownCleanup(MMDApp* app);


std::uint32_t ColorLerp(std::uint32_t a, std::uint32_t b, float t);

wchar_t* ExtractDirFromPath(wchar_t* destination, const wchar_t* fullPath);

void RendererInit(D3DRenderer* renderer);  
void InitAudioContext(void* sub);               
void InitDShowRecorder(DShowRecorder* recorder);  
void InitAccessoryRecord(void* sub);               
bool InitAxisMesh(MMDApp* app);             
bool MakeLineGeometry(MMDApp* app);          
void PhysicsSceneInit(PhysicsScene* scene); 
void FontSubInit(MMDApp* app, const wchar_t* exeDir);  
void LocalizeUI(MMDApp* app);             
bool CreateUIControls(MMDApp* app, HWND hwnd);  
void InitFlagSubsystem(MMDApp* app);       

void LoadSceneFile();                      
void LoadModelFile(MMDApp* app, const wchar_t* path);  
void LoadAccessoryFile(const wchar_t* path);  

void ModelInitDefaults(unsigned char* model);         
void ModelInitMorphSlots(unsigned char* model);       
bool ModelLoadPMD(unsigned char* model, HWND hwnd, const wchar_t* path,
                  D3DRenderer* sub, int a5, std::uint8_t a6,
                  std::uint8_t a7, PhysicsScene* a8,
                  PathResolutionWorkspace& paths);    
bool LoadPMX(unsigned char* model, D3DRenderer* sub,
             std::uint8_t showInfo, std::uint8_t englishUI,
             PathResolutionWorkspace& paths, int fh); 
void ModelDispose(unsigned char* model);              
void DisposeAccessory(void* accessory);               
void InitBoneSortOrder(unsigned char* model);         
void PostLoadInit(unsigned char* model);              
bool SceneConstruct(PhysicsScene* scene, D3DRenderer* d3dSub);  
void ModelKinematicSync(unsigned char* model);        
void ModelDynamicReseat(unsigned char* model);        
void ModelPhysicsReadback(unsigned char* model);      
void PhysicsFrame(MMDApp* app, unsigned char selActive);  
void PlaybackCatchup(MMDApp* app, unsigned char selActive);
void PlaybackPoseAdvance(MMDApp* app, int advance);  
void InitModelTrackCursors(unsigned char* model, float cursor,
                           int physicsMode);  
void AdvanceModelKeyframes(unsigned char* model, float cursor,
                           int physicsMode);  
float BoneEase(unsigned char* model, int channel, int keyIdx,
               float t);  
void NotifyBonePhysicsMode(unsigned char* model, int boneIdx,
                           unsigned char mode);  
int SeekModelFrame(unsigned char* model, int frame,
                  int physicsMode);  
void FinishAviRecord(MMDApp* app);  
void StopPlayback(MMDApp* app);  
void CloseDataFile(void* file);                       
void WaveCtxReset(void* obj);                         
bool WaveFindDataChunk(void* obj, FILE* stream);      
bool WaveStartPlayback(void* obj);                      
bool WaveStreamRead(void* obj, void* buf, int bytes); 
bool WaveStreamFeed(void* obj, void* buf, int bytes, void* buf2,
                    int bytes2);                      
bool WaveLoadFile(void* obj, const wchar_t* path,
                  PathResolutionWorkspace& paths);    
void SetFrameNormalized(int v);                       
void TimelineDrawTicks(int frameOffset, int width);  
void WaveSeekAndFeed(void* obj, double seconds);  

void CommitEditControl(MMDApp* app, HWND edit);      
void CommitEditControlTail(MMDApp* app, HWND edit);  
void InstallControlSubclasses(MMDApp* app, HWND hwnd);  
void InstallControlSubclass(MMDApp* app, HWND control, int id);

INT_PTR CALLBACK FrameRangeDlgProc(HWND, UINT, WPARAM, LPARAM);  
INT_PTR CALLBACK SelectNavDlgProc(HWND, UINT, WPARAM, LPARAM);  
void SetPhysicsMode(unsigned char* model, int a2,
                    unsigned char* const* modelSlots,
                    int a4);                          
void ModelApplyMorphs(unsigned char* model);          
void UpdateModelVertexBuffers(MMDApp* app, unsigned char* model,
                              const float frameWorld[16]); 
void CreateRigidBody(PhysicsScene* scene, void** out, int shape, float sx, float sy,
                     float sz, const float* matrix, int mode, float mass,
                     float dampLin, float dampRot, float restitution,
                     float friction, char group,
                     std::uint16_t mask);             
int CreatePhysJoint(PhysicsScene* scene, void* bodyA, void* bodyB,
                    float ax, float ay, float az, float aq0, float aq1,
                    float aq2, float aq3, float bx, float by, float bz,
                    float bq0, float bq1, float bq2, float bq3,
                    float linUpper0, float linUpper1, float linUpper2,
                    float linLower0, float linLower1, float linLower2,
                    float angUpper0, float angUpper1, float angUpper2,
                    float angLower0, float angLower1, float angLower2,
                    float spring0, float spring1, float spring2,
                    float spring3, float spring4,
                    float spring5);                  
void TransformJointPointOriginal(float out[3], const float point[3],
                                 const float matrix[16]);
float JointRadiusBound(const float limits[6], float distA, float distB);
void BoneFrameTransform(unsigned char* model, unsigned char a2, int a3,
                        unsigned char* const* modelSlots,
                        int a5);  
void ArmBoneTransformIkProbe();

const wchar_t* ResolveUserFilePath(PathResolutionWorkspace& paths,
                                   const wchar_t* path);      
bool ResolveAnsiUserFile(unsigned char* sub, const char* mbName,
                         wchar_t* wideOut, rsize_t sizeWords,
                         PathResolutionWorkspace& paths);     
errno_t ConvertMaterialName(unsigned char* sub, const char* mbName,
                            wchar_t* wideOut, rsize_t sizeWords,
                            const wchar_t* dirW);             
int LoadTextureShared(unsigned char* sub, wchar_t* path);     
void FillPanelBottom(HDC, int x, int y, int w, int h,
                     std::uint32_t color, std::uint32_t edge,
                     int flag);                               
void PostModelReload2(MMDApp*);                       
void RebuildModelModePanel(MMDApp*);     
void RebuildCameraModePanel(MMDApp*);    
void ApplyModelComboSelection(MMDApp*);  

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);  
LRESULT CALLBACK RecWndProc(HWND, UINT, WPARAM, LPARAM);   
LRESULT CALLBACK MicWndProc(HWND, UINT, WPARAM, LPARAM);   

void CommandDispatch(HWND ctrl, WPARAM wParam);              
void SaveFlagSubsystem(MMDApp*);                            
void HandleWindowSize(MMDApp*);                             
void RefreshMainWindowViewport(MMDApp*);      
void RefreshSeparateWindowViewport(MMDApp*);  
void HandleWindowPaint(MMDApp*);                            
LRESULT HandleNotify(HWND, UINT, WPARAM, LPARAM);           
void HandlePaletteChanged(HDC);                             
void HandlePaletteChanged2(HDC);                            
void HandleDropFiles(HDROP);                                

void AviBgOverlayRefresh(MMDApp*);            
void PicBgOverlayRefresh(MMDApp*);            
void StartAviRecordWindow(MMDApp*);           
void StartAviRecordFullscreen(MMDApp*);       
void ApplyFullscreenWindowState(MMDApp*);  
void KickRecordPhysics(MMDApp*);  
void HandleTimer100(MMDApp*);                               
void HandleHScroll(LPARAM lParam, WPARAM wParam);   
void HandleVScroll(LPARAM lParam, WPARAM wParam);   
LRESULT HandleCtlColor(HWND control, HDC dc);  
void HandleMouseMove(std::uint32_t lParam, int mouseY);     
void HandleLButtonDown(MMDApp*);                            
void HandleLButtonUp(MMDApp*);                              
void HandleLButtonDblClk(MMDApp*);                          
void HandleMouseActivate(MMDApp*);                          
void HandleMouseWheel(int wheelDelta);                      
void PostLanguageSweep(MMDApp*);                            
void PostLanguageSweep2(MMDApp*);                           

int GetPmdNum(MMDApp* app);                                   

void CreateUiFont(MMDApp* app, HWND hwnd);        
void ApplyUiFontToControl(MMDApp* app, HWND control, int id);
int  DrawGlyph(MMDApp*, const char*, HDC, int size, int x, int y,
               unsigned char r, unsigned char g, unsigned char b,
               int bold);                         
bool InitToonTextures(MMDApp* app);               
bool InitSceneFontTexture(MMDApp* app);           

void MouseInteractionBegin(MMDApp*);              
void MouseInteractionEnd(MMDApp*);                
void ModeRotate(MMDApp*, int axis);               
void ModeTranslate(MMDApp*, int axis);            
void ModeScale(MMDApp*);                          
void ModeBoneRotate(MMDApp*, int part);
void ModeDirectBoneRotate(MMDApp*);
void ModeDirectBoneTranslate(MMDApp*);           
void ModePhysicsBody(MMDApp*, int axis);          
void ModeCameraAdjust(MMDApp*, int part);         
void ModeAngleAdjust(MMDApp*, int part);          
void BoneLocalAxes(MMDApp*, float out[16]);       
void BoneEditModes(MMDApp*);                      
void PushBoneEditUndo(MMDApp*);                    
void RefreshRequest(int area);                    
void TimelineAdvance(MMDApp*);                    
void ReloadModels(MMDApp*);                       
void PostModelReload(MMDApp*);                    
void SelectionReeval(MMDApp*);                    
void PostDeviceReset(MMDApp*);                    
void UpdateBoneFrames(MMDApp*);                   
void RenderFrameScene(MMDApp*);                   
bool RecordingReadbackPass(MMDApp*);              
ULONG DownsampleCaptureSurface(MMDApp*);          
void DrawAccessoryDebug(MMDApp*);                 
void DrawPhysicsCollisionDebug(PhysicsScene*);    
void DrawBoneOperationAxis(MMDApp*, const float[16]); 
void SetupFrameWorldTransform(MMDApp*);          
bool UseEffectModelRenderer(MMDApp*);            
void RenderShadowMap(MMDApp*, const float[16]);  
void RenderModelsFixed(MMDApp*);                 
void RenderModelsEffect(MMDApp*, const float[16]); 
void RenderAccessoriesFixed(MMDApp*);             
void RenderAccessoriesFixedRange(MMDApp*, int, int);
void RenderAccessoriesProjectedGroundShadowGeometry(MMDApp*);
void RenderAccessoriesShadow(MMDApp*);            
void RenderAccessoriesEffect(MMDApp*);            
void RenderAccessoriesEffectRange(MMDApp*, int, int);

void CmdLoadPose(MMDApp*);                        
void CmdSavePose(MMDApp*);                        
void CmdResetState(MMDApp*);                      
void CmdOpenScene(MMDApp*);                       
void CmdOpenWave(MMDApp*);                        
void CmdSaveScene(MMDApp*);                       
void CmdLoadMotion(MMDApp*);                      
void CmdSaveMotion(MMDApp*);                      
void CmdLoadAvi(MMDApp*);                         

bool InitD3D(MMDApp* app, HWND hwnd, bool english, HMODULE hModule); 
void InitRenderStates(MMDApp*);                   
bool ProbeStereo3D(void* device, void* arg20);    
void UpdateFrameStereo(MMDApp* app);              
int NvapiInitChain();                             
int NvapiStereoCaps(int arg);                     
int NvapiStereoSupportGate();                     
void PostViewRefresh(MMDApp*);                    

void LoadVpdFile(const wchar_t* path);            
void SaveVpdFile(const wchar_t* path);            
void ResetAppState(MMDApp*);                      
void LoadWaveFile(MMDApp* app);                   
void SaveSceneFile(MMDApp* app);                  
void PathToProjectDir(wchar_t* destination, const wchar_t* source);  
void LoadSceneV2(MMDApp* app, int fd);             
void LoadSceneV1(MMDApp* app, int fd);            
void LoadVmdFile(const wchar_t* path);            
int LoadVmdMotion(MMDApp* app, const wchar_t* fileName);  
void PanelPaint(MMDApp* app);                     
int ResetMorphKeyCursor(unsigned char* model);    
bool RegisterBoneKey(unsigned char* model, unsigned char* rec,
                     int frameOffset, unsigned char useSelected);  
bool RegisterMirroredBoneKey(unsigned char* model, unsigned char* rec,
                             int frameOffset);    
void ResetBoneKeyCursor(unsigned char* model);    
void AppendBoneKeyToUndo(unsigned char* model, int index);  
void RebuildBoneKeyInterpolation(unsigned char* model, int index, int lane);

int RegisterBonePoseAtFrame(unsigned char* model, int boneIdx,
                            std::uint32_t frame, int mode);  

void RegisterSelectedBoneKeys(unsigned char* model, int frame, int mode);
void ResetDisplayKeyCursor(unsigned char* model); 
void DeleteMarkedModelKeys(unsigned char* model, int frame);
void UndoModelEdit(unsigned char* model, std::int32_t& frame);
void RedoModelEdit(unsigned char* model, std::int32_t& frame);
void BeginRangeScaleBoneUndo(unsigned char* model, int frame,
                             int transformedKeyCount);
int RegisterCameraKey(MMDApp* app, const void* rec,
                      int overflowAdvertised = 10000);    
int RegisterLightKey(MMDApp* app, const void* rec);       
int RegisterSelfShadowKey(MMDApp* app, const void* rec);  
int RegisterGravityKey(MMDApp* app, const void* rec);     
void ApplyGravityTrack(MMDApp* app);                
void ApplyAccessoryTrack(MMDApp* app, int slot);    

void ClearTimelineAndCurveDCs(MMDApp* app);   
void RefillBoneRegisterCombo(MMDApp* app, int slot);  
INT_PTR CALLBACK DialogFuncStub(HWND hwnd, UINT msg, WPARAM wp,
                                   LPARAM lp); 

void IdentityCtor(void* obj);                    
bool LoadAccessoryObject(MMDApp* app, void* accessory,
                         const wchar_t* path);               
void SyncAccessoryEditPanel(MMDApp* app);        
void RefreshSelfShadowPanel(MMDApp* app);        
void RefreshLightPanel(MMDApp* app);             
void TraceSceneLightState(MMDApp* app, const char* stage);
void RelayoutSidebarControls(MMDApp* app);       
void SetModelColor(MMDApp* model, int r, int g, int b);  
void SeekSelectedModelToCurrentFrame(MMDApp* app);  
void SaveEnhancedModel(MMDApp* app, const wchar_t* path);  
void ReloadTextureCache(void* renderer);        
bool RegisterMorphKeyFromRecord(unsigned char* model, const unsigned char rec[40],
                                int frameOffset);  
void RegisterMorphKeyCurrent(unsigned char* model, int morph, int frame);  
bool RegisterDisplayKeyFromRecord(unsigned char* model, int frame,
                                  unsigned char view, int ikCount,
                                  const unsigned char* ikEntries, int selCount,
                                  const unsigned char* selEntries,
                                  int frameOffset);  
void RegisterDisplayKeyCurrent(unsigned char* model, int frame);  
void RegisterAccessoryKey(MMDApp* app, int frame, int slot);  
void DeleteMarkedKeyframes(MMDApp* app);         
void SaveVmdFile(const wchar_t* path);            
void LoadAviFile(MMDApp* app);                    
void LoadBackgroundPicture(MMDApp* app);          
void CopyPathW(wchar_t* dest, const wchar_t* src);  
void CopyDirPathW(wchar_t* dest, const wchar_t* src);  
void SelectFrameGroup(MMDApp*, int group);        
void InsertBoneCameraFrameLine(MMDApp* app);  
void DeleteBoneCameraFrameLine(MMDApp* app);  
void InsertFacialLightFrameLine(MMDApp* app); 
void DeleteFacialLightFrameLine(MMDApp* app); 

void OpenNiInit(MMDApp* app, const char* sjisPath);  
void DisableKinect(MMDApp* app);                     
void PumpKinectSkeleton(MMDApp* app, unsigned char selActive);
void ManageKinectRecordGate(MMDApp* app, unsigned char selActive);
bool RegisterKinectPoseCapture(unsigned char* model, unsigned frame);  
bool RegisterTraceBoneKey(unsigned char* model, unsigned boneIdx,      
                          int slot, int mode, unsigned frame);
void LoadVsqFile(MMDApp* app, const wchar_t* path);  
int ReadBeWord(int fh, int* outVal, int nbytes);      
int ReadFixedString(int fh, char* out, int len);      
void ShowWin32ErrorMessage(MMDApp* app, const char* context,
                           DWORD messageId);          

}  
