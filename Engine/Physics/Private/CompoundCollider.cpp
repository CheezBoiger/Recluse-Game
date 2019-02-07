// Copyright (c) 2019 Recluse Project. All rights reserved.
#include "CompoundCollider.hpp"
#include "Core/Exception.hpp"

#include "Renderer/Renderer.hpp"


namespace Recluse {


void CompoundCollider::AddCollider(Collider* collider)
{
  SetDirty();
  m_colliders.push_back(collider);
}


Collider* CompoundCollider::RemoveCollider(Collider* collider)
{
  Collider* pCol = nullptr;
  for (auto it : m_colliders) {
    if (it == collider) {
      pCol = it;
      SetDirty();
      m_colliders.remove(it);
      break;
    }
  }

  return pCol;
}
} // Recluse