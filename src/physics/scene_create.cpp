
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstring>

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/BroadphaseCollision/btAxisSweep3.h"  

#include "rinmotiondance/d3d_wrapper.hpp"
#include "rinmotiondance/physics_scene.hpp"
#include "rinmotiondance/physics_world.hpp"

#include "scene_gizmo_data.inc"

namespace rinmotiondance {
namespace {

bool PutVB(IDirect3DDevice9* dev, IDirect3DVertexBuffer9** slot,
           unsigned len, const void* data) {
    IDirect3DVertexBuffer9* vb = nullptr;
    if (FAILED(dev->CreateVertexBuffer(
            len, 0x8  , 0x42  ,
            D3DPOOL_MANAGED  , &vb, nullptr)))
        return false;
    *slot = vb;
    void* p = nullptr;
    if (FAILED(vb->Lock(0, len, &p, 0)))
        return false;
    std::memcpy(p, data, len);
    vb->Unlock();
    return true;
}

bool PutIB(IDirect3DDevice9* dev, IDirect3DIndexBuffer9** slot,
           unsigned len, const void* data) {
    IDirect3DIndexBuffer9* ib = nullptr;
    if (FAILED(dev->CreateIndexBuffer(
            len, 0, D3DFMT_INDEX16  , D3DPOOL_MANAGED  , &ib,
            nullptr)))
        return false;
    *slot = ib;
    void* p = nullptr;
    if (FAILED(ib->Lock(0, 0, &p, 0)))
        return false;
    std::memcpy(p, data, len);
    ib->Unlock();
    return true;
}

}  

bool SceneConstruct(PhysicsScene* scene, D3DRenderer* d3dSub) {
    scene->owner = d3dSub;                                   
    IDirect3DDevice9* dev = d3dSub->device;                  

    if (!PutVB(dev, &scene->gizmoSphereVB, 928, kGizmoSphere58))    
        return false;
    if (!PutIB(dev, &scene->gizmoSphereIB, 480, kGizmoSphereIdx))   
        return false;
    if (!PutVB(dev, &scene->gizmoCubeVB, 128, kGizmoCube8))         
        return false;
    if (!PutIB(dev, &scene->gizmoCubeIB, 48, kGizmoCubeIdx))        
        return false;
    if (!PutVB(dev, &scene->gizmoSphere33VB, 528, kGizmoSphere33))  
        return false;
    if (!PutIB(dev, &scene->gizmoSphere33IB, 256, kGizmoSphere33Idx))  
        return false;
    if (!PutVB(dev, &scene->gizmoArrowVB, 256, kGizmoArrow16))      
        return false;
    if (!PutIB(dev, &scene->gizmoIdentityIB, 32, kGizmoIdentityIdx))  
        return false;
    if (!PutVB(dev, &scene->gizmoBoxSelVB, 256, kGizmoBoxSel16))    
        return false;
    if (!PutIB(dev, &scene->gizmoBoxSelIB, 144, kGizmoBoxSelIdx))   
        return false;

    scene->collisionConfig = new btDefaultCollisionConfiguration();  
    scene->dispatcher =
        new btCollisionDispatcher(scene->collisionConfig);           
    scene->broadphase = new bt32BitAxisSweep3(                       
        btVector3(-10000.0f, -10000.0f, -10000.0f),
        btVector3(10000.0f, 10000.0f, 10000.0f), 1500000);
    scene->solver = new btSequentialImpulseConstraintSolver();       
    scene->world = new PhysicsWorld(                                
        scene->dispatcher, scene->broadphase, scene->solver,
        scene->collisionConfig);
    btDiscreteDynamicsWorld* world = scene->world;

    world->getSolverInfo().m_solverMode &= ~1;             
    world->setGravity(btVector3(
        0.0f, -98.0f, 0.0f));                              

    auto* plane = new btStaticPlaneShape(                  
        btVector3(0.0f, 1.0f, 0.0f), 0.0f);
    btDefaultMotionState* motionState =                    
        new btDefaultMotionState(btTransform::getIdentity(),
                                 btTransform::getIdentity());
    btVector3 zeroInertia(0.0f, 0.0f, 0.0f);
    btRigidBody::btRigidBodyConstructionInfo ci(           
        0.0f, motionState, plane, zeroInertia);

    if (scene->groundBody != nullptr) {                    
        delete scene->groundBody;
        scene->groundBody = nullptr;
    }
    scene->groundBody = new btRigidBody(ci);               
    world->addRigidBody(scene->groundBody,                 
                        static_cast<short>(0x8000), static_cast<short>(-1));
    scene->groundBody->setRestitution(0.88f);              
    return true;
}

}  
