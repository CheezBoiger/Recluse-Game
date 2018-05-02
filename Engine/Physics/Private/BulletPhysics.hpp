// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"
#include "Collision.hpp"

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"


namespace Recluse {


class RigidBody;
class Collider;
class CollisionShape;


class BulletPhysics {
public:
  BulletPhysics()
    : m_pWorld(nullptr) { }

  void        Initialize();
  void        CleanUp();

  void        Update(r64 dt);
  void        SetWorld(btDynamicsWorld* world) { m_pWorld = world; }

  btDynamicsWorld*    GetCurrentWorld() { return m_pWorld; }

  void                  ClearWorld();

  RigidBody*            CreateRigidBody(const Vector3& centerOfMassOffset, Collider* shape);
  void                  FreeRigidBody(RigidBody* body);
  Collider*             CreateBoxCollider(const Vector3& scale);
  Collider*             CreateSphereCollider();

  void                  FreeCollider(Collider* collider);
  void                  SetMass(RigidBody* body, r32 mass);
  void                  ActivateRigidBody(RigidBody* body);
  void                  DeactivateRigidBody(RigidBody* body);

  void                  SetWorldGravity(const Vector3& gravity);

  void                  SetTransform(RigidBody* body, const Vector3& pos, const Quaternion& rot);
private:
  btDynamicsWorld*        m_pWorld;
};


BulletPhysics& gBulletEngine();
} // Recluse