// Copyright (c) 2018 Recluse Project. All rights reserved.

#include "PhysicsComponent.hpp"
#include "GameObject.hpp"
#include "Core/Exception.hpp"


namespace Recluse {

// Updating and offset solution provided by lehoo.
// https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=9546

DEFINE_COMPONENT_MAP(PhysicsComponent);


void PhysicsComponent::OnInitialize(GameObject* owner)
{
  if (!m_pRigidBody) m_pRigidBody = gPhysics().CreateRigidBody();
  m_pRigidBody->m_gameObj = owner;

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
  transform->Rotation = m_pRigidBody->m_qRotation;
  //Vector3 finalOffset = transform->Rotation * m_relOffset;
  transform->Position = m_pRigidBody->m_vPosition; // - finalOffset;
}


void PhysicsComponent::SetMass(r32 mass)
{
  m_mass = mass;
  m_pRigidBody->SetMass(mass);
}


void PhysicsComponent::SetTransform(const Vector3& newPos, const Quaternion& newRot)
{
  m_pRigidBody->SetTransform(newPos, newRot);
}


void PhysicsComponent::OnEnable()
{
  Enabled() ? gPhysics().ActivateRigidBody(m_pRigidBody) : gPhysics().DeactivateRigidBody(m_pRigidBody); 
}


void PhysicsComponent::UpdateFromGameObject()
{
  Transform* transform = GetTransform();
  m_pRigidBody->m_qRotation = transform->Rotation;
  //Vector3 finalOffset = transform->Rotation * m_relOffset;
  m_pRigidBody->m_vPosition = transform->Position; // + finalOffset;
  gPhysics().SetTransform(m_pRigidBody, 
    m_pRigidBody->m_vPosition, 
    m_pRigidBody->m_qRotation);
}


/*
void PhysicsComponent::SetRelativeOffset(const Vector3& offset)
{
  if (!m_pRigidBody) return;
  m_relOffset = offset;

}
*/

void PhysicsComponent::ApplyImpulse(const Vector3& impulse, const Vector3& relPos)
{
  gPhysics().ApplyImpulse(m_pRigidBody, impulse, relPos);
}


void PhysicsComponent::UpdateFromPreviousGameLogic()
{
  for (auto& component : _kPhysicsComponents) {
    component.second->UpdateFromGameObject();
  }
}


void PhysicsComponent::ClearForces()
{
  gPhysics().ClearForces(m_pRigidBody);
}
} // Recluse