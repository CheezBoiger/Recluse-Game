// Copyright (c) 2018, Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Component.hpp"

#include "Physics/Physics.hpp"


namespace Recluse {


class GameObject;
class Collider;
struct RigidBody;
class MeshDescriptor;


// NOTE(): Be sure to initialize Physics Component first, before calling any other function
// within.
class PhysicsComponent : public Component {
  RCOMPONENT(PhysicsComponent);
public:
  static void     UpdateFromPreviousGameLogic();

  PhysicsComponent() 
    : m_pRigidBody(nullptr) { }

protected:
  void            update() override;
  virtual void    onInitialize(GameObject* owner) override;  
  virtual void    onCleanUp() override;
public:

  void            onEnable() override;
  void            addCollider(Collider* collider);
  void            setMass(R32 mass);
  //void SetRelativeOffset(const Vector3& offset);
  void            applyForce(const Vector3& force);
  void            setGravity(const Vector3& gravity);
  void            setLinearVelocity(const Vector3& velocity);
  void            clearForces();

  // Reset this physics body. Clears existing velocity and forces applied to this object.
  void            reset();

  void            applyImpulse(const Vector3& impulse, const Vector3& relPos);
  void            setCenterOfMass(const Vector3& centerOfMass);
  Vector3         getCenterOfMassPosition();

  R32             getMass() const { return m_pRigidBody->_mass; }
  void            updateFromGameObject();
  void            setFriction(R32 friction);
  void            setRollingFriction(R32 friction);
  void            setSpinningFriction(R32 friction);
  
  // Rotation axis that can be influenced by outside forces. 1.0 to enable, 0.0 to disable.
  void            setAngleFactor(const Vector3& angleFactor);

  // Transform axis that can be influeced by outside forces. 1.0 to enable, 0.0 to disable.
  void            setLinearFactor(const Vector3& linearFactor);

  RigidBody*      getRigidBody() { return m_pRigidBody; }

  void            enableDebug(B32 enable);

private:

  void            setTransform(const Vector3& newPos, const Quaternion& newRot);

  RigidBody*              m_pRigidBody;
  physics_update_bits_t   m_updateBits;
  B32                     m_debug;
};
} // Recluse