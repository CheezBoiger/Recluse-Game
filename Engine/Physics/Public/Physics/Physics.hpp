// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Time.hpp"
#include "Core/Utility/Vector.hpp"

#include "PhysicsConfigs.hpp"

#include "Collider.hpp"
#include "RigidBody.hpp"


namespace Recluse {


class Actor;
class BulletPhysics;

// Our physics engine.
class Physics : public EngineModule<Physics> {
public:
  Physics() { }

  void                            OnStartUp() override;
  void                            OnShutDown() override;


  virtual RigidBody*              CreateRigidBody(Collider* collider);
  virtual Collider*               CreateBoxCollider(const Vector3& scale);
  void                            FreeRigidBody(RigidBody* body);

  void                            SetMass(RigidBody* body, r32 mass);
  void                            SetPosition(RigidBody* body, const Vector3& newPos);

  void                            UpdateState(r64 dt);
private:
};


// Global physics.
Physics& gPhysics();
} // Recluse