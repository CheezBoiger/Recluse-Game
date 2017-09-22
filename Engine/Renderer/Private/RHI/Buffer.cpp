// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "RHI/Buffer.hpp"
#include "RHI/PhysicalDevice.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


void Buffer::Initialize(const PhysicalDevice& physical, const VkBufferCreateInfo& info)
{
  if (vkCreateBuffer(mOwner, &info, nullptr, &mBuffer) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create buffer object.\n");
    return;
  }

  VkMemoryRequirements memoryRequirements = { };
  vkGetBufferMemoryRequirements(mOwner, mBuffer, &memoryRequirements);
  
  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryRequirements.size;
  allocInfo.memoryTypeIndex = physical.FindMemoryType(memoryRequirements.memoryTypeBits, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  
  if (vkAllocateMemory(mOwner, &allocInfo, nullptr, &mMemory) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to allocate memory for buffer!\n");
    return;
  }

  vkBindBufferMemory(mOwner, mBuffer, mMemory, 0);
}


void Buffer::CleanUp()
{
  if (mBuffer) {
    vkDestroyBuffer(mOwner, mBuffer, nullptr);
    vkFreeMemory(mOwner, mMemory, nullptr);

    mBuffer = VK_NULL_HANDLE;
    mMemory = VK_NULL_HANDLE;
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