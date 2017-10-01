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
  
  void            Allocate(const VkCommandPool& pool, VkCommandBufferLevel level);
  void            Free();

  void            Begin(const VkCommandBufferBeginInfo& beginInfo);
  void            End();
  void            CopyBuffer(VkBuffer src, VkBuffer dst, u32 regionCount, const VkBufferCopy* regions);

  void            CopyBufferToImage(VkBuffer src, VkImage img, VkImageLayout imgLayout, 
                    u32 regionCount, const VkBufferImageCopy* regions);

  void            PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,  VkDependencyFlags dependencyFlags,
                    u32 memoryBarrierCount, const VkMemoryBarrier* memoryBarriers, u32 bufferMemoryBarrierCount,
                    const VkBufferMemoryBarrier* bufferMemoryBarriers, u32 imageMemoryBarrierCount, const VkImageMemoryBarrier* imageMemoryBarriers);

  void            BeginRenderPass(const VkRenderPassBeginInfo& renderpassInfo, VkSubpassContents contents);
  void            EndRenderPass();
  void            BindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
  void            Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance);
  void            DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance);

  void            BindVertexBuffers(u32 firstBinding, u32 bindingCount, const VkBuffer* buffers, const VkDeviceSize* offsets);
  void            BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

  void            BindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, u32 firstSet, u32 descriptorSetCount,
                    const VkDescriptorSet* descriptorSets, u32 dynamicOffsetCount, const u32* dynamicOffsets);

  void            SetViewPorts(u32 firstViewPort, u32 viewPortCount, const VkViewport* viewports);

  void            Reset(const VkCommandBufferResetFlags flags);
  VkCommandBuffer Handle() { return mHandle; }
  VkCommandPool   PoolOwner() { return mPoolOwner; }
  b8              Recording() { return mRecording; }

private:
  VkCommandBuffer mHandle;
  VkCommandPool   mPoolOwner;
  b8              mRecording;
};
} // Recluse 