// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Buffer.hpp"
#include "PhysicalDevice.hpp"
#include "VulkanRHI.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


void Buffer::initialize(const VkBufferCreateInfo& info, 
  PhysicalDeviceMemoryUsage usage)
{
  if (vkCreateBuffer(mOwner, &info, nullptr, &mBuffer) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create buffer object.\n");
    return;
  }

  VkMemoryRequirements memoryRequirements = { };
  vkGetBufferMemoryRequirements(mOwner, mBuffer, &memoryRequirements);
  
  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryRequirements.size;
  allocInfo.memoryTypeIndex = VulkanRHI::gPhysicalDevice.findMemoryType(memoryRequirements.memoryTypeBits, usage);
  
  VkResult rslt = vkAllocateMemory(mOwner, &allocInfo, nullptr, &mMemory);
  if (rslt != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to allocate memory for buffer!\n");
    R_ASSERT(false, "");
    return;
  }

  VulkanMemoryAllocatorManager::numberOfAllocations++;
  vkBindBufferMemory(mOwner, mBuffer, mMemory, 0);
  mMemSize = info.size;
}


void Buffer::cleanUp()
{
  if (mBuffer) {
    vkDestroyBuffer(mOwner, mBuffer, nullptr);
    vkFreeMemory(mOwner, mMemory, nullptr);
    VulkanMemoryAllocatorManager::numberOfAllocations--;
    mBuffer = VK_NULL_HANDLE;
    mMemory = VK_NULL_HANDLE;
    mMemSize = 0;
  }
}


VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
{
  return vkMapMemory(mOwner, mMemory, offset, size, 0, &mMapped);
}


void Buffer::unmap()
{
  if (mMapped) { 
    vkUnmapMemory(mOwner, mMemory);
    mMapped = nullptr;
  }
}
} // Recluse