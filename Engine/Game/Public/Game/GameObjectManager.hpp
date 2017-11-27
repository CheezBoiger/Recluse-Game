// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Memory/PoolAllocator.hpp"

#include "GameObject.hpp"


namespace Recluse {


// TODO(): This will require a pool allocator instead of a vector!
class GameObjectManager {
public:
  GameObjectManager(size_t initSize = 1024) 
    : mGameObjects(initSize) { }

  GameObject* Allocate() {
    mGameObjects.push_back(std::move(GameObject()));
    mGameObjects[mGameObjects.size() - 1].mId = mGameObjects.size() - 1;
    return &mGameObjects.back();
  }

  void      Deallocate(GameObject* object) {
    // TODO():
  }


  void        Clear() {
    mGameObjects.clear();
  }

private:
  // TODO(): Replace with custom pool memory allocator.
  std::vector<GameObject> mGameObjects;
};


GameObjectManager& gGameObjectManager();
} // Recluse