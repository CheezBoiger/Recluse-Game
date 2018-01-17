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
  GameObjectManager(size_t initSize = 2056) 
    : m_GameObjects(initSize) { }

  GameObject* Allocate() {
    m_GameObjects.push_back(std::move(GameObject()));
    m_GameObjects[m_GameObjects.size() - 1].m_Id = 
      std::hash<game_uuid_t>()(gGameObjectNumber++);
    return &m_GameObjects.back();
  }

  void      Deallocate(GameObject* object) {
    // TODO():
  }


  void        Clear() {
    m_GameObjects.clear();
  }

private:
  // TODO(): Replace with custom pool memory allocator.
  std::vector<GameObject> m_GameObjects;
};


GameObjectManager& gGameObjectManager();
} // Recluse