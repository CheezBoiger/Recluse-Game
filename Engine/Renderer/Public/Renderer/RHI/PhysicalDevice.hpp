// Copyright (c) 2017 Recluse Project.
#pragma once


#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"
#include "Core/Utility/Vector.hpp"

namespace Recluse {


// Container to hold the handle of our physical device. This object
// serves as a query to the provided physical device.
class PhysicalDevice {
public:

  PhysicalDevice()
    : handle(VK_NULL_HANDLE) { }

  static std::vector<VkExtensionProperties> GetExtensionProperties(VkPhysicalDevice device);
  VkSurfaceCapabilitiesKHR                  QuerySwapchainSurfaceCapabilities(VkSurfaceKHR surface);
  std::vector<VkSurfaceFormatKHR>           QuerySwapchainSurfaceFormats(VkSurfaceKHR surface);
  std::vector<VkPresentModeKHR>             QuerySwapchainPresentModes(VkSurfaceKHR surface);

  b8                                        FindQueueFamilies(VkSurfaceKHR surface,
                                              i32* presentation, i32* graphics, i32* compute);
  u32                                       FindMemoryType(u32 filter, VkMemoryPropertyFlags flags);
  VkPhysicalDeviceFeatures                  GetFeatures();
  void                                      Initialize(VkPhysicalDevice device);
  void                                      CleanUp();
  
  VkPhysicalDevice                          Handle() { return handle; }
private:
  VkPhysicalDevice                          handle;
};
} // Recluse