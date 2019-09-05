// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "VulkanRHI.hpp"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "ComputePipeline.hpp"
#include "GraphicsPipeline.hpp"
#include "FrameBuffer.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "UserParams.hpp"
#include "DescriptorSet.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Utility/Profile.hpp"
#include "Core/Exception.hpp"

#include <set>
#include <array>

namespace Recluse {


Context                       VulkanRHI::gContext;
PhysicalDevice                VulkanRHI::gPhysicalDevice;
VulkanMemoryAllocatorManager  VulkanRHI::gAllocator;
std::vector<const TChar*>     gExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


Semaphore::~Semaphore()
{
  R_ASSERT(!mSema, "Semaphore was not cleaned up prior to deletion!\n");
}


void Semaphore::initialize(const VkSemaphoreCreateInfo& info)
{
  VkResult result = vkCreateSemaphore(mOwner, &info, nullptr, &mSema);
  R_ASSERT(result == VK_SUCCESS, "Failed to init semaphore.\n");
  R_DEBUG(rNotify, "Semaphore Handle created: " << mSema << " \n");
}


void Semaphore::cleanUp()
{
  if (mSema) {
    R_DEBUG(rNotify, "Semphore object " << mSema << " destroyed successfully.\n");
    vkDestroySemaphore(mOwner, mSema, nullptr);
    mSema = VK_NULL_HANDLE;
  }
}


void Fence::initialize(const VkFenceCreateInfo& info)
{
  VkResult result = vkCreateFence(mOwner, &info, nullptr, &mFence);
  R_ASSERT(result == VK_SUCCESS, "Failed to init fence.\n");
}


void Fence::cleanUp()
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
  : m_windowHandle(NULL)
  , mSurface(VK_NULL_HANDLE)
  , mComputeCmdPool(VK_NULL_HANDLE)
  , m_TransferCmdPool(VK_NULL_HANDLE)
  , mDescriptorPool(VK_NULL_HANDLE)
  , mOccQueryPool(VK_NULL_HANDLE)
  , mSwapchainCmdBufferBuild(nullptr)
  , mCurrDescSets(0)
  , m_depthBoundsAllowed(VK_FALSE)
  , m_bStencilTestAllowed(VK_FALSE)
  , m_currentFrame(0)
{
  mSwapchainInfo.mComplete = false;
  mSwapchainInfo.mCmdBufferSet = 0;
  mSwapchainInfo.mCurrentImageIndex = 0;

  mGraphicsCmdPools.resize(4);
}


VulkanRHI::~VulkanRHI()
{
}


B32 VulkanRHI::createContext(const char* appName)
{
// setEnable debug mode, should we decide to enable validation layers.
#if defined(_DEBUG) || defined(_NDEBUG)
  gContext.enableDebugMode();
#endif
  return gContext.createInstance(appName);
}


std::set<std::string> VulkanRHI::getMissingExtensions(VkPhysicalDevice device)
{
  std::vector<VkExtensionProperties> availableExtensions = PhysicalDevice::getExtensionProperties(device);
  std::set<std::string> requiredExtensions(gExtensions.begin(), gExtensions.end());
  Log(rDebug) << "Supported extensions: \n";
  for (const auto& extension : availableExtensions) {
    Log(rDebug) << extension.extensionName << "\n";
    requiredExtensions.erase(extension.extensionName);
  }
  return requiredExtensions;
}


B32 VulkanRHI::findPhysicalDevice(U32 rhiBits)
{
  std::vector<VkPhysicalDevice>& devices = gContext.enumerateGpus();
#if VK_NV_ray_tracing
  if (rhiBits & R_RAYTRACING_BIT) gExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
  if (rhiBits & R_MESHSHADER_BIT) gExtensions.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);
#else
  if (rhiBits & R_RAYTRACING_BIT) R_DEBUG(rWarning, "Raytracing is not available for this version of vulkan. Consider updating.\n");
  if (rhiBits & R_MESHSHADER_BIT) R_DEBUG(rWarning, "Mesh shading not avaiable for this version of vulkan. Consider updating.\n");
#endif
#if VK_NV_fragment_shader_barycentric
  if (rhiBits & R_PIXELBARECENTRIC_BIT) gExtensions.push_back(VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
#else
  if (rhiBits & R_PIXELBARECENTRIC_BIT) R_DEBUG(rWarning, "Fragment shader barycentric extension not available for this version of vulkan. Consider updating.\n");
#endif
  VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
  {
    B32 bestScore = 0;
    for (const auto& device : devices) {
      B32 score = suitableDevice(device);
      if (score > bestScore) {
        bestDevice = device;
      }
    }
  }

  if (bestDevice) {
    gPhysicalDevice.initialize(bestDevice);
    VkPhysicalDeviceProperties props = gPhysicalDevice.getDeviceProperties();
  }

  if (gPhysicalDevice.handle() == VK_NULL_HANDLE) {
    R_DEBUG(rError, "Failed to find suitable vulkan device!\n");
    return false;
  }

  R_DEBUG(rNotify, "Vulkan Physical Device found...\n");
  return true;
}


B32 VulkanRHI::suitableDevice(VkPhysicalDevice device)
{
  VkPhysicalDeviceFeatures features = {};
  VkPhysicalDeviceProperties props = {};
  U32 score = 0;
  std::vector<VkExtensionProperties> availableExtensions = PhysicalDevice::getExtensionProperties(device);
  std::set<std::string> requiredExtensions = getMissingExtensions(device);

  vkGetPhysicalDeviceFeatures(device, &features);
  vkGetPhysicalDeviceProperties(device, &props);

  if (!requiredExtensions.empty()) return 0;
  score += 1;
  if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    score += 100;
  }
  return score;
}


