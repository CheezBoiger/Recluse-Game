// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "UIOverlay.hpp"
#include "Core/Exception.hpp"
#include "Renderer.hpp"
#include "Resources.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Commandbuffer.hpp"

namespace Recluse {

void UIOverlay::Render()
{
  // Ignore if no reference to the rhi.
  if (!mRhiRef) return;

  // Render the overlay.
  u32 currCmdIdx = mRhiRef->CurrentImageIndex();
  CommandBuffer* cmdBuffer = mCmdBuffers[currCmdIdx];

  
}


void UIOverlay::Initialize(VulkanRHI* rhi)
{
  mRhiRef = rhi;

  if (!gResources().GetGraphicsPipeline("UIOverlayPipeline")) {
    GraphicsPipeline* pipeline = rhi->CreateGraphicsPipeline();
    gResources().RegisterGraphicsPipeline("UIOverlayPipeline", pipeline);
    VkGraphicsPipelineCreateInfo pipeCI = { };
    VkPipelineLayoutCreateInfo layoutCI = { };

    //pipeline->Initialize(pipeCI, layoutCI);
  }
    
  // Number of framebuffers defines the number of command buffers.
  mCmdBuffers.resize(rhi->NumOfFramebuffers());
  mFrameBuffers.resize(rhi->NumOfFramebuffers());

  for (size_t i = 0; i < mFrameBuffers.size(); ++i) {
    mFrameBuffers[i] = mRhiRef->CreateFrameBuffer();

  }

  for (size_t i = 0; i < mCmdBuffers.size(); ++i) {
    mCmdBuffers[i] = mRhiRef->CreateCommandBuffer();

  }
}


void UIOverlay::CleanUp()
{
  if (!mCmdBuffers.empty()) {
    for (size_t i = 0; i < mCmdBuffers.size(); ++i) {
      CommandBuffer* cmdBuf = mCmdBuffers[i];
      mRhiRef->FreeCommandBuffer(cmdBuf);
      mCmdBuffers[i] = nullptr;
    }
  }

  if (!mFrameBuffers.empty()) {
    for (size_t i = 0; i < mFrameBuffers.size(); ++i) {
      FrameBuffer* framebuffer = mFrameBuffers[i];
      if (framebuffer) {
        mRhiRef->FreeFrameBuffer(framebuffer);
        mFrameBuffers[i] = nullptr;
      }
    }
  }
  
  GraphicsPipeline* pipeline = gResources().UnregisterGraphicsPipeline("UIOverlayPipeline");
  if (pipeline) {
    mRhiRef->FreeGraphicsPipeline(pipeline);
  }
}
} // Recluse