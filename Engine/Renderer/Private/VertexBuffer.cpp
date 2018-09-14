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


void VertexBuffer::Initialize(VulkanRHI* rhi, size_t vertexCount, size_t sizeType, void* data, Type type)
{
  if (mBuffer) {
    R_DEBUG(rNotify, "Vertex buffer already initialized. Skipping...\n");
    return;
  }

  mRhi = rhi;
  mBuffer = rhi->CreateBuffer();
  mVertexCount = static_cast<u32>(vertexCount);
  Buffer* stagingBuffer = rhi->CreateBuffer();

  {
    VkBufferCreateInfo stagingCI = {};
    stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingCI.size = sizeType * vertexCount;
    stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stagingBuffer->Initialize(stagingCI, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    stagingBuffer->Map();
      memcpy(stagingBuffer->Mapped(), data, (size_t)stagingCI.size);
    stagingBuffer->UnMap();
  }

  VkBufferCreateInfo bufferCI = { };
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = sizeType * vertexCount;
  bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  mBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  CommandBuffer* cmdBuffer = rhi->CreateCommandBuffer();
  cmdBuffer->Allocate(rhi->TransferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    
  VkCommandBufferBeginInfo beginInfo = { };
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
  cmdBuffer->Begin(beginInfo);
    VkBufferCopy region = { };
    region.size = sizeType * vertexCount;
    region.srcOffset = 0;
    region.dstOffset = 0;
    cmdBuffer->CopyBuffer(stagingBuffer->NativeBuffer(), 
                          mBuffer->NativeBuffer(), 
                          1, 
                          &region);
  cmdBuffer->End();

  VkCommandBuffer cmd = cmdBuffer->Handle();
  VkSubmitInfo submitInfo = { };
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  rhi->TransferSubmit(DEFAULT_QUEUE_IDX, 1, &submitInfo);
  rhi->TransferWaitIdle(DEFAULT_QUEUE_IDX);

  rhi->FreeCommandBuffer(cmdBuffer);
  rhi->FreeBuffer(stagingBuffer);
}


void VertexBuffer::CleanUp()
{
  if (mBuffer) {
    mRhi->FreeBuffer(mBuffer);
    mBuffer = nullptr;
  }
}
} // Recluse