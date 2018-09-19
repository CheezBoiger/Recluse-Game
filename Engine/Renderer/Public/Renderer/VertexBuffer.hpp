// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


class Buffer;
class VulkanRHI;

class VertexBuffer {
public:
  enum Type {
    DYNAMIC_BUFFER,
    STATIC_BUFFER
  };
  
  VertexBuffer()
    : mBuffer(nullptr)
    , mVertexCount(0) { }

  ~VertexBuffer();

  void          Initialize(VulkanRHI* pRhi, size_t vertexCount, size_t sizeType, void* data, Type type = STATIC_BUFFER);
  void          CleanUp(VulkanRHI* pRhi);

  Buffer*       Handle() { return mBuffer; }
  u32           VertexCount() { return mVertexCount; }
  
private:
  Buffer*       mBuffer;
  u32           mVertexCount;
};
} // Recluse