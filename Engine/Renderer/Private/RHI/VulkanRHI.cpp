// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "VulkanRHI.hpp"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "ComputePipeline.hpp"
#include "GraphicsPipeline.hpp"
#include "FrameBuffer.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "DescriptorSet.hpp"

#include "Core/Utility/Vector.hpp"
#include "Core/Utility/Profile.hpp"
#include "Core/Exception.hpp"

#include <set>
#include <array>

namespace Recluse {


Context                       VulkanRHI::gContext;
PhysicalDevice                VulkanRHI::gPhysicalDevice;
std::vector<const tchar*>     Extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


VkPresentModeKHR GetPresentMode(const GraphicsConfigParams* params)
{
  VkPresentModeKHR mode;
  switch (params->_Buffering) {
    case SINGLE_BUFFER: mode = VK_PRESENT_MODE_IMMEDIATE_KHR; break;
    case DOUBLE_BUFFER: mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR; break; 
    case TRIPLE_BUFFER: mode = VK_PRESENT_MODE_MAILBOX_KHR; break;
    default: mode = VK_PRESENT_MODE_FIFO_KHR; break;
  }

  if (params->_EnableVsync >= 1) {
   mode = VK_PRESENT_MODE_FIFO_KHR;
  }

  return mode;
}


Semaphore::~Semaphore()
{
  R_ASSERT(!mSema, "Semaphore was not cleaned up prior to deletion!\n");
}


void Semaphore::Initialize(const VkSemaphoreCreateInfo& info)
{
  if (vkCreateSemaphore(mOwner, &info, nullptr, &mSema) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create semaphore!\n");
  }
}


void Semaphore::CleanUp()
{
  if (mSema) {
    vkDestroySemaphore(mOwner, mSema, nullptr);
    mSema = VK_NULL_HANDLE;
  }
}


void Fence::Initialize(const VkFenceCreateInfo& info)
{
  if (vkCreateFence(mOwner, &info, nullptr, &mFence) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create fence!\n");
  }
}


void Fence::CleanUp()
{
  if (mFence) {
    vkDestroyFence(mOwner, mFence, nullptr);
    mFence = VK_NULL_HANDLE;
  }
}


Fence::~Fence()
{
  R_ASSERT(!mFence, "Fence was not properly destroyed!\n");
}


VulkanRHI::VulkanRHI()
  : mWindow(NULL)
  , mSurface(VK_NULL_HANDLE)
  , mComputeCmdPool(VK_NULL_HANDLE)
  , m_TransferCmdPool(VK_NULL_HANDLE)
  , mDescriptorPool(VK_NULL_HANDLE)
  , mOccQueryPool(VK_NULL_HANDLE)
  , mSwapchainCmdBufferBuild(nullptr)
  , mCurrDescSets(0)
  , m_depthBoundsAllowed(VK_FALSE)
{
  mSwapchainInfo.mComplete = false;
  mSwapchainInfo.mCmdBufferSet = 0;
  mSwapchainInfo.mCmdBufferSets.resize(2);

  mGraphicsCmdPools.resize(2);
}


VulkanRHI::~VulkanRHI()
{
}


b32 VulkanRHI::CreateContext(const char* appName)
{
// Enable debug mode, should we decide to enable validation layers.
#if defined(_DEBUG) || defined(_NDEBUG)
  gContext.EnableDebugMode();
#endif
  return gContext.CreateInstance(appName);
}


b32 VulkanRHI::FindPhysicalDevice()
{
  std::vector<VkPhysicalDevice>& devices = gContext.EnumerateGpus();
  for (const auto& device : devices) {
    if (SuitableDevice(device)) {
      gPhysicalDevice.Initialize(device);
      VkPhysicalDeviceProperties props = gPhysicalDevice.GetDeviceProperties();
      break;
    }
  }

  if (gPhysicalDevice.Handle() == VK_NULL_HANDLE) {
    R_DEBUG(rError, "Failed to find suitable vulkan device!\n");
    return false;
  }

  R_DEBUG(rNotify, "Vulkan Physical Device found...\n");
  return true;
}


b32 VulkanRHI::SuitableDevice(VkPhysicalDevice device)
{
  std::vector<VkExtensionProperties> availableExtensions = PhysicalDevice::GetExtensionProperties(device);
  std::set<std::string> requiredExtensions(Extensions.begin(), Extensions.end());
  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }
  return requiredExtensions.empty();
}


