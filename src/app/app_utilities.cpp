#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cmath>
#include <cstdint>
#include <cstdio>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void RefreshAfterFrameApply(MMDApp* app);   

namespace {

IDirect3DDevice9* RenderDeviceOf(MMDApp* app) {
    D3DRenderer* r = app->Renderer();
    if (r == nullptr)
        return nullptr;
    return r->device;  
}

struct LineVertex {
    float x;
    float y;
    float z;
    std::uint32_t color;
};
static_assert(sizeof(LineVertex) == 16);

const char kJpMakeLine[] =
    "\203O\203\211\203t\203B\203b\203N\203J\201[\203h"
    "\202\314\220\253\224\\\202\252\221\253\202\350\202\334\202\271\202\361";

void MakeLineFailBox(MMDApp* app, const char* jpSuffix, const char* title) {
    char text[256];
    if (app->state.englishUI != 0)
        sprintf_s(text, 0x100, "%s",
                  "The performance of the graphics card doesn't suffice.");
    else
        sprintf_s(text, 0x100, "%s%s", kJpMakeLine, jpSuffix);
    MessageBoxA(static_cast<HWND>(app->Hwnd()), text, title, 0);
}

}  

float AngleFabsF(float v) {  
    return std::fabs(v);
}

float AngleAsinF(float v) {  
    return static_cast<float>(std::asin(static_cast<double>(v)));
}

float AngleAtan2F(float a, float b) {  
    return static_cast<float>(std::atan2(static_cast<double>(a),
                                         static_cast<double>(b)));
}

float AngleCosF(float v) {  
    return static_cast<float>(std::cos(static_cast<double>(v)));
}

