// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "VulkanConfigs.hpp"
#include "Core/Types.hpp"

namespace Recluse {


class RenderTarget;


// RenderPass instance.
class RenderPass : public VulkanHandle {
public:
  RenderPass()
    : m_renderPass(VK_NULL_HANDLE) { }
  void cleanUp();
  void initialize(const VkRenderPassCreateInfo& info);

  VkRenderPass getHandle() const { return m_renderPass; }

private:
  VkRenderPass m_renderPass;
};


// FrameBuffer instance.
class FrameBuffer : public VulkanHandle {
public:
  FrameBuffer()
    : mHandle(VK_NULL_HANDLE)
    , m_pRenderPassRef(nullptr)
    , m_Width(0)
    , m_Height(0) { }

  void          cleanUp();
  
  // Finalizing handles the .renderPass value for us. As long as the render pass is already
  // created.
  void          Finalize(VkFramebufferCreateInfo& info, const RenderPass* renderpass);

  VkFramebuffer getHandle() { return mHandle; }

  const RenderPass*   RenderPassRef() { return m_pRenderPassRef; }

  u32           getWidth() const { return m_Width; }
  u32           getHeight() const { return m_Height; }

private:
  const RenderPass*   m_pRenderPassRef;
  VkFramebuffer       mHandle;
  u32                 m_Width;
  u32                 m_Height;
};
} // Recluse