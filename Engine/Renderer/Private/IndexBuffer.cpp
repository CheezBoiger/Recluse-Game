// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "IndexBuffer.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/CommandBuffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


IndexBuffer::~IndexBuffer()
{
  DEBUG_OP(
    if ( mBuffer ) {
      R_DEBUG(rWarning, "Index buffer not properly cleaned up prior to descrution!\n");
    }  
  );
}


void IndexBuffer::initialize(VulkanRHI* rhi, size_t indexCount, size_t sizeType, void* data)
{
  if (mBuffer) {
    R_DEBUG(rNotify, "Index buffer already initialized. Skipping...\n");
    return;
  }

  mBuffer = rhi->createBuffer();
  mIndexCount = static_cast<u32>(indexCount);
  m_sizeType = static_cast<u32>(sizeType);
  Buffer* stagingBuffer = rhi->createBuffer();

  {
    VkBufferCreateInfo stagingCI = {};
    stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingCI.size = sizeType * indexCount;
    stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stagingBuffer->initialize(stagingCI, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU);

    stagingBuffer->map();
    memcpy(stagingBuffer->getMapped(), data, (size_t)stagingCI.size);
    stagingBuffer->unmap();
  }

  VkBufferCreateInfo bufferCI = {};
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = sizeType * indexCount;
  bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  mBuffer->initialize(bufferCI, PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY);

  CommandBuffer* cmdBuffer = rhi->createCommandBuffer();
  cmdBuffer->allocate(rhi->transferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmdBuffer->begin(beginInfo);
  VkBufferCopy region = {};
  region.size = sizeType * indexCount;
  region.srcOffset = 0;
  region.dstOffset = 0;
  cmdBuffer->copyBuffer(stagingBuffer->getNativeBuffer(), mBuffer->getNativeBuffer(), 1, &region);
  cmdBuffer->end();

  VkCommandBuffer cmd = cmdBuffer->getHandle();
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  rhi->transferSubmit(DEFAULT_QUEUE_IDX, 1, &submitInfo);
  rhi->transferWaitIdle(DEFAULT_QUEUE_IDX);

  rhi->freeCommandBuffer(cmdBuffer);
  rhi->freeBuffer(stagingBuffer);
}


void IndexBuffer::cleanUp(VulkanRHI* pRhi)
{
  if ( mBuffer ) {
    pRhi->freeBuffer( mBuffer );
    mBuffer = nullptr;
  }
}
} // Recluse