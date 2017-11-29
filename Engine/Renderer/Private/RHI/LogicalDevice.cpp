// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/LogicalDevice.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


b8 LogicalDevice::Initialize(const VkPhysicalDevice physical, const VkDeviceCreateInfo& info)
{
  VkResult Result = vkCreateDevice(physical, &info, nullptr, &handle);
  if (Result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create device!\n");
    return false;
  }
  R_DEBUG(rNotify, "Logical device successfully created.\n");
  return true;
}


void LogicalDevice::CleanUp()
{
  if (handle) {
    vkDestroyDevice(handle, nullptr);
    handle = VK_NULL_HANDLE;
  }
}


VkResult LogicalDevice::FlushMappedMemoryRanges(u32 count, const VkMappedMemoryRange* ranges)
{
  return vkFlushMappedMemoryRanges(handle, count, ranges);
}
} // Recluse