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

#include "CmdList.hpp"
#include "RenderCmd.hpp"


namespace Recluse {

void UIOverlay::Render()
{
  // Ignore if no reference to the rhi.
  if (!m_pRhi) return;

  // Render the overlay.
  u32 currCmdIdx = m_pRhi->CurrentImageIndex();
  CommandBuffer* cmdBuffer = m_CmdBuffers[currCmdIdx];
}


void UIOverlay::Initialize(VulkanRHI* rhi)
{
  m_pRhi = rhi;
  // Number of framebuffers defines the number of command buffers.
  m_CmdBuffers.resize(rhi->NumOfFramebuffers());

  for (size_t i = 0; i < m_CmdBuffers.size(); ++i) {
    m_CmdBuffers[i] = m_pRhi->CreateCommandBuffer();
  }

  m_pSemaphore = m_pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCi = { };
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_pSemaphore->Initialize(semaCi);

  InitializeFrameBuffer();
  SetUpGraphicsPipeline();
}


void UIOverlay::CleanUp()
{
  m_pRhi->FreeVkSemaphore(m_pSemaphore);
  m_pSemaphore = nullptr;

  if (!m_CmdBuffers.empty()) {
    for (size_t i = 0; i < m_CmdBuffers.size(); ++i) {
      CommandBuffer* cmdBuf = m_CmdBuffers[i];
      m_pRhi->FreeCommandBuffer(cmdBuf);
      m_CmdBuffers[i] = nullptr;
    }
  }

  // FrameBuffers are handled by the RHI context. Do not delete!
  
  GraphicsPipeline* pipeline = gResources().UnregisterGraphicsPipeline("UIOverlayPipeline");
  if (pipeline) {
    m_pRhi->FreeGraphicsPipeline(pipeline);
  }
}


void UIOverlay::InitializeFrameBuffer()
{
}


void UIOverlay::SetUpGraphicsPipeline()
{
  if (!gResources().GetGraphicsPipeline("UIOverlayPipeline")) {
    GraphicsPipeline* pipeline = m_pRhi->CreateGraphicsPipeline();
    gResources().RegisterGraphicsPipeline("UIOverlayPipeline", pipeline);
    VkGraphicsPipelineCreateInfo pipeCI = {};
    VkPipelineLayoutCreateInfo layoutCI = {};

    Shader* vert = m_pRhi->CreateShader();
    Shader* frag = m_pRhi->CreateShader();

    RendererPass::LoadShader("UI.vert.spv", vert);
    RendererPass::LoadShader("UI.frag.spv", frag);
    //pipeline->Initialize(pipeCI, layoutCI);

    m_pRhi->FreeShader(vert);
    m_pRhi->FreeShader(frag);
  }
}


void UIOverlay::BuildCmdBuffers(CmdList* cmdList)
{
}
} // Recluse