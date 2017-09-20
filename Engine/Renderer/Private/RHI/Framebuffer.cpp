// Copyright (c) 2017 Recluse Project.
#include "RHI/Framebuffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void FrameBuffer::Finalize(const VkFramebufferCreateInfo& info,
  const VkRenderPassCreateInfo& renderpass)
{
  VkResult result = vkCreateFramebuffer(mOwner, &info, nullptr, &mHandle);
  if (result != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create framebuffer!\n");
    return;
  }

  result = vkCreateRenderPass(mOwner, &renderpass, nullptr, &mRenderPass);
  if (result != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create underlying renderpass for framebuffer!\n");  
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