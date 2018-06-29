// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"
#include "Collision.hpp"
#include "Physics.hpp"

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
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
  RigidBody*            CreateRigidBody(const Vector3& centerOfMassOffset = Vector3(0.0f, 0.0f, 0.0)) override;
  void                  FreeRigidBody(RigidBody* body) override;
  BoxCollider*          CreateBoxCollider(const Vector3& scale) override;
  SphereCollider*       CreateSphereCollider(r32 radius) override;
  CompoundCollider*     CreateCompoundCollider() override;

  void                  ApplyImpulse(RigidBody* body, const Vector3& impulse, const Vector3& relPos) override;

  void                  FreeCollider(Collider* collider) override;
  void                  SetMass(RigidBody* body, r32 mass) override;
  void                  ActivateRigidBody(RigidBody* body) override;
  void                  DeactivateRigidBody(RigidBody* body) override;

  void                  SetWorldGravity(const Vector3& gravity) override;
  void                  ClearForces(RigidBody* body) override;
  void                  AddCollider(RigidBody* body, Collider* collider) override;

  void                  SetTransform(RigidBody* body, const Vector3& pos, const Quaternion& rot) override;
  b32                   RayTest(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHit* output) override;
  b32                   RayTestAll(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHitAll* output) override;
private:
  btDynamicsWorld*        m_pWorld;
};
} // Recluse