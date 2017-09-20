// Copyright (c) 2017 Recluse Project.
#pragma once 

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>


class VulkanHandle {
public:
  VulkanHandle()
    : mOwner(VK_NULL_HANDLE) { }
  virtual ~VulkanHandle() { }

  VkDevice mOwner;
};