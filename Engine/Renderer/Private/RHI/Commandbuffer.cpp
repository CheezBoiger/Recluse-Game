// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/Commandbuffer.hpp"
#include "Core/Exception.hpp"

#define ASSERT_RECORDING() if (!mRecording) { R_DEBUG("ERROR: CommandBuffer not recording! Aborting cmd call.\n"); return; }

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


void CommandBuffer::Begin(const VkCommandBufferBeginInfo& beginInfo)
{
  vkBeginCommandBuffer(mHandle, &beginInfo);
  mRecording = true;
}


void CommandBuffer::End()
{
  vkEndCommandBuffer(mHandle);
  mRecording = false;
}


void CommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo& beginInfo, VkSubpassContents contents)
{
  ASSERT_RECORDING();
  vkCmdBeginRenderPass(mHandle, &beginInfo, contents);
}


void CommandBuffer::EndRenderPass()
{
  ASSERT_RECORDING();
  vkCmdEndRenderPass(mHandle);
}


void CommandBuffer::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
  ASSERT_RECORDING();
  vkCmdDraw(mHandle, vertexCount, instanceCount, firstVertex, firstInstance);
}


void CommandBuffer::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
{
  ASSERT_RECORDING();
  vkCmdDrawIndexed(mHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}


void CommandBuffer::BindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline)
{
  ASSERT_RECORDING();
  vkCmdBindPipeline(mHandle, bindPoint, pipeline);
}


void CommandBuffer::BindVertexBuffers(u32 firstBinding, u32 bindingCount, const VkBuffer* buffers, const VkDeviceSize* offsets)
{
  ASSERT_RECORDING();
  vkCmdBindVertexBuffers(mHandle, firstBinding, bindingCount, buffers, offsets);
}


void CommandBuffer::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
  ASSERT_RECORDING();
  vkCmdBindIndexBuffer(mHandle, buffer, offset, indexType);
}


void CommandBuffer::PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
  u32 memoryBarrierCount, const VkMemoryBarrier* memoryBarriers, u32 bufferMemoryBarrierCount,
  const VkBufferMemoryBarrier* bufferMemoryBarriers, u32 imageMemoryBarrierCount, const VkImageMemoryBarrier* imageMemoryBarriers)
{
  ASSERT_RECORDING();
  vkCmdPipelineBarrier(mHandle, srcStageMask, dstStageMask, dependencyFlags, 
    memoryBarrierCount, memoryBarriers, bufferMemoryBarrierCount, bufferMemoryBarriers,
    imageMemoryBarrierCount, imageMemoryBarriers);
} 


void CommandBuffer::BindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, u32 firstSet, u32 descriptorSetCount,
  const VkDescriptorSet* descriptorSets, u32 dynamicOffsetCount, const u32* dynamicOffsets)
{
  ASSERT_RECORDING();
  vkCmdBindDescriptorSets(mHandle, bindPoint, layout, firstSet, descriptorSetCount, descriptorSets, 
    dynamicOffsetCount, dynamicOffsets);
}
} // Recluse