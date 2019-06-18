// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "Renderer/TextureType.hpp"
#include <unordered_map>

namespace Recluse {


typedef size_t   tcache_t;


// Texture cache, to store textures created by renderer.
class TextureCache {
  TextureCache();
  ~TextureCache();
public:
  enum CacheResult {
    Cache_Success = 0,
    Cache_Null_Pointer = -1,
    Cache_Failed = -2,
    Cache_Map_Exists = -3,
    Cache_Not_Found = -4
  };

  // Cache the texture2D into this data structure.
  static CacheResult                cache(Texture2D* texture, std::string& name = std::string());

#if 1
  // Get the specified texture 2d from cache.
  static CacheResult                get(std::string texname, Texture2D** out);

  // Uncache a texture value from this structure.
  static CacheResult                UnCache(std::string texname, Texture2D** out);
#endif
  // Clean up all texture from this cache. Note that this will also free texture handles
  // back to renderer memory!
  static void                       cleanUpAll();

  // Get the number of textures in cache.
  static size_t                     CacheCount() { return sCache.size(); }

private:
  static std::unordered_map<tcache_t, Texture2D*>  sCache;
};


class SamplerCache {
public:
  static void cache(TextureSampler* pSampler);
#if 0
  static void get(std::string& name, TextureSampler** out);
  static void UnCache(std::string& name, TextureSampler** out);
#endif
  static void           cleanUpAll();
  static size_t         CacheCount() { return sCache.size(); }

private:
  static std::unordered_map<uuid64, TextureSampler*> sCache;
};
} // Recluse