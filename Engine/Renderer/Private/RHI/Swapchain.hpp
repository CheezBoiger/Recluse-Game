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
    , getView(VK_NULL_HANDLE) { }
  VkImage                       Image;
  VkImageView                   getView;
};


// Vulkan Swapchain handler. Handles all information regarding the swapchain of a 
// surface.
class Swapchain {
public:
  Swapchain();
  ~Swapchain();

  void                          initialize(PhysicalDevice& physical, LogicalDevice& device, VkSurfaceKHR surface, VkPresentModeKHR desiredPresent, 
                                    u32 buffers = 1, u32 desiredImages = 2);

  void                          cleanUp(LogicalDevice& device);

  VkSwapchainKHR                getHandle() { return mSwapchain; }

  // Current number of images available in the swapchain.
  u32                        ImageCount() const { return static_cast<u32>(SwapchainImages.size()); }

  SwapchainImage&               get(const size_t index) { return SwapchainImages[index]; }
  SwapchainImage&               operator[](const size_t index) { return SwapchainImages[index]; }
  
  // Recreate the swapchain. desiredBuffers specifies how many swapchain images to use for displaying.
  // default is minimum buffers from gpu query, using 0 to signal default swapchain image count.
  void                          ReCreate(LogicalDevice& device, VkSurfaceKHR surface, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode, 
                                  VkSurfaceCapabilitiesKHR capabilities, u32 buffers, u32 desiredImageCount);

  VkSurfaceFormatKHR            SwapchainSurfaceFormat() const { return mCurrentSurfaceFormat; }
  VkExtent2D                    SwapchainExtent() const { return mSwapchainExtent; }
  VkPresentModeKHR              CurrentPresentMode() const { return mCurrentPresentMode; }
  u32                           CurrentBufferCount() const { return mCurrentBufferCount; }

  VkSemaphore                   ImageAvailableSemaphore(u32 idx = 0) { return m_imageAvailableSemas[idx]; }
  VkSemaphore                   GraphicsFinishedSemaphore(u32 idx = 0) { return m_graphicsFinishedSemas[idx]; }
  VkFence                       InFlightFence(u32 idx = 0) { return m_inFlightFences[idx]; }
  
private:
  void                          QuerySwapchainImages(LogicalDevice& device);
  void                          CreateSemaphores(LogicalDevice& device, u32 count);

  VkSwapchainKHR                mSwapchain;

  // Each semaphore corresponds to the number of swapchain images in the swapchain.
  std::vector<VkSemaphore>      m_imageAvailableSemas;
  std::vector<VkSemaphore>      m_graphicsFinishedSemas;
  std::vector<VkFence>          m_inFlightFences;

  VkExtent2D                    mSwapchainExtent;
  u32                           mCurrentBufferCount;
  VkSurfaceFormatKHR            mCurrentSurfaceFormat;
  VkPresentModeKHR              mCurrentPresentMode;
  std::vector<SwapchainImage>   SwapchainImages;

};
} // Recluse