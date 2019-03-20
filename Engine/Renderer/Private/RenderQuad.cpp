// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RenderQuad.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Vertex.hpp"
#include "Resources.hpp"

#include "RHI/VulkanRHI.hpp"
#include "Core/Exception.hpp"

#include <array>
#include <stdio.h>

namespace Recluse {


void RenderQuad::initialize(VulkanRHI* rhi)
{
  std::array<QuadVertex, 4> vertices;
  vertices[0] = { { -1.0f, -1.0f }, { 0.0f, 0.0f } };
  vertices[1] = { { -1.0f,  1.0f }, { 0.0f, 1.0f } };
  vertices[2] = { {  1.0f,  1.0f }, { 1.0f, 1.0f } };
  vertices[3] = { {  1.0f, -1.0f }, { 1.0f, 0.0f } };

  std::array<u16, 6> indices = {
    0, 1, 2, 2, 3, 0
  };

  quad.initialize(rhi, vertices.size(), sizeof(QuadVertex), vertices.data());
  index.initialize(rhi, indices.size(), sizeof(u16), indices.data());
}


void RenderQuad::cleanUp(VulkanRHI* pRhi)
{
  quad.cleanUp(pRhi);
  index.cleanUp(pRhi);
}
} // Recluse