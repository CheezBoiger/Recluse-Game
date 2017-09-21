// Copyright (c) 2017 Recluse Project.
#include "RHI/Commandbuffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {



void CommandBuffer::Allocate(const VkCommandPool& pool, VkCommandBufferLevel level)
{
  mPoolOwner = pool;
  VkCommandBufferAllocateInfo info = { };
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.pNext = nullptr;
  info.commandBufferCount = 1;
  info.commandPool = pool;
  info.level = level;

  if (vkAllocateCommandBuffers(mOwner, &info, &mHandle) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to allocate commandbuffer!");
  }
}


void CommandBuffer::Free()
{
  if (mHandle) {
    vkFreeCommandBuffers(mOwner, mPoolOwner, 1, &mHandle);
    mHandle = VK_NULL_HANDLE;
  }
}
} // Recluse