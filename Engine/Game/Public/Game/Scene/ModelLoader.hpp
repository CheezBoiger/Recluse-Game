// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Game/MeshComponent.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/MaterialComponent.hpp"
#include "Game/Rendering/TextureCache.hpp"
#include "Game/Rendering/RendererResourcesCache.hpp"
#include "Game/Engine.hpp"


namespace Recluse {
namespace ModelLoader {


enum ModelResult {
  Model_Success,
  Model_Fail
};


struct Primitive {
  Mesh*     _meshRef;
  Material* _materialRef;
  u32       _firstIndex;
  u32       _indexCount;
};


// Model is a container of meshes that correspond to materials.
struct Model {
  std::vector<Mesh*>        meshes;
  std::vector<Material*>    materials;
  std::vector<Texture2D*>   textures;
  std::vector<Primitive>    primitives;
};



// Load a model Mesh and Material, as well as a Skinned mesh if applicable.
ModelResult Load(const std::string filename, Model* model);
ModelResult LoadSkinned(const std::string filename, Model* model);
ModelResult FreeModel(Model** model);
 
} // ModelLoader
} // Recluse