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
class BoxCollider;
class SphereCollider;
class CompoundCollider;


struct RayTestHit {
  // Rigid body that was hit by the ray.
  RigidBody* _rigidbody;
  // Collider of the rigidbody hit by the ray.
  Collider*  _collider;
  
  Vector3   _normal;
};

// Our physics engine.
class Physics : public EngineModule<Physics> {
public:
  virtual ~Physics() { }
  Physics() { }

  virtual void                            OnStartUp() override { }
  virtual void                            OnShutDown() override { }


  virtual RigidBody*                      CreateRigidBody(const Vector3& centerOfMassOffset = Vector3(0.0f, 0.0f, 0.0f)) { return nullptr; }

  virtual BoxCollider*                    CreateBoxCollider(const Vector3& scale) { return nullptr; }
  virtual SphereCollider*                 CreateSphereCollider() { return nullptr; }
  virtual CompoundCollider*               CreateCompoundCollider() { return nullptr; }
  virtual void                            FreeRigidBody(RigidBody* body) { }


  virtual void                            FreeCollider(Collider* collider) { }
  virtual void                            SetWorldGravity(const Vector3& gravity) { }

  virtual void                            SetMass(RigidBody* body, r32 mass) { }
  virtual void                            ApplyImpulse(RigidBody* body, const Vector3& impulse, const Vector3& relPos) { }
  virtual void                            ApplyForce(RigidBody* body, const Vector3& force) { }
  virtual void                            ActivateRigidBody(RigidBody* body) { } 
  virtual void                            DeactivateRigidBody(RigidBody* body) { }
  virtual void                            SetTransform(RigidBody* body, const Vector3& pos, const Quaternion& rot) { }

  virtual void                            ClearForces(RigidBody* body) { }
  virtual void                            UpdateState(r64 dt, r64 fixedTime) { }
  virtual void                            AddCollider(RigidBody* body, Collider* collider) { }
  virtual b32                             RayTest(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHit* output) { return false; }
  virtual b32                             RayTestAll(const Vector3& origin, const Vector3& direction, const r32 maxDistance) { return false; }

private:
};


// Global physics.
Physics& gPhysics();
} // Recluse