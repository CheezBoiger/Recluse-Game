// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Memory/PoolAllocator.hpp"

#include "GameObject.hpp"
#include <algorithm>

namespace Recluse {


// TODO(): This will require a pool allocator instead of a vector!
class GameObjectManager {
  static u64            gGameObjectNumber;
public:
  GameObjectManager(size_t initSize = 4096) 
    : m_GameObjects(initSize)
    , m_Occupied(0) { }

  GameObject* Allocate() {
    if (m_Occupied >= m_GameObjects.size()) {
      m_GameObjects.resize(m_Occupied << 1);
    }
    m_GameObjects[m_Occupied++] = std::move(GameObject());
    m_GameObjects[m_Occupied - 1].m_Id = 
      std::hash<game_uuid_t>()(gGameObjectNumber++);
    return &m_GameObjects[m_Occupied - 1];
  }

  void      Deallocate(GameObject* object) {
    // TODO():
  }

  size_t    NumOccupied() const { return m_Occupied; }


  void        Clear() {
    m_Occupied = 0;
    m_GameObjects.clear();
  }

private:
  // TODO(): Replace with custom pool memory allocator.
  std::vector<GameObject> m_GameObjects;
  size_t                  m_Occupied;
};


GameObjectManager& gGameObjectManager();
} // Recluse