bool MakeLineGeometry(MMDApp* app) {  
    auto& s = *app;
    IDirect3DDevice9* device = RenderDeviceOf(app);

    IDirect3DVertexBuffer9*& lineVb = s.GroundGridVertices();       
    if (lineVb != nullptr) {
        lineVb->Release();
        lineVb = nullptr;
    }
    if (FAILED(device->CreateVertexBuffer(1440, 8  ,
                                          0x42  ,
                                          D3DPOOL_MANAGED  ,
                                          &lineVb, nullptr))) {
        MakeLineFailBox(&s, "(MakeLine)", "MakeLine");              
        return false;
    }

    LineVertex* verts = nullptr;
    lineVb->Lock(0, 1440, reinterpret_cast<void**>(&verts), 0);     
    constexpr std::uint32_t kAxisB = 0xFF0000FFu;   
    constexpr std::uint32_t kAxisR = 0xFFFF0000u;   
    constexpr std::uint32_t kAxisG = 0xFF00FF00u;   
    constexpr std::uint32_t kGrid  = 0xFFB4B4B4u;   

    verts[0]  = {0.0f, 0.1f, 0.0f, kAxisB};    
    verts[1]  = {0.0f, 0.1f, -65.0f, kAxisB};
    verts[2]  = {0.0f, 0.1f, 0.0f, kAxisR};
    verts[3]  = {65.0f, 0.1f, 0.0f, kAxisR};
    verts[4]  = {0.0f, 0.1f, 0.0f, kAxisG};
    verts[5]  = {0.0f, 65.0f, 0.0f, kAxisG};    
    verts[6]  = {0.0f, 0.1f, 0.0f, kGrid};
    verts[7]  = {0.0f, 0.1f, 50.0f, kGrid};
    verts[8]  = {0.0f, 0.1f, 0.0f, kGrid};
    verts[9]  = {-50.0f, 0.1f, 0.0f, kGrid};

    for (int t = 1; t <= 10; ++t) {             
        const float p = static_cast<float>(t) * 5.0f;
        const float n = static_cast<float>(t) * -5.0f;
        LineVertex* w = verts + 10 + 8 * (t - 1);
        w[0] = {p, 0.1f, -50.0f, kGrid};        
        w[1] = {p, 0.1f, 50.0f, kGrid};
        w[2] = {n, 0.1f, -50.0f, kGrid};
        w[3] = {n, 0.1f, 50.0f, kGrid};
        w[4] = {-50.0f, 0.1f, p, kGrid};
        w[5] = {50.0f, 0.1f, p, kGrid};
        w[6] = {-50.0f, 0.1f, n, kGrid};
        w[7] = {50.0f, 0.1f, n, kGrid};
    }
    lineVb->Unlock();                            

    IDirect3DIndexBuffer9*& lineIb = s.GroundGridIndices();         
    if (FAILED(device->CreateIndexBuffer(180, 0,
                                         D3DFMT_INDEX16  ,
                                         D3DPOOL_MANAGED  ,
                                         &lineIb, nullptr))) {      
        MakeLineFailBox(&s, "(MakeLine2)", "MakeLine2");
        return false;
    }
    void* idxRaw = nullptr;
    if (FAILED(lineIb->Lock(0, 0, &idxRaw, 0))) {                   
        MakeLineFailBox(&s, "(MakeLine3)", "MakeLine3");
        return false;
    }
    auto* indices = static_cast<std::uint16_t*>(idxRaw);
    for (int i = 0; i < 90; ++i)                                   
        indices[i] = static_cast<std::uint16_t>(i);
    lineIb->Unlock();                                              

    IDirect3DVertexBuffer9*& groundVb = s.GroundPlaneVertices();    
    device->CreateVertexBuffer(96, 8  , 0x42,
                               D3DPOOL_MANAGED, &groundVb, nullptr);
    LineVertex* ground = nullptr;
    groundVb->Lock(0, 96, reinterpret_cast<void**>(&ground), 0);    
    constexpr std::uint32_t kWhite = 0x00FFFFFFu;
    ground[0] = {-50.0f, 0.0f, -50.0f, kWhite};   
    ground[1] = {-50.0f, 0.0f, 50.0f, kWhite};
    ground[2] = {50.0f, 0.0f, -50.0f, kWhite};
    ground[3] = {50.0f, 0.0f, -50.0f, kWhite};
    ground[4] = {-50.0f, 0.0f, 50.0f, kWhite};
    ground[5] = {50.0f, 0.0f, 50.0f, kWhite};
    groundVb->Unlock();                                            
    return true;
}

void UpdateKeyEdgeState(MMDApp* app, int nVirtKey,
                        std::uint32_t* state) {  
    if ((GetKeyState(nVirtKey) & 0x80) == 0x80) {                  
        if (*state != 0) {
            *state = 3;                                            
        } else {
            *state = 1;                                            
            app->state.messageSeen = 1;      
        }
    } else if (*state == 1 || *state == 3) {
        *state = 2;                                                
        app->state.messageSeen = 1;
    } else {
        *state = 0;                                                
    }
}

void ShowWin32ErrorMessage(MMDApp* app, const char* context,
                           DWORD messageId) {  
    char* message = nullptr;
    char text[256];
    FormatMessageA(0x1300, nullptr, messageId, 0x400,
                   reinterpret_cast<LPSTR>(&message), 0, nullptr);  
    sprintf_s(text, 0x100, "%s: %d:%s\n", context, messageId,
              message);                                            
    MessageBoxA(static_cast<HWND>(app->Hwnd()), text, "error", 0);  
    LocalFree(message);                                            
}

HRESULT DrawGridLines(MMDApp* app) {  
    IDirect3DDevice9* device = RenderDeviceOf(app);
    device->SetRenderState(D3DRS_LIGHTING  , 0);              
    device->SetTexture(0, nullptr);                                 
    device->SetFVF(0x42  );             
    device->SetStreamSource(
        0, app->GroundGridVertices()  ,
        0, 16);                                                     
    device->SetIndices(app->GroundGridIndices()  );           
    return device->DrawIndexedPrimitive(D3DPT_LINELIST  , 0, 0, 90,
                                        0, 45);                     
}

