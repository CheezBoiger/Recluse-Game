// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "BulletPhysics.hpp"
#include "Core/Exception.hpp"
#include "Core/Utility/Time.hpp"


namespace Recluse {


// Global physics manager that holds physics contraint solvers, dispatchers, configuration
// and management.
struct bt_physics_manager 
{
  // 
  btDefaultCollisionConfiguration*      _pCollisionConfiguration;

  // 
  btCollisionDispatcher*                _pDispatcher;


  btBroadphaseInterface*                _pOverlappingPairCache;

  // Interchangeable constraint solver. We will use Sequential Impulse, as it is
  // popular. We can use Projected Gauss-Seidel for experimentation later... 
  btSequentialImpulseConstraintSolver*  _pSolver;

} bt_manager;


BulletPhysics& gBulletEngine()
{
  static BulletPhysics bullet;
  return bullet;
}



void BulletPhysics::Initialize()
{
  bt_manager._pCollisionConfiguration = new btDefaultCollisionConfiguration();
  bt_manager._pDispatcher = new btCollisionDispatcher(bt_manager._pCollisionConfiguration);
  bt_manager._pOverlappingPairCache = new btDbvtBroadphase();
  bt_manager._pSolver = new btSequentialImpulseConstraintSolver();
  R_DEBUG(rNotify, "Bullet Sdk initialized.\n");
}


void BulletPhysics::CleanUp()
{
  delete bt_manager._pSolver;
  delete bt_manager._pOverlappingPairCache;
  delete bt_manager._pDispatcher;
  delete bt_manager._pCollisionConfiguration;
  R_DEBUG(rNotify, "Bullet Sdk cleaned up.\n");
}


void BulletPhysics::Update(r64 dt)
{
  if (!m_pWorld) { return; }

  m_pWorld->stepSimulation(btScalar(dt), 10, btScalar(Time::FixTime));
}
} // Recluse