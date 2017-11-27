// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GameObjectManager.hpp"


namespace Recluse {


GameObjectManager& gGameObjectManager()
{
  static GameObjectManager mnger;
  return mnger;
}
} // Recluse