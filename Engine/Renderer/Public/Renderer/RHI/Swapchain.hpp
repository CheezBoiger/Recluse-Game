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
    : Image(VK_NULL_HANDLE)
    , View(VK_NULL_HANDLE) { }
  VkImage                       Image;
  VkImageView                   View;
};


// Vulkan Swapchain handler. Handles all information regarding the swapchain of a 
// surface.
class Swapchain : public VulkanHandle {
public:
  Swapchain();
  ~Swapchain();

  void                          Initialize(PhysicalDevice& physical, LogicalDevice& device, VkSurfaceKHR surface, VkPresentModeKHR desiredPresent);

  void                          CleanUp();

  VkSwapchainKHR                Handle() { return mSwapchain; }


  size_t                        ImageCount() const { return SwapchainImages.size(); }

  SwapchainImage&               Get(const size_t index) { return SwapchainImages[index]; }
  SwapchainImage&               operator[](const size_t index) { return SwapchainImages[index]; }
  
  void                          ReCreate(VkSurfaceKHR surface, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode, 
                                  VkSurfaceCapabilitiesKHR capabilities);

  VkFormat                      SwapchainFormat() const { return mSwapchainFormat; }
  VkExtent2D                    SwapchainExtent() const { return mSwapchainExtent; }
  VkPresentModeKHR              CurrentPresentMode() const { return mCurrentPresentMode; }
  
private:
  void                          QuerySwapchainImages();

  VkSwapchainKHR                mSwapchain;

  VkFormat                      mSwapchainFormat;
  VkExtent2D                    mSwapchainExtent;
  VkPresentModeKHR              mCurrentPresentMode;
  std::vector<SwapchainImage>   SwapchainImages;

};
} // Recluse