void VulkanRHI::Initialize(HWND windowHandle, const GraphicsConfigParams* params)
{ 
  if (!windowHandle) {
    R_DEBUG(rError, "Renderer can not initialize with a null window handle!\n");
    return;
  }
  // Keep track of the window handle.
  mWindow = windowHandle;

  mSurface = gContext.CreateSurface(windowHandle);

  // TODO(): Instead of just querying queue family indices, have the physical
  // device output queue family indices and the number of queues you can create 
  // with it. Graphics, Compute, and Presentation all share the same queue! Transfer
  // has its own queue.
  QueueFamily presentationQueueFamily;
  QueueFamily graphicsQueueFamily;
  QueueFamily computeQueueFamily;
  QueueFamily transferQueueFamily;

  b8 result = gPhysicalDevice.FindQueueFamilies(mSurface, 
    &presentationQueueFamily, &graphicsQueueFamily, &transferQueueFamily, &computeQueueFamily);
  
  if (!result) {
    R_DEBUG(rError, "Failed to find proper queue families in vulkan context!\n");
    return;
  } 
  
  std::set<QueueFamily, QueueFamilyCompare> queueFamilies = { presentationQueueFamily, 
    graphicsQueueFamily, computeQueueFamily, transferQueueFamily };
  std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
  // Expected 32 queues may be created.
  std::array<r32, 32> priorities = { 1.0f };
  r32 priority = 1.0f;
  r32 priorityFalloff = 1.0f / (priorities.size() / 2);
  for (size_t i = 0; i < priorities.size(); ++i) {
    priorities[i] = priority;
    priority = priority - priorityFalloff;
    priority = priority < 0.0f ? 0.0f : priority;
  }

  for (const QueueFamily& queueFamily : queueFamilies) {
    VkDeviceQueueCreateInfo qInfo = { };
    qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qInfo.pQueuePriorities = priorities.data();
    qInfo.queueCount = queueFamily._queueCount; // Need to change to accomadate more than one queue.
    qInfo.queueFamilyIndex = queueFamily._idx;
    deviceQueueCreateInfos.push_back(qInfo);
  }

  VkPhysicalDeviceFeatures features = gPhysicalDevice.GetFeatures();
  features.samplerAnisotropy = VK_TRUE;
  features.multiViewport = VK_TRUE;
  features.geometryShader = VK_TRUE;
  features.logicOp = VK_TRUE;
  features.alphaToOne = VK_TRUE;
  m_depthBoundsAllowed = features.depthBounds;
  VkDeviceCreateInfo deviceCreate = {};
  deviceCreate.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreate.queueCreateInfoCount = static_cast<u32>(deviceQueueCreateInfos.size());
  deviceCreate.pQueueCreateInfos = deviceQueueCreateInfos.data();
  deviceCreate.enabledExtensionCount = static_cast<u32>(Extensions.size());
  deviceCreate.ppEnabledExtensionNames = Extensions.data();
  deviceCreate.enabledLayerCount = 0;
  deviceCreate.ppEnabledLayerNames = nullptr;
  deviceCreate.pEnabledFeatures = &features;
  if (!mLogicalDevice.Initialize(gPhysicalDevice.Handle(), deviceCreate, mSwapchain.ImageCount(),
      &graphicsQueueFamily, &computeQueueFamily, &transferQueueFamily, &presentationQueueFamily)) {
    R_DEBUG(rError, "Vulkan logical device failed to create.\n");
    return;
  }

  VkPresentModeKHR presentMode = GetPresentMode(params);
  mSwapchain.Initialize(gPhysicalDevice, mLogicalDevice, mSurface, presentMode);

  VkCommandPoolCreateInfo cmdPoolCI = { };
  cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolCI.queueFamilyIndex = static_cast<u32>(mLogicalDevice.GraphicsQueueFamily()._idx);
  cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  
  for (size_t i = 0; i < mGraphicsCmdPools.size(); ++i) {
    if (vkCreateCommandPool(mLogicalDevice.Native(), &cmdPoolCI, nullptr, &mGraphicsCmdPools[i]) != VK_SUCCESS) {
      R_DEBUG(rError, "Failed to create primary command pool!\n");
    } 
  }

  cmdPoolCI.queueFamilyIndex = static_cast<u32>(mLogicalDevice.ComputeQueueFamily()._idx);

  if (vkCreateCommandPool(mLogicalDevice.Native(), &cmdPoolCI, nullptr, &mComputeCmdPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create secondary command pool!\n");
  }

  cmdPoolCI.queueFamilyIndex = static_cast<u32>(mLogicalDevice.TransferQueueFamily()._idx);

  if (vkCreateCommandPool(mLogicalDevice.Native(), &cmdPoolCI, nullptr, &m_TransferCmdPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create secondary command pool!\n");
  }

  VkPhysicalDeviceProperties props = gPhysicalDevice.GetDeviceProperties();
  mPhysicalDeviceProperties = props;

  // Descriptor pool maxes.
  BuildDescriptorPool(UINT16_MAX << 1, UINT16_MAX << 1);
  CreateOcclusionQueryPool(UINT8_MAX);

  CreateDepthAttachment();
  SetUpSwapchainRenderPass();
  QueryFromSwapchain();
}


void VulkanRHI::CleanUp()
{
  mLogicalDevice.WaitOnQueues();

  // Clean up all cmd buffers used for swapchain rendering.
  for (size_t i = 0; i < mSwapchainInfo.mCmdBufferSets.size(); ++i) {
    auto& cmdBufferSet = mSwapchainInfo.mCmdBufferSets[i];
    for (size_t j = 0; j < cmdBufferSet.size(); ++j) {
      CommandBuffer& cmdBuffer = cmdBufferSet[i];
      cmdBuffer.Free();
    }
  }

  for (size_t i = 0; i < mGraphicsCmdPools.size(); ++i) {
    if (mGraphicsCmdPools[i]) {
      vkDestroyCommandPool(mLogicalDevice.Native(), mGraphicsCmdPools[i], nullptr);
      mGraphicsCmdPools[i] = VK_NULL_HANDLE;
    }
  }

  if (mComputeCmdPool) {
    vkDestroyCommandPool(mLogicalDevice.Native(), mComputeCmdPool, nullptr);
    mComputeCmdPool = VK_NULL_HANDLE;
  }

  if (m_TransferCmdPool) {
    vkDestroyCommandPool(mLogicalDevice.Native(), m_TransferCmdPool, nullptr);
    m_TransferCmdPool = VK_NULL_HANDLE;
  }

  if (mDescriptorPool) {
    vkDestroyDescriptorPool(mLogicalDevice.Native(), mDescriptorPool, nullptr);
    mDescriptorPool = VK_NULL_HANDLE;
  }

  if (mOccQueryPool) {
    vkDestroyQueryPool(mLogicalDevice.Native(), mOccQueryPool, nullptr);
    mOccQueryPool = VK_NULL_HANDLE;
  }

  for (auto& framebuffer : mSwapchainInfo.mSwapchainFramebuffers) {
    vkDestroyFramebuffer(mLogicalDevice.Native(), framebuffer, nullptr);
    framebuffer = VK_NULL_HANDLE;
  }

  vkDestroyRenderPass(mLogicalDevice.Native(), mSwapchainInfo.mSwapchainRenderPass, nullptr);
    
  vkDestroyImageView(mLogicalDevice.Native(), mSwapchainInfo.mDepthView, nullptr);
  vkDestroyImage(mLogicalDevice.Native(), mSwapchainInfo.mDepthAttachment, nullptr);
  vkFreeMemory(mLogicalDevice.Native(), mSwapchainInfo.mDepthMemory, nullptr);

  // NOTE(): Clean up any vulkan modules before destroying the logical device!
  mSwapchain.CleanUp();

  if (mSurface != VK_NULL_HANDLE) {
    gContext.DestroySurface(mSurface);
    mSurface = VK_NULL_HANDLE;
  }

  mLogicalDevice.CleanUp();
}


void VulkanRHI::QueryFromSwapchain()
{
  if (!mSwapchain.Handle()) {
    R_DEBUG(rError, "Swapchain failed to initialize! Cannot query images!\n");
    return;
  }
  
  mSwapchainInfo.mSwapchainFramebuffers.resize(mSwapchain.ImageCount());

  for (size_t i = 0; i < mSwapchain.ImageCount(); ++i) {
    VkFramebufferCreateInfo framebufferCI = { };
    std::array<VkImageView, 2> attachments = { mSwapchain[i].View, mSwapchainInfo.mDepthView };
    VkExtent2D& extent = mSwapchain.SwapchainExtent();

    framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCI. width = extent.width;
    framebufferCI.height = extent.height;
    framebufferCI.attachmentCount = static_cast<u32>(attachments.size());
    framebufferCI.pAttachments = attachments.data();
    framebufferCI.renderPass = mSwapchainInfo.mSwapchainRenderPass;
    framebufferCI.layers = 1;
    framebufferCI.flags = 0;
    
    if (vkCreateFramebuffer(mLogicalDevice.Native(), &framebufferCI, nullptr, 
      &mSwapchainInfo.mSwapchainFramebuffers[i]) != VK_SUCCESS) {
      R_DEBUG(rError, "Failed to create framebuffer on swapchain image " 
        + std::to_string(u32(i)) + "!\n");
    }
  }
}


void VulkanRHI::CreateDepthAttachment()
{
  // TODO(): Set up D32_SFLOAT_S8_UINT as well.
  mSwapchainInfo.mDepthFormat = VK_FORMAT_D32_SFLOAT;
  mSwapchainInfo.mDepthAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
  mSwapchainInfo.mDepthUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

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
  imageCI.usage = DepthUsageFlags();
  imageCI.format = DepthFormat();
  imageCI.imageType = VK_IMAGE_TYPE_2D;
  imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if (vkCreateImage(mLogicalDevice.Native(), &imageCI, nullptr, &mSwapchainInfo.mDepthAttachment) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create depth image!\n");
    return;
  }

  VkMemoryRequirements mem;
  vkGetImageMemoryRequirements(mLogicalDevice.Native(), mSwapchainInfo.mDepthAttachment, &mem);
 
  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = mem.size;
  allocInfo.memoryTypeIndex = gPhysicalDevice.FindMemoryType(mem.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  
  if (vkAllocateMemory(mLogicalDevice.Native(), &allocInfo, nullptr, &mSwapchainInfo.mDepthMemory) != VK_SUCCESS) {
    R_DEBUG(rError, "Depth memory was not allocated!\n");
    return;
  }
  vkBindImageMemory(mLogicalDevice.Native(), mSwapchainInfo.mDepthAttachment, mSwapchainInfo.mDepthMemory, 0);
  
  // Now create the depth view.
  VkImageViewCreateInfo ivCI = {};
  ivCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivCI.format = DepthFormat();
  ivCI.image = mSwapchainInfo.mDepthAttachment;
  ivCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ivCI.subresourceRange.aspectMask = DepthAspectFlags();
  ivCI.subresourceRange.baseArrayLayer = 0;
  ivCI.subresourceRange.baseMipLevel = 0;
  ivCI.subresourceRange.layerCount = 1;
  ivCI.subresourceRange.levelCount = 1;
  
  if (vkCreateImageView(mLogicalDevice.Native(), &ivCI, nullptr, &mSwapchainInfo.mDepthView) != VK_SUCCESS) {
    R_DEBUG(rError, "Depth view not created!\n");
  }
 
  
}


void VulkanRHI::SetUpSwapchainRenderPass()
{
  std::array<VkAttachmentDescription, 2> aDs;
  
  // Color description. Our final layout is meant for presentation on a window,
  // if this were a separate framebuffer, we would go with VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.
  aDs[0].format = mSwapchain.SwapchainSurfaceFormat().format;
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
  aDs[1].format = DepthFormat();
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

  if (vkCreateRenderPass(mLogicalDevice.Native(), &renderpassCI, nullptr, &mSwapchainInfo.mSwapchainRenderPass) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create swapchain renderpass!\n");
  }
}


void VulkanRHI::GraphicsSubmit(size_t queueIdx, const u32 count, const VkSubmitInfo* submitInfo, const VkFence fence)
{
  R_TIMED_PROFILE_RENDERER();

  VkResult result = vkQueueSubmit(mLogicalDevice.GraphicsQueue(queueIdx), 
    count, submitInfo, fence);

  DEBUG_OP(
    if (result != VK_SUCCESS) {
      if (result == VK_ERROR_DEVICE_LOST)  {
        R_DEBUG(rWarning, "Vulkan ignoring queue submission! Window possibly minimized?\n");
        return;
      }
      R_DEBUG(rError, "Unsuccessful graphics queue submit!\n");
   }
  );
}


void VulkanRHI::WaitForFences(const u32 fenceCount, const VkFence* pFences, b32 waitAll, const u64 timeout)
{
  vkWaitForFences(mLogicalDevice.Native(), fenceCount, pFences, waitAll, timeout); 
}


void VulkanRHI::ResetFences(const u32 fenceCount, const VkFence* pFences)
{
  vkResetFences(mLogicalDevice.Native(), fenceCount, pFences);
}


Fence* VulkanRHI::CreateVkFence()
{
  Fence* fence = new Fence();
  fence->SetOwner(mLogicalDevice.Native());
  return fence;
}


void VulkanRHI::FreeVkFence(Fence* fence)
{
  if (!fence) return;
  fence->CleanUp();
  delete fence;
}


void VulkanRHI::AcquireNextImage()
{
  R_TIMED_PROFILE_RENDERER();

  VkResult result = vkAcquireNextImageKHR(mLogicalDevice.Native(), mSwapchain.Handle(), UINT64_MAX,
    mLogicalDevice.ImageAvailableSemaphore(), VK_NULL_HANDLE, &mSwapchainInfo.mCurrentImageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    VkExtent2D windowExtent = mSwapchain.SwapchainExtent();
    ReConfigure(mSwapchain.CurrentPresentMode(), windowExtent.width, 
      windowExtent.height, mSwapchain.CurrentBufferCount());
  }
}


void VulkanRHI::SubmitCurrSwapchainCmdBuffer(u32 waitSemaphoreCount, VkSemaphore* waitSemaphores,
  u32 signalSemaphoreCount, VkSemaphore* signalSemaphores, VkFence fence)
{
  if (!signalSemaphores || !signalSemaphoreCount) {
    R_DEBUG(rError, "Fatal! You must specify at least one signal semaphore when calling SubmitCurrSwapchainCmdBuffer (GraphicsFinishedSemaphore() if rendering is complete!)");
    return;
  }

  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  
  VkCommandBuffer cmdBuffer[] = { 
    mSwapchainInfo.mCmdBufferSets[CurrentSwapchainCmdBufferSet()][mSwapchainInfo.mCurrentImageIndex].Handle() 
  };

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = cmdBuffer;
  submitInfo.signalSemaphoreCount = signalSemaphoreCount;
  submitInfo.pSignalSemaphores = signalSemaphores;
  submitInfo.waitSemaphoreCount = waitSemaphoreCount;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  GraphicsSubmit(0, 1, &submitInfo, fence);
}


void VulkanRHI::Present()
{
  VkSemaphore signalSemaphores[] = { mLogicalDevice.GraphicsFinishedSemaphore() };
  VkSwapchainKHR swapchains[] = { mSwapchain.Handle() };

  VkPresentInfoKHR presentInfo = { };
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pResults = nullptr;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.pImageIndices = &mSwapchainInfo.mCurrentImageIndex;

  if (vkQueuePresentKHR(mLogicalDevice.PresentQueue(), &presentInfo) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to present!\n");
  }
}


void VulkanRHI::ComputeSubmit(size_t queueIdx, const VkSubmitInfo& submitInfo, const VkFence fence)
{
  if (vkQueueSubmit(mLogicalDevice.ComputeQueue(queueIdx), 1, &submitInfo, fence) != VK_SUCCESS) {
    R_DEBUG(rError, "Compute failed to submit task!\n");
  }
}


void VulkanRHI::TransferSubmit(size_t queueIdx, const u32 count, const VkSubmitInfo* submitInfo, const VkFence fence)
{
  VkResult result = vkQueueSubmit(mLogicalDevice.TransferQueue(queueIdx), count, submitInfo, fence);
  if (result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to submit to transfer queue!\n");
  }
}


void VulkanRHI::TransferWaitIdle(size_t queueIdx)
{
  vkQueueWaitIdle(mLogicalDevice.TransferQueue(queueIdx));
}


void VulkanRHI::GraphicsWaitIdle(size_t queueIdx)
{
  vkQueueWaitIdle(mLogicalDevice.GraphicsQueue(queueIdx));
}


void VulkanRHI::ComputeWaitIdle(size_t queueIdx)
{
  vkQueueWaitIdle(mLogicalDevice.ComputeQueue(queueIdx));
}


void VulkanRHI::PresentWaitIdle()
{
  vkQueueWaitIdle(mLogicalDevice.PresentQueue());
}


void VulkanRHI::DeviceWaitIdle()
{
  vkDeviceWaitIdle(mLogicalDevice.Native());
}

void VulkanRHI::WaitAllGraphicsQueues()
{
  for (size_t i = 0; i < LogicDevice()->GraphicsQueueFamily()._queueCount; ++i) {
    GraphicsWaitIdle(i);
  }
}


void VulkanRHI::CreateSwapchainCommandBuffers(u32 set)
{
  auto& cmdBufferSet = mSwapchainInfo.mCmdBufferSets[set];
  cmdBufferSet.resize(mSwapchainInfo.mSwapchainFramebuffers.size());

  for (size_t i = 0; i < cmdBufferSet.size(); ++i) {
    CommandBuffer& cmdBuffer = cmdBufferSet[i];
    cmdBuffer.SetOwner(mLogicalDevice.Native());
    cmdBuffer.Allocate(mGraphicsCmdPools[0], VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    
    VkCommandBufferBeginInfo cmdBufferBI = { };
    cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBI.flags= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    cmdBuffer.Begin(cmdBufferBI);
      VkRenderPassBeginInfo rpBI = {};
      VkClearValue clearValues[2];

      clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
      clearValues[1].depthStencil = { 1.0f, 0 };

      rpBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      rpBI.framebuffer = mSwapchainInfo.mSwapchainFramebuffers[i];
      rpBI.renderPass = mSwapchainInfo.mSwapchainRenderPass;
      rpBI.renderArea.extent = mSwapchain.SwapchainExtent();
      rpBI.renderArea.offset = { 0, 0 };
      rpBI.clearValueCount = 2;
      rpBI.pClearValues = clearValues;

      if (mSwapchainCmdBufferBuild) { 
        mSwapchainCmdBufferBuild(cmdBuffer, rpBI);
      }
    cmdBuffer.End();
  }
  mSwapchainInfo.mComplete = true;
}


void VulkanRHI::RebuildCommandBuffers(u32 set)
{
  auto& cmdBufferSet = mSwapchainInfo.mCmdBufferSets[set];
  for (size_t i = 0; i < cmdBufferSet.size(); ++i) {
    CommandBuffer& cmdBuffer = cmdBufferSet[i];
    cmdBuffer.Free();
  }
  mSwapchainInfo.mComplete = false;
  CreateSwapchainCommandBuffers(set);
}


void VulkanRHI::ReConfigure(VkPresentModeKHR presentMode, i32 width, i32 height, u32 desiredBuffers)
{
  if (width <= 0 || height <= 0) return;

  for (size_t i = 0; i < mSwapchainInfo.mSwapchainFramebuffers.size(); ++i) {
    VkFramebuffer framebuffer = mSwapchainInfo.mSwapchainFramebuffers[i];
    vkDestroyFramebuffer(mLogicalDevice.Native(), framebuffer, nullptr);
  }

  vkDestroyRenderPass(mLogicalDevice.Native(), mSwapchainInfo.mSwapchainRenderPass, nullptr);

  vkDestroyImageView(mLogicalDevice.Native(), mSwapchainInfo.mDepthView, nullptr);
  vkDestroyImage(mLogicalDevice.Native(), mSwapchainInfo.mDepthAttachment, nullptr);
  vkFreeMemory(mLogicalDevice.Native(), mSwapchainInfo.mDepthMemory, nullptr);

  std::vector<VkSurfaceFormatKHR> surfaceFormats = gPhysicalDevice.QuerySwapchainSurfaceFormats(mSurface);
  VkSurfaceCapabilitiesKHR capabilities = gPhysicalDevice.QuerySwapchainSurfaceCapabilities(mSurface);

  mSwapchain.ReCreate(mSurface, surfaceFormats[0], presentMode, capabilities, desiredBuffers);
  
  CreateDepthAttachment();
  SetUpSwapchainRenderPass();
  QueryFromSwapchain();
}


Buffer* VulkanRHI::CreateBuffer()
{ 
  Buffer* buffer = new Buffer();
  buffer->SetOwner(mLogicalDevice.Native());

  return buffer;
}


void VulkanRHI::FreeBuffer(Buffer* buffer)
{
  if (!buffer) return;

  if (buffer->Owner() != mLogicalDevice.Native()) {
    R_DEBUG(rNotify, "Unable to free buffer. Device is not same as this vulkan rhi!\n");
    return;
  }

  buffer->CleanUp();

  delete buffer;
}


GraphicsPipeline* VulkanRHI::CreateGraphicsPipeline()
{
  GraphicsPipeline* pipeline = new GraphicsPipeline();
  pipeline->SetOwner(mLogicalDevice.Native());
  return pipeline;
}


void VulkanRHI::FreeGraphicsPipeline(GraphicsPipeline* pipeline)
{
  if (!pipeline) return ;

  if (pipeline->Owner() != mLogicalDevice.Native()) {
    R_DEBUG(rNotify, "Unable to free pipeline. Device is not same as this vulkan rhi!\n");
    return;
  }
  pipeline->CleanUp();
 
  delete pipeline;
}



Shader* VulkanRHI::CreateShader()
{
  Shader* shader = new Shader();
  shader->SetOwner(mLogicalDevice.Native());
  
  return shader;
}


void VulkanRHI::FreeShader(Shader* shader)
{
  if (!shader) return;

  if (shader->Owner() != mLogicalDevice.Native()) {
    R_DEBUG(rNotify, "Unable to free shader. Device is not same as this vulkan rhi!\n");
    return;
  }

  shader->CleanUp();
  
  delete shader;
}


ImageView* VulkanRHI::CreateImageView(const VkImageViewCreateInfo& viewCi)
{
  ImageView* pView = new ImageView();
  pView->Initialize(mLogicalDevice.Native(), viewCi);
  return pView;
}


void VulkanRHI::FreeImageView(ImageView* pView)
{
  if ( !pView ) return;
  pView->CleanUp(mLogicalDevice.Native());
  delete pView;
}


DescriptorSet* VulkanRHI::CreateDescriptorSet()
{
  DescriptorSet* dset = new DescriptorSet();
  dset->SetOwner(mLogicalDevice.Native());
  mCurrDescSets += 1;
  return dset;
}


void VulkanRHI::FreeDescriptorSet(DescriptorSet* dset)
{
  if (!dset) return;

  dset->Free();
  
  delete dset;
  mCurrDescSets -= 1;
}


Sampler* VulkanRHI::CreateSampler()
{
  Sampler* sampler = new Sampler();
  sampler->SetOwner(mLogicalDevice.Native());
  
  return sampler;
}


void VulkanRHI::FreeSampler(Sampler* sampler)
{
  if (!sampler) return;

  sampler->CleanUp();
  
  delete sampler;
}


Texture* VulkanRHI::CreateTexture()
{
  Texture* texture = new Texture();
  texture->SetOwner(mLogicalDevice.Native());
  return texture;
}


void VulkanRHI::FreeTexture(Texture* texture)
{
  if (!texture) return;
  texture->CleanUp();
  
  delete texture;
}


FrameBuffer* VulkanRHI::CreateFrameBuffer()
{
  FrameBuffer* framebuffer = new FrameBuffer();
  framebuffer->SetOwner(mLogicalDevice.Native());

  return framebuffer;
}


void VulkanRHI::FreeFrameBuffer(FrameBuffer* framebuffer)
{
  if (!framebuffer) return;

  framebuffer->CleanUp();
  
  delete framebuffer;
}


CommandBuffer* VulkanRHI::CreateCommandBuffer()
{
  CommandBuffer* buffer = new CommandBuffer();
  buffer->SetOwner(mLogicalDevice.Native());
  
  return buffer;
}


void VulkanRHI::FreeCommandBuffer(CommandBuffer* buffer)
{
  if (!buffer) return;

  buffer->Free();
  delete buffer;
}


DescriptorSetLayout* VulkanRHI::CreateDescriptorSetLayout()
{
  DescriptorSetLayout* layout = new DescriptorSetLayout();
  layout->SetOwner(mLogicalDevice.Native());
  return layout;
}


void VulkanRHI::FreeDescriptorSetLayout(DescriptorSetLayout* layout)
{
  if (!layout) return;

  layout->CleanUp();

  delete layout;
}


ComputePipeline* VulkanRHI::CreateComputePipeline()
{
  ComputePipeline* pipeline = new ComputePipeline();
  pipeline->SetOwner(mLogicalDevice.Native());

  return pipeline;
}


void VulkanRHI::FreeComputePipeline(ComputePipeline* pipeline)
{
  if (!pipeline) return;

  pipeline->CleanUp();

  delete pipeline;
}


Semaphore* VulkanRHI::CreateVkSemaphore()
{
  Semaphore* semaphore = new Semaphore();
  semaphore->SetOwner(mLogicalDevice.Native());
  
  return semaphore;
}


void  VulkanRHI::FreeVkSemaphore(Semaphore* semaphore)
{
  if (!semaphore) return;

  semaphore->CleanUp();
    
  delete semaphore;
}


RenderPass* VulkanRHI::CreateRenderPass()
{
  RenderPass* renderPass = new RenderPass();
  renderPass->SetOwner(mLogicalDevice.Native());
  return renderPass;
}


void VulkanRHI::FreeRenderPass(RenderPass* renderPass)
{
  if (!renderPass) return;
  renderPass->CleanUp();
  delete renderPass;
}


void VulkanRHI::BuildDescriptorPool(u32 maxCount, u32 maxSets)
{
  if (mDescriptorPool) {
    vkDestroyDescriptorPool(mLogicalDevice.Native(), mDescriptorPool, nullptr);
    mDescriptorPool = VK_NULL_HANDLE;
  }

  std::array<VkDescriptorPoolSize, 4> poolSizes;
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = maxCount;
  
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = maxCount;

  poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[2].descriptorCount = 16;

  poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[3].descriptorCount = 16;

  VkDescriptorPoolCreateInfo descriptorPoolCI = { };
  descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCI.poolSizeCount = static_cast<u32>(poolSizes.size());
  descriptorPoolCI.pPoolSizes = poolSizes.data();
  descriptorPoolCI.maxSets = maxSets;
  descriptorPoolCI.pNext = nullptr;
  descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  if (vkCreateDescriptorPool(mLogicalDevice.Native(), &descriptorPoolCI, nullptr, &mDescriptorPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to created descriptor pool!\n");
  }
}


void VulkanRHI::CreateOcclusionQueryPool(u32 queries)
{
  VkQueryPoolCreateInfo queryCi = { };
  queryCi.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  queryCi.queryCount = queries;
  queryCi.queryType = VK_QUERY_TYPE_OCCLUSION;
  
  if (vkCreateQueryPool(mLogicalDevice.Native(), &queryCi, nullptr, &mOccQueryPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create occlusion query pool!\n");
  } 
}
} // Recluse