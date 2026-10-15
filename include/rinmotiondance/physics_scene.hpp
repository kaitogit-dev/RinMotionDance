#pragma once

#include <cstddef>
#include <cstdint>

#include <d3d9.h>

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class bt32BitAxisSweep3;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btRigidBody;

namespace rinmotiondance {

class D3DRenderer;

class PhysicsScene {
public:
    D3DRenderer* owner;              

    IDirect3DVertexBuffer9* gizmoSphereVB;     
    IDirect3DIndexBuffer9* gizmoSphereIB;      
    IDirect3DVertexBuffer9* gizmoCubeVB;       
    IDirect3DIndexBuffer9* gizmoCubeIB;        
    IDirect3DVertexBuffer9* gizmoSphere33VB;   
    IDirect3DIndexBuffer9* gizmoSphere33IB;    
    IDirect3DVertexBuffer9* gizmoArrowVB;      
    IDirect3DIndexBuffer9* gizmoIdentityIB;    
    IDirect3DVertexBuffer9* gizmoBoxSelVB;     
    IDirect3DIndexBuffer9* gizmoBoxSelIB;      

    btDefaultCollisionConfiguration* collisionConfig;        
    btCollisionDispatcher* dispatcher;                       
    bt32BitAxisSweep3* broadphase;                           
    std::int32_t constraintId;        
    btSequentialImpulseConstraintSolver* solver;             
    btDiscreteDynamicsWorld* world;                          
    btRigidBody* groundBody;                                 
};

static_assert(sizeof(PhysicsScene) == 0x48 || sizeof(PhysicsScene) == 0x90,
              "size must be the x86 or x64 original allocation size");

#define RINMOTIONDANCE_SCENE_OFF(f, x86off, x64off)                              \
    static_assert(offsetof(PhysicsScene, f) ==                            \
                      (sizeof(void*) == 8 ? (x64off) : (x86off)),         \
                  #f " offset must match the original binary")

RINMOTIONDANCE_SCENE_OFF(owner, 0, 0);
RINMOTIONDANCE_SCENE_OFF(gizmoSphereVB, 4, 8);
RINMOTIONDANCE_SCENE_OFF(gizmoSphereIB, 8, 16);
RINMOTIONDANCE_SCENE_OFF(gizmoCubeVB, 12, 24);
RINMOTIONDANCE_SCENE_OFF(gizmoCubeIB, 16, 32);
RINMOTIONDANCE_SCENE_OFF(gizmoSphere33VB, 20, 40);
RINMOTIONDANCE_SCENE_OFF(gizmoSphere33IB, 24, 48);
RINMOTIONDANCE_SCENE_OFF(gizmoArrowVB, 28, 56);
RINMOTIONDANCE_SCENE_OFF(gizmoIdentityIB, 32, 64);
RINMOTIONDANCE_SCENE_OFF(gizmoBoxSelVB, 36, 72);
RINMOTIONDANCE_SCENE_OFF(gizmoBoxSelIB, 40, 80);
RINMOTIONDANCE_SCENE_OFF(collisionConfig, 44, 88);
RINMOTIONDANCE_SCENE_OFF(dispatcher, 48, 96);
RINMOTIONDANCE_SCENE_OFF(broadphase, 52, 104);
RINMOTIONDANCE_SCENE_OFF(constraintId, 56, 112);
RINMOTIONDANCE_SCENE_OFF(solver, 60, 120);
RINMOTIONDANCE_SCENE_OFF(world, 64, 128);
RINMOTIONDANCE_SCENE_OFF(groundBody, 68, 136);

#undef RINMOTIONDANCE_SCENE_OFF

void DisposePhysicsWorld(PhysicsScene* scene);

}  
