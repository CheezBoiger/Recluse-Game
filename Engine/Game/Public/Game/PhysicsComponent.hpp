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
  PhysicsComponent() 
    : m_pRigidBody(nullptr)
    , m_pCollider(nullptr) { }

  void Update() override;
  virtual void OnInitialize(GameObject* owner) override;  
  virtual void OnCleanUp() override;

  void SetCollider(Collider* collider) { m_pCollider = collider; }
  void SetPosition(const Vector3& newPos);
  void SetMass(r32 mass);
  r32 GetMass() const { return m_mass; }

  Collider*   GetCollider() { return m_pCollider; }

  RigidBody* m_pRigidBody;
  Collider*  m_pCollider;
private:
  r32         m_mass;
};
} // Recluse