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
  Model_Materials = (1 << 8)
};


using ModelResultBits = u32;
using ModelConfigBits = u32;



// Model is a container of meshes that correspond to materials.
struct Model {
  // Name of the model
  std::string                   name;
  std::vector<Mesh*>            meshes;
  std::vector<Material*>        materials;
  std::vector<Texture2D*>       textures;
  std::vector<AnimClip*>        animations;
  std::vector<Skeleton*>    skeletons;
};


// Load a model Mesh and Material, as well as a Skinned mesh if applicable.
// This will also store the model data in ModelCache, under the name of the file.
// ex. path/to/Apple.gltf 
// name = Apple
ModelResultBits Load(const std::string filename);
ModelResultBits FreeModel(Model** model);
 
// Create a new model from an existing one, with it's own resources.
// Performs a deep copy of the original model object.
ModelResultBits InstantiateModel(Model* dst, Model* src);
} // ModelLoader
} // Recluse