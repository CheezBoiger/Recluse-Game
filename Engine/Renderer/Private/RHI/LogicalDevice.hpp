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


  b32                   Initialize(const VkPhysicalDevice physical, const VkDeviceCreateInfo& info,
                          QueueFamily* graphics, QueueFamily* compute, QueueFamily* transfer, QueueFamily* presentation);
  void                  CleanUp();

  // Performs a manual flush of mapped memory writes to make them visible to the host. This
  // should be used if memory written is not coherent between cpu and gpu.
  VkResult              FlushMappedMemoryRanges(u32 count, const VkMappedMemoryRange* ranges);

  VkMemoryRequirements          GetImageMemoryRequirements(const VkImage& image);
  VkMemoryRequirements          GetBufferMemoryRequirements(const VkBuffer& buffer);
  VkDevice                      Native() const { return handle; }

  void                          WaitOnQueues();

  VkQueue                       PresentQueue() { return mPresentationQueue; }
  VkQueue                       GraphicsQueue(size_t i) { return mGraphicsQueues[i]; }
  VkQueue                       ComputeQueue(size_t i) { return mComputeQueues[i]; }
  VkQueue                       TransferQueue(size_t i) { return mTransferQueues[i]; }

  QueueFamily&                  PresentQueueFamily() { return mPresentationQueueFamily; }
  QueueFamily&                  GraphicsQueueFamily() { return mGraphicsQueueFamily; }
  QueueFamily&                  ComputeQueueFamily() { return mComputeQueueFamily; }
  QueueFamily&                  TransferQueueFamily() { return mTransferQueueFamily; }

  VkFence                       DefaultComputeFence() { return mDefaultComputeFence; }

  u32                           GraphicsQueueCount() const { return static_cast<u32>(mGraphicsQueues.size()); }
  u32                           TransferQueueCount() const { return static_cast<u32>(mTransferQueues.size()); }
  u32                           ComputeQueueCount() const { return static_cast<u32>(mComputeQueues.size()); }

private:
  VkDevice              handle;

  VkQueue                       mPresentationQueue;
  std::vector<VkQueue>          mGraphicsQueues;
  std::vector<VkQueue>          mComputeQueues;
  std::vector<VkQueue>          mTransferQueues;

  QueueFamily                   mPresentationQueueFamily;
  QueueFamily                   mGraphicsQueueFamily;
  QueueFamily                   mComputeQueueFamily;
  QueueFamily                   mTransferQueueFamily;

  VkSemaphore                   mImageAvailableSemaphore;
  VkSemaphore                   mGraphicsFinishedSemaphore;
  VkFence                       mDefaultComputeFence;

  void                          CreateComputeFence();
};
} // Recluse