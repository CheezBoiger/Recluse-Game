// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"
#include "PhysicsConfigs.hpp"


namespace Recluse {


typedef void(*OnCollisionCallback)();


enum ContactType {
  START_COLLISION,
  DURING_COLLISION,
  OFF_COLLISION
};


//
class RigidBody : public PhysicsObject {
public:
  RigidBody() : 
    onCollisionCallback(nullptr) { }

  void ObjectCollided();
  void EnableKinematic(b8 enable);

  void SetPosition(const Vector3& newPos);
  void SetMass(r32 mass);
  b8                    Kinematic() const { return m_bKinematic; }


  OnCollisionCallback   onCollisionCallback;
  r32                   m_mass;
  Vector3               m_vPosition;
  Quaternion            m_qRotation;
  b8                    m_bKinematic;

};
} // Recluse