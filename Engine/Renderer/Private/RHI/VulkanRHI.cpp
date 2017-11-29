// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/CommandBuffer.hpp"
#include "RHI/ComputePipeline.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/FrameBuffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Shader.hpp"
#include "RHI//DescriptorSet.hpp"

#include "Core/Utility/Vector.hpp"

#include "Core/Exception.hpp"

#include <set>
#include <array>

namespace Recluse {


Context                       VulkanRHI::gContext;
PhysicalDevice                VulkanRHI::gPhysicalDevice;
std::vector<const tchar*>     Extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


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


VulkanRHI::VulkanRHI()
  : mWindow(NULL)
  , mSurface(VK_NULL_HANDLE)
  , mCmdPool(VK_NULL_HANDLE)
  , mComputeCmdPool(VK_NULL_HANDLE)
  , mDescriptorPool(VK_NULL_HANDLE)
  , mSwapchainCmdBufferBuild(nullptr)
  , mCurrDescSets(0)
{
  mSwapchainInfo.mComplete = false;
  mSwapchainInfo.mCmdBufferSet = 0;
  mSwapchainInfo.mCmdBufferSets.resize(2);
}


VulkanRHI::~VulkanRHI()
{
}


b8 VulkanRHI::CreateContext()
{
// Enable debug mode, should we decide to enable validation layers.
#if defined(_DEBUG) || defined(_NDEBUG)
  gContext.EnableDebugMode();
#endif
  return gContext.CreateInstance();
}


b8 VulkanRHI::FindPhysicalDevice()
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
  if (!windowHandle) {
    R_DEBUG(rError, "Renderer can not initialize with a null window handle!\n");
    return;
  }
  // Keep track of the window handle.
  mWindow = windowHandle;

  mSurface = gContext.CreateSurface(windowHandle);
  i32 presentationIndex;
  i32 graphicsIndex;
  i32 computeIndex;
  b8 result = gPhysicalDevice.FindQueueFamilies(mSurface, 
    &presentationIndex, &graphicsIndex, &computeIndex);
  
  if (!result) {
    R_DEBUG(rError, "Failed to find proper queue families in vulkan context!\n");
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
  features.samplerAnisotropy = VK_TRUE;
  features.multiViewport = VK_TRUE;
  features.geometryShader = VK_TRUE;

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
    R_DEBUG(rError, "Vulkan logical device failed to create.\n");
    return;
  }
  
  mSwapchain.Initialize(gPhysicalDevice, mLogicalDevice, mSurface,
    graphicsIndex, presentationIndex, computeIndex);

