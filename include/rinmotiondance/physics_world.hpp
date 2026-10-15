#pragma once

#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"

namespace rinmotiondance {

class PhysicsWorld : public btDiscreteDynamicsWorld {
public:
    using btDiscreteDynamicsWorld::btDiscreteDynamicsWorld;

#ifdef RINMOTIONDANCE_DIAG
    btScalar SubstepRemainder() const { return m_localTime; }
    void ClearSubstepRemainder() { m_localTime = btScalar(0); }
#endif
};

}  
