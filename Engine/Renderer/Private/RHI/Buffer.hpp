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

  void              cleanUp();
  void              initialize(const VkBufferCreateInfo& info, 
                               PhysicalDeviceMemoryUsage usage);

  // Map call which will map out the buffer in memory. Returns the pointer of the mapped
  // object. To get the mapped pointer, call Mapped().
  VkResult          map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void              unmap();

  VkBuffer          getNativeBuffer() const { return mBuffer; }
  VkDeviceMemory    getMemory() const { return mMemory; }

  // Memory size of this buffer, in bytes.
  VkDeviceSize      getMemorySize() const { return mMemSize; }

  // Mapped pointer to this buffer.
  void*             getMapped() const { return mMapped; }

private:
  void*             mMapped;
  VkBuffer          mBuffer;
  VkDeviceMemory    mMemory;
  VkDeviceSize      mMemSize;
};
} // Recluse