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
  b8                    Kinematic() const { return m_bKinematic; }
  b8                    Activated() const { return m_bActivated; }


  OnCollisionCallback   onCollisionCallback;
  r32                   m_mass;
  Vector3               m_vPosition;
  Quaternion            m_qRotation;
  b8                    m_bKinematic;
  b8                    m_bActivated;

};
} // Recluse