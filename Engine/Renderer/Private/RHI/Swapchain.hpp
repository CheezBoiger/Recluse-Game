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

  // Current number of images available in the swapchain.
  u32                        ImageCount() const { return static_cast<u32>(SwapchainImages.size()); }

  SwapchainImage&               Get(const size_t index) { return SwapchainImages[index]; }
  SwapchainImage&               operator[](const size_t index) { return SwapchainImages[index]; }
  
  // Recreate the swapchain. desiredBuffers specifies how many swapchain images to use for displaying.
  // default is minimum buffers from gpu query, using 0 to signal default swapchain image count.
  // 2 is double buffered, and 3 is tripled buffered.
  void                          ReCreate(VkSurfaceKHR surface, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode, 
                                  VkSurfaceCapabilitiesKHR capabilities, u32 desiredBuffers = 0);

  VkSurfaceFormatKHR            SwapchainSurfaceFormat() const { return mCurrentSurfaceFormat; }
  VkExtent2D                    SwapchainExtent() const { return mSwapchainExtent; }
  VkPresentModeKHR              CurrentPresentMode() const { return mCurrentPresentMode; }
  u32                           CurrentBufferCount() const { return mCurrentBufferCount; }
  
private:
  void                          QuerySwapchainImages();

  VkSwapchainKHR                mSwapchain;

  VkExtent2D                    mSwapchainExtent;
  u32                           mCurrentBufferCount;
  VkSurfaceFormatKHR            mCurrentSurfaceFormat;
  VkPresentModeKHR              mCurrentPresentMode;
  std::vector<SwapchainImage>   SwapchainImages;

};
} // Recluse