// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/Swapchain.hpp"
#include "Core/Exception.hpp"
#include <limits>

namespace Recluse {


Swapchain::Swapchain()
  : mSwapchain(VK_NULL_HANDLE)
  , mPresentationQueue(VK_NULL_HANDLE)
  , mGraphicsQueue(VK_NULL_HANDLE)
  , mComputeQueue(VK_NULL_HANDLE)
  , mImageAvailableSemaphore(VK_NULL_HANDLE)
  , mGraphicsFinishedSemaphore(VK_NULL_HANDLE)
  , mPresentationQueueIndex(-1)
  , mGraphicsQueueIndex(-1)
  , mComputeQueueIndex(-1)
{
}


Swapchain::~Swapchain()
{
}


void Swapchain::Initialize(PhysicalDevice& physical, LogicalDevice& device, VkSurfaceKHR surface, 
      i32 graphicsIndex, i32 presentationIndex, i32 computeIndex)
{
  mGraphicsQueueIndex = graphicsIndex;
  mPresentationQueueIndex = presentationIndex;
  mComputeQueueIndex = computeIndex;
  mOwner = device.Handle();

  vkGetDeviceQueue(mOwner, graphicsIndex, 0, &mGraphicsQueue);
  vkGetDeviceQueue(mOwner, mPresentationQueueIndex, 0, &mPresentationQueue);
  vkGetDeviceQueue(mOwner, computeIndex, 0, &mComputeQueue);

  CreateSemaphores();
  CreateComputeFence();
  
  std::vector<VkPresentModeKHR> presentModes = physical.QuerySwapchainPresentModes(surface);
  std::vector<VkSurfaceFormatKHR> surfaceFormats = physical.QuerySwapchainSurfaceFormats(surface);
  VkSurfaceCapabilitiesKHR capabilities = physical.QuerySwapchainSurfaceCapabilities(surface);

  VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
  for (VkPresentModeKHR mode : presentModes) {
    if (mode == VK_PRESENT_MODE_FIFO_KHR) {
      desiredPresentMode = mode;
    }
    
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      desiredPresentMode = mode;
      break;
    }
  }

  ReCreate(surface, surfaceFormats[0], desiredPresentMode, capabilities);
}


void Swapchain::ReCreate(VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat, 
  VkPresentModeKHR presentMode, VkSurfaceCapabilitiesKHR capabilities)
{
  VkSwapchainKHR oldSwapChain = mSwapchain;

  u32 imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR sInfo = {};
  sInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sInfo.surface = surface;
  sInfo.imageFormat = surfaceFormat.format;
  sInfo.imageColorSpace = surfaceFormat.colorSpace;
  sInfo.imageArrayLayers = 1;
  sInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  sInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  sInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sInfo.presentMode = presentMode;
  sInfo.imageExtent = capabilities.currentExtent;
  sInfo.preTransform = capabilities.currentTransform;
  sInfo.minImageCount = capabilities.minImageCount;
  sInfo.clipped = VK_TRUE;
  sInfo.oldSwapchain = oldSwapChain;
  
  if (vkCreateSwapchainKHR(mOwner, &sInfo, nullptr, &mSwapchain) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create swapchain!\n");
  }

  R_DEBUG(rNotify, "Swapchain creation succeeded.\n");

  // Storing data of swapchains and querying images.
  mSwapchainExtent = capabilities.currentExtent;
  mSwapchainFormat = surfaceFormat.format;
  mCurrentPresentMode = presentMode;

  if (oldSwapChain) {
    vkDestroySwapchainKHR(mOwner, oldSwapChain, nullptr);
  }

  QuerySwapchainImages();
 
}


void Swapchain::CleanUp()
{
  WaitOnQueues();
  if (mImageAvailableSemaphore) {
    vkDestroySemaphore(mOwner, mImageAvailableSemaphore, nullptr);
    mImageAvailableSemaphore = VK_NULL_HANDLE;
  }

  if (mGraphicsFinishedSemaphore) {
    vkDestroySemaphore(mOwner, mGraphicsFinishedSemaphore, nullptr);
    mGraphicsFinishedSemaphore = VK_NULL_HANDLE;
  }

  if (mComputeFence) {
    vkDestroyFence(mOwner, mComputeFence, nullptr);
    mComputeFence = VK_NULL_HANDLE;
  }

  for (size_t i = 0; i < SwapchainImages.size(); ++i) {
    SwapchainImage& image = SwapchainImages[i];
    vkDestroyImageView(mOwner, image.View, nullptr);
  }

  if (mSwapchain) {
    vkDestroySwapchainKHR(mOwner, mSwapchain, nullptr);
    mSwapchain = VK_NULL_HANDLE;
  }
}


void Swapchain::QuerySwapchainImages()
{
  u32 imageCount;
  vkGetSwapchainImagesKHR(mOwner, mSwapchain, &imageCount, nullptr);

  std::vector<VkImage> images(imageCount);
  vkGetSwapchainImagesKHR(mOwner, mSwapchain, &imageCount, images.data());

  if (!SwapchainImages.empty()) {
    for (size_t i = 0; i < SwapchainImages.size(); ++i) {
      vkDestroyImageView(mOwner, SwapchainImages[i].View, nullptr);  
    }
  }

  SwapchainImages.resize(images.size());
  for (size_t i = 0; i < SwapchainImages.size(); ++i) {
    VkImageViewCreateInfo ivInfo = { };
    ivInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivInfo.format = mSwapchainFormat;
    ivInfo.image = images[i];
    ivInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;  
    ivInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivInfo.subresourceRange.baseArrayLayer = 0;
    ivInfo.subresourceRange.baseMipLevel = 0;
    ivInfo.subresourceRange.layerCount = 1;
    ivInfo.subresourceRange.levelCount = 1;
    
    if (vkCreateImageView(mOwner, &ivInfo, nullptr, &SwapchainImages[i].View) != VK_SUCCESS) {
      R_DEBUG(rError, "Failed to create swapchain image!\n");
      return;
    }
    SwapchainImages[i].Image = images[i];
  }

}


void Swapchain::WaitOnQueues()
{
  vkQueueWaitIdle(mGraphicsQueue);
  vkQueueWaitIdle(mPresentationQueue);
  vkQueueWaitIdle(mComputeQueue);
}


void Swapchain::CreateSemaphores()
{
  VkSemaphoreCreateInfo semaphoreCI = { };
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if (vkCreateSemaphore(mOwner, &semaphoreCI, nullptr, &mImageAvailableSemaphore) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create a semaphore!\n");
  }

  if (vkCreateSemaphore(mOwner, &semaphoreCI, nullptr, &mGraphicsFinishedSemaphore) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create a semaphore!\n");
  }
}


void Swapchain::CreateComputeFence()
{
  VkFenceCreateInfo fenceCI = { };
  fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateFence(mOwner, &fenceCI, nullptr, &mComputeFence) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create a semaphore!\n");
  }
}
} // Recluse