// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "VulkanConfigs.hpp"
#include "Memory/Allocator.hpp"


namespace Recluse {

class PhysicalDevice;

// Buffer handle to a vulkan opaque object.
class Buffer : public VulkanHandle {
public:
  Buffer()
    : mBuffer(nullptr) { }

  void              cleanUp();
  void              initialize(const VkBufferCreateInfo& info, 
                               PhysicalDeviceMemoryUsage usage);

  // Map call which will map out the buffer in memory. Returns the pointer of the mapped
  // object. To get the mapped pointer, call Mapped().
  VkResult          map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void              unmap();

  VkBuffer          getNativeBuffer() const { return mBuffer; }
  VkDeviceMemory    getMemory() const { return m_allocation._deviceMemory; }

  // Memory size of this buffer, in bytes.
  VkDeviceSize      getMemorySize() const { return m_allocation._sz; }

  // Mapped pointer to this buffer.
  void*             getMapped() const { return m_allocation._pData; }

private:

  VkBuffer          mBuffer;
  VulkanAllocation m_allocation;
};
} // Recluse