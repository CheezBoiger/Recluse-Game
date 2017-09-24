// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"
#include "Swapchain.hpp"
#include "VulkanContext.hpp"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"

namespace Recluse {

class Buffer;
class GraphicsPipeline;
class ComputePipeline;
class FrameBuffer;
class Rendertarget;
class Shader;
class CommandBuffer;
class Sampler;
class Texture;
class DescriptorSet;

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
  void                        Initialize(HWND windowHandle);

  // Clean up this object. This must be called once done with this RHI
  // object.
  void                        CleanUp();

  // TODO(): We need to allocate on a custom allocator to save time with
  // memory management.
  Buffer*                     CreateBuffer();
  GraphicsPipeline*           CreateGraphicsPipeline();
  ComputePipeline*            CreateComputePipeline();
  FrameBuffer*                CreateFrameBuffer();
  Sampler*                    CreateSampler();
  Texture*                    CreateTexture();
  Shader*                     CreateShader();
  CommandBuffer*              CreateCommandBuffer();
  DescriptorSet*              CreateDescriptorSet();

  void                        FreeBuffer(Buffer& buffer);
  void                        FreeGraphicsPipeline(GraphicsPipeline& pipeline);
  void                        FreeComputePipeline(ComputePipeline& pipeline);
  void                        FreeFrameBuffer(FrameBuffer& buffer);
  void                        FreeSampler(Sampler& sampler);
  void                        FreeTexture(Texture& texture);
  void                        FreeShader(Shader& shader);
  void                        FreeCommandBuffer(CommandBuffer& buffer);
  void                        FreeDescriptorSet(DescriptorSet& set);

  VkDevice                    Device() { return mLogicalDevice.Handle(); }
  Swapchain*                  SwapchainObject() { return &mSwapchain; }
  VkSurfaceKHR                Surface() { return mSurface; }
  VkCommandPool               PrimaryCmdPool() { return mCmdPool; }
  VkCommandPool               SecondaryCmdPool() { return mSecondaryCmdPool; }

  void                        FlushCommands();
  // Returns the image index.
  void                        AcquireNextImage();
  void                        GraphicsSubmit(const VkSubmitInfo& submitInfo);
  void                        GraphicsWaitIdle();
  void                        ComputeWaitIdle();
  void                        PresentWaitIdle();
  void                        ComputeSubmit(const VkSubmitInfo& submitInfo);
  void                        Present();
  void                        UpdateFromWindowChange();

private:
  void                        SetUpSwapchainRenderPass();
  void                        QueryFromSwapchain();
  void                        CreateDepthAttachment();

  HWND                        mWindow;
  Swapchain                   mSwapchain;
  LogicalDevice               mLogicalDevice;
  VkSurfaceKHR                mSurface;
  VkCommandPool               mCmdPool;
  VkCommandPool               mSecondaryCmdPool;
  // Framebuffers and Renderpass that is used by the swapchain. We must
  // first query the images from the swapchain.
  struct {
    std::vector<VkFramebuffer>  mSwapchainFramebuffers;
    VkRenderPass                mSwapchainRenderPass;
    VkImage                     mDepthAttachment;
    VkImageView                 mDepthView;
    VkDeviceMemory              mDepthMemory;
    u32                         mCurrentImageIndex;
  } mSwapchainInfo;
};
} // Recluse