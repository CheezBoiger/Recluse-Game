// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/Texture.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "Core/Exception.hpp"

#include <stdlib.h>


namespace Recluse {



void Sampler::Initialize(VkSamplerCreateInfo& info)
{
  if (vkCreateSampler(mOwner, &info, nullptr, &mSampler) != VK_SUCCESS) {
    R_DEBUG("ERROR: Sampler failed to initialize!\n");
  }
}


void Sampler::CleanUp()
{
  if (mSampler) {
    vkDestroySampler(mOwner, mSampler, nullptr);
    mSampler = VK_NULL_HANDLE;
  }
}


void Texture::Initialize(const VkImageCreateInfo& imageInfo, 
  VkImageViewCreateInfo& viewInfo, b8 stream)
{
  if (vkCreateImage(mOwner, &imageInfo, nullptr, &mImage) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create image!\n");
    return;
  }

  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements(mOwner, mImage, &memoryRequirements);
  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryRequirements.size;
  allocInfo.memoryTypeIndex = VulkanRHI::gPhysicalDevice.FindMemoryType(memoryRequirements.memoryTypeBits, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  
  if (vkAllocateMemory(mOwner, &allocInfo, nullptr, &mMemory) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to allocate host memory for image!\n");
    return;
  }

  if (vkBindImageMemory(mOwner, mImage, mMemory, 0) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to bind memory to image!\n");
    return;
  }

  viewInfo.image = mImage;
  if (vkCreateImageView(mOwner, &viewInfo, nullptr, &mView) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create image view!\n");
  }

  mFormat = imageInfo.format;
  mWidth = imageInfo.extent.width;
  mHeight = imageInfo.extent.height;
  mSamples = imageInfo.samples;
}


void Texture::CleanUp()
{
  if (mImage) {
    vkDestroyImage(mOwner, mImage, nullptr);
    mImage = VK_NULL_HANDLE;
  }

  if (mView) {
    vkDestroyImageView(mOwner, mView, nullptr);
    mView = VK_NULL_HANDLE;
  }

  if (mMemory) {
    vkFreeMemory(mOwner, mMemory, nullptr);
    mMemory = VK_NULL_HANDLE;
  }
}


void Texture::Upload(VulkanRHI* rhi, Recluse::Image const& image)
{
  VkDeviceSize imageSize = image.Width() * image.Height() * 4;
  Buffer stagingBuffer;
  stagingBuffer.SetOwner(mOwner);

  VkBufferCreateInfo stagingCI = { };
  stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingCI.size = imageSize;
  stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  stagingBuffer.Initialize(stagingCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  
  VkResult result = stagingBuffer.Map();
    memcpy(stagingBuffer.Mapped(), image.Data(), imageSize);
  stagingBuffer.UnMap();

  CommandBuffer buffer;
  buffer.SetOwner(mOwner);
  buffer.Allocate(rhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = { };
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  // TODO(): Copy buffer to image stream.
  buffer.Begin(beginInfo);
    VkImageMemoryBarrier imgBarrier = { };
    imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imgBarrier.image = mImage;
    imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imgBarrier.srcAccessMask = 0;
    imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imgBarrier.subresourceRange.baseArrayLayer = 0;
    imgBarrier.subresourceRange.baseMipLevel = 0;
    imgBarrier.subresourceRange.layerCount = 1;
    imgBarrier.subresourceRange.levelCount = 1;

    // Image memory barrier.
    buffer.PipelineBarrier(
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
      0,  
      0, nullptr, 
      0, nullptr, 
      1, &imgBarrier
    );
    
    VkBufferImageCopy region = { };
    region.bufferOffset = 0;
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = 0;
    region.imageExtent.width = image.Width();
    region.imageExtent.height = image.Height();
    region.imageExtent.depth = 1;
    region.imageOffset = { 0, 0, 0 };

    // Send buffer image copy cmd.
    buffer.CopyBufferToImage(stagingBuffer.Handle(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL , 1, &region);

    imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    buffer.PipelineBarrier(
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &imgBarrier
    );
  
  buffer.End();

  // TODO(): Submit it to graphics queue!
  VkCommandBuffer commandbuffers[] = { buffer.Handle() };

  VkSubmitInfo submit = { };
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = commandbuffers;

  rhi->GraphicsSubmit(submit);
  rhi->GraphicsWaitIdle();

  buffer.Free();
  stagingBuffer.CleanUp();
}
} // Recluse