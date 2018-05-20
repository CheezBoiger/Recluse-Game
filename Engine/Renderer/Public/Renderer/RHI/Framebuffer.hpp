// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "VulkanConfigs.hpp"
#include "Core/Types.hpp"

namespace Recluse {


class RenderTarget;


class FrameBuffer : public VulkanHandle {
public:
  FrameBuffer()
    : mHandle(VK_NULL_HANDLE)
    , mRenderPass(VK_NULL_HANDLE)
    , m_Width(0)
    , m_Height(0) { }

  void          CleanUp();
  void          Finalize(VkFramebufferCreateInfo& info, const VkRenderPassCreateInfo& renderpass);

  VkRenderPass  RenderPass() { return mRenderPass; } 
  VkFramebuffer Handle() { return mHandle; }

  u32           Width() const { return m_Width; }
  u32           Height() const { return m_Height; }

private:
  VkFramebuffer mHandle;
  VkRenderPass  mRenderPass;
  u32           m_Width;
  u32           m_Height;
};


class RenderPass : public VulkanHandle {
public:
  void CleanUp();
  void Initialize(const VkRenderPassCreateInfo& info);

  VkRenderPass Handle() { return m_renderPass; }

private:
  VkRenderPass m_renderPass;
};
} // Recluse