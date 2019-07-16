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
  
  VkResult rslt = vkAllocateMemory(mOwner, &allocInfo, nullptr, &m_allocation._deviceMemory);
  if (rslt != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to allocate memory for buffer!\n");
    R_ASSERT(false, "");
    return;
  }

  VulkanMemoryAllocatorManager::numberOfAllocations++;
  vkBindBufferMemory(mOwner, mBuffer, m_allocation._deviceMemory, 0);
  m_allocation._sz = info.size;
}


void Buffer::cleanUp()
{
  if (mBuffer) {
    vkDestroyBuffer(mOwner, mBuffer, nullptr);
    vkFreeMemory(mOwner, m_allocation._deviceMemory, nullptr);
    VulkanMemoryAllocatorManager::numberOfAllocations--;
    mBuffer = VK_NULL_HANDLE;
    m_allocation._deviceMemory = VK_NULL_HANDLE;
    m_allocation._sz = 0;
  }
}


VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
{
  return vkMapMemory(mOwner, 
                     m_allocation._deviceMemory, 
                     offset, 
                     size, 
                     0, 
                     (void**)&m_allocation._pData);
}


void Buffer::unmap()
{
  if (m_allocation._pData) { 
    vkUnmapMemory(mOwner, m_allocation._deviceMemory);
    m_allocation._pData = nullptr;
  }
}
} // Recluse