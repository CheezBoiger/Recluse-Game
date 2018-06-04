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
namespace ModelLoader {


enum ModelResult {
  Model_Success,
  Model_Fail
};


// Primitive handle to the native primitive gpu friendly data. Handle holds material and mesh
// handles that update when configured.
struct PrimitiveHandle {
  Mesh*           _pMesh;       // Better handle to the mesh data held by this primitive.
  Material*       _pMaterial;   // Better Handle to the material descriptor held by this primitive.
  Primitive       _primitive;   // native primitive data to be sent to a renderer component.
};


// Model is a container of meshes that correspond to materials.
struct Model {
  // Name of the model
  std::string                   name;
  std::vector<Mesh*>            meshes;
  std::vector<Material*>        materials;
  std::vector<Texture2D*>       textures;
  std::vector<PrimitiveHandle>    primitives;
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
 
} // ModelLoader
} // Recluse