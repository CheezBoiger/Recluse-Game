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
  VkImageViewCreateInfo& viewInfo)
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

  vkBindImageMemory(mOwner, mImage, mMemory, 0);

  viewInfo.image = mImage;
  if (vkCreateImageView(mOwner, &viewInfo, nullptr, &mView) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create image view!\n");
  }
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


void Texture::Upload(VulkanRHI* rhi, Recluse::Image& image)
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
  
  VkResult result = stagingBuffer.Map(imageSize, 0);
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