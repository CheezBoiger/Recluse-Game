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

  CommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer cmdBuffer)
    : mHandle(cmdBuffer)
    , mPoolOwner(pool)
   { SetOwner(device); }

  ~CommandBuffer() { }
  
  void            allocate(const VkCommandPool& pool, VkCommandBufferLevel level);
  void            free();

  void            begin(const VkCommandBufferBeginInfo& beginInfo);
  void            end();

  void            copyImage(VkImage srcImage, 
                            VkImageLayout srcImageLayout, 
                            VkImage dstImage, 
                            VkImageLayout dstImageLayout, 
                            U32 regionCount, 
                            const VkImageCopy* pRegions);
  void            copyBuffer(VkBuffer src, VkBuffer dst, U32 regionCount, const VkBufferCopy* regions);

  void            copyBufferToImage(VkBuffer src, 
                                    VkImage img, 
                                    VkImageLayout imgLayout, 
                                    U32 regionCount, 
                                    const VkBufferImageCopy* regions);
  void            copyImageToBuffer(VkImage srcImage, 
                                    VkImageLayout srcImageLayout, 
                                    VkBuffer dstBuffer, 
                                    U32 regionCount, 
                                    VkBufferImageCopy* pRegions);

  void            pipelineBarrier(VkPipelineStageFlags srcStageMask, 
                                  VkPipelineStageFlags dstStageMask,  
                                  VkDependencyFlags dependencyFlags,
                                  U32 memoryBarrierCount, 
                                  const VkMemoryBarrier* memoryBarriers, 
                                  U32 bufferMemoryBarrierCount,
                                  const VkBufferMemoryBarrier* bufferMemoryBarriers, 
                                  U32 imageMemoryBarrierCount, 
                                  const VkImageMemoryBarrier* imageMemoryBarriers);

  void            beginRenderPass(const VkRenderPassBeginInfo& renderpassInfo, VkSubpassContents contents);
  void            endRenderPass();
  void            bindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
  void            draw(U32 vertexCount, U32 instanceCount, U32 firstVertex, U32 firstInstance);
  void            drawIndexed(U32 indexCount, 
                              U32 instanceCount, 
                              U32 firstIndex, 
                              I32 vertexOffset, 
                              U32 firstInstance);

  void            bindVertexBuffers(U32 firstBinding, 
                                    U32 bindingCount, 
                                    const VkBuffer* buffers, 
                                    const VkDeviceSize* offsets);
  void            bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

  void            bindDescriptorSets(VkPipelineBindPoint bindPoint, 
                                     VkPipelineLayout layout, 
                                     U32 firstSet, 
                                     U32 descriptorSetCount,
                                     const VkDescriptorSet* descriptorSets, 
                                     U32 dynamicOffsetCount, 
                                     const U32* dynamicOffsets);
  void            setScissor(U32 firstScissor, U32 scissorCount, const VkRect2D* pScissors);
  void            setViewPorts(U32 firstViewPort, U32 viewPortCount, const VkViewport* viewports);

  void            beginQuery(VkQueryPool queryPool, U32 query, VkQueryControlFlags flags);
  void            endQuery(VkQueryPool queryPool, U32 query);

  void            pushConstants(VkPipelineLayout getLayout, 
                                VkShaderStageFlags StageFlags, 
                                U32 Offset, 
                                U32 Size, 
                                const void* p_Values);
  void            dispatch(U32 groupCountX, U32 groupCountY, U32 groupCountZ);

  void            clearColorImage(VkImage image, 
                                  VkImageLayout imageLayout, 
                                  const VkClearColorValue* pColor, 
                                  U32 rangeCount, 
                                  const VkImageSubresourceRange* pRanges);
  void            clearAttachments(U32 attachmentCount, 
                                   const VkClearAttachment* pAttachments, 
                                   U32 rectCount, 
                                   const VkClearRect* pRects);
  void            reset(const VkCommandBufferResetFlags flags);
  void            nextSubpass(VkSubpassContents contents);
  void            imageBlit(VkImage srcImage, 
                            VkImageLayout srcImageLayout, 
                            VkImage dstImage, 
                            VkImageLayout dstImageLayout, 
                            U32 regionCount, 
                            const VkImageBlit* pRegions, 
                            VkFilter filter);

  VkCommandBuffer getHandle() { return mHandle; }
  VkCommandPool   getPoolOwner() { return mPoolOwner; }
  B32             recording() { return mRecording; }

private:
  VkCommandBuffer mHandle;
  VkCommandPool   mPoolOwner;
  B32             mRecording;
};
} // Recluse 