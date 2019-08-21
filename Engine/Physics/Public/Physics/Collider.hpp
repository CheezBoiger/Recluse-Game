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
  virtual void update() { }
  ColliderType GetColliderType() const { return m_colliderType; }

  B32         NeedsUpdate() const { return m_needsUpdate; }

protected:

  void              SetDirty() { m_needsUpdate = true; }
  Vector3           m_center;
  B32               m_needsUpdate;

private:

  ColliderType      m_colliderType;
};
} // Recluse