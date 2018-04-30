// Copyright (c) 2018 Recluse Project. All rights reserved.

#include "PhysicsComponent.hpp"
#include "GameObject.hpp"
#include "Core/Exception.hpp"


namespace Recluse {



DEFINE_COMPONENT_MAP(PhysicsComponent);


void PhysicsComponent::OnInitialize(GameObject* owner)
{
  if (!m_pRigidBody) m_pRigidBody = gPhysics().CreateRigidBody(m_pCollider);
  REGISTER_COMPONENT(PhysicsComponent, this);
}


void PhysicsComponent::OnCleanUp()
{
  if (m_pRigidBody) { gPhysics().FreeRigidBody(m_pRigidBody); }
  UNREGISTER_COMPONENT(PhysicsComponent);
}

void PhysicsComponent::Update()
{
  R_ASSERT(m_pRigidBody, "No rigidbody assigned to this physics component.");
  Transform* transform = GetOwner()->GetTransform();
  transform->Position = m_pRigidBody->m_vPosition;
  transform->Rotation = transform->Rotation * m_pRigidBody->m_qRotation;
}


void PhysicsComponent::SetMass(r32 mass)
{
  m_mass = mass;
  m_pRigidBody->SetMass(mass);
}


void PhysicsComponent::SetPosition(const Vector3& newPos)
{
  m_pRigidBody->SetPosition(newPos);
}
} // Recluse