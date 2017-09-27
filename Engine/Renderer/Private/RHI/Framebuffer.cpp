// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/FrameBuffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void FrameBuffer::Finalize(VkFramebufferCreateInfo& info,
  const VkRenderPassCreateInfo& renderpass)
{
  VkResult result = vkCreateRenderPass(mOwner, &renderpass, nullptr, &mRenderPass);
  if (result != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create underlying renderpass for framebuffer!\n");
  }

  info.renderPass = mRenderPass;

  result = vkCreateFramebuffer(mOwner, &info, nullptr, &mHandle);
  if (result != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create framebuffer!\n");
    return;
  }
}


void FrameBuffer::CleanUp()
{
  if (mHandle) {
    vkDestroyFramebuffer(mOwner, mHandle, nullptr);
    vkDestroyRenderPass(mOwner, mRenderPass, nullptr);
    mHandle = VK_NULL_HANDLE;
    mRenderPass = VK_NULL_HANDLE;
  }
}
} // Recluse