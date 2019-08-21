// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Animation/Animation.hpp"
#include "Animation/Clip.hpp"

#include "Game/MeshComponent.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/MaterialComponent.hpp"
#include "Game/Rendering/TextureCache.hpp"


namespace Recluse {

class Material;
class Mesh;
class TextureSampler;

namespace ModelLoader {

enum ModelResult {
  Model_None = 0,
  Model_Success = (1 << 0),
  Model_Fail = (1 << 1),
  Model_Animated = (1 << 2),
  Model_Textured = (1 << 3),
  Model_Static = (1 << 4),
  Model_Skinned = (1 << 5),
  Model_Unknown = (1 << 6),
  Model_Cached = (1 << 7),
  Model_Materials = (1 << 8),
  Model_Mesh = (1 << 9)
};


using ModelResultBits = U32;
using ModelConfigBits = U32;
using NodeId = U32;
using NodeChildren = std::vector<NodeId>;

struct NodeInfo {
  NodeId _parentId;
  U32 _nodeConfig;
  U32 _meshId;
};


// Model is a container of meshes that correspond to materials.
struct Model {
  // Name of the model
  std::string name;
  std::map<NodeId, 
           NodeInfo> nodeHierarchy;
  std::vector<Mesh*> meshes;
  std::vector<Material*> materials;
  std::vector<Texture2D*> textures;
  std::vector<AnimClip*> animations;
  std::vector<Skeleton*> skeletons;
  std::vector<TextureSampler*> samplers;
};


// Load a model Mesh and Material, as well as a Skinned mesh if applicable.
// This will also store the model data in ModelCache, under the name of the file.
// ex. path/to/Apple.gltf 
// name = Apple
ModelResultBits load(const std::string filename);
ModelResultBits freeModel(Model** model);
 
// Create a new model from an existing one, with it's own resources.
// Performs a deep copy of the original model object.
ModelResultBits instantiateModel(Model* dst, Model* src);
} // ModelLoader
} // Recluse