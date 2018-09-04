// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "VulkanConfigs.hpp"


namespace Recluse {

class PhysicalDevice;

// Buffer handle to a vulkan opaque object.
class Buffer : public VulkanHandle {
public:
  Buffer()
    : mMapped(nullptr)
    , mBuffer(nullptr)
    , mMemory(nullptr)
    , mMemSize(0) { }

  void              CleanUp();
  void              Initialize(const VkBufferCreateInfo& info, 
                      VkMemoryPropertyFlags memFlags);

  // Map call which will map out the buffer in memory. Returns the pointer of the mapped
  // object. To get the mapped pointer, call Mapped().
  VkResult          Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void              UnMap();

  VkBuffer          NativeBuffer() const { return mBuffer; }
  VkDeviceMemory    Memory() const { return mMemory; }

  // Memory size of this buffer, in bytes.
  VkDeviceSize      MemorySize() const { return mMemSize; }

  // Mapped pointer to this buffer.
  void*             Mapped() const { return mMapped; }

private:
  void*             mMapped;
  VkBuffer          mBuffer;
  VkDeviceMemory    mMemory;
  VkDeviceSize      mMemSize;
};
} // Recluse