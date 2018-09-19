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
  Model_Success,
  Model_Fail
};



// Model is a container of meshes that correspond to materials.
struct Model {
  // Name of the model
  std::string                   name;
  std::vector<Mesh*>            meshes;
  std::vector<Material*>        materials;
  std::vector<Texture2D*>       textures;
};


// Animation model.
struct AnimModel : public Model {
  std::vector<Skeleton*>    skeletons;
  std::vector<AnimClip*>    animations;
};

// Load a model Mesh and Material, as well as a Skinned mesh if applicable.
// This will also store the model data in ModelCache, under the name of the file.
// ex. path/to/Apple.gltf 
// name = Apple
ModelResult Load(const std::string filename);
ModelResult LoadAnimatedModel(const std::string filename);
ModelResult FreeModel(Model** model);
 
// Create a new model from an existing one, with it's own resources.
// Performs a deep copy of the original model object.
ModelResult InstantiateModel(Model* dst, Model* src);
} // ModelLoader
} // Recluse