  VkCommandPoolCreateInfo cmdPoolCI = { };
  cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolCI.queueFamilyIndex = static_cast<u32>(mSwapchain.GraphicsIndex());
  cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  
  if (vkCreateCommandPool(mLogicalDevice.Native(), &cmdPoolCI, nullptr, &mCmdPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create primary command pool!\n");
  } 

  cmdPoolCI.queueFamilyIndex = static_cast<u32>(mSwapchain.ComputeIndex());

  if (vkCreateCommandPool(mLogicalDevice.Native(), &cmdPoolCI, nullptr, &mComputeCmdPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create secondary command pool!\n");
  }

  VkPhysicalDeviceProperties props = gPhysicalDevice.GetDeviceProperties();
  mPhysicalDeviceProperties = props;

  // Descriptor pool maxes.
  BuildDescriptorPool(UINT16_MAX, UINT16_MAX);

  CreateDepthAttachment();
  SetUpSwapchainRenderPass();
  QueryFromSwapchain();
}


void VulkanRHI::CleanUp()
{
  mSwapchain.WaitOnQueues();

  // Clean up all cmd buffers used for swapchain rendering.
  for (size_t i = 0; i < mSwapchainInfo.mCmdBufferSets.size(); ++i) {
    auto& cmdBufferSet = mSwapchainInfo.mCmdBufferSets[i];
    for (size_t j = 0; j < cmdBufferSet.size(); ++j) {
      CommandBuffer& cmdBuffer = cmdBufferSet[i];
      cmdBuffer.Free();
    }
  }

  if (mCmdPool) {
    vkDestroyCommandPool(mLogicalDevice.Native(), mCmdPool, nullptr);
  }

  if (mComputeCmdPool) {
    vkDestroyCommandPool(mLogicalDevice.Native(), mComputeCmdPool, nullptr);
  }

  if (mDescriptorPool) {
    vkDestroyDescriptorPool(mLogicalDevice.Native(), mDescriptorPool, nullptr);
    mDescriptorPool = VK_NULL_HANDLE;
  }

  for (auto& framebuffer : mSwapchainInfo.mSwapchainFramebuffers) {
    vkDestroyFramebuffer(mLogicalDevice.Native(), framebuffer, nullptr);
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
    
    if (vkCreateFramebuffer(mLogicalDevice.Native(), &framebufferCI, nullptr, 
      &mSwapchainInfo.mSwapchainFramebuffers[i]) != VK_SUCCESS) {
      R_DEBUG(rError, "Failed to create framebuffer on swapchain image " 
        + std::to_string(u32(i)) + "!\n");
    }
  }
}


void VulkanRHI::CreateDepthAttachment()
{
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


void VulkanRHI::GraphicsSubmit(const VkSubmitInfo& submitInfo)
{
  VkResult result = vkQueueSubmit(mSwapchain.GraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  if (result != VK_SUCCESS) {
    if (result == VK_ERROR_DEVICE_LOST)  {
      R_DEBUG(rWarning, "Vulkan ignoring queue submission! Window possibly minimized?\n");
      return;
    }
    R_DEBUG(rError, "Unsuccessful graphics queue submit!\n");
  }
}


void VulkanRHI::AcquireNextImage()
{
  vkAcquireNextImageKHR(mLogicalDevice.Native(), mSwapchain.Handle(), UINT_MAX,
    mSwapchain.ImageAvailableSemaphore(), VK_NULL_HANDLE, &mSwapchainInfo.mCurrentImageIndex);
}


void VulkanRHI::SubmitCurrSwapchainCmdBuffer(u32 waitSemaphoreCount, VkSemaphore* waitSemaphores)
{
  // TODO(): This must be set outside in the renderer scope, instead of in here. 
  // more freedom for offscreen rendering that way.
  VkSemaphore signalSemaphores[] = { mSwapchain.GraphicsFinishedSemaphore() };
  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  
  VkCommandBuffer cmdBuffer[] = { 
    mSwapchainInfo.mCmdBufferSets[CurrentSwapchainCmdBufferSet()][mSwapchainInfo.mCurrentImageIndex].Handle() 
  };

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = cmdBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  submitInfo.waitSemaphoreCount = waitSemaphoreCount;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  GraphicsSubmit(submitInfo);
}


void VulkanRHI::Present()
{
  VkSemaphore signalSemaphores[] = { mSwapchain.GraphicsFinishedSemaphore() };
  VkSwapchainKHR swapchains[] = { mSwapchain.Handle() };

  VkPresentInfoKHR presentInfo = { };
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pResults = nullptr;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.pImageIndices = &mSwapchainInfo.mCurrentImageIndex;

  if (vkQueuePresentKHR(mSwapchain.PresentQueue(), &presentInfo) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to present!\n");
  }
}


void VulkanRHI::ComputeSubmit(const VkSubmitInfo& submitInfo)
{
  VkFence fence = mSwapchain.ComputeFence();
  vkWaitForFences(mLogicalDevice.Native(), 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(mLogicalDevice.Native(), 1, &fence);

  if (vkQueueSubmit(mSwapchain.ComputeQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
    R_DEBUG(rError, "Compute failed to submit task!\n");
  }
}


void VulkanRHI::GraphicsWaitIdle()
{
  vkQueueWaitIdle(mSwapchain.GraphicsQueue());
}


void VulkanRHI::ComputeWaitIdle()
{
  vkQueueWaitIdle(mSwapchain.ComputeQueue());
}


void VulkanRHI::PresentWaitIdle()
{
  vkQueueWaitIdle(mSwapchain.PresentQueue());
}


void VulkanRHI::DeviceWaitIdle()
{
  vkDeviceWaitIdle(mLogicalDevice.Native());
}


void VulkanRHI::CreateSwapchainCommandBuffers(u32 set)
{
  auto& cmdBufferSet = mSwapchainInfo.mCmdBufferSets[set];
  cmdBufferSet.resize(mSwapchainInfo.mSwapchainFramebuffers.size());

  for (size_t i = 0; i < cmdBufferSet.size(); ++i) {
    CommandBuffer& cmdBuffer = cmdBufferSet[i];
    cmdBuffer.SetOwner(mLogicalDevice.Native());
    cmdBuffer.Allocate(mCmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    
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


void VulkanRHI::ReConfigure(VkPresentModeKHR presentMode, i32 width, i32 height)
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

  mSwapchain.ReCreate(mSurface, surfaceFormats[0], presentMode, capabilities);
  
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


void VulkanRHI::BuildDescriptorPool(u32 maxCount, u32 maxSets)
{
  if (mDescriptorPool) {
    vkDestroyDescriptorPool(mLogicalDevice.Native(), mDescriptorPool, nullptr);
    mDescriptorPool = VK_NULL_HANDLE;
  }

  std::array<VkDescriptorPoolSize, 3> poolSizes;
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = maxCount;
  
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = maxCount;

  poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[2].descriptorCount = maxCount;

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
} // Recluse