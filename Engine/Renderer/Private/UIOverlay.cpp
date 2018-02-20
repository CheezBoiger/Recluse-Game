// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "UIOverlay.hpp"
#include "Core/Exception.hpp"
#include "Renderer.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "Filesystem/Filesystem.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/Framebuffer.hpp"


namespace Recluse {

void UIOverlay::Render()
{
  // Ignore if no reference to the rhi.
  if (!m_pRhiRef) return;

  // Render the overlay.
  u32 currCmdIdx = m_pRhiRef->CurrentImageIndex();
  CommandBuffer* cmdBuffer = m_CmdBuffers[currCmdIdx];
}


void UIOverlay::Initialize(VulkanRHI* rhi)
{
  m_pRhiRef = rhi;

  if (!gResources().GetGraphicsPipeline("UIOverlayPipeline")) {
    GraphicsPipeline* pipeline = rhi->CreateGraphicsPipeline();
    gResources().RegisterGraphicsPipeline("UIOverlayPipeline", pipeline);
    VkGraphicsPipelineCreateInfo pipeCI = { };
    VkPipelineLayoutCreateInfo layoutCI = { };

    Shader* vert = rhi->CreateShader();
    Shader* frag = rhi->CreateShader();

    RendererPass::LoadShader("UI.vert.spv", vert);
    RendererPass::LoadShader("UI.frag.spv", frag);
    //pipeline->Initialize(pipeCI, layoutCI);

    rhi->FreeShader(vert);
    rhi->FreeShader(frag);
  }
    
  // Number of framebuffers defines the number of command buffers.
  m_CmdBuffers.resize(rhi->NumOfFramebuffers());
  m_FrameBuffers.resize(rhi->NumOfFramebuffers());
 
  for (size_t i = 0; i < m_FrameBuffers.size(); ++i) {
    m_FrameBuffers[i] = rhi->SwapchainFrameBuffer(i);
  }

  for (size_t i = 0; i < m_CmdBuffers.size(); ++i) {
    m_CmdBuffers[i] = m_pRhiRef->CreateCommandBuffer();
  }

  m_pSemaphore = m_pRhiRef->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCi = { };
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_pSemaphore->Initialize(semaCi);
}


void UIOverlay::CleanUp()
{
  m_pRhiRef->FreeVkSemaphore(m_pSemaphore);
  m_pSemaphore = nullptr;

  if (!m_CmdBuffers.empty()) {
    for (size_t i = 0; i < m_CmdBuffers.size(); ++i) {
      CommandBuffer* cmdBuf = m_CmdBuffers[i];
      m_pRhiRef->FreeCommandBuffer(cmdBuf);
      m_CmdBuffers[i] = nullptr;
    }
  }

  // FrameBuffers are handled by the RHI context. Do not delete!
  
  GraphicsPipeline* pipeline = gResources().UnregisterGraphicsPipeline("UIOverlayPipeline");
  if (pipeline) {
    m_pRhiRef->FreeGraphicsPipeline(pipeline);
  }
}
} // Recluse