// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"
#include "Swapchain.hpp"
#include "VulkanContext.hpp"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"

#include <functional>


namespace Recluse {

class Buffer;
class GraphicsPipeline;
class ComputePipeline;
class FrameBuffer;
class Shader;
class CommandBuffer;
class Sampler;
class Texture;
class DescriptorSet;
class DescriptorSetLayout;
class GPUConfigParams;
class Query;

// Set swapchain command buffer function. Assume that the commandbuffer automatically
// calls Begin() before the this function and End() and the end of the function.
// Also passes the default renderpass begin info to render onscreen.
typedef std::function<void(CommandBuffer&, VkRenderPassBeginInfo&)> SwapchainCmdBufferBuildFunc;


class Semaphore : public VulkanHandle {
public:
  Semaphore()
    : mSema(VK_NULL_HANDLE) { }

  ~Semaphore();

  void          Initialize(const VkSemaphoreCreateInfo& info);
  void          CleanUp();

  VkSemaphore   Handle() { return mSema; }

private:
  VkSemaphore   mSema;
};


// Render Hardware Interface for Vulkan.
class VulkanRHI {
public:
  // Global variables.
  static Context              gContext;
  static PhysicalDevice       gPhysicalDevice;

  // Context and physical device set up.
  static b8                   CreateContext();
  static b8                   FindPhysicalDevice();

private:
  static b8                   SuitableDevice(VkPhysicalDevice device);
public:

  VulkanRHI();
  ~VulkanRHI();

  // Initialize this RHI object. Must be done before calling any function
  // in this object.
  void                          Initialize(HWND windowHandle);

  // Clean up this object. This must be called once done with this RHI
  // object. BE SURE TO FREE UP ANY OBJECTS CREATED FROM THIS RHI OBJECT BEFORE
  // CALLING THIS FUNCTION.
  void                          CleanUp();

  // TODO(): We need to allocate on a custom allocator to save time with
  // memory management.
  Buffer*                       CreateBuffer();
  GraphicsPipeline*             CreateGraphicsPipeline();
  ComputePipeline*              CreateComputePipeline();
  FrameBuffer*                  CreateFrameBuffer();
  Sampler*                      CreateSampler();
  Texture*                      CreateTexture();
  Shader*                       CreateShader();
  CommandBuffer*                CreateCommandBuffer();
  DescriptorSet*                CreateDescriptorSet();
  DescriptorSetLayout*          CreateDescriptorSetLayout();
  Query*                        CreateQuery();

  void                          FreeBuffer(Buffer* buffer);
  void                          FreeGraphicsPipeline(GraphicsPipeline* pipeline);
  void                          FreeComputePipeline(ComputePipeline* pipeline);
  void                          FreeFrameBuffer(FrameBuffer* buffer);
  void                          FreeSampler(Sampler* sampler);
  void                          FreeTexture(Texture* texture);
  void                          FreeShader(Shader* shader);
  void                          FreeCommandBuffer(CommandBuffer* buffer);
  void                          FreeDescriptorSet(DescriptorSet* set);
  void                          FreeDescriptorSetLayout(DescriptorSetLayout* layout);
  void                          FreeQuery(Query* query);

  // Set the default swapchain command buffers, which is defined by the programmer of the renderer.
  void                          SetSwapchainCmdBufferBuild(SwapchainCmdBufferBuildFunc func) { mSwapchainCmdBufferBuild = func; }

  // Rebuild the swapchain commandbuffers with using the function provided from SetSwapchainCmdBufferBuild.
  // This will build the swapchain command buffers.
  void                          RebuildCommandBuffers(u32 set);

  size_t                        SwapchainImageCount() const { return mSwapchain.ImageCount(); }

  LogicalDevice*                LogicDevice() { return &mLogicalDevice; }

  // Get the Swapchain handle.
  Swapchain*                    SwapchainObject() { return &mSwapchain; }

  // Get the window surface that is used to render onto.
  VkSurfaceKHR                  Surface() { return mSurface; }

  // Get the command pool on the graphics side.
  VkCommandPool                 GraphicsCmdPool() { return mCmdPool; }

  // Get the command pool on the compute side.
  VkCommandPool                 ComputeCmdPool() { return mComputeCmdPool; }

  // Get the descriptor pool that is used to create our descriptor sets.
  VkDescriptorPool              DescriptorPool() { return mDescriptorPool; }

  // Get the query pool that is used to create queries.
  VkQueryPool                   OcclusionQueryPool() { return mOccQueryPool; }

  // Create a semaphore object.
  Semaphore*                    CreateVkSemaphore();

  u32                           NumDescriptorSets() { return mCurrDescSets; }

  // Free a semaphore that was created by this RHI.
  void                          FreeVkSemaphore(Semaphore* semaphore);

  // Flush out commands that are on hold.
  void                          FlushCommands();

  // Returns the image index to the current swapchain surface to render onto.
  void                          AcquireNextImage();

  // Submit a command buffer to the graphics queue.
  void                          GraphicsSubmit(const VkSubmitInfo& submitInfo, const VkFence fence = VK_NULL_HANDLE);

  // Wait until the graphics queue has completely finished all submittals.
  void                          GraphicsWaitIdle();

