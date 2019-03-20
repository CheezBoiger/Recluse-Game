// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"

#include "Core/Types.hpp"

#include <list>

namespace Recluse {


class Physics;

class CompoundCollider : public Collider {
public:
  CompoundCollider()
    : Collider(PHYSICS_COLLIDER_TYPE_COMPOUND) { }
  ~CompoundCollider() { }

  void addCollider(Collider* collider);
  Collider* RemoveCollider(Collider* collider);

  std::list<Collider*>& GetColliders() { return m_colliders; }

private:

  std::list<Collider*>    m_colliders;
};
} // Recluse