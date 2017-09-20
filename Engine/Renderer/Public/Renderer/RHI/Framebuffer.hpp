// Copyright (c) 2017 Recluse Project.
#pragma once

#include "VulkanConfigs.hpp"
#include "Core/Types.hpp"

namespace Recluse {


class RenderTarget;


class FrameBuffer : public VulkanHandle {
public:
  FrameBuffer()
    : mHandle(VK_NULL_HANDLE)
    , mRenderPass(VK_NULL_HANDLE) { }
  void          CleanUp();
  void          Finalize(const VkFramebufferCreateInfo& info, const VkRenderPassCreateInfo& renderpass);

  VkRenderPass  RenderPass() { return mRenderPass; } 
  VkFramebuffer Handle() { return mHandle; }
private:
  VkFramebuffer mHandle;
  VkRenderPass  mRenderPass;
};
} // Recluse