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
class UserParams;

// Set swapchain command buffer function. Assume that the commandbuffer automatically
// calls Begin() before the this function and End() and the end of the function.
// Also passes the default renderpass begin info to render onscreen.
typedef std::function<void(CommandBuffer&, VkRenderPassBeginInfo&)> SwapchainCmdBufferBuildFunc;


class Semaphore : public VulkanHandle {
public:
  Semaphore()
    : mSema(VK_NULL_HANDLE) { }

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

  // Set the default swapchain command buffers, which is defined by the programmer of the renderer.
  void                          SetSwapchainCmdBufferBuild(SwapchainCmdBufferBuildFunc func) { mSwapchainCmdBufferBuild = func; }

  // Rebuild the swapchain commandbuffers with using the function provided from SetSwapchainCmdBufferBuild.
  // This will build the swapchain command buffers.
  void                          RebuildCommandBuffers();

  // Get the logical device.
  VkDevice                      Device() { return mLogicalDevice.Handle(); }

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

  // Create a semaphore object.
  Semaphore*                    CreateVkSemaphore();

  // Free a semaphore that was created by this RHI.
  void                          FreeVkSemaphore(Semaphore* semaphore);

  // Flush out commands that are on hold.
  void                          FlushCommands();

  // Returns the image index to the current swapchain surface to render onto.
  void                          AcquireNextImage();

  // Submit a command buffer to the graphics queue.
  void                          GraphicsSubmit(const VkSubmitInfo& submitInfo);

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
  void                          SubmitCurrSwapchainCmdBuffer(u32 waitSemaphoreCount, VkSemaphore* waitSemaphores);
  
  // Present the rendered surface.
  void                          Present();

  // Updates the renderer pipeline as a result of window resizing. This will effectively
  // recreate the entire pipeline! If any objects were referenced and whatnot, be sure to 
  // requery their resources as they have been recreated!
  void                          ReConfigure(VkPresentModeKHR presentMode, i32 width, i32 height);

  // Get the current image index that is used during rendering, the current image from the 
  // swapchain that we are rendering onto.
  u32                           CurrentImageIndex() { return mSwapchainInfo.mCurrentImageIndex; }

  VkFramebuffer                 SwapchainFrameBuffer(size_t index) { return mSwapchainInfo.mSwapchainFramebuffers[index]; }
  size_t                        NumOfFramebuffers() { return mSwapchainInfo.mSwapchainFramebuffers.size(); }
  VkRenderPass                  SwapchainRenderPass() { return mSwapchainInfo.mSwapchainRenderPass; }
  VkPhysicalDeviceLimits        PhysicalDeviceLimits() { return mPhysicalDeviceLimits; }

private:
  void                          SetUpSwapchainRenderPass();
  void                          QueryFromSwapchain();
  void                          CreateDepthAttachment();
  void                          CreateSwapchainCommandBuffers();  

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
  VkPhysicalDeviceLimits        mPhysicalDeviceLimits;

  // Framebuffers and Renderpass that is used by the swapchain. We must
  // first query the images from the swapchain.
  struct {
    std::vector<VkFramebuffer>  mSwapchainFramebuffers;
    std::vector<CommandBuffer>  mSwapchainCmdBuffers;
    VkRenderPass                mSwapchainRenderPass;
    VkImage                     mDepthAttachment;
    VkImageView                 mDepthView;
    VkDeviceMemory              mDepthMemory;
    u32                         mCurrentImageIndex;
  } mSwapchainInfo;

  SwapchainCmdBufferBuildFunc   mSwapchainCmdBufferBuild;
};
} // Recluse