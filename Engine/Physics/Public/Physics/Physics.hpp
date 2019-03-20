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

  virtual void                            onStartUp() override;
  virtual void                            onShutDown() override;


  virtual RigidBody*                      createRigidBody(const Vector3& centerOfMassOffset = Vector3(0.0f, 0.0f, 0.0f)) { return nullptr; }

  virtual BoxCollider*                    createBoxCollider(const Vector3& scale) { return nullptr; }
  virtual SphereCollider*                 createSphereCollider(r32 radius) { return nullptr; }
  virtual CompoundCollider*               createCompoundCollider() { return nullptr; }
  virtual void                            updateCompoundCollider(RigidBody* body, CompoundCollider* compound) { }
  virtual void                            freeRigidBody(RigidBody* body) { }

  virtual void                            reset(RigidBody* body) { }
  virtual void                            freeCollider(Collider* collider) { }
  virtual void                            updateRigidBody(RigidBody* body, physics_update_bits_t bits) { }
  virtual void                            SetWorldGravity(const Vector3& gravity) { }

  virtual void                            setMass(RigidBody* body, r32 mass) { }
  virtual void                            applyImpulse(RigidBody* body, const Vector3& impulse, const Vector3& relPos) { }
  virtual void                            applyForce(RigidBody* body, const Vector3& force) { }
  virtual void                            activateRigidBody(RigidBody* body) { } 
  virtual void                            deactivateRigidBody(RigidBody* body) { }
  virtual void                            setTransform(RigidBody* body, const Vector3& pos, const Quaternion& rot) { }

  virtual void                            setFriction(RigidBody* body, r32 friction) { }
  virtual void                            setRollingFriction(RigidBody* body, r32 friction) { }
  virtual void                            setSpinningFriction(RigidBody* body, r32 friction) { }
  virtual void                            clearForces(RigidBody* body) { }
  virtual void                            updateState(r64 dt, r64 fixedTime) { }
  virtual void                            addCollider(RigidBody* body, Collider* collider) { }
  virtual b32                             rayTest(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHit* output) { return false; }
  virtual b32                             rayTestAll(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHitAll* output) { return false; }
  virtual void                            updateCollider(Collider* collider) { }
private:
};


// Global physics.
Physics& gPhysics();
} // Recluse