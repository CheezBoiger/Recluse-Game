// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GameObjectManager.hpp"


namespace Recluse {

u64 GameObjectManager::gGameObjectNumber = 0;

GameObjectManager& gGameObjectManager()
{
  static GameObjectManager mnger;
  return mnger;
}
} // Recluse