  // Wait until compute queue has completely finished all submittals.
  void                          ComputeWaitIdle();

  // Wait until present queue has completely finished presenting onto the screen.
  void                          PresentWaitIdle();

  // Wait for the device to finished its submittals.
  void                          DeviceWaitIdle();

  // Submit a command buffer to the compute queue.
  void                          ComputeSubmit(const VkSubmitInfo& submitInfo);

  // Submit the current swapchain command buffer to the gpu. This will essentially be the 
  // call to the default render pass, which specifies the swapchain surface to render onto.
  // If no signals are specified, or signalSemaphoreCount is equal to 0, cmdbuffer will default
  // to GraphicsFinished semaphore, which is used by the Present queue to present to the screen!
  void                          SubmitCurrSwapchainCmdBuffer(u32 waitSemaphoreCount, VkSemaphore* waitSemaphores, 
                                  u32 signalSemaphoreCount, VkSemaphore* signalSemaphores);
  
  // Present the rendered surface.
  void                          Present();

  // Updates the renderer pipeline as a result of window resizing. This will effectively
  // recreate the entire pipeline! If any objects were referenced and whatnot, be sure to 
  // requery their resources as they have been recreated!
  void                          ReConfigure(VkPresentModeKHR presentMode, i32 width, i32 height);

  // Get the current image index that is used during rendering, the current image from the 
  // swapchain that we are rendering onto.
  u32                           CurrentImageIndex() { return mSwapchainInfo.mCurrentImageIndex; }

  // Swap commandbuffer sets within this vulkan rhi. Be sure that the set is already built!
  void                          SwapCommandBufferSets(u32 set) { mSwapchainInfo.mCmdBufferSet = set; }

  // Obtain the Graphics Finished Semaphore from swapchain.
  VkSemaphore                   GraphicsFinishedSemaphore() { return mSwapchain.GraphicsFinishedSemaphore(); }

  // Current set of swapchain commandbuffers that are currently in use by the gpu. Use this to determine which
  // set we shouldn't rebuild, while the gpu is using them!
  u32                           CurrentSwapchainCmdBufferSet() const { return mSwapchainInfo.mCmdBufferSet; }
  VkFramebuffer                 SwapchainFrameBuffer(size_t index) { return mSwapchainInfo.mSwapchainFramebuffers[index]; }
  size_t                        NumOfFramebuffers() { return mSwapchainInfo.mSwapchainFramebuffers.size(); }
  VkRenderPass                  SwapchainRenderPass() { return mSwapchainInfo.mSwapchainRenderPass; }
  VkPhysicalDeviceLimits        PhysicalDeviceLimits() { return mPhysicalDeviceProperties.limits; }
  u32                           VendorID() { return mPhysicalDeviceProperties.vendorID; }
  
  VkFormat                      DepthFormat() const { return mSwapchainInfo.mDepthFormat; }
  VkImageAspectFlags            DepthAspectFlags() const { return mSwapchainInfo.mDepthAspectFlags; }
  VkImageUsageFlags             DepthUsageFlags() const { return mSwapchainInfo.mDepthUsageFlags; }
  b8                            CmdBuffersComplete() { return mSwapchainInfo.mComplete; }
  const char*                   DeviceName() { return mPhysicalDeviceProperties.deviceName; }

private:
  void                          SetUpSwapchainRenderPass();
  void                          QueryFromSwapchain();
  void                          CreateDepthAttachment();
  void                          CreateSwapchainCommandBuffers(u32 swapSet);  
  void                          CreateOcclusionQueryPool(u32 queries);

  // Builds the descriptor pool for materials. WARNING: Recalling this function will
  // destroy old descriptor pool and replace with a new one, be sure to destroy all
  // descriptor sets previously allocated from old pool before recalling!
  void                          BuildDescriptorPool(u32 maxCount, u32 maxSets);

  HWND                          mWindow;
  Swapchain                     mSwapchain;
  LogicalDevice                 mLogicalDevice;
  VkSurfaceKHR                  mSurface;
  VkCommandPool                 mCmdPool;
  VkCommandPool                 mComputeCmdPool;
  VkDescriptorPool              mDescriptorPool;
  VkQueryPool                   mOccQueryPool;
  VkPhysicalDeviceProperties    mPhysicalDeviceProperties;

  // Framebuffers and Renderpass that is used by the swapchain. We must
  // first query the images from the swapchain.
  struct {
    std::vector<VkFramebuffer>                mSwapchainFramebuffers;
    //std::vector<CommandBuffer>                mSwapchainCmdBuffers;
    std::vector<std::vector<CommandBuffer> >  mCmdBufferSets;
    b8                                        mComplete;
    VkRenderPass                              mSwapchainRenderPass;
    VkImage                                   mDepthAttachment;
    VkImageView                               mDepthView;
    VkDeviceMemory                            mDepthMemory;
    VkFormat                                  mDepthFormat;
    VkImageAspectFlags                        mDepthAspectFlags;
    VkImageUsageFlags                         mDepthUsageFlags;
    u32                                       mCurrentImageIndex;
    i32                                       mCmdBufferSet;
  } mSwapchainInfo;

  SwapchainCmdBufferBuildFunc   mSwapchainCmdBufferBuild;
  u32                           mCurrDescSets;
};
} // Recluse