std::uintptr_t DrawGroundPolygon(MMDApp* app) {  
    D3DRenderer* r = app->Renderer();
    IDirect3DDevice9* device = r->device;  
    if (r->d3dInitialized != 0)   
        device->SetRenderState(D3DRS_STENCILENABLE  , 0);
    device->SetStreamSource(
        0, app->GroundPlaneVertices()  ,
        0, 16);                                                     
    device->SetFVF(0x42);                                           
    device->DrawPrimitive(D3DPT_TRIANGLELIST  , 0, 2);          
    device->SetRenderState(D3DRS_ALPHABLENDENABLE  , 0);       
    if (r->d3dInitialized != 0)   
        return static_cast<std::uintptr_t>(
            device->SetRenderState(D3DRS_STENCILENABLE  , 1));
    return reinterpret_cast<std::uintptr_t>(r);
}

void JumpNextKeyframe(MMDApp* app) {  
    auto& s = *app;
    const std::uint32_t cur = s.state.currentFrame;

    if (s.state.optflag[0] != 0) {        
        std::uint32_t best = 0xFFFFFFFAu;

        if (s.GlobalTrackSelected(GlobalTimelineTrack::Camera) != 0) {
            std::uint32_t* n = reinterpret_cast<std::uint32_t*>(s.CameraKeys());
            std::uint32_t last = 0;
            if (n[0] <= cur) {                                      
                std::uint32_t idx = 0;
                for (;;) {                                          
                    const std::uint32_t next = n[idx + 2];
                    if (next == 0)
                        break;
                    last = next;
                    idx = 21 * next;
                    if (n[idx] > cur)
                        break;
                }
            }
            const std::uint32_t f = n[21 * last];                   
            if (f > cur && f < 0xFFFFFFFAu)                         
                best = f;
        }
        if (s.GlobalTrackSelected(GlobalTimelineTrack::Light) != 0) {
            std::uint32_t* n = reinterpret_cast<std::uint32_t*>(s.LightKeys());
            std::uint32_t last = 0;
            if (n[0] <= cur) {
                std::uint32_t idx = 0;
                for (;;) {                                          
                    const std::uint32_t next = n[idx + 2];
                    if (next == 0)
                        break;
                    last = next;
                    idx = 10 * next;
                    if (n[idx] > cur)
                        break;
                }
            }
            const std::uint32_t f = n[10 * last];                   
            if (f > cur && f < best)
                best = f;
        }
        if (s.GlobalTrackSelected(GlobalTimelineTrack::SelfShadow) != 0) {
            std::uint32_t* n = reinterpret_cast<std::uint32_t*>(s.ShadowKeys());
            std::uint32_t last = 0;
            if (n[0] <= cur) {
                std::uint32_t idx = 0;
                for (;;) {                                          
                    const std::uint32_t next = n[idx + 2];
                    if (next == 0)
                        break;
                    last = next;
                    idx = 6 * next;
                    if (n[idx] > cur)
                        break;
                }
            }
            const std::uint32_t f = n[6 * last];                    
            if (f > cur && f < best)
                best = f;
        }
        if (s.GlobalTrackSelected(GlobalTimelineTrack::Gravity) != 0) {
            std::uint32_t* n = reinterpret_cast<std::uint32_t*>(s.GravityKeys());
            std::uint32_t last = 0;
            if (n[0] <= cur) {
                std::uint32_t idx = 0;
                for (;;) {                                          
                    const std::uint32_t next = n[idx + 2];
                    if (next == 0)
                        break;
                    last = next;
                    idx = 9 * next;
                    if (n[idx] > cur)
                        break;
                }
            }
            const std::uint32_t f = n[9 * last];                    
            if (f > cur && f < best)
                best = f;
        }
        for (int i = 0; i < 255; ++i) {                             
            std::uint32_t* n =
                reinterpret_cast<std::uint32_t*>(s.AccessoryKeys(i));
            const auto* accessory = s.AccessorySlots()[i];
            if (accessory != nullptr && accessory->rowSelected != 0) {       
                std::uint32_t last = 0;
                if (n[0] <= cur) {                                  
                    const std::uint32_t* p = n;
                    for (;;) {                                      
                        const std::uint32_t next = p[2];
                        if (next == 0)
                            break;
                        last = next;
                        p = n + 15 * next;
                        if (p[0] > cur)
                            break;
                    }
                }
                const std::uint32_t f = n[15 * last];               
                if (f > cur && f < best)                            
                    best = f;
            }
        }

        if (best < 0xFFFFFFFAu) {                                   
            s.state.currentFrame = best;        
            char buf[256];
            sprintf_s(buf, 0x100, "%d", best);                      
            SetWindowTextA(GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kCurrentFrameEdit),
                           buf);                                    
            RefreshAfterFrameApply(app);                            
            PostViewRefresh(app);                                   
        }
        return;
    }

    unsigned char* model = s.SelectedModel();                       
    std::uint32_t best = 0xFFFFFFFAu;

    if (mdl::Mdl(model)->displayKeyframesPresent != 0) {             
        mdl::DisplayKey* n = mdl::DisplayKeys(model);
        std::uint32_t last = 0;
        if (n[0].frame <= cur) {
            std::uint32_t idx = 0;
            for (;;) {                                              
                const std::uint32_t next = n[last].next;
                if (next == 0)
                    break;
                last = next;
                idx = next;
                if (n[idx].frame > cur)
                    break;
            }
        }
        const std::uint32_t f = n[last].frame;
        if (f > cur && f < 0xFFFFFFFAu)
            best = f;
    }
    if (rinmotiondance::mdl::Mdl(model)->facialFrameCount != 0) {                                    
        const auto& record = *mdl::Mdl(model);
        const mdl::MorphKey* base = record.morphKeys;
        for (int i = 0; i < record.facialFrameCount; ++i) {
            const mdl::FrameGroup& row = record.displayFrames[i];
            if (row.selected != 0) {
                std::uint32_t node = row.targetIndex;
                if (base[node].frame <= cur) {
                    for (;;) {                                      
                        const std::uint32_t next = base[node].next;
                        if (next == 0)
                            break;
                        node = next;
                        if (base[node].frame > cur)
                            break;
                    }
                }
                const std::uint32_t f = base[node].frame;
                if (f > cur && f < best)
                    best = f;
            }
        }
    }
    {
        const int count =
            static_cast<int>(rinmotiondance::mdl::Mdl(model)->boneCount);   
        if (count > 0) {
            mdl::BoneKey* base = mdl::BoneKeys(model);
            const unsigned char* flags =
                rinmotiondance::mdl::Mdl(model)->boneSelection;
            for (int k = 0; k < count; ++k) {
                if (flags[k] != 0) {                                
                    std::uint32_t node = k;
                    if (base[node].frame <= cur) {
                        for (;;) {                                  
                            const std::uint32_t next = base[node].next;
                            if (next == 0)
                                break;
                            node = next;
                            if (base[node].frame > cur)
                                break;
                        }
                    }
                    const std::uint32_t f = base[node].frame;
                    if (f > cur && f < best)
                        best = f;
                }
            }
        }
    }

    if (best < 0xFFFFFFFAu) {                                       
        s.state.currentFrame = best;            
        char buf[256];
        sprintf_s(buf, 0x100, "%d", best);                          
        SetWindowTextA(GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kCurrentFrameEdit),
                       buf);                                        
        RefreshAfterFrameApply(app);                                
        PostViewRefresh(app);                                       
    }
}

