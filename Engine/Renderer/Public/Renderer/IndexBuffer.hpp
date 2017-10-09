// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


class Buffer;
class VulkanRHI;


class IndexBuffer {
public:
  IndexBuffer()
    : mBuffer(nullptr)
    , mRhi(nullptr)
    , mIndexCount(0) { }

  void        Initialize(VulkanRHI* rhi, size_t indexCount, size_t sizeType, void* data);
  void        CleanUp();

  Buffer*     Handle() { return mBuffer; }
  u32         IndexCount() { return mIndexCount; }

private:
  VulkanRHI*  mRhi;
  Buffer*     mBuffer;
  u32         mIndexCount;
};
} // Recluse