// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "ScreenQuad.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Vertex.hpp"
#include "Resources.hpp"

#include "RHI/VulkanRHI.hpp"
#include "Core/Exception.hpp"

#include <array>
#include <stdio.h>

namespace Recluse {


void ScreenQuad::Initialize(VulkanRHI* rhi)
{
  std::array<QuadVertex, 4> vertices;
  vertices[0] = { { 1.0f, 1.0f }, { 1.0f, 1.0f } };
  vertices[1] = { { 0.0f, 1.0f }, { 0.0f, 1.0f } };
  vertices[2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
  vertices[3] = { { 1.0f, 1.0f }, { 1.0f, 1.0f } };

  std::array<u32, 6> indices = {
    0, 1, 2, 2, 3, 0
  };

  quad.Initialize(rhi, vertices.size(), vertices.data());
  index.Initialize(rhi, indices.size(), indices.data());
}


void ScreenQuad::CleanUp()
{
  quad.CleanUp();
  index.CleanUp();
}
} // Recluse