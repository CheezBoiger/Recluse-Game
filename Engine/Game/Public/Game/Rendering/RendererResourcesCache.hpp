// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "MaterialComponent.hpp"
#include "MeshComponent.hpp"
#include "RendererComponent.hpp"
#include "TextureCache.hpp"

#include <unordered_map>

namespace Recluse {


class MeshData;
class Mesh;
struct Material;

class MeshCache {
public:
  static void       CleanUpAll() {
    // TODO(): Automate cleaning up all materials within this cache.
    for (auto& it : m_Cache) {
      Mesh* mesh = it.second;
      mesh->CleanUp();
      delete mesh;
    }
    m_Cache.clear();
  }

  static b8         Cache(std::string name, Mesh* mesh) {
    if (m_Cache.find(name) != m_Cache.end()) {
      return false;
    }
    m_Cache[name] = mesh;
    return true;
  }

  static b8         UnCache(std::string name, Mesh** out) {
    auto it = m_Cache.find(name);
    if (it != m_Cache.end()) {
      Mesh* pMesh = it->second;
      *out = pMesh;
      m_Cache.erase(it);
      return true;
    }

    return false;
  }

  static b8         Get(std::string name, Mesh** out) {
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
  static void       CleanUpAll() {
    // TODO(): Automate cleaning up all materials within this cache.
    for (auto& it : m_Cache) {
      Material* material = it.second;
      material->CleanUp();
      delete material;
    }
    m_Cache.clear();
  }

  static b8         Cache(std::string name, Material* mat) {
    if (m_Cache.find(name) != m_Cache.end()) {
      return false;
    }
    m_Cache[name] = mat;
    return true;
  }

  static b8         UnCache(std::string name, Material** out) {
    auto it = m_Cache.find(name);
    if (it != m_Cache.end()) {
      Material* pMat = it->second;
      *out = pMat;
      m_Cache.erase(it);
     return true;
    }

    return false;
  }

  static b8         Get(std::string name, Material** out) {
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


class ResourceManager {
public:
  
};


ResourceManager& gResourceManager();
} // Recluse