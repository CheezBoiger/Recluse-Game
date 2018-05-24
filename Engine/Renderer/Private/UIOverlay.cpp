// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "UIOverlay.hpp"
#include "Core/Exception.hpp"
#include "Renderer.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "Filesystem/Filesystem.hpp"

#include "VertexDescription.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/Framebuffer.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Shader.hpp"

#include "CmdList.hpp"
#include "RenderCmd.hpp"

#include <array>

#define NK_INCLUDE_FIXED_TYPES
#define NK_IMPLEMENTATION
#define NK_PRIVATE
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#include "nuklear.hpp"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

namespace Recluse {


// Primitive canvas definition used by nuklear gui.
struct NkCanvas
{
  struct nk_buffer*           _pCmds;
  struct nk_vec2              _vItemSpacing;
  struct nk_vec2              _vPanelSpacing;
  struct nk_style_item       _windowBackground;
};


static const struct nk_draw_vertex_layout_element vertLayout[] = {
  {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(UIVertex, position)},
  {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(UIVertex, texcoord)},
  {NK_VERTEX_COLOR, NK_FORMAT_FLOAT, NK_OFFSETOF(UIVertex, color)},
  {NK_VERTEX_LAYOUT_END}
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
  R_ASSERT(m_pRhi, "Null RHI for ui overlay!");

  // Render the overlay.
  CommandBuffer* cmdBuffer = m_CmdBuffer;

  
}


void UIOverlay::Initialize(VulkanRHI* rhi)
{
  m_pRhi = rhi;

  m_CmdBuffer = m_pRhi->CreateCommandBuffer();
  m_CmdBuffer->Allocate(m_pRhi->GraphicsCmdPool(1), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  m_pSemaphore = m_pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCi = { };
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_pSemaphore->Initialize(semaCi);

  InitializeRenderPass();
  CreateDescriptorSetLayout();
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
  CleanUpDescriptorSetLayout();

  m_pRhi->FreeVkSemaphore(m_pSemaphore);
  m_pSemaphore = nullptr;

  CommandBuffer* cmdBuf = m_CmdBuffer;
  m_pRhi->FreeCommandBuffer(cmdBuf);
  m_CmdBuffer = nullptr;

  if (m_renderPass) {
    m_pRhi->FreeRenderPass(m_renderPass);
    m_renderPass = nullptr;
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
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD,
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

  m_renderPass = m_pRhi->CreateRenderPass();
  m_renderPass->Initialize(renderpassCI);
}


void UIOverlay::SetUpGraphicsPipeline()
{
  if (!m_pGraphicsPipeline) {
    GraphicsPipeline* pipeline = m_pRhi->CreateGraphicsPipeline();
    m_pGraphicsPipeline = pipeline;

    VkPipelineInputAssemblyStateCreateInfo vertInputAssembly = { };
    vertInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vertInputAssembly.primitiveRestartEnable = VK_FALSE;
    // Triangle strip would be more preferable...
    vertInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    auto vertBinding = UIVertexDescription::GetBindingDescription();
    auto vertAttribs = UIVertexDescription::GetVertexAttributes();

    VkPipelineVertexInputStateCreateInfo vertInputState = { };
    vertInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputState.vertexAttributeDescriptionCount = static_cast<u32>(vertAttribs.size());
    vertInputState.pVertexAttributeDescriptions = vertAttribs.data();
    vertInputState.vertexBindingDescriptionCount = 1;
    vertInputState.pVertexBindingDescriptions = &vertBinding;
    
    VkPipelineRasterizationStateCreateInfo rasterCI = { };
    rasterCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterCI.cullMode =   VK_CULL_MODE_NONE;
    rasterCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterCI.depthBiasClamp = 0.0f;
    rasterCI.depthBiasEnable = VK_FALSE;
    rasterCI.lineWidth = 1.0f;
    rasterCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasterCI.rasterizerDiscardEnable = VK_FALSE;
    
    VkPipelineDepthStencilStateCreateInfo depthStencilCI = { };
    depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCI.depthWriteEnable = VK_FALSE;
    depthStencilCI.depthTestEnable = VK_FALSE;
    depthStencilCI.minDepthBounds = 0.0f;
    depthStencilCI.maxDepthBounds = 1.0f;
    depthStencilCI.front = { };
    depthStencilCI.back = { };
    depthStencilCI.stencilTestEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleCI = { };
    multisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCI.sampleShadingEnable = VK_FALSE;
    multisampleCI.alphaToOneEnable = VK_FALSE;
    multisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCI.alphaToCoverageEnable = VK_FALSE;
    multisampleCI.pSampleMask = nullptr;
    multisampleCI.minSampleShading = 1.0f;


    VkDynamicState dynamicStates[2] = {
      VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_VIEWPORT
    };
    VkPipelineDynamicStateCreateInfo dynamicCI = { };
    dynamicCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicCI.dynamicStateCount = 2;
    dynamicCI.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipeCI = {};
    VkPipelineLayoutCreateInfo layoutCI = {};

    VkExtent2D extent = m_pRhi->SwapchainObject()->SwapchainExtent();
  
    VkRect2D scissor;
    scissor.extent = extent;
    scissor.offset = { 0, 0 };

    VkViewport viewport = { };
    viewport.height = static_cast<r32>(extent.height);
    viewport.width = static_cast<r32>(extent.width);
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.minDepth = 0.0f;

    VkPipelineViewportStateCreateInfo viewportCI = { };
    viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCI.viewportCount = 1;
    viewportCI.scissorCount = 1;
    viewportCI.pScissors = &scissor;
    viewportCI.pViewports = &viewport;

    std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachments;
    blendAttachments[0].blendEnable = VK_TRUE;
    blendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachments[0].colorWriteMask = 0xf; // for rgba components.

    VkPipelineColorBlendStateCreateInfo colorBlendCI = { };
    colorBlendCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // No op as we are dealing with 
    colorBlendCI.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCI.logicOpEnable = VK_FALSE;
    colorBlendCI.attachmentCount = static_cast<u32>(blendAttachments.size());
    colorBlendCI.pAttachments = blendAttachments.data();

    Shader* vert = m_pRhi->CreateShader();
    Shader* frag = m_pRhi->CreateShader();

    RendererPass::LoadShader("UI.vert.spv", vert);
    RendererPass::LoadShader("UI.frag.spv", frag);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName = kDefaultShaderEntryPointStr;
    shaderStages[0].pSpecializationInfo = nullptr;
    shaderStages[0].module = vert->Handle();
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].pName = kDefaultShaderEntryPointStr;
    shaderStages[1].pSpecializationInfo = nullptr;
    shaderStages[1].module = frag->Handle();
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;


    pipeCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeCI.pInputAssemblyState = &vertInputAssembly;
    pipeCI.pVertexInputState = &vertInputState;
    pipeCI.pRasterizationState = &rasterCI;
    pipeCI.pDepthStencilState = &depthStencilCI;
    pipeCI.pMultisampleState = &multisampleCI;
    pipeCI.pTessellationState = nullptr;
    pipeCI.pViewportState = &viewportCI;
    pipeCI.pDynamicState = &dynamicCI;
    pipeCI.pColorBlendState = &colorBlendCI;
    pipeCI.pStages = shaderStages.data();
    pipeCI.stageCount = static_cast<u32>(shaderStages.size());
    pipeCI.renderPass = m_renderPass->Handle();
    pipeCI.subpass = 0;
    pipeCI.basePipelineHandle = VK_NULL_HANDLE;

    VkDescriptorSetLayout dSetLayouts[1] = {
      m_pDescLayout->Layout()
    };

    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.pPushConstantRanges = nullptr;
    layoutCI.pushConstantRangeCount = 0;
    layoutCI.setLayoutCount = 1;
    layoutCI.pSetLayouts = dSetLayouts;
   
    pipeline->Initialize(pipeCI, layoutCI);

    m_pRhi->FreeShader(vert);
    m_pRhi->FreeShader(frag);
  }
}


void UIOverlay::BuildCmdBuffers(CmdList<UiRenderCmd>& cmdList)
{
  // TODO:
  CommandBuffer* cmdBuffer = m_CmdBuffer;
  cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

  VkCommandBufferBeginInfo beginInfo = { };
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  VkClearValue clearValues[1];
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

  VkExtent2D extent = m_pRhi->SwapchainObject()->SwapchainExtent();
  VkRenderPassBeginInfo renderPassBeginInfo = { };
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.framebuffer = final_frameBufferKey->Handle();
  renderPassBeginInfo.renderPass = m_renderPass->Handle();
  renderPassBeginInfo.renderArea.extent = extent;
  renderPassBeginInfo.renderArea.offset = { 0, 0 };
  renderPassBeginInfo.clearValueCount = 1;
  renderPassBeginInfo.pClearValues = clearValues;

  // Map vertex and index buffers.
#if 0
  NkCanvas canvas;
  m_indicesBuffer->Map();
  m_vertBuffer->Map();
  {
    struct nk_convert_config cfg = { };

    cfg.line_AA = NK_ANTI_ALIASING_ON;
    cfg.shape_AA = NK_ANTI_ALIASING_ON;
    cfg.vertex_layout = vertLayout;
    cfg.vertex_size = sizeof(UIVertex);
    cfg.vertex_alignment = NK_ALIGNOF(UIVertex);
    cfg.global_alpha = 1.0f;

    {
      struct nk_buffer vbuf, ebuf;
      nk_buffer_init_fixed(&vbuf, m_vertBuffer->Mapped(), MAX_VERTEX_MEMORY);
      nk_buffer_init_fixed(&ebuf, m_indicesBuffer->Mapped(), MAX_ELEMENT_MEMORY);
      // TODO(): canvas needs to be defined by the ui instead.
      nk_convert(&gNkDevice()->_ctx, canvas._pCmds, &vbuf, &ebuf, &cfg);
    }
  }
  m_vertBuffer->UnMap();
  m_indicesBuffer->UnMap();
#endif
  // Unmap vertices and index buffers, then perform drawing here.
  cmdBuffer->Begin(beginInfo);  
    // When we render with secondary command buffers, we use VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS.
    cmdBuffer->BeginRenderPass(renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
      for (size_t i = 0; i < cmdList.Size(); ++i) {
        const UiRenderCmd& uiCmd = cmdList[i];
        if (!(uiCmd._config & CMD_RENDERABLE_BIT)) { continue; }
        const struct nk_draw_command* cmd;
#if 0
        nk_draw_foreach(cmd, &gNkDevice()->_ctx,  canvas._pCmds) {
          if (!cmd->elem_count) continue;
        }
#endif
      }
    cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void UIOverlay::CreateDescriptorSetLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 1> bindings;
  bindings[0].binding = 0;
  bindings[0].descriptorCount = 1;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[0].pImmutableSamplers = nullptr;
  bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo descLayoutCI = { };
  descLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descLayoutCI.bindingCount = static_cast<u32>(bindings.size());
  descLayoutCI.pBindings = bindings.data();

  m_pDescLayout = m_pRhi->CreateDescriptorSetLayout();
  m_pDescLayout->Initialize(descLayoutCI);
}

void UIOverlay::CleanUpDescriptorSetLayout()
{
  m_pRhi->FreeDescriptorSetLayout(m_pDescLayout);
  m_pDescLayout = nullptr;
}
} // Recluse