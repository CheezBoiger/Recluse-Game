// Copyright (c) 2017 Recluse Project.
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/ComputePipeline.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Framebuffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Rendertarget.hpp"
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
  mSwapchain.WaitOnQueues();

  for (auto& framebuffer : mSwapchainInfo.mSwapchainFramebuffers) {
    vkDestroyFramebuffer(mLogicalDevice.Handle(), framebuffer, nullptr);
  }
  vkDestroyRenderPass(mLogicalDevice.Handle(), mSwapchainInfo.mSwapchainRenderPass, nullptr);
  
  vkDestroyImageView(mLogicalDevice.Handle(), mSwapchainInfo.mDepthView, nullptr);
  vkDestroyImage(mLogicalDevice.Handle(), mSwapchainInfo.mDepthAttachment, nullptr);
  vkFreeMemory(mLogicalDevice.Handle(), mSwapchainInfo.mDepthMemory, nullptr);

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
    std::array<VkImageView, 2> attachments = { mSwapchain[i].View, mSwapchainInfo.mDepthView };
    VkExtent2D& extent = mSwapchain.SwapchainExtent();

    framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCI.width = extent.width;
    framebufferCI.height = extent.height;
    framebufferCI.attachmentCount = static_cast<u32>(attachments.size());
    framebufferCI.pAttachments = attachments.data();
    framebufferCI.renderPass = mSwapchainInfo.mSwapchainRenderPass;
    framebufferCI.layers = 1;
    
    if (vkCreateFramebuffer(mLogicalDevice.Handle(), &framebufferCI, nullptr, 
      &mSwapchainInfo.mSwapchainFramebuffers[i]) != VK_SUCCESS) {
      R_DEBUG("ERROR: Failed to create framebuffer on swapchain image %d!\n", u32(i));
    }
  }
}


void VulkanRHI::CreateDepthAttachment()
{
  VkImageCreateInfo imageCI = { };
  imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCI.extent.width = mSwapchain.SwapchainExtent().width;
  imageCI.extent.height = mSwapchain.SwapchainExtent().height;
  imageCI.extent.depth = 1;
  imageCI.mipLevels = 1;
  imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCI.arrayLayers = 1;
  imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
  imageCI.imageType = VK_IMAGE_TYPE_2D;
  imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if (vkCreateImage(mLogicalDevice.Handle(), &imageCI, nullptr, &mSwapchainInfo.mDepthAttachment) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create depth image!\n");
    return;
  }

  VkMemoryRequirements mem;
  vkGetImageMemoryRequirements(mLogicalDevice.Handle(), mSwapchainInfo.mDepthAttachment, &mem);
 
  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = mem.size;
  allocInfo.memoryTypeIndex = gPhysicalDevice.FindMemoryType(mem.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  
  if (vkAllocateMemory(mLogicalDevice.Handle(), &allocInfo, nullptr, &mSwapchainInfo.mDepthMemory) != VK_SUCCESS) {
    R_DEBUG("ERROR: Depth memory was not allocated!\n");
    return;
  }
  vkBindImageMemory(mLogicalDevice.Handle(), mSwapchainInfo.mDepthAttachment, mSwapchainInfo.mDepthMemory, 0);
  
  // Now create the depth view.
  VkImageViewCreateInfo ivCI = {};
  ivCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
  ivCI.image = mSwapchainInfo.mDepthAttachment;
  ivCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ivCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  ivCI.subresourceRange.baseArrayLayer = 0;
  ivCI.subresourceRange.baseMipLevel = 0;
  ivCI.subresourceRange.layerCount = 1;
  ivCI.subresourceRange.levelCount = 1;
  
  if (vkCreateImageView(mLogicalDevice.Handle(), &ivCI, nullptr, &mSwapchainInfo.mDepthView) != VK_SUCCESS) {
    R_DEBUG("ERROR: Depth view not created!\n");
  }
 
  
}


void VulkanRHI::SetUpSwapchainRenderPass()
{
  std::array<VkAttachmentDescription, 2> aDs;
  
  // Color description. Our final layout is meant for presentation on a window,
  // if this were a separate framebuffer, we would go with VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.
  aDs[0].format = mSwapchain.SwapchainFormat();
  aDs[0].samples = VK_SAMPLE_COUNT_1_BIT;
  aDs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  aDs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  aDs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  aDs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  aDs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  aDs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  aDs[0].flags = 0;
  
  // Depth description.
  aDs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  aDs[1].samples = VK_SAMPLE_COUNT_1_BIT;
  aDs[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
  aDs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  aDs[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  
  aDs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  aDs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  aDs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  aDs[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  aDs[1].flags = 0;

  VkAttachmentReference colorRef = { };
  colorRef.attachment = 0;
  colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthRef = { };
  depthRef.attachment = 1;
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // TODO(): Do we want MSAA in our renderer?
  VkSubpassDescription subpass = { };
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;  
  subpass.pDepthStencilAttachment = &depthRef;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  // Image layout transitions are handled implicitly, but we have two (before and after)
  // dependencies that dont get properly transitioned (images are written before they are obtained).
  // so we need to explicitly define barriers.
  std::array<VkSubpassDependency, 2> dependencies;
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderpassCI = { };
  renderpassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpassCI.attachmentCount = static_cast<u32>(aDs.size());
  renderpassCI.pAttachments = aDs.data();
  renderpassCI.subpassCount = 1;
  renderpassCI.pSubpasses = &subpass;
  renderpassCI.dependencyCount = static_cast<u32>(dependencies.size());
  renderpassCI.pDependencies = dependencies.data();

  if (vkCreateRenderPass(mLogicalDevice.Handle(), &renderpassCI, nullptr, &mSwapchainInfo.mSwapchainRenderPass) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create swapchain renderpass!\n");
  }
}
} // Recluse