// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Rendering/TextureCache.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Renderer/Renderer.hpp"

#include <algorithm>


namespace Recluse {


std::unordered_map<tcache_t, Texture2D*> TextureCache::sCache;
std::unordered_map<uuid64, TextureSampler*> SamplerCache::sCache;


TextureCache::TextureCache()
{
}


TextureCache::~TextureCache()
{
}



TextureCache::CacheResult TextureCache::Cache(Texture2D* texture)
{
  if (!texture) return Cache_Null_Pointer;
  auto it = sCache.find(texture->UUID());
  if (it != sCache.end()) {
    return Cache_Map_Exists;
  }

  sCache[texture->UUID()] = texture;
  return Cache_Success;
}

#if 0
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
#endif

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


void SamplerCache::Cache(TextureSampler* pSampler)
{
  auto& it = sCache.find(pSampler->UUID());
  if (it != sCache.end()) {
    return;
  }

  sCache[pSampler->UUID()] = pSampler;
}

#if 0
void SamplerCache::Get(std::string& name, TextureSampler** out)
{
  auto& it = sCache.find(name);
  if (it != sCache.end()) {
    *out = it->second;
  }
}


void SamplerCache::UnCache(std::string& name, TextureSampler** out)
{
  auto& it = sCache.find(name);
  if (it != sCache.end()) {
    *out = it->second;
    sCache.erase(it);
  }
}
#endif

void SamplerCache::CleanUpAll()
{
  for (auto& sampler : sCache) {
    gRenderer().FreeTextureSampler(sampler.second);
  }
  sCache.clear();
}
} // Recluse