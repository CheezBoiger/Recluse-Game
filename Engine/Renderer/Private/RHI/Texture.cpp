// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/Texture.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include <stdlib.h>


namespace Recluse {



void Sampler::Initialize(VkSamplerCreateInfo& info)
{
  if (vkCreateSampler(mOwner, &info, nullptr, &mSampler) != VK_SUCCESS) {
    R_DEBUG(rError, "Sampler failed to initialize!\n");
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
    R_DEBUG(rError, "Failed to create image!\n");
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
    R_DEBUG(rError, "Failed to allocate host memory for image!\n");
    return;
  }

  if (vkBindImageMemory(mOwner, mImage, mMemory, 0) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to bind memory to image!\n");
    return;
  }

  viewInfo.image = mImage;
  if (vkCreateImageView(mOwner, &viewInfo, nullptr, &mView) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create image view!\n");
  }

  mFormat = imageInfo.format;
  mWidth = imageInfo.extent.width;
  mHeight = imageInfo.extent.height;
  mSamples = imageInfo.samples;
  mMipLevels = imageInfo.mipLevels;
  mArrayLayers = imageInfo.arrayLayers;
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
} // Recluse