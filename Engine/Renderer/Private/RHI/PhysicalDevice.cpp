// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "PhysicalDevice.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


std::vector<VkExtensionProperties> PhysicalDevice::getExtensionProperties(VkPhysicalDevice physical)
{
  u32 extensionCount;
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount, availableExtensions.data());
  return availableExtensions;
}


b32 PhysicalDevice::findQueueFamilies(VkSurfaceKHR surface,
                                      QueueFamily* presentation, 
                                      QueueFamily* graphics, 
                                      QueueFamily* transfer, 
                                      QueueFamily* compute) const
{
  if (!m_handle) {
    R_DEBUG(rError, "No handle is set to query queue families from!\n");
    return false;
  }
  u32 familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &familyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &familyCount, queueFamilies.data());

  i32 i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphics->_idx = i;
      graphics->_queueCount = queueFamily.queueCount;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_handle, i, surface, &presentSupport);
    if (queueFamily.queueCount > 0 && presentSupport) {
      presentation->_idx = i;
      presentation->_queueCount = queueFamily.queueCount;
    }

    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      compute->_idx = i;
      compute->_queueCount = queueFamily.queueCount;
    }

    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      transfer->_idx = i;
      transfer->_queueCount = queueFamily.queueCount;
    }

    ++i;
  }
  return true;
}


VkSurfaceCapabilitiesKHR PhysicalDevice::querySwapchainSurfaceCapabilities(VkSurfaceKHR surface) const
{
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_handle, surface, &capabilities);
  return capabilities;
}


std::vector<VkSurfaceFormatKHR> PhysicalDevice::querySwapchainSurfaceFormats(VkSurfaceKHR surface) const
{
  std::vector<VkSurfaceFormatKHR> formats;
  u32 formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle, surface, &formatCount, nullptr);
  formats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle, surface, &formatCount, formats.data());
  return formats;
}


std::vector<VkPresentModeKHR> PhysicalDevice::querySwapchainPresentModes(VkSurfaceKHR surface) const
{
  std::vector<VkPresentModeKHR> presentModes;
  u32 presentCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_handle, surface, &presentCount, nullptr);
  presentModes.resize(presentCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_handle, surface, &presentCount, presentModes.data());
  return presentModes;
}


VkPhysicalDeviceFeatures PhysicalDevice::getFeatures() const
{
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(m_handle, &features);
  return features;
}


u32 PhysicalDevice::findMemoryType(u32 filter, PhysicalDeviceMemoryUsage usage) const
{ 
  VkMemoryPropertyFlags required = 0;
  VkMemoryPropertyFlags preferred = 0;
  u32 index = 0xffffffff;

  switch (usage) {
    case PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY:
    {
      required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      preferred |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    } break;
    case PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY:
    {
      preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    } break;
    case PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU:
    {
      required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    } break;
    case PHYSICAL_DEVICE_MEMORY_USAGE_GPU_TO_CPU:
    {
      required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      preferred |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    } break;
    default:
      break;
  };

  for (u32 i = 0; i < m_memoryProperties.memoryTypeCount; ++i) {
    const VkMemoryPropertyFlags propFlags = m_memoryProperties.memoryTypes[i].propertyFlags;
    if ((filter & (1 << i))) {
      if ((propFlags & required) == required && ((propFlags & preferred) == preferred)) {
        index = i;
      } else if ((propFlags & required) == required) {
        index = i;
      }
    }
  }

  return index;
}


VkResult PhysicalDevice::getImageFormatProperties(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, 
  VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties) const
{
  return vkGetPhysicalDeviceImageFormatProperties(m_handle, format, type, tiling, usage, flags, pImageFormatProperties);
}


void PhysicalDevice::initialize(VkPhysicalDevice device)
{
  m_handle = device;
  vkGetPhysicalDeviceMemoryProperties(m_handle, &m_memoryProperties);

  m_maintenanceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
  m_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  m_properties.pNext = &m_maintenanceProperties;
  vkGetPhysicalDeviceProperties2(m_handle, &m_properties);
}


void PhysicalDevice::cleanUp()
{
  if (m_handle) {
  }
}
} // Recluse