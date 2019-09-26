// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Buffer.hpp"
#include "PhysicalDevice.hpp"
#include "VulkanRHI.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


void Buffer::initialize(VkDevice device,
                        const VkBufferCreateInfo& info, 
                        PhysicalDeviceMemoryUsage usage)
{
  if (vkCreateBuffer(device, &info, nullptr, &mBuffer) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create buffer object.\n");
    return;
  }

  VkMemoryRequirements memoryRequirements = { };
  vkGetBufferMemoryRequirements(device, mBuffer, &memoryRequirements);
  
  B32 result = VulkanRHI::gAllocator.allocate(device, 
                                                memoryRequirements.size, 
                                                memoryRequirements.alignment, 
                                                memoryRequirements.memoryTypeBits,
                                                usage,
                                                VULKAN_ALLOCATION_TYPE_BUFFER,
                                                &m_allocation);

  if (!result) {
    R_ASSERT(false, "Failed to allocate memory for buffer!");
    return;
  }

  vkBindBufferMemory(device, mBuffer, m_allocation._deviceMemory, m_allocation._offset);
}


void Buffer::cleanUp(VkDevice device)
{
  if (mBuffer) {
    vkDestroyBuffer(device, mBuffer, nullptr);

    VulkanRHI::gAllocator.free(m_allocation);

    mBuffer = VK_NULL_HANDLE;
    m_allocation._deviceMemory = VK_NULL_HANDLE;
    m_allocation._sz = 0;
  }
}
} // Recluse