// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "VulkanConfigs.hpp"
#include "Memory/Allocator.hpp"


namespace Recluse {

class PhysicalDevice;

// Buffer handle to a vulkan opaque object.
class Buffer {
public:
  Buffer()
    : mBuffer(nullptr) { }

  void              cleanUp(VkDevice device);
  void              initialize(VkDevice device,
                               const VkBufferCreateInfo& info, 
                               PhysicalDeviceMemoryUsage usage);

  VkBuffer          getNativeBuffer() const { return mBuffer; }
  VkDeviceMemory    getMemory() const { return m_allocation._deviceMemory; }

  // Memory size of this buffer, in bytes.
  VkDeviceSize      getMemorySize() const { return m_allocation._sz; }

  // Mapped pointer to this buffer.
  void*             getMapped() const { return m_allocation._pData; }

  VkDeviceSize getMemoryOffset() const { return m_allocation._offset; }

private:

  VkBuffer          mBuffer;
  VulkanAllocation m_allocation;
};
} // Recluse