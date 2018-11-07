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


// NOTE(): Be sure to initialize Physics Component first, before calling any other function
// within.
class PhysicsComponent : public Component {
  RCOMPONENT(PhysicsComponent);
public:
  static void     UpdateFromPreviousGameLogic();

  PhysicsComponent() 
    : m_pRigidBody(nullptr) { }

protected:
  void            Update() override;
  virtual void    OnInitialize(GameObject* owner) override;  
  virtual void    OnCleanUp() override;
public:

  void            OnEnable() override;
  void            AddCollider(Collider* collider);
  void            SetMass(r32 mass);
  //void SetRelativeOffset(const Vector3& offset);
  void            ApplyForce(const Vector3& force);
  void            SetGravity(const Vector3& gravity);
  void            ClearForces();

  // Reset this physics body. Clears existing velocity and forces applied to this object.
  void            Reset();

  void            ApplyImpulse(const Vector3& impulse, const Vector3& relPos);
  void            SetCenterOfMass(const Vector3& centerOfMass);
  Vector3         GetCenterOfMassPosition();

  r32             GetMass() const { return m_pRigidBody->_mass; }
  void            UpdateFromGameObject();
  void            SetFriction(r32 friction);
  void            SetRollingFriction(r32 friction);
  void            SetSpinningFriction(r32 friction);
  
  // Rotation axis that can be influenced by outside forces. 1.0 to enable, 0.0 to disable.
  void            SetAngleFactor(const Vector3& angleFactor);

  // Transform axis that can be influeced by outside forces. 1.0 to enable, 0.0 to disable.
  void            SetLinearFactor(const Vector3& linearFactor);

  RigidBody*      GetRigidBody() { return m_pRigidBody; }

private:

  void            SetTransform(const Vector3& newPos, const Quaternion& newRot);

  RigidBody*              m_pRigidBody;
  physics_update_bits_t   m_updateBits;
};
} // Recluse