// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "Renderer/TextureType.hpp"
#include <unordered_map>

namespace Recluse {


typedef u64   tcache_t;


// Texture cache, to store textures created by renderer.
class TextureCache {
  TextureCache();
  ~TextureCache();
public:

  tcache_t                  Cache(TextureBase* texture);
  TextureBase*              Get(tcache_t  token);
  TextureBase*              UnCache(tcache_t token);

  u32                       CacheCount() { return m_Cache.size(); }

private:
  static std::unordered_map<tcache_t, TextureBase*> m_Cache;  
};
} // Recluse