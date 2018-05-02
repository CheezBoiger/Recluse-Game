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
  transform->Position = m_pRigidBody->m_vPosition - m_relOffset;
  transform->Rotation = m_pRigidBody->m_qRotation;
}


void PhysicsComponent::SetMass(r32 mass)
{
  m_mass = mass;
  m_pRigidBody->SetMass(mass);
}


void PhysicsComponent::SetTransform(const Vector3& newPos, const Quaternion& newRot)
{
  m_pRigidBody->SetTransform(newPos + m_relOffset, newRot);
}


void PhysicsComponent::OnEnable()
{
  Enabled() ? gPhysics().ActivateRigidBody(m_pRigidBody) : gPhysics().DeactivateRigidBody(m_pRigidBody); 
}


void PhysicsComponent::UpdateFromGameObject()
{
  Transform* transform = GetTransform();
  m_pRigidBody->m_vPosition = transform->Position + m_relOffset;
  m_pRigidBody->m_qRotation = transform->Rotation;
  gPhysics().SetTransform(m_pRigidBody, 
    m_pRigidBody->m_vPosition, 
    m_pRigidBody->m_qRotation);
}


void PhysicsComponent::SetRelativeOffset(const Vector3& offset)
{
  m_pRigidBody->m_vPosition = m_pRigidBody->m_vPosition - m_relOffset;
  m_relOffset = offset;
  m_pRigidBody->m_vPosition = m_pRigidBody->m_vPosition + m_relOffset;
  m_pRigidBody->SetTransform(m_pRigidBody->m_vPosition, m_pRigidBody->m_qRotation);
}


void PhysicsComponent::UpdateFromPreviousGameLogic()
{
  for (auto& component : _kPhysicsComponents) {
    component.second->UpdateFromGameObject();
  }
}
} // Recluse