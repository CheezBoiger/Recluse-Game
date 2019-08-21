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

  void          initialize(VulkanRHI* pRhi, size_t vertexCount, size_t sizeType, void* data, Type type = STATIC_BUFFER);
  void          cleanUp(VulkanRHI* pRhi);

  Buffer*       getHandle() { return mBuffer; }
  U32           VertexCount() { return mVertexCount; }
  
private:
  Buffer*       mBuffer;
  U32           mVertexCount;
};
} // Recluse