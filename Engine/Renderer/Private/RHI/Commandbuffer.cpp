// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "CommandBuffer.hpp"
#include "Core/Exception.hpp"

#define ASSERT_RECORDING() R_ASSERT(mRecording, "Command buffer not in record state prior to issued command!");

namespace Recluse {



void CommandBuffer::allocate(const VkCommandPool& pool, VkCommandBufferLevel level)
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


void CommandBuffer::free()
{
  if (mHandle) {
    vkFreeCommandBuffers(mOwner, mPoolOwner, 1, &mHandle);
    mHandle = VK_NULL_HANDLE;
  }
}


void CommandBuffer::reset(const VkCommandBufferResetFlags flags)
{
  if (vkResetCommandBuffer(mHandle, flags) != VK_SUCCESS) {
    R_DEBUG(rWarning, "Unsuccessful command buffer reset.\n");
  }
}


void CommandBuffer::begin(const VkCommandBufferBeginInfo& beginInfo)
{
  vkBeginCommandBuffer(mHandle, &beginInfo);
  mRecording = true;
}


void CommandBuffer::end()
{
  vkEndCommandBuffer(mHandle);
  mRecording = false;
}


void CommandBuffer::beginRenderPass(const VkRenderPassBeginInfo& beginInfo, VkSubpassContents contents)
{
  ASSERT_RECORDING();
  vkCmdBeginRenderPass(mHandle, &beginInfo, contents);
}


void CommandBuffer::endRenderPass()
{
  ASSERT_RECORDING();
  vkCmdEndRenderPass(mHandle);
}


void CommandBuffer::draw(U32 vertexCount, U32 instanceCount, U32 firstVertex, U32 firstInstance)
{
  ASSERT_RECORDING();
  vkCmdDraw(mHandle, vertexCount, instanceCount, firstVertex, firstInstance);
}


void CommandBuffer::drawIndexed(U32 indexCount, U32 instanceCount, U32 firstIndex, I32 vertexOffset, U32 firstInstance)
{
  ASSERT_RECORDING();
  vkCmdDrawIndexed(mHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}


void CommandBuffer::bindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline)
{
  ASSERT_RECORDING();
  vkCmdBindPipeline(mHandle, bindPoint, pipeline);
}


void CommandBuffer::bindVertexBuffers(U32 firstBinding, U32 bindingCount, const VkBuffer* buffers, const VkDeviceSize* offsets)
{
  ASSERT_RECORDING();
  vkCmdBindVertexBuffers(mHandle, firstBinding, bindingCount, buffers, offsets);
}


void CommandBuffer::bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
  ASSERT_RECORDING();
  vkCmdBindIndexBuffer(mHandle, buffer, offset, indexType);
}


void CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
  U32 memoryBarrierCount, const VkMemoryBarrier* memoryBarriers, U32 bufferMemoryBarrierCount,
  const VkBufferMemoryBarrier* bufferMemoryBarriers, U32 imageMemoryBarrierCount, const VkImageMemoryBarrier* imageMemoryBarriers)
{
  ASSERT_RECORDING();
  vkCmdPipelineBarrier(mHandle, srcStageMask, dstStageMask, dependencyFlags, 
    memoryBarrierCount, memoryBarriers, bufferMemoryBarrierCount, bufferMemoryBarriers,
    imageMemoryBarrierCount, imageMemoryBarriers);
} 


void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, U32 firstSet, U32 descriptorSetCount,
  const VkDescriptorSet* descriptorSets, U32 dynamicOffsetCount, const U32* dynamicOffsets)
{
  ASSERT_RECORDING();
  vkCmdBindDescriptorSets(mHandle, bindPoint, layout, firstSet, descriptorSetCount, descriptorSets, 
    dynamicOffsetCount, dynamicOffsets);
}


void CommandBuffer::setScissor(U32 firstScissor, U32 scissorCount, const VkRect2D* pScissors)
{
  ASSERT_RECORDING();
  vkCmdSetScissor(mHandle, firstScissor, scissorCount, pScissors);
}


void CommandBuffer::copyBufferToImage(VkBuffer src, VkImage img, VkImageLayout imgLayout, 
  U32 regionCount, const VkBufferImageCopy* regions)
{
  ASSERT_RECORDING();
  vkCmdCopyBufferToImage(mHandle, src, img, imgLayout, regionCount, regions);
}


void CommandBuffer::copyBuffer(VkBuffer src, VkBuffer dst, U32 regionCount, const VkBufferCopy* regions)
{
  ASSERT_RECORDING();
  vkCmdCopyBuffer(mHandle, src, dst, regionCount, regions);
}


void CommandBuffer::setViewPorts(U32 firstViewPort, U32 viewPortCount, const VkViewport* viewports)
{
  ASSERT_RECORDING(); 
  vkCmdSetViewport(mHandle, firstViewPort, viewPortCount, viewports);
}


void CommandBuffer::beginQuery(VkQueryPool queryPool, U32 query, VkQueryControlFlags flags)
{
  ASSERT_RECORDING();
  vkCmdBeginQuery(mHandle, queryPool, query, flags);
}


void CommandBuffer::endQuery(VkQueryPool queryPool, U32 query)
{
  ASSERT_RECORDING();
  vkCmdEndQuery(mHandle, queryPool, query);
}


void CommandBuffer::pushConstants(VkPipelineLayout getLayout, VkShaderStageFlags StageFlags, U32 Offset, U32 Size, const void* p_Values)
{
  ASSERT_RECORDING();
  vkCmdPushConstants(mHandle, getLayout, StageFlags, Offset, Size, p_Values);
}


void CommandBuffer::copyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, U32 regionCount, VkBufferImageCopy* pRegions)
{
  ASSERT_RECORDING();
  vkCmdCopyImageToBuffer(mHandle, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}


void CommandBuffer::copyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, U32 regionCount, const VkImageCopy* pRegions)
{
  ASSERT_RECORDING();
  vkCmdCopyImage(mHandle, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}


void CommandBuffer::dispatch(U32 groupCountX, U32 groupCountY, U32 groupCountZ)
{
  ASSERT_RECORDING();
  vkCmdDispatch(mHandle, groupCountX, groupCountY, groupCountZ);
}


void CommandBuffer::clearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, U32 rangeCount, const VkImageSubresourceRange* pRanges)
{
  ASSERT_RECORDING();
  vkCmdClearColorImage(mHandle, image, imageLayout, pColor, rangeCount, pRanges);
}


void CommandBuffer::clearAttachments(U32 attachmentCount, const VkClearAttachment* pAttachments, U32 rectCount, const VkClearRect* pRects)
{
  ASSERT_RECORDING();
  vkCmdClearAttachments(mHandle, attachmentCount, pAttachments, rectCount, pRects);
}


void CommandBuffer::imageBlit(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, U32 regionCount, const VkImageBlit* pRegions, VkFilter filter)
{
  ASSERT_RECORDING();
  vkCmdBlitImage(mHandle,
    srcImage,
    srcImageLayout,
    dstImage,
    dstImageLayout, 
    regionCount,
    pRegions,
    filter);
}


void CommandBuffer::nextSubpass(VkSubpassContents contents)
{
  ASSERT_RECORDING();
  vkCmdNextSubpass(mHandle, contents);
}
} // Recluse