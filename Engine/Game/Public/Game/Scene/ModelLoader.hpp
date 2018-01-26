// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Game/MeshComponent.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/Engine.hpp"


namespace Recluse {
namespace Model {


enum ModelResult {
  Model_Success,
  Model_Fail
};


struct ModelContainer {
  std::vector<Mesh> Meshes;
};



// Load a model Mesh and Material, as well as a Skinned mesh if applicable.
ModelResult Load(const std::string* filename, Mesh* mesh);
 
} // Recluse
} // Recluse