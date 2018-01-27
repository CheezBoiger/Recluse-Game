// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"
#include "Collision.hpp"

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "Core/Types.hpp"


namespace Recluse {


class BulletPhysics {
public:


  void        Initialize();
  void        CleanUp();

  void        Update(r64 dt);
private:
  
};
} // Recluse