void JumpPrevKeyframe(MMDApp* app) {  
    auto& s = *app;
    const std::uint32_t cur = s.state.currentFrame;

    if (s.state.optflag[0] != 0) {        
        std::uint32_t best = 0;

        if (s.GlobalTrackSelected(GlobalTimelineTrack::Camera) != 0) {
            std::uint32_t* n = reinterpret_cast<std::uint32_t*>(s.CameraKeys());
            std::uint32_t last = 0;
            if (n[0] < cur) {                                       
                std::uint32_t idx = 0;
                for (;;) {                                          
                    const std::uint32_t next = n[idx + 2];
                    if (next == 0)
                        break;
                    last = next;
                    idx = 21 * next;
                    if (n[idx] >= cur)
                        break;
                }
            }
            const std::uint32_t f = n[21 * last];                   
            if (f >= cur) {                                         
                const std::uint32_t f2 = n[21 * n[21 * last + 1]];  
                if (f2 < cur && f2 != 0)
                    best = f2;
            } else if (f != 0) {                                    
                best = f;
            }
        }
        if (s.GlobalTrackSelected(GlobalTimelineTrack::Light) != 0) {
            std::uint32_t* n = reinterpret_cast<std::uint32_t*>(s.LightKeys());
            std::uint32_t last = 0;
            if (n[0] < cur) {
                std::uint32_t idx = 0;
                for (;;) {                                          
                    const std::uint32_t next = n[idx + 2];
                    if (next == 0)
                        break;
                    last = next;
                    idx = 10 * next;
                    if (n[idx] >= cur)
                        break;
                }
            }
            const std::uint32_t f = n[10 * last];                   
            if (f < cur) {                                          
                if (f > best)
                    best = f;
            } else {
                const std::uint32_t f2 = n[10 * n[10 * last + 1]];  
                if (f2 < cur && f2 > best)
                    best = f2;
            }
        }
        if (s.GlobalTrackSelected(GlobalTimelineTrack::SelfShadow) != 0) {
            std::uint32_t* n = reinterpret_cast<std::uint32_t*>(s.ShadowKeys());
            std::uint32_t last = 0;
            if (n[0] < cur) {
                std::uint32_t idx = 0;
                for (;;) {                                          
                    const std::uint32_t next = n[idx + 2];
                    if (next == 0)
                        break;
                    last = next;
                    idx = 6 * next;
                    if (n[idx] >= cur)
                        break;
                }
            }
            const std::uint32_t f = n[6 * last];                    
            if (f >= cur) {                                         
                const std::uint32_t f2 = n[6 * n[6 * last + 1]];    
                if (f2 < cur && f2 > best)
                    best = f2;
            } else if (f > best) {                                  
                best = f;
            }
        }
        if (s.GlobalTrackSelected(GlobalTimelineTrack::Gravity) != 0) {
            std::uint32_t* n = reinterpret_cast<std::uint32_t*>(s.GravityKeys());
            std::uint32_t last = 0;
            if (n[0] < cur) {
                std::uint32_t idx = 0;
                for (;;) {                                          
                    const std::uint32_t next = n[idx + 2];
                    if (next == 0)
                        break;
                    last = next;
                    idx = 9 * next;
                    if (n[idx] >= cur)
                        break;
                }
            }
            const std::uint32_t f = n[9 * last];                    
            if (f >= cur) {                                         
                const std::uint32_t f2 = n[9 * n[9 * last + 1]];    
                if (f2 < cur && f2 > best)
                    best = f2;
            } else if (f > best) {                                  
                best = f;
            }
        }
        for (int i = 0; i < 255; ++i) {                             
            std::uint32_t* n =
                reinterpret_cast<std::uint32_t*>(s.AccessoryKeys(i));
            const auto* accessory = s.AccessorySlots()[i];
            if (accessory == nullptr || accessory->rowSelected == 0)         
                continue;
            std::uint32_t last = 0;
            if (n[0] < cur) {                                       
                const std::uint32_t* p = n;
                for (;;) {                                          
                    const std::uint32_t next = p[2];
                    if (next == 0)
                        break;
                    last = next;
                    p = n + 15 * next;
                    if (p[0] >= cur)
                        break;
                }
            }
            const std::uint32_t f = n[15 * last];                   
            if (f >= cur) {                                         
                const std::uint32_t f2 = n[15 * n[15 * last + 1]];  
                if (f2 < cur && f2 > best)
                    best = f2;
            } else if (f > best) {                                  
                best = f;
            }
        }

        s.state.currentFrame = best;            
        char buf[256];
        sprintf_s(buf, 0x100, "%d", best);                          
        SetWindowTextA(GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kCurrentFrameEdit),
                       buf);                                        
        RefreshAfterFrameApply(app);                                
        PostViewRefresh(app);                                       
        return;
    }

    unsigned char* model = s.SelectedModel();                       
    std::uint32_t best = 0;                                         

    if (mdl::Mdl(model)->displayKeyframesPresent != 0) {             
        mdl::DisplayKey* n = mdl::DisplayKeys(model);
        std::uint32_t last = 0;
        if (n[0].frame < cur) {
            std::uint32_t idx = 0;
            for (;;) {                                              
                const std::uint32_t next = n[last].next;
                if (next == 0)
                    break;
                last = next;
                idx = next;
                if (n[idx].frame >= cur)
                    break;
            }
        }
        const std::uint32_t f = n[last].frame;
        if (f >= cur) {                                             
            const std::uint32_t f2 =
                n[n[last].previous].frame;
            if (f2 < cur && f2 != 0)
                best = f2;
        } else if (f != 0) {                                        
            best = f;
        }
    }
    if (rinmotiondance::mdl::Mdl(model)->facialFrameCount != 0) {                                    
        const auto& record = *mdl::Mdl(model);
        const mdl::MorphKey* base = record.morphKeys;
        for (int i = 0; i < record.facialFrameCount; ++i) {
            const mdl::FrameGroup& row = record.displayFrames[i];
            if (row.selected != 0) {
                std::uint32_t node = row.targetIndex;
                if (base[node].frame < cur) {
                    for (;;) {                                      
                        const std::uint32_t next = base[node].next;
                        if (next == 0)
                            break;
                        node = next;
                        if (base[node].frame >= cur)
                            break;
                    }
                }
                const std::uint32_t f = base[node].frame;
                if (f >= cur) {                                     
                    const std::uint32_t f2 =
                        base[base[node].previous].frame;
                    if (f2 < cur && f2 > best)
                        best = f2;
                } else if (f > best) {                              
                    best = f;
                }
            }
        }
    }
    {
        const int count =
            static_cast<int>(rinmotiondance::mdl::Mdl(model)->boneCount);   
        if (count > 0) {
            mdl::BoneKey* base = mdl::BoneKeys(model);
            const unsigned char* flags =
                rinmotiondance::mdl::Mdl(model)->boneSelection;
            for (int k = 0; k < count; ++k) {
                if (flags[k] != 0) {                                
                    std::uint32_t node = k;
                    if (base[node].frame < cur) {
                        for (;;) {                                  
                            const std::uint32_t next = base[node].next;
                            if (next == 0)
                                break;
                            node = next;
                            if (base[node].frame >= cur)
                                break;
                        }
                    }
                    const std::uint32_t f = base[node].frame;
                    if (f >= cur) {                                 
                        const std::uint32_t f2 =
                            base[base[node].previous].frame;
                        if (f2 < cur && f2 > best)
                            best = f2;
                    } else if (f > best) {                          
                        best = f;
                    }
                }
            }
        }
    }

    s.state.currentFrame = best;                
    char buf[256];
    sprintf_s(buf, 0x100, "%d", best);                              
    SetWindowTextA(GetDlgItem(static_cast<HWND>(s.Hwnd()), panel::kCurrentFrameEdit),
                   buf);                                            
    RefreshAfterFrameApply(app);                                    
    PostViewRefresh(app);                                           
}

}  
