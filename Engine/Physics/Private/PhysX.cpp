// Copyright (c) 2017 Recluse Project.
#include "PhysX.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


b8 PhysX::Initialize()
{
  mFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, mDefaultAllocatorCallback, mDefaultErrorCallback);
  mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, mTolerance);

  if (!mPhysics || !mFoundation) return false;
  return true;
}


void PhysX::CleanUp()
{
  mPhysics->release();
  mFoundation->release();
}
} // Recluse