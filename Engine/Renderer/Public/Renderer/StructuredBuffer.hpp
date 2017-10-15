// Copyright (c) 2017 Recluse Project. All rights reserved.


#include "Core/Types.hpp"

namespace Recluse {


class Buffer;
class VulkanRHI;

// StructuredBuffer, or RWStructuredBuffer, or better yet, Shader Storage Buffer in OpenGL
// and Vulkan terms. They are essentially arbitrary buffers that store info by the programmer
// that are then used in any shader stage, mainly for compute shaders however.
class StructuredBuffer {
public:

  void    Initialize();

  void*   Map();
  void    UnMap();

  Buffer* Handle() { return mBuffer; }

private:
  Buffer* mBuffer;
  u32     sizeBytes;
};
} // Recluse