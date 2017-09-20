// Copyright (c) 2017 Recluse Project.
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/ComputePipeline.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Framebuffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Shader.hpp"

#include "Core/Utility/Vector.hpp"

#include "Core/Exception.hpp"

#include <set>
#include <array>

namespace Recluse {


Context                       VulkanRHI::gContext;
PhysicalDevice                VulkanRHI::gPhysicalDevice;
std::vector<const tchar*>     Extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VulkanRHI::VulkanRHI()
  : mWindow(NULL)
  , mSurface(VK_NULL_HANDLE)
  , mCommandPool(VK_NULL_HANDLE)
{
}


VulkanRHI::~VulkanRHI()
{
}


b8 VulkanRHI::CreateContext()
{
#if _DEBUG
  gContext.EnableDebugMode();
#endif
  return gContext.CreateInstance();
}


b8 VulkanRHI::FindPhysicalDevice()
{
  std::vector<VkPhysicalDevice>& devices = gContext.EnumerateGpus();
  for (const auto& device : devices) {
    if (SuitableDevice(device)) {
      VkPhysicalDeviceProperties properties = { };
      vkGetPhysicalDeviceProperties(device, &properties); 
      R_DEBUG("GPU: %s\n", properties.deviceName);
      R_DEBUG("Vendor ID: %d\n", properties.vendorID);
      gPhysicalDevice.Initialize(device);
      break;
    }
  }

  if (gPhysicalDevice.Handle() == VK_NULL_HANDLE) {
    R_DEBUG("ERROR: Failed to find suitable vulkan device!\n");
    return false;
  }

  R_DEBUG("NOTIFY: Vulkan Physical Device found...\n");
  return true;
}


b8 VulkanRHI::SuitableDevice(VkPhysicalDevice device)
{
  std::vector<VkExtensionProperties> availableExtensions = PhysicalDevice::GetExtensionProperties(device);
  std::set<std::string> requiredExtensions(Extensions.begin(), Extensions.end());
  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }
  return requiredExtensions.empty();
}


void VulkanRHI::Initialize(HWND windowHandle)
{
  mSurface = gContext.CreateSurface(windowHandle);
  i32 presentationIndex;
  i32 graphicsIndex;
  i32 computeIndex;
  b8 result = gPhysicalDevice.FindQueueFamilies(mSurface, 
    &presentationIndex, &graphicsIndex, &computeIndex);
  
  if (!result) {
    R_DEBUG("ERROR: Failed to find proper queue families in vulkan context!\n");
    return;
  } 
  
  std::set<i32> queueFamilies = { presentationIndex, graphicsIndex, computeIndex };
  std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
  r32 priority = 1.0f;
  for (i32 queueFamily : queueFamilies) {
    VkDeviceQueueCreateInfo qInfo = { };
    qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qInfo.pQueuePriorities = &priority;
    qInfo.queueCount = 1;
    qInfo.queueFamilyIndex = queueFamily;
    deviceQueueCreateInfos.push_back(qInfo);
  }

  VkPhysicalDeviceFeatures features = gPhysicalDevice.GetFeatures();

  VkDeviceCreateInfo deviceCreate = {};
  deviceCreate.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreate.queueCreateInfoCount = static_cast<u32>(deviceQueueCreateInfos.size());
  deviceCreate.pQueueCreateInfos = deviceQueueCreateInfos.data();
  deviceCreate.enabledExtensionCount = static_cast<u32>(Extensions.size());
  deviceCreate.ppEnabledExtensionNames = Extensions.data();
  deviceCreate.enabledLayerCount = 0;
  deviceCreate.ppEnabledLayerNames = nullptr;
  deviceCreate.pEnabledFeatures = &features;
  if (!mLogicalDevice.Initialize(gPhysicalDevice.Handle(), deviceCreate)) {
    R_DEBUG("ERROR: Vulkan logical device failed to create.\n");
    return;
  }
  
  mSwapchain.Initialize(gPhysicalDevice, mLogicalDevice, mSurface,
    graphicsIndex, presentationIndex, computeIndex);

  // Keep track of the window handle.
  mWindow = windowHandle;

  CreateDepthAttachment();
  SetUpSwapchainRenderPass();
  QueryFromSwapchain();
}


void VulkanRHI::CleanUp()
{

  for (auto& framebuffer : mSwapchainInfo.mSwapchainFramebuffers) {
    vkDestroyFramebuffer(mLogicalDevice.Handle(), framebuffer, nullptr);
  }
  vkDestroyRenderPass(mLogicalDevice.Handle(), mSwapchainInfo.mSwapchainRenderPass, nullptr);

  // NOTE(): Clean up any vulkan modules before destroying the logical device!
  mSwapchain.CleanUp();

  if (mSurface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(gContext.CurrentInstance(), mSurface, nullptr);
    mSurface = VK_NULL_HANDLE;
  }

  mLogicalDevice.CleanUp();
}


void VulkanRHI::QueryFromSwapchain()
{
  if (!mSwapchain.Handle()) {
    R_DEBUG("ERROR: Swapchain failed to initialize! Cannot query images!\n");
    return;
  }
  
  mSwapchainInfo.mSwapchainFramebuffers.resize(mSwapchain.ImageCount());

  for (size_t i = 0; i < mSwapchain.ImageCount(); ++i) {
    VkFramebufferCreateInfo framebufferCI = { };
    std::array<VkImageView, 2> attachments = { mSwapchain[i].View, mSwapchainInfo.mDepthTexture->View() };
    VkExtent2D& extent = mSwapchain.SwapchainExtent();

    framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCI.width = extent.width;
    framebufferCI.height = extent.height;
    framebufferCI.attachmentCount = static_cast<u32>(attachments.size());
    framebufferCI.pAttachments = attachments.data();
    framebufferCI.renderPass = mSwapchainInfo.mSwapchainRenderPass;
    framebufferCI.layers = 1;
    
    vkCreateFramebuffer(mLogicalDevice.Handle(), &framebufferCI, nullptr, 
      &mSwapchainInfo.mSwapchainFramebuffers[i]);
  }
}


void VulkanRHI::CreateDepthAttachment()
{
  
}


void VulkanRHI::SetUpSwapchainRenderPass()
{
  std::array<VkAttachmentDescription, 2> aDs;
  
  // Color description.
  aDs[0].format = mSwapchain.SwapchainFormat();
  aDs[0].samples = VK_SAMPLE_COUNT_1_BIT;
  aDs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  aDs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  aDs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  aDs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  aDs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  aDs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  
  // Depth description.
  aDs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  aDs[1].samples = VK_SAMPLE_COUNT_1_BIT;
}
} // Recluse