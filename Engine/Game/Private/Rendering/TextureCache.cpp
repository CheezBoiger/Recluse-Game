// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Rendering/TextureCache.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Renderer/Renderer.hpp"

#include <algorithm>


namespace Recluse {


std::unordered_map<tcache_t, Texture2D*> TextureCache::sCache;


TextureCache::TextureCache()
{
}


TextureCache::~TextureCache()
{
}



TextureCache::CacheResult TextureCache::Cache(Texture2D* texture)
{
  if (!texture) return Cache_Null_Pointer;
  tcache_t v = std::hash<std::string>()(texture->_Name);
  auto it = sCache.find(v);
  if (it != sCache.end()) {
    return Cache_Map_Exists;
  }

  sCache[v] = texture;
  return Cache_Success;
}


TextureCache::CacheResult TextureCache::Get(std::string texname, Texture2D** out)
{
  tcache_t v = std::hash<std::string>()(texname);
  auto it = sCache.find(v);
  if (it == sCache.end()) {
    return Cache_Not_Found;
  }

  *out = it->second;
  return Cache_Success;
}


TextureCache::CacheResult TextureCache::UnCache(std::string texname, Texture2D** out)
{
  tcache_t v = std::hash<std::string>()(texname);
  auto it = sCache.find(v);
  if (it == sCache.end()) {
    return Cache_Not_Found;
  }

  *out = it->second;
  sCache.erase(v);
  return Cache_Success;
}


void TextureCache::CleanUpAll()
{
  for (auto& it : sCache) {
    switch (it.second->TexType()) {
      case TextureBase::TEXTURE_2D:
      {
        gRenderer().FreeTexture2D(reinterpret_cast<Texture2D*>(it.second));
      } break;
      case TextureBase::TEXTURE_1D:
      {
        gRenderer().FreeTexture1D(reinterpret_cast<Texture1D*>(it.second));
      } break;
      case TextureBase::TEXTURE_CUBE:
      {
        gRenderer().FreeTextureCube(reinterpret_cast<TextureCube*>(it.second));
      } break;
      case TextureBase::TEXTURE_2D_ARRAY:
      {
        gRenderer().FreeTexture2DArray(reinterpret_cast<Texture2DArray*>(it.second));
      } break;
      default:
      {
        Log(rError) << "Unknown texture type in cache. Can not clean up!\n";
      } break;
    }
  }

  sCache.clear();
}
} // Recluse