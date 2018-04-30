// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "RigidBody.hpp"
#include "Physics.hpp"


namespace Recluse {


uuid64 PhysicsObject::genIdx = 0;



void RigidBody::SetMass(r32 mass)
{
  m_mass = mass;
  gPhysics().SetMass(this, mass);
}


void RigidBody::SetPosition(const Vector3& newPos) 
{
  gPhysics().SetPosition(this, newPos);
}
} // Recluse 