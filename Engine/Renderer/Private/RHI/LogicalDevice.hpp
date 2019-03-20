// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"

#include "PhysicalDevice.hpp"

#include <vector>


namespace Recluse {


// LogicalDevice object, which holds the handle of the native
// device. This object is used to query data and objects from the 
// native logical device.
class LogicalDevice {
public:
  LogicalDevice()
    : handle(VK_NULL_HANDLE)
    , mPresentationQueue(VK_NULL_HANDLE)
    , mImageAvailableSemaphore(VK_NULL_HANDLE)
    , mGraphicsFinishedSemaphore(VK_NULL_HANDLE) { }


  b32 initialize(const VkPhysicalDevice physical, 
                  const VkDeviceCreateInfo& info,
                  QueueFamily* graphics, 
                  QueueFamily* compute, 
                  QueueFamily* transfer, 
                  QueueFamily* presentation);
  void cleanUp();

  // Performs a manual flush of mapped memory writes to make them visible to the host. This
  // should be used if memory written is not coherent between cpu and gpu.
  VkResult FlushMappedMemoryRanges(u32 count, const VkMappedMemoryRange* ranges);

  VkMemoryRequirements getImageMemoryRequirements(const VkImage& image);
  VkMemoryRequirements getBufferMemoryRequirements(const VkBuffer& buffer);
  VkDevice getNative() const { return handle; }

  void                          waitOnQueues();

  VkQueue getPresentQueue() { return mPresentationQueue; }
  VkQueue getGraphicsQueue(size_t i) { return mGraphicsQueues[i]; }
  VkQueue getComputeQueue(size_t i) { return mComputeQueues[i]; }
  VkQueue getTransferQueue(size_t i) { return mTransferQueues[i]; }

  QueueFamily& getPresentQueueFamily() { return mPresentationQueueFamily; }
  QueueFamily& getGraphicsQueueFamily() { return mGraphicsQueueFamily; }
  QueueFamily& getComputeQueueFamily() { return mComputeQueueFamily; }
  QueueFamily& getTransferQueueFamily() { return mTransferQueueFamily; }

  VkFence getDefaultComputeFence() { return mDefaultComputeFence; }

  u32 getGraphicsQueueCount() const { return static_cast<u32>(mGraphicsQueues.size()); }
  u32 getTransferQueueCount() const { return static_cast<u32>(mTransferQueues.size()); }
  u32 getComputeQueueCount() const { return static_cast<u32>(mComputeQueues.size()); }

private:
  VkDevice handle;

  VkQueue mPresentationQueue;
  std::vector<VkQueue> mGraphicsQueues;
  std::vector<VkQueue> mComputeQueues;
  std::vector<VkQueue> mTransferQueues;

  QueueFamily mPresentationQueueFamily;
  QueueFamily mGraphicsQueueFamily;
  QueueFamily mComputeQueueFamily;
  QueueFamily mTransferQueueFamily;

  VkSemaphore mImageAvailableSemaphore;
  VkSemaphore mGraphicsFinishedSemaphore;
  VkFence mDefaultComputeFence;

  void createComputeFence();
};
} // Recluse