// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"
#include "PhysicsConfigs.hpp"


#include <functional>


namespace Recluse {


typedef std::function<void()> OnCollisionCallback;


class Collider;
class GameObject;

enum ContactType {
  START_COLLISION,
  DURING_COLLISION,
  OFF_COLLISION
};


// RigidBody object create by Physics interface. Be sue to assign the proper
// game object id to this body, otherwise nullptr will be returned!
class RigidBody : public PhysicsObject {
public:
  RigidBody() 
    : onCollisionCallback(nullptr)
    , m_bActivated(true)
    , m_pCollider(nullptr)
    , m_gameObj(nullptr) { }

  void                  ObjectCollided();
  void                  EnableKinematic(b8 enable);
  void                  SetTransform(const Vector3& newPos, const Quaternion& newRot);
  void                  SetMass(r32 mass);

  b32                   Kinematic() const { return m_bKinematic; }
  b32                   Activated() const { return m_bActivated; }
  GameObject*           GetGameObject() const { return m_gameObj; }
  Collider*             GetCollider() { return m_pCollider; }

  OnCollisionCallback   onCollisionCallback;
  Collider*             m_pCollider;
  Vector3               m_vPosition;
  r32                   m_mass;
  Quaternion            m_qRotation;
  b32                   m_bKinematic;
  b32                   m_bActivated;
  GameObject*           m_gameObj;
};
} // Recluse