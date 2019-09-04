// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Utility/Vector.hpp"
#include "Commandbuffer.hpp"
#include "VulkanConfigs.hpp"
#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"

namespace Recluse {


struct SwapchainImage {
  SwapchainImage() 
    : image(VK_NULL_HANDLE)
    , view(VK_NULL_HANDLE) { }
  VkImage                       image;
  VkImageView                   view;
};


// Vulkan Swapchain handler. Handles all information regarding the swapchain of a 
// surface.
class Swapchain {
public:
  Swapchain();
  ~Swapchain();

  void                          initialize(PhysicalDevice& physical, 
                                           LogicalDevice& device, 
                                           VkSurfaceKHR surface, 
                                           VkPresentModeKHR desiredPresent, 
                                           U32 desiredImages = 2);

  void                          cleanUp(LogicalDevice& device);

  VkSwapchainKHR                getHandle() { return m_swapchain; }

  // Current number of images available in the swapchain.
  U32                        getImageCount() const { return static_cast<U32>(m_swapchainImages.size()); }

  SwapchainImage&               get(const size_t index) { return m_swapchainImages[index]; }
  SwapchainImage&               operator[](const size_t index) { return m_swapchainImages[index]; }
  
  // Recreate the swapchain. desiredBuffers specifies how many swapchain images to use for displaying.
  // default is minimum buffers from gpu query, using 0 to signal default swapchain image count.
  void                          reCreate(LogicalDevice& device, 
                                         VkSurfaceKHR surface, 
                                         VkSurfaceFormatKHR format, 
                                         VkPresentModeKHR presentMode, 
                                         VkSurfaceCapabilitiesKHR capabilities, 
                                         U32 desiredImageCount);

  VkSurfaceFormatKHR            getSurfaceFormat() const { return m_currentSurfaceFormat; }
  VkExtent2D                    getSurfaceExtent() const { return m_swapchainExtent; }
  VkPresentModeKHR              getPresentMode() const { return m_currentPresentMode; }
  
private:
  void                          querySwapchainImages(LogicalDevice& device);

  VkSwapchainKHR                m_swapchain;

  VkExtent2D                    m_swapchainExtent;
  VkSurfaceFormatKHR            m_currentSurfaceFormat;
  VkPresentModeKHR              m_currentPresentMode;
  std::vector<SwapchainImage>   m_swapchainImages;

};
} // Recluse