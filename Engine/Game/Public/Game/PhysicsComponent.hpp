// Copyright (c) 2018, Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Component.hpp"

#include "Physics/Physics.hpp"


namespace Recluse {


class GameObject;
class Collider;
class RigidBody;


class PhysicsComponent : public Component {
  RCOMPONENT(PhysicsComponent);
public:
  static void UpdateFromPreviousGameLogic();

  PhysicsComponent() 
    : m_pRigidBody(nullptr)
    , m_pCollider(nullptr) { }

  void Update() override;
  virtual void OnInitialize(GameObject* owner) override;  
  virtual void OnCleanUp() override;
  void OnEnable() override;

  void SetCollider(Collider* collider) { m_pCollider = collider; }
  void SetTransform(const Vector3& newPos, const Quaternion& newRot);
  void SetMass(r32 mass);
  void SetRelativeOffset(const Vector3& offset);
  r32 GetMass() const { return m_mass; }
  void UpdateFromGameObject();


  Collider*   GetCollider() { return m_pCollider; }

  RigidBody* m_pRigidBody;
  Collider*  m_pCollider;
private:
  Vector3     m_relOffset;
  r32         m_mass;
};
} // Recluse