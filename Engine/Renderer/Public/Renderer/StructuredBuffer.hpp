// Copyright (c) 2017 Recluse Project. All rights reserved.


#include "Core/Types.hpp"

namespace Recluse {


class Buffer;
class VulkanRHI;

// StructuredBuffer, or RWStructuredBuffer, or better yet, Shader Storage Buffer in OpenGL
// and Vulkan terms. They are essentially arbitrary buffers that store info by the programmer
// that are then used in any shader stage, mainly for compute shaders however.
//
// Structured buffers will be seen as vertex buffers in graphics pipeline, and as storage
// buffers in compute pipeline.
class StructuredBuffer {
public:

  StructuredBuffer();
  ~StructuredBuffer();

  void    initialize(VulkanRHI* rhi, size_t elementCount, size_t sizeType, void* data);
  void    cleanUp();

  // TODO(): Structured buffer is not currently host visible! We may want this if we
  // intend to perform writes to the shader storage.
  void*   Map();
  void    UnMap();

  Buffer* getHandle() { return mBuffer; }

private:
  VulkanRHI*  mRhi;
  Buffer*     mBuffer;
  U32         sizeBytes;
};
} // Recluse