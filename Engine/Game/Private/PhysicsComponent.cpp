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


void PhysicsComponent::onInitialize(GameObject* owner)
{
  if (!m_pRigidBody) m_pRigidBody = gPhysics().createRigidBody();
  m_pRigidBody->_gameObj = owner;

  REGISTER_COMPONENT(PhysicsComponent, this);
}


void PhysicsComponent::onCleanUp()
{
  if (m_pRigidBody) { gPhysics().freeRigidBody(m_pRigidBody); }
  UNREGISTER_COMPONENT(PhysicsComponent);
}

void PhysicsComponent::update()
{
  R_ASSERT(m_pRigidBody, "No rigidbody assigned to this physics component.");
  Transform* transform = getOwner()->getTransform();
  transform->_rotation = m_pRigidBody->_rotation;
  //Vector3 finalOffset = transform->_rotation * m_relOffset;
  transform->_position = m_pRigidBody->_position; // - finalOffset;

  if (m_debug && m_pRigidBody->_collider) {
    Collider* pCol = m_pRigidBody->_collider;
    if (pCol->GetColliderType() == PHYSICS_COLLIDER_TYPE_COMPOUND) {
      CompoundCollider* compound = static_cast<CompoundCollider*>(pCol);
      auto colliders = compound->GetColliders();
      for (auto pCol : colliders) {
        Collider* coll = pCol;
        coll->update();
        BasicDebugRenderCmd rC = coll->getRenderCmd();
        gRenderer().pushMeshRender(rC);
      }
    } else {
      m_pRigidBody->_collider->update();
      BasicDebugRenderCmd rC = m_pRigidBody->_collider->getRenderCmd();
      gRenderer().pushMeshRender(rC);
    }
  }
}


void PhysicsComponent::setMass(r32 mass)
{
  m_pRigidBody->_mass = mass;
  m_updateBits |= PHYSICS_UPDATE_MASS;
}


void PhysicsComponent::setTransform(const Vector3& newPos, const Quaternion& newRot)
{
  m_pRigidBody->_position = newPos;
  m_pRigidBody->_rotation = newRot;
  gPhysics().setTransform(m_pRigidBody, newPos, newRot);
}


void PhysicsComponent::onEnable()
{
  enabled() ? gPhysics().activateRigidBody(m_pRigidBody) : gPhysics().deactivateRigidBody(m_pRigidBody); 
}


void PhysicsComponent::updateFromGameObject()
{
  Transform* transform = getTransform();
  m_pRigidBody->_rotation = transform->_rotation;
  //Vector3 finalOffset = transform->_rotation * m_relOffset;
  m_pRigidBody->_position = transform->_position; // + finalOffset;
  gPhysics().setTransform(m_pRigidBody, 
    m_pRigidBody->_position, 
    m_pRigidBody->_rotation);
  if (m_updateBits & PHYSICS_UPDATE_ALL)
    gPhysics().updateRigidBody(m_pRigidBody, m_updateBits);
  m_updateBits = PHYSICS_UPDATE_NONE;
}


/*
void PhysicsComponent::SetRelativeOffset(const Vector3& offset)
{
  if (!m_pRigidBody) return;
  m_relOffset = offset;

}
*/

void PhysicsComponent::applyImpulse(const Vector3& impulse, const Vector3& relPos)
{
  m_pRigidBody->_impulses.push_back(impulse);
  m_pRigidBody->_impulseRelativePositions.push_back(relPos);
  m_updateBits |= PHYSICS_UPDATE_IMPULSE;
}


void PhysicsComponent::UpdateFromPreviousGameLogic()
{
  for (auto& component : _kPhysicsComponents) {
    component.second->updateFromGameObject();
  }
}


void PhysicsComponent::clearForces()
{
  m_updateBits |= PHYSICS_UPDATE_CLEAR_FORCES;
}


void PhysicsComponent::reset()
{
  m_updateBits |= PHYSICS_UPDATE_RESET;
}


void PhysicsComponent::setFriction(r32 friction)
{
  // gPhysics().setFriction(m_pRigidBody, friction);
  m_pRigidBody->_friction = friction;
  m_updateBits |= PHYSICS_UPDATE_FRICTION;
}


void PhysicsComponent::setRollingFriction(r32 friction)
{
  // gPhysics().setRollingFriction(m_pRigidBody, friction);
  m_pRigidBody->_rollingFriction = friction;
  m_updateBits |= PHYSICS_UPDATE_ROLLING_FRICTION;
}


void PhysicsComponent::setSpinningFriction(r32 friction)
{
  // gPhysics().setSpinningFriction(m_pRigidBody, friction);
  m_pRigidBody->_spinningFriction = friction;
  m_updateBits |= PHYSICS_UPDATE_SPINNING_FRICTION;
}


void PhysicsComponent::setLinearVelocity(const Vector3& velocity)
{
  m_pRigidBody->_desiredVelocity = velocity;
  m_updateBits |= PHYSICS_UPDATE_LINEAR_VELOCITY;
}


void PhysicsComponent::addCollider(Collider* coll)
{
  R_ASSERT(coll, "Collider was null.");
  gPhysics().addCollider(m_pRigidBody, coll);
}


void PhysicsComponent::setAngleFactor(const Vector3& angleFactor)
{
  m_pRigidBody->_angleFactor = angleFactor;
  m_updateBits |= PHYSICS_UPDATE_ANGLE_FACTOR;
}


void PhysicsComponent::setLinearFactor(const Vector3& linearFactor)
{
  m_pRigidBody->_linearFactor = linearFactor;
  m_updateBits |= PHYSICS_UPDATE_LINEAR_FACTOR;
}


void PhysicsComponent::enableDebug(b32 enable)
{
  m_debug = enable;
}
} // Recluse