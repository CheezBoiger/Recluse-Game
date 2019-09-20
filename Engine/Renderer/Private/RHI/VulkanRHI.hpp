// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"
#include "Swapchain.hpp"
#include "VulkanContext.hpp"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"

#include "Memory/Allocator.hpp"
#include "Renderer/UserParams.hpp"

#include <functional>
#include <set>

#define DEFAULT_QUEUE_IDX 0

namespace Recluse {

class Buffer;
class GraphicsPipeline;
class ComputePipeline;
class FrameBuffer;
class RenderPass;
class Shader;
class CommandBuffer;
class Sampler;
class Texture;
class DescriptorSet;
class ImageView;
class DescriptorSetLayout;
class GPUConfigParams;
class Query;


struct FrameResource {
  std::vector<VkCommandPool> graphicsCmdPools;
  std::vector<VkCommandPool> computeCmdPools;
  std::vector<VkCommandPool> transferCmdPools; 
  CommandBuffer cmdBuffer;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore presentableSemaphore;
  VkFence fenceInFlight;
};

// Set swapchain command buffer function. Assume that the commandbuffer automatically
// calls Begin() before the this function and End() and the end of the function.
// Also passes the default renderpass begin info to render onscreen.
typedef std::function<void(CommandBuffer&, VkRenderPassBeginInfo&)> SwapchainCmdBufferBuildFunc;


class Semaphore : public VulkanHandle {
public:
  Semaphore()
    : mSema(VK_NULL_HANDLE) { }

  ~Semaphore();

  void initialize(const VkSemaphoreCreateInfo& info);
  void cleanUp();

  VkSemaphore getHandle() { return mSema; }

private:
  VkSemaphore mSema;
};


class Fence : public VulkanHandle {
public:
  Fence()
    : mFence(VK_NULL_HANDLE) { }

  ~Fence();

  void initialize(const VkFenceCreateInfo& info);
  void cleanUp();

  VkFence getHandle() { return mFence; }

private:
  VkFence mFence;
};


// render Hardware Interface for Vulkan.
class VulkanRHI {
public:
  // Global variables.
  static Context gContext;
  static PhysicalDevice gPhysicalDevice;
  static VulkanMemoryAllocatorManager gAllocator;

  // Context and physical device set up.
  static B32 createContext(const char* appName);
  static B32 findPhysicalDevice(U32 rhiConfigBits = 0);
  static std::set<std::string> getMissingExtensions(VkPhysicalDevice device);

private:
  static B32 suitableDevice(VkPhysicalDevice device);
public:

  VulkanRHI();
  ~VulkanRHI();

  // Initialize this RHI object. Must be done before calling any function
  // in this object.
  void initialize(HWND windowHandle, 
                  VkPresentModeKHR presentMode,
                  U32 frameResourceBuffers = 2,
                  U32 desiredImageCount = 1);

  // Clean up this object. This must be called once done with this RHI
  // object. BE SURE TO FREE UP ANY OBJECTS CREATED FROM THIS RHI OBJECT BEFORE
  // CALLING THIS FUNCTION.
  void cleanUp();

  // TODO(): We need to allocate on a custom allocator to save time with
  // memory management.
  Buffer* createBuffer();
  GraphicsPipeline* createGraphicsPipeline();
  ComputePipeline* createComputePipeline();
  FrameBuffer* createFrameBuffer();
  Sampler* createSampler();
  ImageView* createImageView(const VkImageViewCreateInfo& info);
  Texture* createTexture();
  Shader* createShader();
  CommandBuffer* createCommandBuffer();
  DescriptorSet* createDescriptorSet();
  DescriptorSetLayout* createDescriptorSetLayout();
  Query* createQuery();
  RenderPass* createRenderPass();

  void freeImageView(ImageView* imgView);
  void freeBuffer(Buffer* buffer);
  void freeGraphicsPipeline(GraphicsPipeline* pipeline);
  void freeComputePipeline(ComputePipeline* pipeline);
  void freeFrameBuffer(FrameBuffer* buffer);
  void freeSampler(Sampler* sampler);
  void freeRenderPass(RenderPass* pass);
  void freeTexture(Texture* texture);
  void freeShader(Shader* shader);
  void freeCommandBuffer(CommandBuffer* buffer);
  void freeDescriptorSet(DescriptorSet* set);
  void freeDescriptorSetLayout(DescriptorSetLayout* layout);
  void freeQuery(Query* query);

  FrameResource& getFrameResource(U32 frameIndex) { return m_frameResources[frameIndex]; }
  FrameResource& getCurrentFrameResource() { return getFrameResource(m_currentFrame); }

  // Set the default swapchain command buffers, which is defined by the programmer of the renderer.
  void setSwapchainCmdBufferBuild(SwapchainCmdBufferBuildFunc func) { mSwapchainCmdBufferBuild = func; }

