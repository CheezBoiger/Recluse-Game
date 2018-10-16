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

class Material;
class Mesh;
class TextureSampler;

namespace ModelLoader {
namespace GLTF {

// Load a model Mesh and Material, as well as a Skinned mesh if applicable.
// This will also store the model data in ModelCache, under the name of the file.
// ex. path/to/Apple.gltf 
// name = Apple
ModelResultBits Load(const std::string filename);
ModelResultBits FreeModel(Model** model);

// Create a new model from an existing one, with it's own resources.
// Performs a deep copy of the original model object.
ModelResultBits InstantiateModel(Model* dst, Model* src);
}
}
}