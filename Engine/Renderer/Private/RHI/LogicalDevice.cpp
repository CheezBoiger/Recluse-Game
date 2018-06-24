// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/LogicalDevice.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


b32 LogicalDevice::Initialize(const VkPhysicalDevice physical, const VkDeviceCreateInfo& info,
  u32 swapchainImageCount,
  QueueFamily* graphics, QueueFamily* compute, QueueFamily* transfer, QueueFamily* presentation)
{
  VkResult Result = vkCreateDevice(physical, &info, nullptr, &handle);
  if (Result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create device!\n");
    return false;
  }
  R_DEBUG(rNotify, "Logical device successfully created.\n");

  R_DEBUG(rNotify, "Creating queues.\n");
  mGraphicsQueueFamily = *graphics;
  mComputeQueueFamily = *compute;
  mTransferQueueFamily = *transfer;
  mPresentationQueueFamily = *presentation;

  mGraphicsQueues.resize(mGraphicsQueueFamily._queueCount);
  mComputeQueues.resize(mComputeQueueFamily._queueCount);
  mTransferQueues.resize(mTransferQueueFamily._queueCount);

  // TODO(): Read initialize() call from vulkan context, queue index will change in the future.
  for (size_t i = 0; i < mGraphicsQueueFamily._queueCount; ++i) {
    vkGetDeviceQueue(handle, mGraphicsQueueFamily._idx, 0, &mGraphicsQueues[i]);
  }
  for (size_t i = 0; i < mComputeQueueFamily._queueCount; ++i) {
    vkGetDeviceQueue(handle, mComputeQueueFamily._idx, 0, &mComputeQueues[i]);
  }

  for (size_t i = 0; i < mTransferQueueFamily._queueCount; ++i) {
    vkGetDeviceQueue(handle, mTransferQueueFamily._idx, 0, &mTransferQueues[i]);
  }
  vkGetDeviceQueue(handle, mPresentationQueueFamily._idx, 0, &mPresentationQueue);

  R_DEBUG(rNotify, "Queues created.\n");
  CreateSemaphores(swapchainImageCount);
  CreateComputeFence();
  return true;
}


void LogicalDevice::CleanUp()
{
  WaitOnQueues();
  if (mImageAvailableSemaphore) {
    vkDestroySemaphore(handle, mImageAvailableSemaphore, nullptr);
    mImageAvailableSemaphore = VK_NULL_HANDLE;
  }

  if (mGraphicsFinishedSemaphore) {
    vkDestroySemaphore(handle, mGraphicsFinishedSemaphore, nullptr);
    mGraphicsFinishedSemaphore = VK_NULL_HANDLE;
  }

  if (mDefaultComputeFence) {
    vkDestroyFence(handle, mDefaultComputeFence, nullptr);
    mDefaultComputeFence = VK_NULL_HANDLE;
  }

  if (handle) {
    vkDestroyDevice(handle, nullptr);
    handle = VK_NULL_HANDLE;
  }
}


VkResult LogicalDevice::FlushMappedMemoryRanges(u32 count, const VkMappedMemoryRange* ranges)
{
  return vkFlushMappedMemoryRanges(handle, count, ranges);
}


void LogicalDevice::WaitOnQueues()
{
  vkQueueWaitIdle(mPresentationQueue);

  for (VkQueue& queue : mGraphicsQueues) {
    vkQueueWaitIdle(queue);
  }
  
  for (VkQueue& queue : mComputeQueues) {
    vkQueueWaitIdle(queue);
  }

  for (VkQueue& queue : mTransferQueues) {
    vkQueueWaitIdle(queue);
  }
}


void LogicalDevice::CreateSemaphores(u32 count)
{
  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if (vkCreateSemaphore(handle, &semaphoreCI, nullptr, &mImageAvailableSemaphore) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create a semaphore!\n");
  }

  if (vkCreateSemaphore(handle, &semaphoreCI, nullptr, &mGraphicsFinishedSemaphore) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create a semaphore!\n");
  }
}


void LogicalDevice::CreateComputeFence()
{
  VkFenceCreateInfo fenceCI = {};
  fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateFence(handle, &fenceCI, nullptr, &mDefaultComputeFence) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create a semaphore!\n");
  }
}
} // Recluse