// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"

#include "Core/Math/Vector3.hpp"
#include "Core/Types.hpp"


namespace Recluse {


class SphereCollider : public Collider {
public:
  SphereCollider(r32 radius = 0.0f)
    : m_radius(radius) { }

  r32             GetRadius() const { return m_radius; }

  void            SetRadius(r32 radius);

private:
  r32 m_radius;
};
} // Recluse