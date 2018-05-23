// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"
#include "PhysicsConfigs.hpp"

#include "Collider.hpp"
#include "CompoundCollider.hpp"

#include <functional>


namespace Recluse {



class Collider;
struct Collision;
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
    : m_bActivated(true)
    , m_gameObj(nullptr)
    , m_mass(1.0f)
    , m_friction(0.0f) { }

  void                  ObjectCollided();
  void                  EnableKinematic(b8 enable);
  void                  SetTransform(const Vector3& newPos, const Quaternion& newRot);
  void                  SetMass(r32 mass);
  void                  SetFriction(r32 friction);

  b32                   Kinematic() const { return m_bKinematic; }
  b32                   Activated() const { return m_bActivated; }
  GameObject*           GetGameObject() const { return m_gameObj; }
  CompoundCollider*     GetCompound() { return &m_compound; }

  void                  AddCollider(Collider* collider);
  void                  InvokeCollision(Collision* body);
  void                  RemoveCollider();
  
  void                  ClearForces();

  Vector3               m_vPosition;
  r32                   m_mass;
  r32                   m_friction;
  Quaternion            m_qRotation;
  b32                   m_bKinematic;
  b32                   m_bActivated;
  GameObject*           m_gameObj;

private:

  CompoundCollider     m_compound;
  friend class Physics;
};
} // Recluse