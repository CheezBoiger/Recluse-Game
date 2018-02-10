// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Game/MeshComponent.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/MaterialComponent.hpp"
#include "Game/Rendering/TextureCache.hpp"
#include "Game/Rendering/MeshCache.hpp"
#include "Game/Engine.hpp"


namespace Recluse {
namespace Model {


enum ModelResult {
  Model_Success,
  Model_Fail
};


// Model is a container of meshes that correspond to materials.
struct Model {
  std::vector<Mesh*>     Meshes;
  std::vector<Material*> Materials;
};



// Load a model Mesh and Material, as well as a Skinned mesh if applicable.
ModelResult Load(const std::string filename, Model* model);
ModelResult LoadSkinned(const std::string filename, Model* model);
ModelResult FreeModel(Model** model);
 
} // Recluse
} // Recluse