// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Physics.hpp"

#include "BulletPhysics.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Core/Core.hpp"
#include "Renderer/Renderer.hpp"


namespace Recluse {


Physics& gPhysics()
{
  return EngineModule<BulletPhysics>::Instance();
}
} // Recluse