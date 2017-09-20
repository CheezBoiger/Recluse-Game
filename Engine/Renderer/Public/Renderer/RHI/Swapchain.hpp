// Copyright (c) 2017 Recluse Project.
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

  void                          Initialize(PhysicalDevice& physical, LogicalDevice& device, VkSurfaceKHR surface, 
                                            i32 graphicsIndex, i32 presentationIndex, i32 computeIndex);

  void                          CleanUp();
  void                          WaitOnQueues();

  VkSwapchainKHR                Handle() { return mSwapchain; }


  size_t                        ImageCount() { return SwapchainImages.size(); }

  SwapchainImage&               Get(const size_t index) { return SwapchainImages[index]; }
  SwapchainImage&               operator[](const size_t index) { return SwapchainImages[index]; }
  
  void                          ReCreate(VkSurfaceKHR surface, PhysicalDevice& physical);

  VkFormat                      SwapchainFormat() { return mSwapchainFormat; }
  VkExtent2D                    SwapchainExtent() { return mSwapchainExtent; }

private:
  void                          QuerySwapchainImages();

  VkSwapchainKHR                mSwapchain;
  VkSemaphore                   mSemaphore;
  VkFormat                      mSwapchainFormat;
  VkExtent2D                    mSwapchainExtent;

  VkQueue                       mPresentationQueue;
  VkQueue                       mGraphicsQueue;
  VkQueue                       mComputeQueue;

  i32                           mPresentationQueueIndex;
  i32                           mGraphicsQueueIndex;
  i32                           mComputeQueueIndex;

  std::vector<SwapchainImage>   SwapchainImages;

};
} // Recluse