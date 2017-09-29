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
    , mRhi(nullptr) { }

  void        Initialize(VulkanRHI* rhi, size_t size, void* data);
  void        CleanUp();

  Buffer*     Handle() { return mBuffer; }

private:
  VulkanRHI*  mRhi;
  Buffer*     mBuffer;
};
} // Recluse