void VulkanRHI::initialize(HWND windowHandle,
                           VkPresentModeKHR presentMode,
                           U32 frameResourceBuffers,
                           U32 desiredImageCount) { 
  if (!windowHandle) {
    R_DEBUG(rError, "Renderer can not initialize with a null window handle!\n");
    return;
  }

  // Keep track of the window handle.
  m_windowHandle = windowHandle;

  mSurface = gContext.createSurface(windowHandle);

  // TODO(): Instead of just querying queue family indices, have the physical
  // device output queue family indices and the number of queues you can create 
  // with it. Graphics, Compute, and Presentation all share the same queue! Transfer
  // has its own queue.
  QueueFamily presentationQueueFamily;
  QueueFamily graphicsQueueFamily;
  QueueFamily computeQueueFamily;
  QueueFamily transferQueueFamily;

  B8 result = gPhysicalDevice.findQueueFamilies(mSurface, 
    &presentationQueueFamily, &graphicsQueueFamily, &transferQueueFamily, &computeQueueFamily);
  
  if (!result) {
    R_DEBUG(rError, "Failed to find proper queue families in vulkan context!\n");
    return;
  } 
  
  std::set<QueueFamily, QueueFamilyCompare> queueFamilies = { presentationQueueFamily, 
    graphicsQueueFamily, computeQueueFamily, transferQueueFamily };
  std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
  // Expected 32 queues may be created.
  std::array<R32, 32> priorities = { 1.0f };
  R32 priority = 1.0f;
  R32 priorityFalloff = 1.0f / (priorities.size() / 2);
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

  VkPhysicalDeviceFeatures features = gPhysicalDevice.getFeatures();
  features.samplerAnisotropy = VK_TRUE;
  features.multiViewport = VK_TRUE;
  features.geometryShader = VK_TRUE;
  features.logicOp = VK_TRUE;
  features.alphaToOne = VK_TRUE;
  m_depthBoundsAllowed = features.depthBounds;
  VkDeviceCreateInfo deviceCreate = {};
  deviceCreate.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreate.queueCreateInfoCount = static_cast<U32>(deviceQueueCreateInfos.size());
  deviceCreate.pQueueCreateInfos = deviceQueueCreateInfos.data();
  deviceCreate.enabledExtensionCount = static_cast<U32>(gExtensions.size());
  deviceCreate.ppEnabledExtensionNames = gExtensions.data();
  deviceCreate.enabledLayerCount = 0;
  deviceCreate.ppEnabledLayerNames = nullptr;
  deviceCreate.pEnabledFeatures = &features;
  if (!mLogicalDevice.initialize(gPhysicalDevice.handle(), deviceCreate,
      &graphicsQueueFamily, &computeQueueFamily, &transferQueueFamily, &presentationQueueFamily)) {
    R_DEBUG(rError, "Vulkan logical device failed to create.\n");
    return;
  }

  m_swapchain.initialize(gPhysicalDevice, mLogicalDevice, mSurface, presentMode, desiredImageCount);

  VkCommandPoolCreateInfo cmdPoolCI = { };
  cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolCI.queueFamilyIndex = static_cast<U32>(mLogicalDevice.getGraphicsQueueFamily()._idx);
  cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  
  for (size_t i = 0; i < mGraphicsCmdPools.size(); ++i) {
    if (vkCreateCommandPool(mLogicalDevice.getNative(), &cmdPoolCI, nullptr, &mGraphicsCmdPools[i]) != VK_SUCCESS) {
      R_DEBUG(rError, "Failed to create primary command pool!\n");
    } 
  }

  cmdPoolCI.queueFamilyIndex = static_cast<U32>(mLogicalDevice.getComputeQueueFamily()._idx);

  if (vkCreateCommandPool(mLogicalDevice.getNative(), &cmdPoolCI, nullptr, &mComputeCmdPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create secondary command pool!\n");
  }

  cmdPoolCI.queueFamilyIndex = static_cast<U32>(mLogicalDevice.getTransferQueueFamily()._idx);

  if (vkCreateCommandPool(mLogicalDevice.getNative(), &cmdPoolCI, nullptr, &m_TransferCmdPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create secondary command pool!\n");
  }

  VkPhysicalDeviceProperties props = gPhysicalDevice.getDeviceProperties();
  VkPhysicalDeviceMaintenance3Properties mp = gPhysicalDevice.getMaintenanceProperties();
  mPhysicalDeviceProperties = props;

  VkPhysicalDeviceMemoryProperties memProps = gPhysicalDevice.getMemoryProperties();
  m_memProps = memProps;

  // Descriptor pool maxes.
  buildDescriptorPool(UINT16_MAX << 1, UINT16_MAX << 1);
  createOcclusionQueryPool(UINT8_MAX);

  createDepthAttachment();
  setUpSwapchainRenderPass();
  queryFromSwapchain();
  createFrameResources(frameResourceBuffers);
}


void VulkanRHI::cleanUp()
{
  mLogicalDevice.waitOnQueues();
  gAllocator.cleanUp(this);

  cleanUpFrameResources();

  for (size_t i = 0; i < mGraphicsCmdPools.size(); ++i) {
    if (mGraphicsCmdPools[i]) {
      vkDestroyCommandPool(mLogicalDevice.getNative(), mGraphicsCmdPools[i], nullptr);
      mGraphicsCmdPools[i] = VK_NULL_HANDLE;
    }
  }

  if (mComputeCmdPool) {
    vkDestroyCommandPool(mLogicalDevice.getNative(), mComputeCmdPool, nullptr);
    mComputeCmdPool = VK_NULL_HANDLE;
  }

  if (m_TransferCmdPool) {
    vkDestroyCommandPool(mLogicalDevice.getNative(), m_TransferCmdPool, nullptr);
    m_TransferCmdPool = VK_NULL_HANDLE;
  }

  if (mDescriptorPool) {
    vkDestroyDescriptorPool(mLogicalDevice.getNative(), mDescriptorPool, nullptr);
    mDescriptorPool = VK_NULL_HANDLE;
  }

  if (mOccQueryPool) {
    vkDestroyQueryPool(mLogicalDevice.getNative(), mOccQueryPool, nullptr);
    mOccQueryPool = VK_NULL_HANDLE;
  }

  for (auto& framebuffer : m_swapchainFrameBuffers) {
    vkDestroyFramebuffer(mLogicalDevice.getNative(), framebuffer, nullptr);
    framebuffer = VK_NULL_HANDLE;
  } 

  vkDestroyRenderPass(mLogicalDevice.getNative(), mSwapchainInfo.mSwapchainRenderPass, nullptr);
    
  vkDestroyImageView(mLogicalDevice.getNative(), mSwapchainInfo.mDepthView, nullptr);
  vkDestroyImage(mLogicalDevice.getNative(), mSwapchainInfo.mDepthAttachment, nullptr);
  vkFreeMemory(mLogicalDevice.getNative(), mSwapchainInfo.mDepthMemory, nullptr);

  // NOTE(): Clean up any vulkan modules before destroying the logical device!
  m_swapchain.cleanUp(mLogicalDevice);

  if (mSurface != VK_NULL_HANDLE) {
    gContext.destroySurface(mSurface);
    mSurface = VK_NULL_HANDLE;
  }

  mLogicalDevice.cleanUp();
}


void VulkanRHI::queryFromSwapchain()
{
  if (!m_swapchain.getHandle()) {
    R_DEBUG(rError, "Swapchain failed to initialize! Cannot query images!\n");
    return;
  }

  for (size_t i = 0; i < m_swapchainFrameBuffers.size(); ++i) {
    vkDestroyFramebuffer(mLogicalDevice.getNative(), m_swapchainFrameBuffers[i], nullptr);
  }
  
  m_swapchainFrameBuffers.resize(m_swapchain.getImageCount());
  for (size_t i = 0; i < m_swapchain.getImageCount(); ++i) {
    VkFramebufferCreateInfo framebufferCI = { };
    std::array<VkImageView, 2> attachments = { m_swapchain[i].view, mSwapchainInfo.mDepthView };
    VkExtent2D& extent = m_swapchain.getSurfaceExtent();

    framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCI. width = extent.width;
    framebufferCI.height = extent.height;
    framebufferCI.attachmentCount = static_cast<U32>(attachments.size());
    framebufferCI.pAttachments = attachments.data();
    framebufferCI.renderPass = mSwapchainInfo.mSwapchainRenderPass;
    framebufferCI.layers = 1;
    framebufferCI.flags = 0;
    
    if (vkCreateFramebuffer(mLogicalDevice.getNative(), &framebufferCI, nullptr, 
      &m_swapchainFrameBuffers[i]) != VK_SUCCESS) {
      R_DEBUG(rError, "Failed to create framebuffer on swapchain image " 
        + std::to_string(U32(i)) + "!\n");
    }
  }
}


void VulkanRHI::createDepthAttachment()
{
  // TODO(): Set up D32_SFLOAT_S8_UINT as well.
  mSwapchainInfo.mDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
  mSwapchainInfo.mDepthAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
  mSwapchainInfo.mDepthUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  m_bStencilTestAllowed = VK_TRUE;

  VkImageFormatProperties imageFormatProps = { };
  if (gPhysicalDevice.getImageFormatProperties(VK_FORMAT_D32_SFLOAT_S8_UINT, 
                                           VK_IMAGE_TYPE_2D, 
                                           VK_IMAGE_TILING_OPTIMAL, 
                                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
                                           &imageFormatProps) != VK_SUCCESS) {
    mSwapchainInfo.mDepthFormat = VK_FORMAT_D32_SFLOAT;
    mSwapchainInfo.mDepthAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    mSwapchainInfo.mDepthUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    m_bStencilTestAllowed = VK_FALSE;
  }

  VkImageCreateInfo imageCI = { };
  imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCI.extent.width = m_swapchain.getSurfaceExtent().width;
  imageCI.extent.height = m_swapchain.getSurfaceExtent().height;
  imageCI.extent.depth = 1;
  imageCI.mipLevels = 1;
  imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCI.arrayLayers = 1;
  imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCI.usage = depthUsageFlags();
  imageCI.format = depthFormat();
  imageCI.imageType = VK_IMAGE_TYPE_2D;
  imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if (vkCreateImage(mLogicalDevice.getNative(), &imageCI, nullptr, &mSwapchainInfo.mDepthAttachment) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create depth image!\n");
    return;
  }

  VkMemoryRequirements mem;
  vkGetImageMemoryRequirements(mLogicalDevice.getNative(), mSwapchainInfo.mDepthAttachment, &mem);
 
  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = mem.size;
  allocInfo.memoryTypeIndex = gPhysicalDevice.findMemoryType(mem.memoryTypeBits, PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY);
  
  if (vkAllocateMemory(mLogicalDevice.getNative(), &allocInfo, nullptr, &mSwapchainInfo.mDepthMemory) != VK_SUCCESS) {
    R_DEBUG(rError, "Depth memory was not allocated!\n");
    return;
  }
  VulkanMemoryAllocatorManager::numberOfAllocations++;
  vkBindImageMemory(mLogicalDevice.getNative(), mSwapchainInfo.mDepthAttachment, mSwapchainInfo.mDepthMemory, 0);
  
  // Now create the depth view.
  VkImageViewCreateInfo ivCI = {};
  ivCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivCI.format = depthFormat();
  ivCI.image = mSwapchainInfo.mDepthAttachment;
  ivCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ivCI.subresourceRange.aspectMask = depthAspectFlags();
  ivCI.subresourceRange.baseArrayLayer = 0;
  ivCI.subresourceRange.baseMipLevel = 0;
  ivCI.subresourceRange.layerCount = 1;
  ivCI.subresourceRange.levelCount = 1;
  
  if (vkCreateImageView(mLogicalDevice.getNative(), &ivCI, nullptr, &mSwapchainInfo.mDepthView) != VK_SUCCESS) {
    R_DEBUG(rError, "Depth view not created!\n");
  }
 
  
}


void VulkanRHI::setUpSwapchainRenderPass()
{
  std::array<VkAttachmentDescription, 2> aDs;
  
  // Color description. Our final layout is meant for presentation on a window,
  // if this were a separate framebuffer, we would go with VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.
  aDs[0].format = m_swapchain.getSurfaceFormat().format;
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
  aDs[1].format = depthFormat();
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
  renderpassCI.attachmentCount = static_cast<U32>(aDs.size());
  renderpassCI.pAttachments = aDs.data();
  renderpassCI.subpassCount = 1;
  renderpassCI.pSubpasses = &subpass;
  renderpassCI.dependencyCount = static_cast<U32>(dependencies.size());
  renderpassCI.pDependencies = dependencies.data();

  if (vkCreateRenderPass(mLogicalDevice.getNative(), &renderpassCI, nullptr, &mSwapchainInfo.mSwapchainRenderPass) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create swapchain renderpass!\n");
  }
}


void VulkanRHI::graphicsSubmit(size_t queueIdx, const U32 count, const VkSubmitInfo* submitInfo, const VkFence fence)
{
  R_TIMED_PROFILE_RENDERER();

  VkResult result = vkQueueSubmit(mLogicalDevice.getGraphicsQueue(queueIdx), 
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


void VulkanRHI::waitForFences(const U32 fenceCount, const VkFence* pFences, B32 waitAll, const U64 timeout)
{
  vkWaitForFences(mLogicalDevice.getNative(), fenceCount, pFences, waitAll, timeout); 
}


void VulkanRHI::resetFences(const U32 fenceCount, const VkFence* pFences)
{
  vkResetFences(mLogicalDevice.getNative(), fenceCount, pFences);
}


Fence* VulkanRHI::createVkFence()
{
  Fence* fence = new Fence();
  fence->SetOwner(mLogicalDevice.getNative());
  return fence;
}


void VulkanRHI::freeVkFence(Fence* fence)
{
  if (!fence) return;
  fence->cleanUp();
  delete fence;
}


void VulkanRHI::acquireNextImage()
{
  R_TIMED_PROFILE_RENDERER();

  VkResult result = vkAcquireNextImageKHR(mLogicalDevice.getNative(), 
                                          m_swapchain.getHandle(), 
                                          UINT64_MAX,
                                          m_frameResources[m_currentFrame].imageAvailableSemaphore, 
                                          VK_NULL_HANDLE, 
                                          &mSwapchainInfo.mCurrentImageIndex);
  R_ASSERT(result == VK_SUCCESS, "Failed to acquire next swapchain image.");
}


void VulkanRHI::waitForFrameInFlightFence()
{
  R_TIMED_PROFILE_RENDERER();

  VkFence inflight = currentInFlightFence();
  waitForFences(1, &inflight, VK_TRUE, UINT64_MAX);
  resetFences(1, &inflight);
}


void VulkanRHI::submitCurrSwapchainCmdBuffer(U32 waitSemaphoreCount, VkSemaphore* waitSemaphores,
  U32 signalSemaphoreCount, VkSemaphore* signalSemaphores, VkFence fence)
{
  if (!signalSemaphores || !signalSemaphoreCount) {
    R_DEBUG(rError, "Fatal! You must specify at least one signal semaphore when calling SubmitCurrSwapchainCmdBuffer (GraphicsFinishedSemaphore() if rendering is complete!)");
    return;
  }

  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  
  VkCommandBuffer cmdBuffer[] = { 
    m_frameResources[getCurrentFrame()].cmdBuffer.getHandle() 
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

  graphicsSubmit(0, 1, &submitInfo, fence);
}


VkResult VulkanRHI::present()
{
  VkSemaphore signalSemaphores[] = { currentGraphicsFinishedSemaphore() };
  VkSwapchainKHR swapchains[] = { m_swapchain.getHandle() };

  VkPresentInfoKHR presentInfo = { };
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pResults = nullptr;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.pImageIndices = &mSwapchainInfo.mCurrentImageIndex;

  VkResult result = vkQueuePresentKHR(mLogicalDevice.getPresentQueue(), &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    VkExtent2D windowExtent = m_swapchain.getSurfaceExtent();
    reConfigure(m_swapchain.getPresentMode(), windowExtent.width,
      windowExtent.height, m_swapchain.getImageCount());
  }
  //R_ASSERT(result == VK_SUCCESS, "Failed to present!\n");

  // Increment to next frame after every present of the current frame.
  m_currentFrame = (m_currentFrame + 1) % m_frameResources.size();
  return result;
}


void VulkanRHI::computeSubmit(size_t queueIdx, const VkSubmitInfo& submitInfo, const VkFence fence)
{
  if (vkQueueSubmit(mLogicalDevice.getComputeQueue(queueIdx), 1, &submitInfo, fence) != VK_SUCCESS) {
    R_DEBUG(rError, "Compute failed to submit task!\n");
  }
}


void VulkanRHI::transferSubmit(size_t queueIdx, const U32 count, const VkSubmitInfo* submitInfo, const VkFence fence)
{
  VkResult result = vkQueueSubmit(mLogicalDevice.getTransferQueue(queueIdx), count, submitInfo, fence);
  if (result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to submit to transfer queue!\n");
  }
}


void VulkanRHI::transferWaitIdle(size_t queueIdx)
{
  vkQueueWaitIdle(mLogicalDevice.getTransferQueue(queueIdx));
}


void VulkanRHI::graphicsWaitIdle(size_t queueIdx)
{
  vkQueueWaitIdle(mLogicalDevice.getGraphicsQueue(queueIdx));
}


void VulkanRHI::computeWaitIdle(size_t queueIdx)
{
  vkQueueWaitIdle(mLogicalDevice.getComputeQueue(queueIdx));
}


void VulkanRHI::presentWaitIdle()
{
  vkQueueWaitIdle(mLogicalDevice.getPresentQueue());
}


void VulkanRHI::deviceWaitIdle()
{
  vkDeviceWaitIdle(mLogicalDevice.getNative());
}

void VulkanRHI::waitAllGraphicsQueues()
{
  for (size_t i = 0; i < logicDevice()->getGraphicsQueueFamily()._queueCount; ++i) {
    graphicsWaitIdle(i);
  }
}


void VulkanRHI::renderFrameCommandBuffer()
{
  CommandBuffer& cmdBuffer = m_frameResources[getCurrentFrame()].cmdBuffer;

  VkCommandBufferBeginInfo cmdBufferBI = {};
  cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBufferBI.flags =   VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmdBuffer.reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

  cmdBuffer.begin(cmdBufferBI);

  VkRenderPassBeginInfo rpBI = {};
  VkClearValue clearValues[2];

  clearValues[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
  clearValues[1].depthStencil = {1.0f, 0};

  rpBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpBI.framebuffer = m_swapchainFrameBuffers[mSwapchainInfo.mCurrentImageIndex];
  rpBI.renderPass = mSwapchainInfo.mSwapchainRenderPass;
  rpBI.renderArea.extent = m_swapchain.getSurfaceExtent();
  rpBI.renderArea.offset = {0, 0};
  rpBI.clearValueCount = 2;
  rpBI.pClearValues = clearValues;

  if (mSwapchainCmdBufferBuild) {
    mSwapchainCmdBufferBuild(cmdBuffer, rpBI);
  }

  cmdBuffer.end();

}


void VulkanRHI::createSwapchainCommandBuffers()
{
  for (size_t i = 0; i < m_frameResources.size(); ++i) {
    CommandBuffer& cmdBuffer = m_frameResources[i].cmdBuffer;
    cmdBuffer.SetOwner(mLogicalDevice.getNative());
    cmdBuffer.allocate(mGraphicsCmdPools[0], VK_COMMAND_BUFFER_LEVEL_PRIMARY);   
  }

  mSwapchainInfo.mComplete = true;
}


void VulkanRHI::rebuildCommandBuffers()
{
  for (size_t i = 0; i < m_frameResources.size(); ++i) {
    CommandBuffer& cmdBuffer = m_frameResources[i].cmdBuffer;
    cmdBuffer.free();
  }
  mSwapchainInfo.mComplete = false;
  createSwapchainCommandBuffers();
}


void VulkanRHI::reConfigure(VkPresentModeKHR presentMode, 
                            I32 width, 
                            I32 height, 
                            U32 frameResourceBuffers, 
                            U32 desiredImageCount)
{
  if (width <= 0 || height <= 0) return;
  deviceWaitIdle();

  vkDestroyRenderPass(mLogicalDevice.getNative(), mSwapchainInfo.mSwapchainRenderPass, nullptr);

  vkDestroyImageView(mLogicalDevice.getNative(), mSwapchainInfo.mDepthView, nullptr);
  vkDestroyImage(mLogicalDevice.getNative(), mSwapchainInfo.mDepthAttachment, nullptr);
  vkFreeMemory(mLogicalDevice.getNative(), mSwapchainInfo.mDepthMemory, nullptr);

  std::vector<VkSurfaceFormatKHR> surfaceFormats = gPhysicalDevice.querySwapchainSurfaceFormats(mSurface);
  VkSurfaceCapabilitiesKHR capabilities = gPhysicalDevice.querySwapchainSurfaceCapabilities(mSurface);

  m_swapchain.reCreate(mLogicalDevice, mSurface, surfaceFormats[0], presentMode, capabilities, desiredImageCount);
  
  createDepthAttachment();
  setUpSwapchainRenderPass();
  queryFromSwapchain();
  createFrameResources(frameResourceBuffers);
  
  // Readjust frame count since we are now changing swapchain frame resource count.
  m_currentFrame = m_currentFrame % m_frameResources.size();
}


Buffer* VulkanRHI::createBuffer()
{ 
  Buffer* buffer = new Buffer();
  buffer->SetOwner(mLogicalDevice.getNative());

  return buffer;
}


void VulkanRHI::freeBuffer(Buffer* buffer)
{
  if (!buffer) return;

  if (buffer->Owner() != mLogicalDevice.getNative()) {
    R_DEBUG(rNotify, "Unable to free buffer. Device is not same as this vulkan rhi!\n");
    return;
  }

  buffer->cleanUp();

  delete buffer;
}


GraphicsPipeline* VulkanRHI::createGraphicsPipeline()
{
  GraphicsPipeline* pipeline = new GraphicsPipeline();
  pipeline->SetOwner(mLogicalDevice.getNative());
  return pipeline;
}


void VulkanRHI::freeGraphicsPipeline(GraphicsPipeline* pipeline)
{
  if (!pipeline) return ;

  if (pipeline->Owner() != mLogicalDevice.getNative()) {
    R_DEBUG(rNotify, "Unable to free pipeline. Device is not same as this vulkan rhi!\n");
    return;
  }
  pipeline->cleanUp();
 
  delete pipeline;
}



Shader* VulkanRHI::createShader()
{
  Shader* shader = new Shader();
  shader->SetOwner(mLogicalDevice.getNative());
  
  return shader;
}


void VulkanRHI::freeShader(Shader* shader)
{
  if (!shader) return;

  if (shader->Owner() != mLogicalDevice.getNative()) {
    R_DEBUG(rNotify, "Unable to free shader. Device is not same as this vulkan rhi!\n");
    return;
  }

  shader->cleanUp();
  
  delete shader;
}


ImageView* VulkanRHI::createImageView(const VkImageViewCreateInfo& viewCi)
{
  ImageView* pView = new ImageView();
  pView->initialize(mLogicalDevice.getNative(), viewCi);
  return pView;
}


void VulkanRHI::freeImageView(ImageView* pView)
{
  if ( !pView ) return;
  pView->cleanUp(mLogicalDevice.getNative());
  delete pView;
}


DescriptorSet* VulkanRHI::createDescriptorSet()
{
  DescriptorSet* dset = new DescriptorSet();
  dset->SetOwner(mLogicalDevice.getNative());
  mCurrDescSets += 1;
  return dset;
}


void VulkanRHI::freeDescriptorSet(DescriptorSet* dset)
{
  if (!dset) return;

  dset->free();
  
  delete dset;
  mCurrDescSets -= 1;
}


Sampler* VulkanRHI::createSampler()
{
  Sampler* sampler = new Sampler();
  sampler->SetOwner(mLogicalDevice.getNative());
  
  return sampler;
}


void VulkanRHI::freeSampler(Sampler* sampler)
{
  if (!sampler) return;

  sampler->cleanUp();
  
  delete sampler;
}


Texture* VulkanRHI::createTexture()
{
  Texture* texture = new Texture();
  texture->SetOwner(mLogicalDevice.getNative());
  return texture;
}


void VulkanRHI::freeTexture(Texture* texture)
{
  if (!texture) return;
  texture->cleanUp();
  
  delete texture;
}


FrameBuffer* VulkanRHI::createFrameBuffer()
{
  FrameBuffer* framebuffer = new FrameBuffer();
  framebuffer->SetOwner(mLogicalDevice.getNative());

  return framebuffer;
}


void VulkanRHI::freeFrameBuffer(FrameBuffer* framebuffer)
{
  if (!framebuffer) return;

  framebuffer->cleanUp();
  
  delete framebuffer;
}


CommandBuffer* VulkanRHI::createCommandBuffer()
{
  CommandBuffer* buffer = new CommandBuffer();
  buffer->SetOwner(mLogicalDevice.getNative());
  
  return buffer;
}


void VulkanRHI::freeCommandBuffer(CommandBuffer* buffer)
{
  if (!buffer) return;

  buffer->free();
  delete buffer;
}


DescriptorSetLayout* VulkanRHI::createDescriptorSetLayout()
{
  DescriptorSetLayout* layout = new DescriptorSetLayout();
  layout->SetOwner(mLogicalDevice.getNative());
  return layout;
}


void VulkanRHI::freeDescriptorSetLayout(DescriptorSetLayout* layout)
{
  if (!layout) return;

  layout->cleanUp();

  delete layout;
}


ComputePipeline* VulkanRHI::createComputePipeline()
{
  ComputePipeline* pipeline = new ComputePipeline();
  pipeline->SetOwner(mLogicalDevice.getNative());

  return pipeline;
}


void VulkanRHI::freeComputePipeline(ComputePipeline* pipeline)
{
  if (!pipeline) return;

  pipeline->cleanUp();

  delete pipeline;
}


Semaphore* VulkanRHI::createVkSemaphore()
{
  Semaphore* semaphore = new Semaphore();
  semaphore->SetOwner(mLogicalDevice.getNative());
  
  return semaphore;
}


void  VulkanRHI::freeVkSemaphore(Semaphore* semaphore)
{
  if (!semaphore) return;

  semaphore->cleanUp();
    
  delete semaphore;
}


RenderPass* VulkanRHI::createRenderPass()
{
  RenderPass* renderPass = new RenderPass();
  renderPass->SetOwner(mLogicalDevice.getNative());
  return renderPass;
}


void VulkanRHI::freeRenderPass(RenderPass* renderPass)
{
  if (!renderPass) return;
  renderPass->cleanUp();
  delete renderPass;
}


void VulkanRHI::buildDescriptorPool(U32 maxCount, U32 maxSets)
{
  if (mDescriptorPool) {
    vkDestroyDescriptorPool(mLogicalDevice.getNative(), mDescriptorPool, nullptr);
    mDescriptorPool = VK_NULL_HANDLE;
  }

  std::array<VkDescriptorPoolSize, 4> poolSizes;
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = maxCount;
  
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = maxCount;

  poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[2].descriptorCount = maxCount;

  poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[3].descriptorCount = 64;

  VkDescriptorPoolCreateInfo descriptorPoolCI = { };
  descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCI.poolSizeCount = static_cast<U32>(poolSizes.size());
  descriptorPoolCI.pPoolSizes = poolSizes.data();
  descriptorPoolCI.maxSets = maxSets;
  descriptorPoolCI.pNext = nullptr;
  descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  if (vkCreateDescriptorPool(mLogicalDevice.getNative(), &descriptorPoolCI, nullptr, &mDescriptorPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to created descriptor pool!\n");
  }
}


void VulkanRHI::createOcclusionQueryPool(U32 queries)
{
  VkQueryPoolCreateInfo queryCi = { };
  queryCi.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  queryCi.queryCount = queries;
  queryCi.queryType = VK_QUERY_TYPE_OCCLUSION;
  
  if (vkCreateQueryPool(mLogicalDevice.getNative(), &queryCi, nullptr, &mOccQueryPool) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create occlusion query pool!\n");
  } 
}


void VulkanRHI::createFrameResources(U32 frameResourceBufferCount) 
{
  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceCi = {};
  fenceCi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  /*
    if (vkCreateSemaphore(handle, &semaphoreCI, nullptr,
    &mImageAvailableSemaphore) != VK_SUCCESS) { R_DEBUG(rError, "Failed to
    create a semaphore!\n");
    }

    if (vkCreateSemaphore(handle, &semaphoreCI, nullptr,
    &mGraphicsFinishedSemaphore) != VK_SUCCESS) { R_DEBUG(rError, "Failed to
    create a semaphore!\n");
    }
  */
  for (U32 i = 0; i < m_frameResources.size(); ++i) {
    vkDestroySemaphore(mLogicalDevice.getNative(), m_frameResources[i].imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(mLogicalDevice.getNative(), m_frameResources[i].presentableSemaphore, nullptr);
    vkDestroyFence(mLogicalDevice.getNative(), m_frameResources[i].fenceInFlight, nullptr);
    m_frameResources[i].cmdBuffer.free();
  }

  m_frameResources.resize(frameResourceBufferCount);

  for (U32 i = 0; i < m_frameResources.size(); ++i) {
    VkResult result = vkCreateSemaphore(mLogicalDevice.getNative(), 
                                        &semaphoreCI,
                                        nullptr, 
                                        &m_frameResources[i].imageAvailableSemaphore);
    R_ASSERT(result == VK_SUCCESS, "");
    result = vkCreateSemaphore(mLogicalDevice.getNative(), 
                               &semaphoreCI, 
                               nullptr,
                               &m_frameResources[i].presentableSemaphore);
    R_ASSERT(result == VK_SUCCESS, "");
    result = vkCreateFence(mLogicalDevice.getNative(), &fenceCi, nullptr,
                           &m_frameResources[i].fenceInFlight);
    R_ASSERT(result == VK_SUCCESS, "");
  }

  rebuildCommandBuffers();
}


void VulkanRHI::cleanUpFrameResources()
{
  for (U32 i = 0; i < m_frameResources.size(); ++i) {
    vkDestroySemaphore(mLogicalDevice.getNative(), m_frameResources[i].imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(mLogicalDevice.getNative(), m_frameResources[i].presentableSemaphore, nullptr);
    vkDestroyFence(mLogicalDevice.getNative(), m_frameResources[i].fenceInFlight, nullptr);
    m_frameResources[i].cmdBuffer.free();
  }
}
} // Recluse