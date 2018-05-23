// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/FrameBuffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void FrameBuffer::Finalize(VkFramebufferCreateInfo& info,
  const RenderPass* renderpass)
{
  R_ASSERT(renderpass, "RenderPass is null for this frame buffer.");
  info.renderPass = renderpass->Handle();
  m_Width = info.width;
  m_Height = info.height;

  VkResult result = vkCreateFramebuffer(mOwner, &info, nullptr, &mHandle);
  if (result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create framebuffer!\n");
    return;
  }

  m_pRenderPassRef = renderpass;
}


void FrameBuffer::CleanUp()
{
  if (mHandle) {
    vkDestroyFramebuffer(mOwner, mHandle, nullptr);
    mHandle = VK_NULL_HANDLE;
  }

  m_pRenderPassRef = nullptr;
}


void RenderPass::Initialize(const VkRenderPassCreateInfo& info)
{
  VkResult result = vkCreateRenderPass(mOwner, &info, nullptr, &m_renderPass);
  R_ASSERT(result == VK_SUCCESS, "Failed to create a renderpass!");
}


void RenderPass::CleanUp()
{
  if (m_renderPass) {
    vkDestroyRenderPass(mOwner, m_renderPass, nullptr);
    m_renderPass = VK_NULL_HANDLE;
  }
}
} // Recluse