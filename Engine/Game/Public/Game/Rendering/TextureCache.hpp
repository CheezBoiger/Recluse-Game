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
  static CacheResult                Cache(Texture2D* texture);

  // Get the specified texture 2d from cache.
  static CacheResult                Get(std::string texname, Texture2D** out);

  // Uncache a texture value from this structure.
  static CacheResult                UnCache(std::string texname, Texture2D** out);

  // Clean up all texture from this cache. Note that this will also free texture handles
  // back to renderer memory!
  static void                       CleanUpAll();

  // Get the number of textures in cache.
  static size_t                     CacheCount() { return sCache.size(); }

private:
  static std::unordered_map<tcache_t, Texture2D*>  sCache;
};


class SamplerCache {
public:
  static void Cache(std::string& name, TextureSampler* pSampler);
  static void Get(std::string& name, TextureSampler** out);
  static void UnCache(std::string& name, TextureSampler** out);
  static void           CleanUpAll();
  static size_t         CacheCount() { return sCache.size(); }

private:
  static std::unordered_map<std::string, TextureSampler*> sCache;
};
} // Recluse