  size_t swapchainImageCount() const { return m_swapchain.getImageCount(); }

  LogicalDevice* logicDevice() { return &mLogicalDevice; }

  // Get the Swapchain handle.
  Swapchain* swapchainObject() { return &m_swapchain; }

  // Get the window surface that is used to render onto.
  VkSurfaceKHR surface() { return mSurface; }

  // Get the command pool on the graphics side.
  VkCommandPool getGraphicsCmdPool(size_t frameIdx, size_t i) { 
    return m_frameResources[frameIdx].graphicsCmdPools[i]; 
  }

  // Get the number of graphics command pools this RHI is using.
  size_t numGraphicsCmdPools(size_t frameIdx = 0) { 
    return m_frameResources[frameIdx].graphicsCmdPools.size(); 
  }

  // Get the command pool on the compute side.
  VkCommandPool getComputeCmdPool(size_t frameIdx, size_t i) { 
    return m_frameResources[frameIdx].computeCmdPools[i]; 
  }

  VkCommandPool getTransferCmdPool(size_t frameIdx, size_t i) { 
    return m_frameResources[frameIdx].transferCmdPools[i]; 
  }

  // Get the descriptor pool that is used to create our descriptor sets.
  VkDescriptorPool descriptorPool() { return mDescriptorPool; }

  // Get the query pool that is used to create queries.
  VkQueryPool occlusionQueryPool() { return mOccQueryPool; }

  // Create a semaphore object.
  Semaphore* createVkSemaphore();

  Fence* createVkFence();

  U32 numDescriptorSets() { return mCurrDescSets; }

  // Free a semaphore that was created by this RHI.
  void freeVkSemaphore(Semaphore* semaphore);

  void freeVkFence(Fence* fence);

  // Flush out commands that are on hold.
  void flushCommands();

  // Returns the image index to the current swapchain surface to render onto.
  void acquireNextImage();

  // Submit a command buffer to the graphics queue.
  void graphicsSubmit(size_t queueIdx, 
                      const U32 count, 
                      const VkSubmitInfo* submitInfo, 
                      const VkFence fence = VK_NULL_HANDLE);

  // Submit a command buffer to the transfer queue.
  void transferSubmit(size_t queueIdx, 
                      const U32 count, 
                      const VkSubmitInfo* submitInfo, 
                      const VkFence fence = VK_NULL_HANDLE);

  // Wait until transfer queue has completely finished all submittals.
  void transferWaitIdle(size_t queueIdx);

  // Wait until the graphics queue has completely finished all submittals.
  void graphicsWaitIdle(size_t queueIdx);

  // Wait until compute queue has completely finished all submittals.
  void computeWaitIdle(size_t queueIdx);

  void waitAllGraphicsQueues();

  // Wait until present queue has completely finished presenting onto the screen.
  void presentWaitIdle();

  // Wait for the device to finished its submittals.
  void deviceWaitIdle();

  // Submit a command buffer to the compute queue.
  void computeSubmit(size_t queueIdx, const VkSubmitInfo& submitInfo, const VkFence fence = VK_NULL_HANDLE);

  void waitForFences(const U32 fenceCount, const VkFence* pFences, B32 waitAll, const U64 timeout);

  void resetFences(const U32 fenceCount, const VkFence* pFences);

  void waitForFrameInFlightFence();

  VkFence currentInFlightFence() { return getCurrentFrameResource().fenceInFlight; }

  // Submit the current swapchain command buffer to the gpu. This will essentially be the 
  // call to the default render pass, which specifies the swapchain surface to render onto.
  // If no signals are specified, or signalSemaphoreCount is equal to 0, cmdbuffer will default
  // to GraphicsFinished semaphore, which is used by the Present queue to present to the screen!
  void submitCurrSwapchainCmdBuffer(U32 waitSemaphoreCount, 
                                    VkSemaphore* waitSemaphores, 
                                    U32 signalSemaphoreCount, 
                                    VkSemaphore* signalSemaphores, 
                                    VkFence fence = nullptr);
  
  // Present the rendered surface.
  VkResult present();

  // Updates the renderer pipeline as a result of window resizing. This will effectively
  // recreate the entire pipeline! If any objects were referenced and whatnot, be sure to 
  // requery their resources as they have been recreated!
  void reConfigure(VkPresentModeKHR presentMode, 
                   I32 width, 
                   I32 height, 
                   U32 frameResourceBuffers = 2, 
                   U32 desiredImageCount = 1,
                   B32 preserveFrameResources = false);

  // Get the current image index that is used during rendering, the current image from the 
  // swapchain that we are rendering onto.
  //U32 currentImageIndex() { return mSwapchainInfo.mCurrentImageIndex; }

