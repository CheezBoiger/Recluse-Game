// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "ModelLoaderFBX.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Image.hpp"
#include "Core/Exception.hpp"

#include "Game/Rendering/RendererResourcesCache.hpp"
#include "Rendering/TextureCache.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation/Clip.hpp"
#include "Game/Scene/AssetManager.hpp"

#include "Renderer/Vertex.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Renderer.hpp"


namespace Recluse {
namespace ModelLoader {
namespace FBX {


ModelResultBits Load(const std::string filename)
{
  return 0;
}
}
}
}