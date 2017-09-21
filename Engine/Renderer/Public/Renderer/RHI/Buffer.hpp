// Copyright (c) 2017 Recluse Project.
#pragma once

#include "Renderer/RHI/VulkanConfigs.hpp"


namespace Recluse {



// Buffer handle to a vulkan opaque object.
class Buffer : public VulkanHandle {
public:
  Buffer()
    : mMapped(nullptr)
    , mBuffer(nullptr)
    , mMemory(nullptr) { }

  void              CleanUp();
  void              Initialize(VkDevice device, VkBufferCreateInfo& info);

  // Map call which will map out the buffer in memory. Returns the pointer of the mapped
  // object. To get the mapped pointer, call Mapped().
  VkResult          Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void              UnMap();

  VkBuffer          Handle() { return mBuffer; }
  VkDeviceMemory    Memory() { return mMemory; }
  void*             Mapped(){ return mMapped; }

private:
  void*             mMapped;
  VkBuffer          mBuffer;
  VkDeviceMemory    mMemory;
};
} // Recluse