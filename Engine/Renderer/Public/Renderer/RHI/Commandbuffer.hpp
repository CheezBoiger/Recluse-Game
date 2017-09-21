// Copyright (c) 2017 Recluse Project.
#pragma once

#include "Core/Types.hpp"

#include "VulkanConfigs.hpp"

namespace Recluse {


class CommandBuffer : public VulkanHandle {
public:
  CommandBuffer() 
    : mHandle(VK_NULL_HANDLE)
    , mPoolOwner(VK_NULL_HANDLE) { }

  ~CommandBuffer() { }
  
  void            Allocate(const VkCommandPool& pool, VkCommandBufferLevel level);
  void            Free();
  
  VkCommandBuffer Handle() { return mHandle; }
  VkCommandPool   PoolOwner() { return mPoolOwner; }

private:
  VkCommandBuffer mHandle;
  VkCommandPool   mPoolOwner;
};
} // Recluse 