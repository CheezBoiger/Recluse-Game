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
      VkPresentModeKHR desiredPresent, u32 buffers, u32 desiredImages)
{

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

  ReCreate(device, surface, surfaceFormats[0], desiredPresentMode, capabilities, buffers, desiredImages);
}


void Swapchain::ReCreate(LogicalDevice& device, VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat, 
  VkPresentModeKHR presentMode, VkSurfaceCapabilitiesKHR capabilities, u32 buffers, u32 desiredImages)
{
  VkSwapchainKHR oldSwapChain = mSwapchain;

  u32 imageCount = capabilities.minImageCount;
  if (desiredImages > imageCount) {
    imageCount = desiredImages;
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
  sInfo.minImageCount = imageCount;
  sInfo.clipped = VK_TRUE;
  sInfo.oldSwapchain = oldSwapChain;
  
  if (vkCreateSwapchainKHR(device.Native(), &sInfo, nullptr, &mSwapchain) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create swapchain!\n");
  }

  R_DEBUG(rNotify, "Swapchain creation succeeded.\n");

  // Storing data of swapchains and querying images.
  mSwapchainExtent = capabilities.currentExtent;
  mCurrentSurfaceFormat = surfaceFormat;
  mCurrentPresentMode = presentMode;
  mCurrentBufferCount = buffers;

  if (oldSwapChain) {
    vkDestroySwapchainKHR(device.Native(), oldSwapChain, nullptr);
  }

  QuerySwapchainImages(device);
  CreateSemaphores(device, buffers);
}


void Swapchain::CleanUp(LogicalDevice& device)
{
  for (size_t i = 0; i < SwapchainImages.size(); ++i) {
    SwapchainImage& image = SwapchainImages[i];
    vkDestroyImageView(device.Native(), image.View, nullptr);
  }

  for (u32 i = 0; i < m_imageAvailableSemas.size(); ++i) {
    vkDestroySemaphore(device.Native(), m_imageAvailableSemas[i], nullptr);
    vkDestroySemaphore(device.Native(), m_graphicsFinishedSemas[i], nullptr);
    vkDestroyFence(device.Native(), m_inFlightFences[i], nullptr);
  }

  if (mSwapchain) {
    vkDestroySwapchainKHR(device.Native(), mSwapchain, nullptr);
    mSwapchain = VK_NULL_HANDLE;
  }
}


void Swapchain::QuerySwapchainImages(LogicalDevice& device)
{
  u32 imageCount;
  vkGetSwapchainImagesKHR(device.Native(), mSwapchain, &imageCount, nullptr);

  std::vector<VkImage> images(imageCount);
  vkGetSwapchainImagesKHR(device.Native(), mSwapchain, &imageCount, images.data());

  if (!SwapchainImages.empty()) {
    for (size_t i = 0; i < SwapchainImages.size(); ++i) {
      vkDestroyImageView(device.Native(), SwapchainImages[i].View, nullptr);  
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
    
    if (vkCreateImageView(device.Native(), &ivInfo, nullptr, &SwapchainImages[i].View) != VK_SUCCESS) {
      R_DEBUG(rError, "Failed to create swapchain image!\n");
      return;
    }
    SwapchainImages[i].Image = images[i];
  }

}


void Swapchain::CreateSemaphores(LogicalDevice& device, u32 count)
{
  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceCi = { };
  fenceCi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
/*
  if (vkCreateSemaphore(handle, &semaphoreCI, nullptr, &mImageAvailableSemaphore) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create a semaphore!\n");
  }

  if (vkCreateSemaphore(handle, &semaphoreCI, nullptr, &mGraphicsFinishedSemaphore) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create a semaphore!\n");
  }
*/
  for (u32 i = 0; i < m_imageAvailableSemas.size(); ++i) {
    vkDestroySemaphore(device.Native(), m_imageAvailableSemas[i], nullptr);
    vkDestroySemaphore(device.Native(), m_graphicsFinishedSemas[i], nullptr);
    vkDestroyFence(device.Native(), m_inFlightFences[i], nullptr);
  }

  m_imageAvailableSemas.resize(count);
  m_graphicsFinishedSemas.resize(count);
  m_inFlightFences.resize(count);
  for (u32 i = 0; i < count; ++i) {
    VkResult result = vkCreateSemaphore(device.Native(), &semaphoreCI, nullptr, &m_imageAvailableSemas[i]);
    R_ASSERT(result == VK_SUCCESS, "");
    result = vkCreateSemaphore(device.Native(), &semaphoreCI, nullptr, &m_graphicsFinishedSemas[i]);
    R_ASSERT(result == VK_SUCCESS, "");
    result = vkCreateFence(device.Native(), &fenceCi, nullptr, &m_inFlightFences[i]);
    R_ASSERT(result == VK_SUCCESS, "");
  }
}
} // Recluse