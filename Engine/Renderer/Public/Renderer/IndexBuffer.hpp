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
    , mIndexCount(0)
    , m_sizeType(0) { }

  ~IndexBuffer();

  void        initialize(VulkanRHI* pRhi, size_t indexCount, size_t sizeType, void* data);
  void        cleanUp(VulkanRHI* pRhi);

  Buffer*     getHandle() { return mBuffer; }
  U32         IndexCount() { return mIndexCount; }

  U32         GetSizeType() const { return m_sizeType; }
private:
  Buffer*     mBuffer;
  U32         mIndexCount;
  U32         m_sizeType;
};
} // Recluse