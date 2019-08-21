// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"
#include "Core/Utility/Vector.hpp"

namespace Recluse {


struct QueueFamily {
  I32             _idx;
  U32             _queueCount;
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
    : m_handle(VK_NULL_HANDLE) { }

  static std::vector<VkExtensionProperties> getExtensionProperties(VkPhysicalDevice device);
  VkSurfaceCapabilitiesKHR querySwapchainSurfaceCapabilities(VkSurfaceKHR surface) const;
  std::vector<VkSurfaceFormatKHR> querySwapchainSurfaceFormats(VkSurfaceKHR surface) const;
  std::vector<VkPresentModeKHR> querySwapchainPresentModes(VkSurfaceKHR surface) const;

  B32 findQueueFamilies(VkSurfaceKHR surface,
                        QueueFamily* presentation, 
                        QueueFamily* graphics, 
                        QueueFamily* transfer, 
                        QueueFamily* compute) const;

  U32 findMemoryType(U32 filter, PhysicalDeviceMemoryUsage usage) const;

  VkResult getImageFormatProperties(VkFormat format, 
                                    VkImageType type, 
                                    VkImageTiling tiling, 
                                    VkImageUsageFlags usage,
                                    VkImageCreateFlags flags, 
                                    VkImageFormatProperties* pImageFormatProperties) const;

  VkPhysicalDeviceFeatures getFeatures() const;
  VkPhysicalDeviceProperties getDeviceProperties() const { return m_properties.properties; }
  VkPhysicalDeviceMaintenance3Properties getMaintenanceProperties() const { return m_maintenanceProperties; }
  VkPhysicalDeviceMemoryProperties getMemoryProperties() const { return m_memoryProperties; }
  void initialize(VkPhysicalDevice device);
  void cleanUp();
  
  VkPhysicalDevice handle() const { return m_handle; }
private:
  VkPhysicalDevice m_handle;
  VkPhysicalDeviceMemoryProperties m_memoryProperties;
  VkPhysicalDeviceProperties2 m_properties;
  VkPhysicalDeviceMaintenance3Properties m_maintenanceProperties;
};
} // Recluse