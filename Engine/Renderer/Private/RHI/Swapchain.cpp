// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Swapchain.hpp"
#include "Core/Exception.hpp"
#include <limits>

namespace Recluse {


Swapchain::Swapchain()
  : mSwapchain(VK_NULL_HANDLE)
{
}


Swapchain::~Swapchain()
{
}


void Swapchain::Initialize(PhysicalDevice& physical, LogicalDevice& device, VkSurfaceKHR surface, 
      VkPresentModeKHR desiredPresent)
{
  mOwner = device.Native();

  std::vector<VkPresentModeKHR> presentModes = physical.QuerySwapchainPresentModes(surface);
  std::vector<VkSurfaceFormatKHR> surfaceFormats = physical.QuerySwapchainSurfaceFormats(surface);
  VkSurfaceCapabilitiesKHR capabilities = physical.QuerySwapchainSurfaceCapabilities(surface);

  VkPresentModeKHR desiredPresentMode = desiredPresent;

  // Don't know why, but let's remove this for now...
  //for (VkPresentModeKHR mode : presentModes) {
  //  if (mode == desiredPresent) {
  //    desiredPresentMode = mode;
  //  }
  //}

  ReCreate(surface, surfaceFormats[0], desiredPresentMode, capabilities);
}


void Swapchain::ReCreate(VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat, 
  VkPresentModeKHR presentMode, VkSurfaceCapabilitiesKHR capabilities, u32 desiredBuffers)
{
  VkSwapchainKHR oldSwapChain = mSwapchain;

  u32 imageCount = capabilities.minImageCount;
  if (desiredBuffers > imageCount) {
    imageCount = desiredBuffers;
  }

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
  mCurrentSurfaceFormat = surfaceFormat;
  mCurrentPresentMode = presentMode;
  mCurrentBufferCount = imageCount;

  if (oldSwapChain) {
    vkDestroySwapchainKHR(mOwner, oldSwapChain, nullptr);
  }

  QuerySwapchainImages();
 
}


void Swapchain::CleanUp()
{
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
    ivInfo.format = mCurrentSurfaceFormat.format;
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
} // Recluse