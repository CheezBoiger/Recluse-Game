// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"

#include "Core/Types.hpp"

namespace Recluse {


class Physics;

class CompoundCollider : public Collider {
public:

  void AddCollider(Collider* collider);
  void RemoveCollider(Collider* collider);
  void RecalculateLocalInertia();

  std::vector<Collider*> m_colliders;
};
} // Recluse