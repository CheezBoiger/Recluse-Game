// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Memory/PoolAllocator.hpp"

#include "GameObject.hpp"
#include <algorithm>
#include <list>

namespace Recluse {


// TODO(): This will require a pool allocator instead of a list!
class GameObjectManager {
  static U64            gGameObjectNumber;
public:
  GameObjectManager() { }

  GameObject* allocate() {
    m_GameObjects.push_back(std::move(GameObject()));
    m_GameObjects.back().m_id = 
      std::hash<game_uuid_t>()(gGameObjectNumber++);
    return &m_GameObjects.back();
  }

  void      Deallocate(GameObject* object) {
    // TODO():
  }

  size_t    NumOccupied() const { return m_GameObjects.size(); }


  void        clear() {
    m_GameObjects.clear();
  }

private:
  // TODO(): Replace with custom pool memory allocator.
  std::list<GameObject>   m_GameObjects;
};


GameObjectManager& gGameObjectManager();
} // Recluse