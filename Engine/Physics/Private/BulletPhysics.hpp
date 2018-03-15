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
  BulletPhysics()
    : m_pWorld(nullptr) { }

  void        Initialize();
  void        CleanUp();

  void        Update(r64 dt);
  void        SetWorld(btDynamicsWorld* world) { m_pWorld = world; }

  btDynamicsWorld*    GetCurrentWorld() { return m_pWorld; }

  

private:
  btDynamicsWorld*        m_pWorld;
};


BulletPhysics& gBulletEngine();
} // Recluse