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
    , mRhi(nullptr)
    , mVertexCount(0) { }

  void          Initialize(VulkanRHI* rhi, size_t vertexCount, size_t sizeType, void* data, Type type = STATIC_BUFFER);
  void          CleanUp();

  Buffer*       Handle() { return mBuffer; }
  u32           VertexCount() { return mVertexCount; }
  
private:
  VulkanRHI*    mRhi;
  Buffer*       mBuffer;
  u32           mVertexCount;
};
} // Recluse