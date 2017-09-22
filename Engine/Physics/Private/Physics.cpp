// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Physics.hpp"

#include "PhysX.hpp"
#include "Core/Exception.hpp"
#include "Core/Core.hpp"
#include "Renderer/Renderer.hpp"

namespace Recluse {


Physics& gPhysics()
{
  return Physics::Instance();
}

void Physics::OnStartUp()
{
  if (!gRenderer().IsActive()) {
    R_DEBUG("WARNING: Renderer is not active! Physics will carry on however...\n");
  }
  mDevice = new PhysX();
  if (!mDevice->Initialize()) {
    R_DEBUG("ERROR: Failed to initialize PhysX device!\n");
    return;
  }

  R_DEBUG("NOTIFY: PhysX Device is successfully initialized.\n");
}


void Physics::OnShutDown()
{
  mDevice->CleanUp();
  delete mDevice;
  mDevice = nullptr;
}


void Physics::UpdateState(r64 fixTime)
{
  
}
} // Recluse