// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"
#include "Core/Utility/Vector.hpp"

namespace Recluse {


struct QueueFamily {
  i32             _idx;
  u32             _queueCount;
};


struct QueueFamilyCompare {
  constexpr bool operator()(const QueueFamily &lhs, const QueueFamily &rhs) const
  {
    return lhs._idx < rhs._idx;
  }
};

// Container to hold the handle of our physical device. This object
// serves as a query to the provided physical device.
class PhysicalDevice {
public:

  PhysicalDevice()
    : handle(VK_NULL_HANDLE) { }

  static std::vector<VkExtensionProperties> GetExtensionProperties(VkPhysicalDevice device);
  VkSurfaceCapabilitiesKHR                  QuerySwapchainSurfaceCapabilities(VkSurfaceKHR surface) const;
  std::vector<VkSurfaceFormatKHR>           QuerySwapchainSurfaceFormats(VkSurfaceKHR surface) const;
  std::vector<VkPresentModeKHR>             QuerySwapchainPresentModes(VkSurfaceKHR surface) const;

  b32                                       FindQueueFamilies(VkSurfaceKHR surface,
                                              QueueFamily* presentation, QueueFamily* graphics, 
                                              QueueFamily* transfer, QueueFamily* compute) const;

  u32                                       FindMemoryType(u32 filter, VkMemoryPropertyFlags flags) const;
  VkResult                                  GetImageFormatProperties(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage,
                                              VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties) const;
  VkPhysicalDeviceFeatures                  GetFeatures() const;
  VkPhysicalDeviceProperties                GetDeviceProperties() const { return properties; }
  VkPhysicalDeviceMemoryProperties          GetMemoryProperties() const { return memoryProperties; }
  void                                      Initialize(VkPhysicalDevice device);
  void                                      CleanUp();
  
  VkPhysicalDevice                          Handle() const { return handle; }
private:
  VkPhysicalDevice                          handle;
  VkPhysicalDeviceMemoryProperties          memoryProperties;
  VkPhysicalDeviceProperties                properties;
};
} // Recluse