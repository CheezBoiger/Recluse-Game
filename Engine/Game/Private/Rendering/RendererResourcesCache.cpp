// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Rendering/RendererResourcesCache.hpp"

namespace Recluse {


std::unordered_map<std::string, Material*> MaterialCache::m_Cache;
std::unordered_map<std::string, Mesh*> MeshCache::m_Cache;

ResourceManager& gResourceManager()
{
  static ResourceManager manager;
  return manager;
}


} // Recluse