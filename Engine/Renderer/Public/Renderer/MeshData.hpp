// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"


namespace Recluse {


class VulkanRHI;

class MeshData {
public:
  MeshData();
  ~MeshData();

  void            Initialize(size_t elementCount, size_t sizeType, void* data,
    b8 isStatic, size_t indexCount = 0, void* indices = nullptr);

  void            CleanUp();

  VertexBuffer*   VertexData() { return &mVertexBuffer; }
  
  IndexBuffer*    IndexData() { 
    if (mIndexBuffer.IndexCount() > 0) return &mIndexBuffer; 
    else return nullptr; 
  }

private:
  VertexBuffer    mVertexBuffer;
  IndexBuffer     mIndexBuffer;
  VulkanRHI*      mRhi;

  friend class Renderer;
};
} // Recluse