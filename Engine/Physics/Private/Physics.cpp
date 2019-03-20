// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Physics.hpp"

#include "BulletPhysics.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Core/Core.hpp"
#include "Renderer/Renderer.hpp"


#include "SphereCollider.hpp"
#include "BoxCollider.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/MeshDescriptor.hpp"

namespace Recluse {


Physics& gPhysics()
{
  return EngineModule<BulletPhysics>::instance();
}


void Physics::onStartUp()
{
  BoxCollider::InitMeshDebugData();
  SphereCollider::InitMeshDebugData();
}


void Physics::onShutDown()
{
  BoxCollider::CleanUpMeshDebugData();
  SphereCollider::CleanUpMeshDebugData();
}
} // Recluse