// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "RigidBody.hpp"
#include "Core/Exception.hpp"
#include "Collision.hpp"
#include "Game/GameObject.hpp"
#include "Physics.hpp"


namespace Recluse {


uuid64 PhysicsObject::genIdx = 0;



void RigidBody::SetMass(r32 mass)
{
  m_mass = mass;
  gPhysics().SetMass(this, mass);
}


void RigidBody::SetTransform(const Vector3& newPos, const Quaternion& newRot) 
{
  gPhysics().SetTransform(this, newPos, newRot);
}


void RigidBody::AddCollider(Collider* collider) 
{
  if (!collider) return;
  gPhysics().AddCollider(this, collider);
}


void RigidBody::InvokeCollision(Collision* collision)
{
  R_ASSERT(m_gameObj, "NULL game object assigned to this rigid body!");
  m_gameObj->DispatchCollisionEvent(collision);
}
} // Recluse 