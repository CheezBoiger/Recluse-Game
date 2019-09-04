// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "StructuredBuffer.hpp"

#include "RHI/Buffer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/LogicalDevice.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


StructuredBuffer::StructuredBuffer()
  : mRhi(nullptr)
  , mBuffer(nullptr)
  , sizeBytes(0)
{
}


StructuredBuffer::~StructuredBuffer()
{
  if (mBuffer) {
    R_DEBUG(rWarning, "Structured buffer did not call CleanUp() prior to destruction!\n");    
  }
}


void StructuredBuffer::initialize(VulkanRHI* Rhi, size_t ElementCount, size_t SizeType, void* getData)
{ 
  if (!Rhi) {
    R_DEBUG(rWarning, "RHI was null. Avoiding structured buffer initialization.\n");
    return;
  }

  mRhi = Rhi;
  
  VkDeviceSize MemSize = ElementCount * SizeType;
  Buffer* StagingBuffer = mRhi->createBuffer();
  {
    VkBufferCreateInfo sInfo = { };
    sInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sInfo.size = MemSize;
    sInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    sInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sInfo.flags = 0;

    StagingBuffer->initialize(sInfo,
                              PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU);

    StagingBuffer->map();
      memcpy(StagingBuffer->getMapped(), getData, (size_t)MemSize);
    StagingBuffer->unmap();
  }

  // Device local buffer.
  mBuffer = mRhi->createBuffer();
  VkBufferCreateInfo bInfo = { };
  bInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; 
  bInfo.size = MemSize;
  bInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |  
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  mBuffer->initialize(bInfo, PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY);

  CommandBuffer* getCmdBuffer = mRhi->createCommandBuffer();
  getCmdBuffer->allocate(mRhi->computeCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  VkCommandBufferBeginInfo CmdBi = { };
  CmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  CmdBi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  getCmdBuffer->begin(CmdBi);
    VkBufferCopy region = { };
    region.size = MemSize;
    region.srcOffset = 0;
    region.dstOffset = 0;
    getCmdBuffer->copyBuffer(StagingBuffer->getNativeBuffer(),
                          mBuffer->getNativeBuffer(),
                          1,
                          &region);
  getCmdBuffer->end();

  VkCommandBuffer native = getCmdBuffer->getHandle();
  VkSubmitInfo submit = { };
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &native;
  
  mRhi->computeSubmit(DEFAULT_QUEUE_IDX, submit);
  mRhi->computeWaitIdle(DEFAULT_QUEUE_IDX);
  
  mRhi->freeBuffer(StagingBuffer);
  mRhi->freeCommandBuffer(getCmdBuffer);
}


void StructuredBuffer::cleanUp()
{
  if (mBuffer) {
    mRhi->freeBuffer(mBuffer);
    mBuffer = nullptr;
  }
}
} // Recluse