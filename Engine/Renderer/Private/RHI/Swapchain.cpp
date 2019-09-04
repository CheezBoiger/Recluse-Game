// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Swapchain.hpp"
#include "Core/Exception.hpp"
#include <limits>

namespace Recluse {


Swapchain::Swapchain()
  : m_swapchain(VK_NULL_HANDLE)
{
}


Swapchain::~Swapchain()
{
}


void Swapchain::initialize(PhysicalDevice& physical, LogicalDevice& device, VkSurfaceKHR surface, 
      VkPresentModeKHR desiredPresent, U32 desiredImages)
{

  std::vector<VkPresentModeKHR> presentModes = physical.querySwapchainPresentModes(surface);
  std::vector<VkSurfaceFormatKHR> surfaceFormats = physical.querySwapchainSurfaceFormats(surface);
  VkSurfaceCapabilitiesKHR capabilities = physical.querySwapchainSurfaceCapabilities(surface);

  VkPresentModeKHR desiredPresentMode = desiredPresent;

  // Don't know why, but let's remove this for now...
  //for (VkPresentModeKHR mode : presentModes) {
  //  if (mode == desiredPresent) {
  //    desiredPresentMode = mode;
  //  }
  //}

  reCreate(device, surface, surfaceFormats[0], desiredPresentMode, capabilities, desiredImages);
}


void Swapchain::reCreate(LogicalDevice& device, VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat, 
  VkPresentModeKHR presentMode, VkSurfaceCapabilitiesKHR capabilities, U32 desiredImages)
{
  VkSwapchainKHR oldSwapChain = m_swapchain;

  U32 imageCount = capabilities.minImageCount;
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
  
  if (vkCreateSwapchainKHR(device.getNative(), &sInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create swapchain!\n");
  }

  R_DEBUG(rNotify, "Swapchain creation succeeded.\n");

  // Storing data of swapchains and querying images.
  m_swapchainExtent = capabilities.currentExtent;
  m_currentSurfaceFormat = surfaceFormat;
  m_currentPresentMode = presentMode;

  if (oldSwapChain) {
    vkDestroySwapchainKHR(device.getNative(), oldSwapChain, nullptr);
  }

  querySwapchainImages(device);
}


void Swapchain::cleanUp(LogicalDevice& device)
{
  for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
    SwapchainImage& image = m_swapchainImages[i];
    vkDestroyImageView(device.getNative(), image.view, nullptr);
  }

  if (m_swapchain) {
    vkDestroySwapchainKHR(device.getNative(), m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
  }
}


void Swapchain::querySwapchainImages(LogicalDevice& device)
{
  U32 imageCount;
  vkGetSwapchainImagesKHR(device.getNative(), m_swapchain, &imageCount, nullptr);

  std::vector<VkImage> images(imageCount);
  vkGetSwapchainImagesKHR(device.getNative(), m_swapchain, &imageCount, images.data());

  if (!m_swapchainImages.empty()) {
    for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
      vkDestroyImageView(device.getNative(), m_swapchainImages[i].view, nullptr);  
    }
  }

  m_swapchainImages.resize(images.size());
  for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
    VkImageViewCreateInfo ivInfo = { };
    ivInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivInfo.format = m_currentSurfaceFormat.format;
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
    
    if (vkCreateImageView(device.getNative(), &ivInfo, nullptr, &m_swapchainImages[i].view) != VK_SUCCESS) {
      R_DEBUG(rError, "Failed to create swapchain image!\n");
      return;
    }
    m_swapchainImages[i].image = images[i];
  }

}
} // Recluse