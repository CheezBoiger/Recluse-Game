// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include <unordered_map>

namespace Recluse {


class MeshData;
class Mesh;
class Material;

class MeshCache {
public:
  static void       CleanUpAll();

  static b8         Cache(std::string name, Mesh* mesh);
  static b8         UnCache(std::string name, Mesh** out);
  static b8         Get(std::string name, Mesh** out);

private:
  static std::unordered_map<std::string, Mesh*> m_Cache;
};


class MaterialCache {
public:
  static void       CleanUpAll();
  static b8         Cache(std::string name, Material* mat);
  static b8         UnCache(std::string name, Material** out);
  static b8         Get(std::string name, Material** out);

private:
  static std::unordered_map<std::string, Material*> m_Cache;
};


class ResourceManager {
public:
  
};


ResourceManager& gResourceManager();
} // Recluse