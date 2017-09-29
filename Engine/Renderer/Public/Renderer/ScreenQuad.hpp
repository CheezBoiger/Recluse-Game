// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"


namespace Recluse {


class VertexBuffer;
class VulkanRHI;
class IndexBuffer;

class ScreenQuad {
public:
  ScreenQuad() { }


  void            Initialize(VulkanRHI* rhi);
  void            CleanUp();


  VertexBuffer*   Quad() { return &quad; }
  IndexBuffer*    Indices() { return &index; }

private:
  VertexBuffer    quad;
  IndexBuffer     index;
};
} // Recluse