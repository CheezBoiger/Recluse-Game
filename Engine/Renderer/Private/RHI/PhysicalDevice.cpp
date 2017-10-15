// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/PhysicalDevice.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


std::vector<VkExtensionProperties> PhysicalDevice::GetExtensionProperties(VkPhysicalDevice physical)
{
  u32 extensionCount;
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount, availableExtensions.data());
  return availableExtensions;
}


b8 PhysicalDevice::FindQueueFamilies(VkSurfaceKHR surface,
    i32* presentation, i32* graphics, i32* compute) const
{
  if (!handle) {
    R_DEBUG("ERROR: No handle is set to query queue families from!\n");
    return false;
  }
  u32 familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(handle, &familyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(handle, &familyCount, queueFamilies.data());

  i32 i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      *graphics = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(handle, i, surface, &presentSupport);
    if (queueFamily.queueCount > 0 && presentSupport) {
      *presentation = i;
    }

    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      *compute = i;
    }

    ++i;
  }
  return true;
}


VkSurfaceCapabilitiesKHR PhysicalDevice::QuerySwapchainSurfaceCapabilities(VkSurfaceKHR surface) const
{
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handle, surface, &capabilities);
  return capabilities;
}


std::vector<VkSurfaceFormatKHR> PhysicalDevice::QuerySwapchainSurfaceFormats(VkSurfaceKHR surface) const
{
  std::vector<VkSurfaceFormatKHR> formats;
  u32 formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(handle, surface, &formatCount, nullptr);
  formats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(handle, surface, &formatCount, formats.data());
  return formats;
}


std::vector<VkPresentModeKHR> PhysicalDevice::QuerySwapchainPresentModes(VkSurfaceKHR surface) const
{
  std::vector<VkPresentModeKHR> presentModes;
  u32 presentCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(handle, surface, &presentCount, nullptr);
  presentModes.resize(presentCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(handle, surface, &presentCount, presentModes.data());
  return presentModes;
}


VkPhysicalDeviceFeatures PhysicalDevice::GetFeatures() const
{
  return { };
}


u32 PhysicalDevice::FindMemoryType(u32 filter, VkMemoryPropertyFlags flags) const
{ 
  for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
    if ((filter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
      return i;
    }
  }

  return -1;
}


void PhysicalDevice::Initialize(VkPhysicalDevice device)
{
  handle = device;
  vkGetPhysicalDeviceMemoryProperties(handle, &memoryProperties);
  vkGetPhysicalDeviceProperties(handle, &properties);
}


void PhysicalDevice::CleanUp()
{
  if (handle) {
  }
}
} // Recluse