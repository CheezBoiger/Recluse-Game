// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>


#define NVIDIA_VENDOR_ID        0x10DE
#define INTEL_VENDOR_ID         0x1F96
#define AMD_VENDOR_ID           0x1022

class VulkanHandle {
public:
  VulkanHandle()
    : mOwner(VK_NULL_HANDLE) { }

  void      SetOwner(VkDevice owner) { mOwner = owner; }
  VkDevice  Owner() { return mOwner; }

protected:
  VkDevice  mOwner;
};