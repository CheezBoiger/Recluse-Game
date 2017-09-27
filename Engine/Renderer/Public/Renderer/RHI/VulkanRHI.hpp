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

// Set swapchain command buffer function. Assume that the commandbuffer automatically
// calls Begin() before the this function and End() and the end of the function.
// Also passes the default renderpass begin info to render onscreen.
typedef std::function<void(CommandBuffer&, VkRenderPassBeginInfo&)> SwapchainCmdBufferBuildFunc;

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

  void                          SetSwapchainCmdBufferBuild(SwapchainCmdBufferBuildFunc func) { mSwapchainCmdBufferBuild = func; }
  void                          RebuildCommandBuffers();

  VkDevice                      Device() { return mLogicalDevice.Handle(); }
  Swapchain*                    SwapchainObject() { return &mSwapchain; }
  VkSurfaceKHR                  Surface() { return mSurface; }
  VkCommandPool                 GraphicsCmdPool() { return mCmdPool; }
  VkCommandPool                 ComputeCmdPool() { return mComputeCmdPool; }

  void                          FlushCommands();
  // Returns the image index.
  void                          AcquireNextImage();
  void                          GraphicsSubmit(const VkSubmitInfo& submitInfo);
  void                          GraphicsWaitIdle();
  void                          ComputeWaitIdle();
  void                          PresentWaitIdle();
  void                          ComputeSubmit(const VkSubmitInfo& submitInfo);
  void                          SubmitCurrSwapchainCmdBuffer();
  void                          Present();
  void                          UpdateFromWindowChange();

private:
  void                          SetUpSwapchainRenderPass();
  void                          QueryFromSwapchain();
  void                          CreateDepthAttachment();
  void                          CreateSwapchainCommandBuffers();

  HWND                          mWindow;
  Swapchain                     mSwapchain;
  LogicalDevice                 mLogicalDevice;
  VkSurfaceKHR                  mSurface;
  VkCommandPool                 mCmdPool;
  VkCommandPool                 mComputeCmdPool;
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