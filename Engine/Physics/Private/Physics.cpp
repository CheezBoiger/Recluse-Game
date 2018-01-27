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

  m_pPhysics = new BulletPhysics();
  m_pPhysics->Initialize();

  R_DEBUG(rNotify, "PhysX Device is successfully initialized.\n");
}


void Physics::OnShutDown()
{
  m_pPhysics->CleanUp();
  delete m_pPhysics;
  m_pPhysics = nullptr;
}


void Physics::UpdateState(r64 dt)
{
  m_pPhysics->Update(dt);
}
} // Recluse