// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Renderer/RHI/VulkanConfigs.hpp"


namespace Recluse {

class PhysicalDevice;

// Buffer handle to a vulkan opaque object.
class Buffer : public VulkanHandle {
public:
  Buffer()
    : mMapped(nullptr)
    , mBuffer(nullptr)
    , mMemory(nullptr) { }

  void              CleanUp();
  void              Initialize(const VkBufferCreateInfo& info, 
                      VkMemoryPropertyFlags memFlags);

  // Map call which will map out the buffer in memory. Returns the pointer of the mapped
  // object. To get the mapped pointer, call Mapped().
  VkResult          Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void              UnMap();

  VkBuffer          NativeBuffer() const { return mBuffer; }
  VkDeviceMemory    Memory() const { return mMemory; }
  void*             Mapped() const { return mMapped; }

private:
  void*             mMapped;
  VkBuffer          mBuffer;
  VkDeviceMemory    mMemory;
};
} // Recluse