// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Scene/ModelLoader.hpp"

#include "Animation/Animation.hpp"
#include "Animation/Clip.hpp"

#include "Game/MeshComponent.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/MaterialComponent.hpp"
#include "Game/Rendering/TextureCache.hpp"


namespace Recluse {
namespace ModelLoader {
namespace FBX {


// Load a model Mesh and Material, as well as a Skinned mesh if applicable.
// This will also store the model data in ModelCache, under the name of the file.
// ex. path/to/Apple.gltf 
// name = Apple
ModelResultBits load(const std::string filename);
ModelResultBits freeModel(Model** model);

// Create a new model from an existing one, with it's own resources.
// Performs a deep copy of the original model object.
ModelResultBits instantiateModel(Model* dst, Model* src);
}
}
}