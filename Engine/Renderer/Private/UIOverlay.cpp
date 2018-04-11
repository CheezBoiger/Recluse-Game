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

#define NK_INCLUDE_FIXED_TYPES
#define NK_IMPLEMENTATION
#define NK_PRIVATE
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#include "nuklear.hpp"

namespace Recluse {


// Primitive canvas definition used by nuklear gui.
struct NkCanvas
{
  struct nk_command_buffer*  _pCmds;
  struct nk_vec2              _vItemSpacing;
  struct nk_vec2              _vPanelSpacing;
  struct nk_style_item       _windowBackground;
};


struct NkObject 
{
  nk_context              _ctx;
  std::vector<NkCanvas>   _canvasObjects;
};


void    InitializeNkObject(NkObject* obj)
{
  // TODO:
  nk_init_default(&obj->_ctx, 0); 
  R_DEBUG(rNotify, "GUI Nuklear initialized.\n");
}


void  CleanUpNkObject(NkObject* obj)
{
  // TODO:
  nk_free(&obj->_ctx);
  R_DEBUG(rNotify, "Finished cleaning up Nuklear Gui.\n");
}


void NkFontAtlasBegin(struct nk_font_atlas** atlas)
{
  // TODO:
}


void NkFontAtlasEnd()
{
  // TODO:
}


NkObject* gNkDevice()
{
  static NkObject nk;
  return &nk;
}


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

  CreateAttachmentTextures();
  InitializeFrameBuffer();
  SetUpGraphicsPipeline();

  // After initialization of our graphics gui pipeline, it's time to 
  // initialize nuklear.
  InitializeNkObject(gNkDevice());

  // TODO: Fonts should be stashed and loaded on an atlas in gpu memory.
}


void UIOverlay::CleanUp()
{
  // Allow us to clean up and release our nk context and object.
  CleanUpNkObject(gNkDevice());  

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
  // TODO:
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


void UIOverlay::CreateAttachmentTextures()
{
  
}


void UIOverlay::BuildCmdBuffers(CmdList* cmdList)
{
  // TODO:
}
} // Recluse