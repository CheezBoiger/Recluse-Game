// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "CommandBuffer.hpp"
#include "Core/Exception.hpp"

#define ASSERT_RECORDING() R_ASSERT(mRecording, "Command buffer not in record state prior to issued command!");

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
    R_DEBUG(rError, "Failed to allocate commandbuffer!\n");
  }
}


void CommandBuffer::Free()
{
  if (mHandle) {
    vkFreeCommandBuffers(mOwner, mPoolOwner, 1, &mHandle);
    mHandle = VK_NULL_HANDLE;
  }
}


void CommandBuffer::Reset(const VkCommandBufferResetFlags flags)
{
  if (vkResetCommandBuffer(mHandle, flags) != VK_SUCCESS) {
    R_DEBUG(rWarning, "Unsuccessful command buffer reset.\n");
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


void CommandBuffer::SetScissor(u32 firstScissor, u32 scissorCount, const VkRect2D* pScissors)
{
  ASSERT_RECORDING();
  vkCmdSetScissor(mHandle, firstScissor, scissorCount, pScissors);
}


void CommandBuffer::CopyBufferToImage(VkBuffer src, VkImage img, VkImageLayout imgLayout, 
  u32 regionCount, const VkBufferImageCopy* regions)
{
  ASSERT_RECORDING();
  vkCmdCopyBufferToImage(mHandle, src, img, imgLayout, regionCount, regions);
}


void CommandBuffer::CopyBuffer(VkBuffer src, VkBuffer dst, u32 regionCount, const VkBufferCopy* regions)
{
  ASSERT_RECORDING();
  vkCmdCopyBuffer(mHandle, src, dst, regionCount, regions);
}


void CommandBuffer::SetViewPorts(u32 firstViewPort, u32 viewPortCount, const VkViewport* viewports)
{
  ASSERT_RECORDING(); 
  vkCmdSetViewport(mHandle, firstViewPort, viewPortCount, viewports);
}


void CommandBuffer::BeginQuery(VkQueryPool queryPool, u32 query, VkQueryControlFlags flags)
{
  ASSERT_RECORDING();
  vkCmdBeginQuery(mHandle, queryPool, query, flags);
}


void CommandBuffer::EndQuery(VkQueryPool queryPool, u32 query)
{
  ASSERT_RECORDING();
  vkCmdEndQuery(mHandle, queryPool, query);
}


void CommandBuffer::PushConstants(VkPipelineLayout Layout, VkShaderStageFlags StageFlags, u32 Offset, u32 Size, const void* p_Values)
{
  ASSERT_RECORDING();
  vkCmdPushConstants(mHandle, Layout, StageFlags, Offset, Size, p_Values);
}


void CommandBuffer::CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, u32 regionCount, VkBufferImageCopy* pRegions)
{
  ASSERT_RECORDING();
  vkCmdCopyImageToBuffer(mHandle, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}


void CommandBuffer::CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageCopy* pRegions)
{
  ASSERT_RECORDING();
  vkCmdCopyImage(mHandle, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}


void CommandBuffer::Dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ)
{
  ASSERT_RECORDING();
  vkCmdDispatch(mHandle, groupCountX, groupCountY, groupCountZ);
}


void CommandBuffer::ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, u32 rangeCount, const VkImageSubresourceRange* pRanges)
{
  ASSERT_RECORDING();
  vkCmdClearColorImage(mHandle, image, imageLayout, pColor, rangeCount, pRanges);
}


void CommandBuffer::ClearAttachments(u32 attachmentCount, const VkClearAttachment* pAttachments, u32 rectCount, const VkClearRect* pRects)
{
  ASSERT_RECORDING();
  vkCmdClearAttachments(mHandle, attachmentCount, pAttachments, rectCount, pRects);
}
} // Recluse