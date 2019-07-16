// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "VulkanConfigs.hpp"

namespace Recluse {


// Commandbuffer helper object, that holds the information of the buffer. This 
// buffer is COM oriented.
class CommandBuffer : public VulkanHandle {
public:
  CommandBuffer() 
    : mHandle(VK_NULL_HANDLE)
    , mPoolOwner(VK_NULL_HANDLE)
    , mRecording(VK_NULL_HANDLE) { }

  ~CommandBuffer() { }
  
  void            allocate(const VkCommandPool& pool, VkCommandBufferLevel level);
  void            free();

  void            begin(const VkCommandBufferBeginInfo& beginInfo);
  void            end();

  void            copyImage(VkImage srcImage, 
                            VkImageLayout srcImageLayout, 
                            VkImage dstImage, 
                            VkImageLayout dstImageLayout, 
                            u32 regionCount, 
                            const VkImageCopy* pRegions);
  void            copyBuffer(VkBuffer src, VkBuffer dst, u32 regionCount, const VkBufferCopy* regions);

  void            copyBufferToImage(VkBuffer src, 
                                    VkImage img, 
                                    VkImageLayout imgLayout, 
                                    u32 regionCount, 
                                    const VkBufferImageCopy* regions);
  void            copyImageToBuffer(VkImage srcImage, 
                                    VkImageLayout srcImageLayout, 
                                    VkBuffer dstBuffer, 
                                    u32 regionCount, 
                                    VkBufferImageCopy* pRegions);

  void            pipelineBarrier(VkPipelineStageFlags srcStageMask, 
                                  VkPipelineStageFlags dstStageMask,  
                                  VkDependencyFlags dependencyFlags,
                                  u32 memoryBarrierCount, 
                                  const VkMemoryBarrier* memoryBarriers, 
                                  u32 bufferMemoryBarrierCount,
                                  const VkBufferMemoryBarrier* bufferMemoryBarriers, 
                                  u32 imageMemoryBarrierCount, 
                                  const VkImageMemoryBarrier* imageMemoryBarriers);

  void            beginRenderPass(const VkRenderPassBeginInfo& renderpassInfo, VkSubpassContents contents);
  void            endRenderPass();
  void            bindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
  void            draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance);
  void            drawIndexed(u32 indexCount, 
                              u32 instanceCount, 
                              u32 firstIndex, 
                              i32 vertexOffset, 
                              u32 firstInstance);

  void            bindVertexBuffers(u32 firstBinding, 
                                    u32 bindingCount, 
                                    const VkBuffer* buffers, 
                                    const VkDeviceSize* offsets);
  void            bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

  void            bindDescriptorSets(VkPipelineBindPoint bindPoint, 
                                     VkPipelineLayout layout, 
                                     u32 firstSet, 
                                     u32 descriptorSetCount,
                                     const VkDescriptorSet* descriptorSets, 
                                     u32 dynamicOffsetCount, 
                                     const u32* dynamicOffsets);
  void            setScissor(u32 firstScissor, u32 scissorCount, const VkRect2D* pScissors);
  void            setViewPorts(u32 firstViewPort, u32 viewPortCount, const VkViewport* viewports);

  void            beginQuery(VkQueryPool queryPool, u32 query, VkQueryControlFlags flags);
  void            endQuery(VkQueryPool queryPool, u32 query);

  void            pushConstants(VkPipelineLayout getLayout, 
                                VkShaderStageFlags StageFlags, 
                                u32 Offset, 
                                u32 Size, 
                                const void* p_Values);
  void            dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ);

  void            clearColorImage(VkImage image, 
                                  VkImageLayout imageLayout, 
                                  const VkClearColorValue* pColor, 
                                  u32 rangeCount, 
                                  const VkImageSubresourceRange* pRanges);
  void            clearAttachments(u32 attachmentCount, 
                                   const VkClearAttachment* pAttachments, 
                                   u32 rectCount, 
                                   const VkClearRect* pRects);
  void            reset(const VkCommandBufferResetFlags flags);
  void            nextSubpass(VkSubpassContents contents);
  void            imageBlit(VkImage srcImage, 
                            VkImageLayout srcImageLayout, 
                            VkImage dstImage, 
                            VkImageLayout dstImageLayout, 
                            u32 regionCount, 
                            const VkImageBlit* pRegions, 
                            VkFilter filter);

  VkCommandBuffer getHandle() { return mHandle; }
  VkCommandPool   getPoolOwner() { return mPoolOwner; }
  b32             recording() { return mRecording; }

private:
  VkCommandBuffer mHandle;
  VkCommandPool   mPoolOwner;
  b32             mRecording;
};
} // Recluse 