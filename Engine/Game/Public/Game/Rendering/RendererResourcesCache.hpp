// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "MaterialComponent.hpp"
#include "MeshComponent.hpp"
#include "RendererComponent.hpp"
#include "TextureCache.hpp"
#include "Renderer/Renderer.hpp"

#include "Scene/ModelLoader.hpp"

#include <unordered_map>

namespace Recluse {


class MeshCache {
public:
  static void       cleanUpAll() {
    // TODO(): Automate cleaning up all materials within this cache.
    for (auto& it : m_Cache) {
      Mesh* mesh = it.second;
      mesh->cleanUp(&gRenderer());
      delete mesh;
    }
    m_Cache.clear();
  }

  static B32         cache(std::string name, Mesh* mesh) {
    if (m_Cache.find(name) != m_Cache.end()) {
      return false;
    }
    m_Cache[name] = mesh;
    return true;
  }

  static B32         UnCache(std::string name, Mesh** out) {
    auto it = m_Cache.find(name);
    if (it != m_Cache.end()) {
      Mesh* pMesh = it->second;
      *out = pMesh;
      m_Cache.erase(it);
      return true;
    }

    return false;
  }

  static B32         get(std::string name, Mesh** out) {
    auto it = m_Cache.find(name);
    if (it != m_Cache.end()) {
      *out = it->second;
      return true;
    }

    return false;
  }

private:
  static std::unordered_map<std::string, Mesh*> m_Cache;
};


class MaterialCache {
public:
  static void       cleanUpAll() {
    // TODO(): Automate cleaning up all materials within this cache.
    for (auto& it : m_Cache) {
      Material* material = it.second;
      material->cleanUp(&gRenderer());
      delete material;
    }
    m_Cache.clear();
  }

  static B32         cache(std::string name, Material* mat) {
    if (m_Cache.find(name) != m_Cache.end()) {
      return false;
    }
    m_Cache[name] = mat;
    return true;
  }

  static B32         UnCache(std::string name, Material** out) {
    auto it = m_Cache.find(name);
    if (it != m_Cache.end()) {
      Material* pMat = it->second;
      *out = pMat;
      m_Cache.erase(it);
     return true;
    }

    return false;
  }

  static B32         get(std::string name, Material** out) {
    auto it = m_Cache.find(name);
    if (it != m_Cache.end()) {
      *out = it->second;
      return true;
    }

    return false;
  }

private:
  static std::unordered_map<std::string, Material*> m_Cache;
};


class ModelCache {
public:
  static void       cleanUpAll() {
    // TODO(): Automate cleaning up all materials within this cache.
    for (auto& it : m_Cache) {
      ModelLoader::Model* model = it.second;
      delete model;
    }
    m_Cache.clear();
  }

  static B32         cache(std::string name, ModelLoader::Model* model) {
    if (m_Cache.find(name) != m_Cache.end()) {
      return false;
    }
    m_Cache[name] = model;
    return true;
  }

  static B32         UnCache(std::string name, ModelLoader::Model** out) {
    auto it = m_Cache.find(name);
    if (it != m_Cache.end()) {
      ModelLoader::Model* pMod = it->second;
      *out = pMod;
      m_Cache.erase(it);
      return true;
    }

    return false;
  }

  static B32         get(std::string name, ModelLoader::Model** out) {
    auto it = m_Cache.find(name);
    if (it != m_Cache.end()) {
      *out = it->second;
      return true;
    }

    return false;
  }
private:
  static std::unordered_map<std::string, ModelLoader::Model* > m_Cache;
};


class ResourceManager {
public:
  
};


ResourceManager& gResourceManager();
} // Recluse