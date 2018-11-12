// Copyright (c) 2018 Recluse Project. All rights reserved.

#include "PhysicsComponent.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "GameObject.hpp"
#include "Core/Exception.hpp"


namespace Recluse {

// Updating and offset solution provided by lehoo.
// https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=9546

DEFINE_COMPONENT_MAP(PhysicsComponent);


void PhysicsComponent::OnInitialize(GameObject* owner)
{
  if (!m_pRigidBody) m_pRigidBody = gPhysics().CreateRigidBody();
  m_pRigidBody->_gameObj = owner;

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
  transform->Rotation = m_pRigidBody->_rotation;
  //Vector3 finalOffset = transform->Rotation * m_relOffset;
  transform->Position = m_pRigidBody->_position; // - finalOffset;

  if (m_debug && m_pRigidBody->_collider) {
    Collider* pCol = m_pRigidBody->_collider;
    if (pCol->GetColliderType() == PHYSICS_COLLIDER_TYPE_COMPOUND) {
      CompoundCollider* compound = static_cast<CompoundCollider*>(pCol);
      auto colliders = compound->GetColliders();
      for (size_t i = 0; i < colliders.size(); ++i) {
        Collider* coll = colliders[i];
        coll->Update();
        BasicDebugRenderCmd rC = coll->GetRenderCmd();
        gRenderer().PushMeshRender(rC);
      }
    } else {
      m_pRigidBody->_collider->Update();
      BasicDebugRenderCmd rC = m_pRigidBody->_collider->GetRenderCmd();
      gRenderer().PushMeshRender(rC);
    }
  }
}


void PhysicsComponent::SetMass(r32 mass)
{
  m_pRigidBody->_mass = mass;
  m_updateBits |= PHYSICS_UPDATE_MASS;
}


void PhysicsComponent::SetTransform(const Vector3& newPos, const Quaternion& newRot)
{
  m_pRigidBody->_position = newPos;
  m_pRigidBody->_rotation = newRot;
  gPhysics().SetTransform(m_pRigidBody, newPos, newRot);
}


void PhysicsComponent::OnEnable()
{
  Enabled() ? gPhysics().ActivateRigidBody(m_pRigidBody) : gPhysics().DeactivateRigidBody(m_pRigidBody); 
}


void PhysicsComponent::UpdateFromGameObject()
{
  Transform* transform = GetTransform();
  m_pRigidBody->_rotation = transform->Rotation;
  //Vector3 finalOffset = transform->Rotation * m_relOffset;
  m_pRigidBody->_position = transform->Position; // + finalOffset;
  gPhysics().SetTransform(m_pRigidBody, 
    m_pRigidBody->_position, 
    m_pRigidBody->_rotation);
  if (m_updateBits & PHYSICS_UPDATE_ALL) 
    gPhysics().UpdateRigidBody(m_pRigidBody, m_updateBits);
  m_updateBits = PHYSICS_UPDATE_NONE;
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
  m_pRigidBody->_impulses.push_back(impulse);
  m_pRigidBody->_impulseRelativePositions.push_back(relPos);
  m_updateBits |= PHYSICS_UPDATE_IMPULSE;
}


void PhysicsComponent::UpdateFromPreviousGameLogic()
{
  for (auto& component : _kPhysicsComponents) {
    component.second->UpdateFromGameObject();
  }
}


void PhysicsComponent::ClearForces()
{
  m_updateBits |= PHYSICS_UPDATE_CLEAR_FORCES;
}


void PhysicsComponent::Reset()
{
  m_updateBits |= PHYSICS_UPDATE_RESET;
}


void PhysicsComponent::SetFriction(r32 friction)
{
  // gPhysics().SetFriction(m_pRigidBody, friction);
  m_pRigidBody->_friction = friction;
  m_updateBits |= PHYSICS_UPDATE_FRICTION;
}


void PhysicsComponent::SetRollingFriction(r32 friction)
{
  // gPhysics().SetRollingFriction(m_pRigidBody, friction);
  m_pRigidBody->_rollingFriction = friction;
  m_updateBits |= PHYSICS_UPDATE_ROLLING_FRICTION;
}


void PhysicsComponent::SetSpinningFriction(r32 friction)
{
  // gPhysics().SetSpinningFriction(m_pRigidBody, friction);
  m_pRigidBody->_spinningFriction = friction;
  m_updateBits |= PHYSICS_UPDATE_SPINNING_FRICTION;
}


void PhysicsComponent::AddCollider(Collider* coll)
{
  R_ASSERT(coll, "Collider was null.");
  gPhysics().AddCollider(m_pRigidBody, coll);
}


void PhysicsComponent::SetAngleFactor(const Vector3& angleFactor)
{
  m_pRigidBody->_angleFactor = angleFactor;
  m_updateBits |= PHYSICS_UPDATE_ANGLE_FACTOR;
}


void PhysicsComponent::SetLinearFactor(const Vector3& linearFactor)
{
  m_pRigidBody->_linearFactor = linearFactor;
  m_updateBits |= PHYSICS_UPDATE_LINEAR_FACTOR;
}


void PhysicsComponent::EnableDebug(b32 enable)
{
  m_debug = enable;
}
} // Recluse