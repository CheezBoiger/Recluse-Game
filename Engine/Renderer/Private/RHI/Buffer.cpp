// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "RHI/Buffer.hpp"
#include "RHI/PhysicalDevice.hpp"
#include "RHI/VulkanRHI.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void Buffer::Initialize(const VkBufferCreateInfo& info, 
  VkMemoryPropertyFlags memFlags)
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
  allocInfo.memoryTypeIndex = VulkanRHI::gPhysicalDevice.FindMemoryType(memoryRequirements.memoryTypeBits, memFlags);
  
  if (vkAllocateMemory(mOwner, &allocInfo, nullptr, &mMemory) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to allocate memory for buffer!\n");
    return;
  }

  vkBindBufferMemory(mOwner, mBuffer, mMemory, 0);
  mMemSize = info.size;
}


void Buffer::CleanUp()
{
  if (mBuffer) {
    vkDestroyBuffer(mOwner, mBuffer, nullptr);
    vkFreeMemory(mOwner, mMemory, nullptr);

    mBuffer = VK_NULL_HANDLE;
    mMemory = VK_NULL_HANDLE;
    mMemSize = 0;
  }
}


VkResult Buffer::Map(VkDeviceSize size, VkDeviceSize offset)
{
  return vkMapMemory(mOwner, mMemory, offset, size, 0, &mMapped);
}


void Buffer::UnMap()
{
  if (mMapped) { 
    vkUnmapMemory(mOwner, mMemory);
    mMapped = nullptr;
  }
}
} // Recluse