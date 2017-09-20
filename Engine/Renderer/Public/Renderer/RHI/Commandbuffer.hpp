// Copyright (c) 2017 Recluse Project.
#pragma once

#include "Core/Types.hpp"

#include "VulkanConfigs.hpp"

namespace Recluse {


class CommandBuffer : public VulkanHandle {
public:
  CommandBuffer() 
    : mHandle(VK_NULL_HANDLE) { }

  ~CommandBuffer() { }
  
  void            Allocate(const VkCommandBufferAllocateInfo& info);
  void            CleanUp();
  
  VkCommandBuffer Handle() { return mHandle; }

private:
  VkCommandBuffer mHandle;
};
} // Recluse 