// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"


namespace Recluse {


class VertexBuffer;
class VulkanRHI;
class IndexBuffer;

class RenderQuad {
public:
  RenderQuad() { }


  void            initialize(VulkanRHI* rhi);
  void            cleanUp(VulkanRHI* pRhi);


  VertexBuffer*   getQuad() { return &quad; }
  IndexBuffer*    getIndices() { return &index; }

private:
  VertexBuffer    quad;
  IndexBuffer     index;
};
} // Recluse