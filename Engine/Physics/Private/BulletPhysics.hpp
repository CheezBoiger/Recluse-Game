// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"
#include "Collision.hpp"
#include "Physics.hpp"

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"


namespace Recluse {


class RigidBody;
class Collider;
class CollisionShape;


class BulletPhysics : public Physics, public EngineModule<BulletPhysics> {
public:
  BulletPhysics()
    : m_pWorld(nullptr) { }

  void                  Initialize();
  void                  CleanUp();

  void                  UpdateState(r64 dt, r64 fixedTime) override;
  void                  SetWorld(btDynamicsWorld* world) { m_pWorld = world; }

  void                  OnStartUp() override;
  void                  OnShutDown() override;

  btDynamicsWorld*      GetCurrentWorld() { return m_pWorld; }

  void                  ClearWorld() { }
  RigidBody*            CreateRigidBody(Collider* shape, const Vector3& centerOfMassOffset = Vector3(0.0f, 0.0f, 0.0)) override;
  void                  FreeRigidBody(RigidBody* body) override;
  Collider*             CreateBoxCollider(const Vector3& scale) override;
  Collider*             CreateSphereCollider() override { return nullptr; }

  void                  ApplyImpulse(RigidBody* body, const Vector3& impulse, const Vector3& relPos) override;

  void                  FreeCollider(Collider* collider) override { }
  void                  SetMass(RigidBody* body, r32 mass) override;
  void                  ActivateRigidBody(RigidBody* body) override;
  void                  DeactivateRigidBody(RigidBody* body) override;

  void                  SetWorldGravity(const Vector3& gravity) override;

  void                  SetTransform(RigidBody* body, const Vector3& pos, const Quaternion& rot) override;
private:
  btDynamicsWorld*        m_pWorld;
};
} // Recluse