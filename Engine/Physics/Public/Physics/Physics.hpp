// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Time.hpp"
#include "Core/Utility/Vector.hpp"

#include "PhysicsConfigs.hpp"

#include "Collider.hpp"
#include "Collision.hpp"
#include "RigidBody.hpp"


namespace Recluse {


class Actor;
class BulletPhysics;
class BoxCollider;
class SphereCollider;
class CompoundCollider;


// Ray test output that results in the closest rigidbody hit by the ray.
struct RayTestHit {
  // Rigid body that was hit by the ray.
  RigidBody* _rigidbody;
  // Collider of the rigidbody hit by the ray.
  Collider*  _collider;
  
  Vector3   _normal;
};


// Ray test output that results in all rigidbodies hit by the ray.
struct RayTestHitAll {
  std::vector<RigidBody*> _rigidBodies;
  std::vector<Collider*>  _colliders;
  std::vector<Vector3>    _normals;
};


enum PhysicsUpdateBits {
  PHYSICS_UPDATE_ALL = 0x7fffffff,
  PHYSICS_UPDATE_CLEAR_ALL = 0xffffffff,
  PHYSICS_UPDATE_NONE = 0x0,
  PHYSICS_UPDATE_FORCES = (1 << 0),
  PHYSICS_UPDATE_LINEAR_VELOCITY = (1 << 1),
  PHYSICS_UPDATE_ANGULAR_VELOCITY = (1 << 2),
  PHYSICS_UPDATE_FRICTION = (1 << 3),
  PHYSICS_UPDATE_ROLLING_FRICTION = (1 << 4),
  PHYSICS_UPDATE_SPINNING_FRICTION = (1 << 5),
  PHYSICS_UPDATE_ACTIVATE = (1 << 6),
  PHYSICS_UPDATE_DEACTIVATE = (1 << 7),
  PHYSICS_UPDATE_IMPULSE = (1 << 8),
  PHYSICS_UPDATE_GRAVITY = (1 << 9),
  PHYSICS_UPDATE_RESET = (1 << 10),
  PHYSICS_UPDATE_MASS = (1 << 11),
  PHYSICS_UPDATE_CENTER_OF_MASS = (1 << 12),
  PHYSICS_UPDATE_CLEAR_FORCES = (1 << 13),
  PHYSICS_UPDATE_ANGLE_FACTOR = (1 << 14),
  PHYSICS_UPDATE_LINEAR_FACTOR = (1 << 15)
};


typedef u32 physics_update_bits_t;

// Our physics engine interface.
class Physics : public EngineModule<Physics> {
public:
  virtual ~Physics() { }
  Physics() { }

  virtual void                            OnStartUp() override;
  virtual void                            OnShutDown() override;


  virtual RigidBody*                      CreateRigidBody(const Vector3& centerOfMassOffset = Vector3(0.0f, 0.0f, 0.0f)) { return nullptr; }

  virtual BoxCollider*                    CreateBoxCollider(const Vector3& scale) { return nullptr; }
  virtual SphereCollider*                 CreateSphereCollider(r32 radius) { return nullptr; }
  virtual CompoundCollider*               CreateCompoundCollider() { return nullptr; }
  virtual void                            UpdateCompoundCollider(RigidBody* body, CompoundCollider* compound) { }
  virtual void                            FreeRigidBody(RigidBody* body) { }

  virtual void                            Reset(RigidBody* body) { }
  virtual void                            FreeCollider(Collider* collider) { }
  virtual void                            UpdateRigidBody(RigidBody* body, physics_update_bits_t bits) { }
  virtual void                            SetWorldGravity(const Vector3& gravity) { }

  virtual void                            SetMass(RigidBody* body, r32 mass) { }
  virtual void                            ApplyImpulse(RigidBody* body, const Vector3& impulse, const Vector3& relPos) { }
  virtual void                            ApplyForce(RigidBody* body, const Vector3& force) { }
  virtual void                            ActivateRigidBody(RigidBody* body) { } 
  virtual void                            DeactivateRigidBody(RigidBody* body) { }
  virtual void                            SetTransform(RigidBody* body, const Vector3& pos, const Quaternion& rot) { }

  virtual void                            SetFriction(RigidBody* body, r32 friction) { }
  virtual void                            SetRollingFriction(RigidBody* body, r32 friction) { }
  virtual void                            SetSpinningFriction(RigidBody* body, r32 friction) { }
  virtual void                            ClearForces(RigidBody* body) { }
  virtual void                            UpdateState(r64 dt, r64 fixedTime) { }
  virtual void                            AddCollider(RigidBody* body, Collider* collider) { }
  virtual b32                             RayTest(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHit* output) { return false; }
  virtual b32                             RayTestAll(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHitAll* output) { return false; }
  virtual void                            UpdateCollider(Collider* collider) { }
private:
};


// Global physics.
Physics& gPhysics();
} // Recluse