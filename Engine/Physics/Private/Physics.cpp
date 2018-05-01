// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Physics.hpp"

#include "BulletPhysics.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Core/Core.hpp"
#include "Renderer/Renderer.hpp"

#include "BulletPhysics.hpp"

namespace Recluse {


Physics& gPhysics()
{
  return Physics::Instance();
}

void Physics::OnStartUp()
{
  if (!gRenderer().IsActive()) {
    R_DEBUG(rWarning, "Renderer is not active! Physics will carry on however...\n");
  }

  gBulletEngine().Initialize();

  R_DEBUG(rNotify, "Physics Engine is successfully initialized.\n");
}


RigidBody* Physics::CreateRigidBody(Collider* collider)
{
#if !FORCE_PHYSICS_OFF
  return gBulletEngine().CreateRigidBody(Vector3(0.0f, 50.0f, 0.0f), collider);
#else
  return nullptr;
#endif
}


Collider* Physics::CreateBoxCollider(const Vector3& scale)
{
  return gBulletEngine().CreateBoxCollider(scale);
}


void Physics::OnShutDown()
{
  gBulletEngine().CleanUp();
}


void Physics::UpdateState(r64 dt)
{
  gBulletEngine().Update(dt);
}


void Physics::FreeRigidBody(RigidBody* body)
{
  gBulletEngine().FreeRigidBody(body);
}


void Physics::SetMass(RigidBody* body, r32 mass)
{
  gBulletEngine().SetMass(body, mass);
}


void Physics::SetPosition(RigidBody* body, const Vector3& newPos)
{
  gBulletEngine().SetPosition(body, newPos);
}


void Physics::ActivateRigidBody(RigidBody* body)
{
  gBulletEngine().ActivateRigidBody(body);
}


void Physics::DeactivateRigidBody(RigidBody* body)
{
  gBulletEngine().DeactivateRigidBody(body);
}
} // Recluse