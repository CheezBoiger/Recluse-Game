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
#include "RHI/Texture.hpp"

#include "CmdList.hpp"
#include "RenderCmd.hpp"

#include <array>

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

  InitializeRenderPass();
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

  if (m_renderPass) {
    vkDestroyRenderPass(m_pRhi->LogicDevice()->Native(), m_renderPass, nullptr);
    m_renderPass = VK_NULL_HANDLE;
  }
 
  if (m_pGraphicsPipeline) {
    m_pRhi->FreeGraphicsPipeline(m_pGraphicsPipeline);
    m_pGraphicsPipeline = nullptr;
  }
}


void UIOverlay::InitializeRenderPass()
{
  std::array<VkAttachmentDescription, 1> attachmentDescriptions;
  VkSubpassDependency dependencies[2];
  attachmentDescriptions[0] = CreateAttachmentDescription(
    final_renderTargetKey->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    final_renderTargetKey->Samples()
  );

  dependencies[0] = CreateSubPassDependency(
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  dependencies[1] = CreateSubPassDependency(
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  std::array<VkAttachmentReference, 1> attachmentColors;
  attachmentColors[0].attachment = 0;
  attachmentColors[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<u32>(attachmentColors.size());
  subpass.pColorAttachments = attachmentColors.data();
  subpass.pDepthStencilAttachment = nullptr;

  VkRenderPassCreateInfo renderpassCI = CreateRenderPassInfo(
    static_cast<u32>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    2,
    dependencies,
    1,
    &subpass
  );

  VkResult result = vkCreateRenderPass(m_pRhi->LogicDevice()->Native(), &renderpassCI, nullptr, &m_renderPass);
  R_ASSERT(result ==  VK_SUCCESS, "Failed to create renderpass for ui!\n");
}


void UIOverlay::SetUpGraphicsPipeline()
{
  if (!m_pGraphicsPipeline) {
    GraphicsPipeline* pipeline = m_pRhi->CreateGraphicsPipeline();
    m_pGraphicsPipeline = pipeline;

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


void UIOverlay::BuildCmdBuffers(CmdList<UiRenderCmd>* cmdList)
{
  // TODO:
}
} // Recluse