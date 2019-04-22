// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Texture.hpp"

#include "VulkanRHI.hpp"
#include "Buffer.hpp"
#include "Commandbuffer.hpp"
#include "VulkanRHI.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include <stdlib.h>


namespace Recluse {



void Sampler::initialize(VkSamplerCreateInfo& info)
{
  if (vkCreateSampler(mOwner, &info, nullptr, &mSampler) != VK_SUCCESS) {
    R_DEBUG(rError, "Sampler failed to initialize!\n");
  }
}


void Sampler::cleanUp()
{
  if (mSampler) {
    vkDestroySampler(mOwner, mSampler, nullptr);
    mSampler = VK_NULL_HANDLE;
  }
}


void Texture::initialize(const VkImageCreateInfo& imageInfo, 
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
  allocInfo.memoryTypeIndex = VulkanRHI::gPhysicalDevice.findMemoryType(memoryRequirements.memoryTypeBits, 
                                                                        PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY);
  
  VkResult rslt = vkAllocateMemory(mOwner, &allocInfo, nullptr, &mMemory);
  if (rslt != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to allocate host memory for image!\n");
    R_ASSERT(false, "");
    return;
  }
  VulkanMemoryAllocatorManager::numberOfAllocations++;
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

  R_DEBUG(rNotify, "Texture Image created for ");
  R_DEBUG(rNormal, m_name);
  R_DEBUG(rNormal, " ");
  R_DEBUG(rNormal, mImage);
  R_DEBUG(rNormal, " for ");
  R_DEBUG(rNormal,  imageInfo.extent.width * imageInfo.extent.height << " bytes.");
  R_DEBUG(rNormal, "\n");
  //R_DEBUG(rNotify, "Texture Image: " << Image() << "\n");
  //R_DEBUG(rNotify, "Texture View: " << View() << "\n");
  //R_DEBUG(rNotify, "Texture Memory: " << Memory() << "\n");
}


void Texture::cleanUp()
{
  R_DEBUG(rNotify, "Freeing Texture: ");
  R_DEBUG(rNormal, m_name);
  R_DEBUG(rNormal, " ");
  R_DEBUG(rNormal, mImage);
  R_DEBUG(rNormal, "\n");
  if (mImage) {
    //R_DEBUG(rNotify, "Texture Image: " << Image() << "\n");
    vkDestroyImage(mOwner, mImage, nullptr);
    mImage = VK_NULL_HANDLE;
  }

  if (mView) {
    //R_DEBUG(rNotify, "Image View: " << View() << "\n");
    vkDestroyImageView(mOwner, mView, nullptr);
    mView = VK_NULL_HANDLE;
  }

  if (mMemory) {
    //R_DEBUG(rNotify, "Image Memory: " << Memory() << "\n");
    vkFreeMemory(mOwner, mMemory, nullptr);
    mMemory = VK_NULL_HANDLE;
  }
  VulkanMemoryAllocatorManager::numberOfAllocations--;
}


ImageView::~ImageView()
{
}


void ImageView::initialize(VkDevice device, const VkImageViewCreateInfo& info)
{
  VkResult result = vkCreateImageView(device, &info, nullptr, &m_view);
  DEBUG_OP(if (result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create image view!\n");
  });
}


void ImageView::cleanUp(VkDevice device)
{
  if (m_view) {
    vkDestroyImageView(device, m_view, nullptr);
    m_view = VK_NULL_HANDLE;
  }
}
} // Recluse