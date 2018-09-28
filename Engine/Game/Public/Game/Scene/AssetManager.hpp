// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Animation/Clip.hpp"
#include "Animation/Skeleton.hpp"

#include "Core/Types.hpp"

#include "Game/Rendering/RendererResourcesCache.hpp"

#include <unordered_map>


namespace Recluse {


class AnimAssetManager {
public:
  static void          CleanUpAll() {
    for (auto& clip : kClips) {
      delete clip.second;
    }
    kClips.clear();
  }
  
  static void           Cache(std::string name, AnimClip* clip) {
    if (!clip) return;
    kClips[name] = clip;
  }

  static AnimClip*           Uncache(std::string name) {
    AnimClip* clip = nullptr;
    auto it = kClips.find(name);
    if (it != kClips.end()) {
      clip = it->second;
      kClips.erase(it);
    }
    return clip;
  }

  static AnimClip*      Get(std::string name) {
    AnimClip* clip = nullptr;
    auto it = kClips.find(name);
    if (it != kClips.end()) {
      clip = it->second;
    }
    return clip;
  }
private:
  static std::unordered_map<std::string, AnimClip*> kClips;
};


class AssetManager {
public:
  static void CleanUpAssets() {
    ModelCache::CleanUpAll();
    AnimAssetManager::CleanUpAll();
    MaterialCache::CleanUpAll();
    MeshCache::CleanUpAll();
    TextureCache::CleanUpAll();
    SamplerCache::CleanUpAll();
  }
};
} // Recluse