  // Get the current frame index, this is usually the next frame in the swapchain. Use this value instead.
  U32 getCurrentFrame() const { return m_currentFrame; }

  U32 getFrameCount() const { return static_cast<U32>(m_frameResources.size()); }

  // Swap commandbuffer sets within this vulkan rhi. Be sure that the set is already built!
  void swapCommandBufferSets(U32 set) { mSwapchainInfo.mCmdBufferSet = set; }

  // Obtain the Graphics Finished Semaphore from swapchain.
  VkSemaphore currentGraphicsFinishedSemaphore() { return getCurrentFrameResource().presentableSemaphore; }
  VkSemaphore currentImageAvailableSemaphore() { return getCurrentFrameResource().imageAvailableSemaphore; }

  // Current set of swapchain commandbuffers that are currently in use by the gpu. Use this to determine which
  // set we shouldn't rebuild, while the gpu is using them!
  U32 currentSwapchainCmdBufferSet() const { return mSwapchainInfo.mCmdBufferSet; }
  VkFramebuffer swapchainFrameBuffer(size_t index) { return m_swapchainFrameBuffers[index]; }
  size_t numOfFramebuffers() { return m_frameResources.size(); }
  VkRenderPass swapchainRenderPass() { return mSwapchainInfo.mSwapchainRenderPass; }
  const VkPhysicalDeviceLimits& PhysicalDeviceLimits() const { return mPhysicalDeviceProperties.limits; }
  const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const { return m_memProps; }
  U32 vendorID() { return mPhysicalDeviceProperties.vendorID; }
  B32 depthBoundsAllowed() const { return m_depthBoundsAllowed; }
  B32 stencilTestAllowed() const { return m_bStencilTestAllowed; }
  
  VkFormat depthFormat() const { return mSwapchainInfo.mDepthFormat; }
  VkImageAspectFlags depthAspectFlags() const { return mSwapchainInfo.mDepthAspectFlags; }
  VkImageUsageFlags depthUsageFlags() const { return mSwapchainInfo.mDepthUsageFlags; }
  B32 cmdBuffersComplete() { return mSwapchainInfo.mComplete; }
  U32 graphicsQueueCount() const { return mLogicalDevice.getGraphicsQueueCount(); }
  U32 transferQueueCount() const { return mLogicalDevice.getTransferQueueCount(); }
  U32 computeQueueCount() const { return mLogicalDevice.getComputeQueueCount(); }

  const char* deviceName() { return mPhysicalDeviceProperties.deviceName; }

  void renderFrameCommandBuffer();

private:
  void setUpSwapchainRenderPass();
  void queryFromSwapchain();
  void createDepthAttachment();
  void createSwapchainCommandBuffers();  
  void createOcclusionQueryPool(U32 queries);

  // Rebuild the swapchain commandbuffers with using the function provided from
  // SetSwapchainCmdBufferBuild. This will build the swapchain command buffers.
  void recreateCommandBuffers();

  void createFrameResources(U32 frameResourceBufferCount);
  void cleanUpFrameResources();

  // Builds the descriptor pool for materials. WARNING: Recalling this function will
  // destroy old descriptor pool and replace with a new one, be sure to destroy all
  // descriptor sets previously allocated from old pool before recalling!
  void buildDescriptorPool(U32 maxCount, U32 maxSets);

  HWND m_windowHandle;
  Swapchain m_swapchain;
  B32 m_depthBoundsAllowed;
  B32 m_bStencilTestAllowed;
  LogicalDevice mLogicalDevice;
  VkSurfaceKHR mSurface;
  VkDescriptorPool mDescriptorPool;
  VkQueryPool mOccQueryPool;
  VkPhysicalDeviceProperties mPhysicalDeviceProperties;
  VkPhysicalDeviceMemoryProperties m_memProps;
  VkPhysicalDeviceMaintenance3Properties m_maintenanceProperties;
  
  std::vector<FrameResource> m_frameResources;

  // Framebuffers and Renderpass that is used by the swapchain. We must
  // first query the images from the swapchain.
  struct {
    B32 mComplete;
    VkRenderPass mSwapchainRenderPass;
    VkImage mDepthAttachment;
    VkImageView mDepthView;
    VkDeviceMemory mDepthMemory;
    VkFormat mDepthFormat;
    VkImageAspectFlags mDepthAspectFlags;
    VkImageUsageFlags mDepthUsageFlags;
    U32 mCurrentImageIndex;
    I32 mCmdBufferSet;
  } mSwapchainInfo;

  std::vector<VkFramebuffer> m_swapchainFrameBuffers;

  SwapchainCmdBufferBuildFunc mSwapchainCmdBufferBuild;
  U32 mCurrDescSets;
  U32 m_currentFrame;
};
} // Recluse