// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/VulkanContext.hpp"
#include "Core/Exception.hpp"

// This is win32 specific.
std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };


const std::vector<const char*> validationLayers = {
  "VK_LAYER_LUNARG_standard_validation"
};

graphics_uuid_t VulkanHandle::uuid = 0;

namespace Recluse {


static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
  VkDebugReportFlagsEXT flags,
  VkDebugReportObjectTypeEXT objType,
  u64 obj,
  size_t location,
  i32 code,
  const char* layerPrefix,
  const char* msg,
  void* usrData)
{
  printf("Validation layer: %s\n", msg);
  return VK_FALSE;
}


b32 Context::CreateInstance()
{
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pApplicationName = "Recluse";
  appInfo.pEngineName = ENGINE_NAME;

  VkInstanceCreateInfo instCreateInfo = {};
  instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instCreateInfo.pApplicationInfo = &appInfo;

  if (mDebugEnabled) {
    instCreateInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
    instCreateInfo.ppEnabledLayerNames = validationLayers.data();
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  }
  else {
    instCreateInfo.enabledLayerCount = 0;
    instCreateInfo.ppEnabledLayerNames = nullptr;
  }

  instCreateInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
  instCreateInfo.ppEnabledExtensionNames = extensions.data();
  VkResult result = vkCreateInstance(&instCreateInfo, nullptr, &mInstance);

  if (result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create Vulkan instance!\n");
    return false;
  }

  if (mDebugEnabled) SetUpDebugCallback();

  R_DEBUG(rNotify, "Vulkan Instance created...\n");
  return true;
}


void Context::CleanUp()
{
  if (mInstance) {
    if (mDebugEnabled) CleanUpDebugCallback();
    vkDestroyInstance(mInstance, nullptr);
  }
}


std::vector<VkPhysicalDevice>& Context::EnumerateGpus()
{
  u32 deviceCount = 0;
  vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);


  mGpus.resize(deviceCount);
  vkEnumeratePhysicalDevices(mInstance, &deviceCount, mGpus.data());
  return mGpus;
}


VkSurfaceKHR Context::CreateSurface(HWND handle)
{
  VkSurfaceKHR surface;
  // Windows specific.
  VkWin32SurfaceCreateInfoKHR cInfo;
  cInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  cInfo.hinstance = GetModuleHandle(NULL);
  cInfo.hwnd = handle;
  cInfo.flags = 0;
  cInfo.pNext = nullptr;

  PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)
    vkGetInstanceProcAddr(mInstance, "vkCreateWin32SurfaceKHR");

  if (!vkCreateWin32SurfaceKHR) {
    R_DEBUG(rError, "Failed to proc address for vkCreateWin32SurfaceKHR.\n");
    return VK_NULL_HANDLE;
  }

  if (vkCreateWin32SurfaceKHR(mInstance, &cInfo, nullptr, &surface) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create win32 surface...\n");
    return VK_NULL_HANDLE;
  } 

  R_DEBUG(rNotify, "Win32 surface successfully created and attached...\n");
  return surface;
}


void Context::DestroySurface(VkSurfaceKHR surface)
{
  vkDestroySurfaceKHR(mInstance, surface, nullptr);
}


void Context::EnableDebugMode()
{
  u32 count;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  std::vector<VkLayerProperties> layerPropertiesVector(count);
  vkEnumerateInstanceLayerProperties(&count, layerPropertiesVector.data());

  for (const char* layerName : validationLayers) {
    b32 layerFound = false;
    for (const auto& layerProperties : layerPropertiesVector) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        mDebugEnabled = true;
        break;
      }
    }
    if (!layerFound) {
      R_DEBUG(rWarning, "A validation layer was not found!\n");
    }
  }
}


void Context::SetUpDebugCallback()
{
  VkDebugReportCallbackCreateInfoEXT ci = { };
  ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  ci.pfnCallback = DebugCallback;

  auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) 
    vkGetInstanceProcAddr(mInstance, "vkCreateDebugReportCallbackEXT");
  if (vkCreateDebugReportCallbackEXT == nullptr) {
    R_DEBUG(rError, "Failed to find debug report callback function create!\n");
    return;
  }

  if (vkCreateDebugReportCallbackEXT(mInstance, &ci, nullptr, &mDebugReportCallback) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create debug report callback.\n");
    return;
  }
}


void Context::CleanUpDebugCallback()
{
  auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)
    vkGetInstanceProcAddr(mInstance, "vkDestroyDebugReportCallbackEXT");
  if (vkDestroyDebugReportCallbackEXT == nullptr) {
    R_DEBUG(rError, "Failed to find vkDestroyDebugReportCallbackEXT!\n");
    return;
  }

  vkDestroyDebugReportCallbackEXT(mInstance, mDebugReportCallback, nullptr);
}
} // Recluse 