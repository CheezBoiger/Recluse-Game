// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "PhysicsConfigs.hpp"


namespace Recluse {


class Actor;

enum ColliderType { 
  PHYSICS_COLLIDER_TYPE_UNKNOWN = 0,
  PHYSICS_COLLIDER_TYPE_SPHERE,
  PHYSICS_COLLIDER_TYPE_MESH,
  PHYSICS_COLLIDER_TYPE_COMPOUND,
  PHYSICS_COLLIDER_TYPE_TRIANGLE,
  PHYSICS_COLLIDER_TYPE_BOX
};

class Collider : public PhysicsObject {
public: 
  Collider(const ColliderType type = PHYSICS_COLLIDER_TYPE_UNKNOWN,
    const Vector3& center = Vector3())
    : m_center(center)
    , m_needsUpdate(true)
    , m_colliderType(type) { }

  Vector3     GetCenter() const { return m_center; }
  // Center point of this collider.

  void        SetCenter(const Vector3& center) { m_center = center; }
  virtual void Update() { }
  ColliderType GetColliderType() const { return m_colliderType; }

  b32         NeedsUpdate() const { return m_needsUpdate; }

private:
  Vector3           m_center;
  b32               m_needsUpdate;
  ColliderType      m_colliderType;
};
} // Recluse