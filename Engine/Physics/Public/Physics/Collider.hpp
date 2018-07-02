// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "PhysicsConfigs.hpp"


namespace Recluse {


class Actor;

class Collider : public PhysicsObject {
public:
  Collider(const Vector3& center = Vector3())
    : m_center(center) { }

  Vector3     GetCenter() const { return m_center; }
  // Center point of this collider.

  void        SetCenter(const Vector3& center) { m_center = center; }
private:
  Vector3     m_center;
};
} // Recluse