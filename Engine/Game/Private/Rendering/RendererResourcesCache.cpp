// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Rendering/RendererResourcesCache.hpp"

namespace Recluse {


ResourceManager& gResourceManager()
{
  static ResourceManager manager;
  return manager;
}


} // Recluse