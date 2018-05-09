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


enum ContactType {
  START_COLLISION,
  DURING_COLLISION,
  OFF_COLLISION
};


//
class RigidBody : public PhysicsObject {
public:
  RigidBody() 
    : onCollisionCallback(nullptr)
    , m_bActivated(true) { }

  void ObjectCollided();
  void EnableKinematic(b8 enable);

  void SetTransform(const Vector3& newPos, const Quaternion& newRot);
  void SetMass(r32 mass);
  b32                   Kinematic() const { return m_bKinematic; }
  b32                   Activated() const { return m_bActivated; }
  uuid64                GetGameObjectUUID() const { return m_gameObjUUID; }

  OnCollisionCallback   onCollisionCallback;
  r32                   m_mass;
  Vector3               m_vPosition;
  Quaternion            m_qRotation;
  b32                   m_bKinematic : 1;
  b32                   m_bActivated : 1;
  uuid64                m_gameObjUUID;
};
} // Recluse