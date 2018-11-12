// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"

#include "Core/Types.hpp"

namespace Recluse {


class Physics;

class CompoundCollider : public Collider {
public:
  CompoundCollider()
    : Collider(PHYSICS_COLLIDER_TYPE_COMPOUND) { }
  ~CompoundCollider() { }

  void AddCollider(Collider* collider);
  void RemoveCollider(Collider* collider);

  std::vector<Collider*>& GetColliders( void ) { return m_colliders; }
private:

  std::vector<Collider*> m_colliders;
};
} // Recluse