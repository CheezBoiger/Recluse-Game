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


struct RigidBody;
class Collider;
class CollisionShape;


class BulletPhysics : public Physics, public EngineModule<BulletPhysics> {
public:
  BulletPhysics()
    : m_pWorld(nullptr) { }

  void                  initialize();
  void                  cleanUp();

  void                  updateState(r64 dt, r64 fixedTime) override;
  void                  SetWorld(btDynamicsWorld* world) { m_pWorld = world; }

  void                  onStartUp() override;
  void                  onShutDown() override;

  btDynamicsWorld*      GetCurrentWorld() { return m_pWorld; }

  void                  ClearWorld() { }
  void                  reset(RigidBody* body) override;
  RigidBody*            createRigidBody(const Vector3& centerOfMassOffset = Vector3(0.0f, 0.0f, 0.0)) override;
  void                  freeRigidBody(RigidBody* body) override;
  BoxCollider*          createBoxCollider(const Vector3& scale) override;
  SphereCollider*       createSphereCollider(r32 radius) override;
  CompoundCollider*     createCompoundCollider() override;
  void                  updateCompoundCollider(RigidBody* body, CompoundCollider* compound) override;
  void                  updateRigidBody(RigidBody* body, physics_update_bits_t bits) override;

  void                  applyImpulse(RigidBody* body, const Vector3& impulse, const Vector3& relPos) override;

  void                  freeCollider(Collider* collider) override;
  void                  setMass(RigidBody* body, r32 mass) override;
  void                  activateRigidBody(RigidBody* body) override;
  void                  deactivateRigidBody(RigidBody* body) override;
  void                  setFriction(RigidBody* body, r32 friction) override;
  void                  setRollingFriction(RigidBody* body, r32 friction) override;
  void                  setSpinningFriction(RigidBody* body, r32 friction) override;

  void                  setWorldGravity(const Vector3& gravity) override;
  void                  clearForces(RigidBody* body) override;
  void                  addCollider(RigidBody* body, Collider* collider) override;
  void                  invokeCollisions(RigidBody* pRigidBody, Collision* pCollision);
  void                  updateCollisions();

  void                  setTransform(RigidBody* body, const Vector3& pos, const Quaternion& rot) override;
  b32                   rayTest(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHit* output) override;
  b32                   rayTestAll(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHitAll* output) override;
private:
  btDynamicsWorld*        m_pWorld;
};
} // Recluse