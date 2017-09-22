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
  , mSemaphore(VK_NULL_HANDLE)
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

  ReCreate(surface, physical);
}


void Swapchain::ReCreate(VkSurfaceKHR surface, PhysicalDevice& physical)
{
  VkSwapchainKHR oldSwapChain = mSwapchain;

  // TODO(): Begin swap chain querying for proper formats and whatnot.
  std::vector<VkSurfaceFormatKHR> formats = physical.QuerySwapchainSurfaceFormats(surface);
  std::vector<VkPresentModeKHR> presentModes = physical.QuerySwapchainPresentModes(surface);
  VkSurfaceCapabilitiesKHR capabilities = physical.QuerySwapchainSurfaceCapabilities(surface);

  u32 imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }


  VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
  for (const auto& availableMode : presentModes) {
    // Triple Buffering availability.
    if (availableMode == VK_PRESENT_MODE_FIFO_KHR) {
      presentMode = availableMode;
    }

    if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = availableMode;
      break;
    }
  }

  VkSwapchainCreateInfoKHR sInfo = {};
  sInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sInfo.surface = surface;
  sInfo.imageFormat = formats[0].format;
  sInfo.imageColorSpace = formats[0].colorSpace;
  sInfo.imageArrayLayers = 1;
  sInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  sInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  sInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sInfo.presentMode = presentMode;
  sInfo.imageExtent = capabilities.currentExtent;
  sInfo.preTransform = capabilities.currentTransform;
  sInfo.minImageCount = capabilities.minImageCount;
  sInfo.clipped = VK_TRUE;
  
  if (vkCreateSwapchainKHR(mOwner, &sInfo, nullptr, &mSwapchain) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create swapchain!\n");
  }

  R_DEBUG("NOTIFY: Swapchain creation succeeded.\n");

  // Storing data of swapchains and querying images.
  mSwapchainExtent = capabilities.currentExtent;
  mSwapchainFormat = formats[0].format;

  QuerySwapchainImages();
}


void Swapchain::CleanUp()
{
  WaitOnQueues();
  if (mSemaphore) {
    vkDestroySemaphore(mOwner, mSemaphore, nullptr);
    mSemaphore = VK_NULL_HANDLE;
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
      R_DEBUG("ERROR: Failed to create swapchain image!\n");
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
} // Recluse