// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "VertexBuffer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/CommandBuffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


VertexBuffer::~VertexBuffer()
{
  DEBUG_OP(
    if ( mBuffer ) {
      R_DEBUG(rWarning, "Vertex Buffer not cleaned up prior to descruction!\n");
    }
  );
}


void VertexBuffer::initialize(VulkanRHI* rhi, size_t vertexCount, size_t sizeType, void* data, Type type)
{
  if (mBuffer) {
    R_DEBUG(rNotify, "Vertex buffer already initialized. Skipping...\n");
    return;
  }

  mBuffer = rhi->createBuffer();
  mVertexCount = static_cast<U32>(vertexCount);
  Buffer* stagingBuffer = rhi->createBuffer();

  {
    VkBufferCreateInfo stagingCI = {};
    stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingCI.size = sizeType * vertexCount;
    stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stagingBuffer->initialize(rhi->logicDevice()->getNative(),
                              stagingCI, 
                              PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(stagingBuffer->getMapped(), data, (size_t)stagingCI.size);
  }

  VkBufferCreateInfo bufferCI = { };
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = sizeType * vertexCount;
  bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  mBuffer->initialize(rhi->logicDevice()->getNative(),
                      bufferCI, 
                      PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY);

  CommandBuffer* cmdBuffer = rhi->createCommandBuffer();
  cmdBuffer->allocate(rhi->getTransferCmdPool(0, 0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    
  VkCommandBufferBeginInfo beginInfo = { };
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
  cmdBuffer->begin(beginInfo);
    VkBufferCopy region = { };
    region.size = sizeType * vertexCount;
    region.srcOffset = 0;
    region.dstOffset = 0;
    cmdBuffer->copyBuffer(stagingBuffer->getNativeBuffer(), 
                          mBuffer->getNativeBuffer(), 
                          1, 
                          &region);
  cmdBuffer->end();

  VkCommandBuffer cmd = cmdBuffer->getHandle();
  VkSubmitInfo submitInfo = { };
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  rhi->transferSubmit(DEFAULT_QUEUE_IDX, 1, &submitInfo);
  rhi->transferWaitIdle(DEFAULT_QUEUE_IDX);

  rhi->freeCommandBuffer(cmdBuffer);
  rhi->freeBuffer(stagingBuffer);
}


void VertexBuffer::cleanUp(VulkanRHI* pRhi)
{
  if ( mBuffer ) {
    pRhi->freeBuffer( mBuffer );
    mBuffer = nullptr;
  }
}
} // Recluse