// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "UIOverlay.hpp"
#include "Core/Exception.hpp"
#include "Core/Math/Common.hpp"
#include "Renderer.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "GlobalDescriptor.hpp"
#include "Filesystem/Filesystem.hpp"

#include "Core/Utility/Time.hpp"
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

#define MAX_VERTEX_MEMORY 512 * 4096
#define MAX_ELEMENT_MEMORY 128 * 4096

namespace Recluse {


// Primitive canvas definition used by nuklear gui.
struct NkCanvas
{
  struct nk_command_buffer    _commandBuffer;
  struct nk_vec2              _vItemSpacing;
  struct nk_vec2              _vPanelSpacing;
  struct nk_style_item       _windowBackground;
};


static const struct nk_draw_vertex_layout_element vertLayout[] = {
  {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(UIVertex, position)},
  {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(UIVertex, texcoord)},
  {NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(UIVertex, color)},
  {NK_VERTEX_LAYOUT_END}
};


struct NkObject 
{
  nk_context                  _ctx;       // Nuklear context.
  struct nk_buffer            _cmds;      // We'll stick to one canvas for now, since we are
                                          // still working on the UI.
  struct nk_font_atlas        _atlas;
  struct nk_font*             _font;
  struct nk_draw_null_texture _null;
  Buffer*                     _cache;
  Texture*                    _texture;
  Sampler*                    _sampler;
  DescriptorSet*              _font_set;
};


void UploadAtlas(NkObject* obj, const void* image, i32 w, i32 h, VulkanRHI* rhi)
{
  // Copy over to host visible cache buffer.
  memcpy(obj->_cache->Mapped(), image, w * h * 4);
  {
    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.size = w * h * 4;
    range.memory = obj->_cache->Memory();
    rhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }
  
  // Submit command buffer to stream over the cache data to gpu texture.
  CommandBuffer cmdBuffer;
  cmdBuffer.SetOwner(rhi->LogicDevice()->Native());
  cmdBuffer.Allocate(rhi->TransferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  VkCommandBufferBeginInfo begin = { };
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
  // Max barriers.
  std::vector<VkBufferImageCopy> bufferCopies(obj->_texture->MipLevels());
  size_t offset = 0;
  for (u32 mipLevel = 0; mipLevel < obj->_texture->MipLevels(); ++mipLevel) {
    VkBufferImageCopy region = { };
    region.bufferOffset = offset;
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageExtent.width = obj->_texture->Width();
    region.imageExtent.height = obj->_texture->Height();
    region.imageExtent.depth = 1;
    region.imageOffset = { 0, 0, 0 };
    bufferCopies[mipLevel] = region;
  }

  VkImageMemoryBarrier imgBarrier = {};
  imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imgBarrier.image = obj->_texture->Image();
  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgBarrier.srcAccessMask = 0;
  imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgBarrier.subresourceRange.baseArrayLayer = 0;
  imgBarrier.subresourceRange.baseMipLevel = 0;
  imgBarrier.subresourceRange.layerCount = obj->_texture->ArrayLayers();
  imgBarrier.subresourceRange.levelCount = obj->_texture->MipLevels();

  // TODO(): Copy buffer to image stream.
  cmdBuffer.Begin(begin);
  // Image memory barrier.
  cmdBuffer.PipelineBarrier(
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  // Send buffer image copy cmd.
  cmdBuffer.CopyBufferToImage(
    obj->_cache->NativeBuffer(),
    obj->_texture->Image(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    static_cast<u32>(bufferCopies.size()),
    bufferCopies.data()
  );

  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  cmdBuffer.PipelineBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  cmdBuffer.End();

  // TODO(): Submit it to graphics queue!
  VkCommandBuffer commandbuffers[] = { cmdBuffer.Handle() };

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = commandbuffers;

  rhi->TransferSubmit(DEFAULT_QUEUE_IDX, 1, &submit);
  rhi->TransferWaitIdle(DEFAULT_QUEUE_IDX);

  // The buffer after use.
  cmdBuffer.Free();
}


void InitImageBuffers(NkObject* obj, i32 w, i32 h, VulkanRHI* rhi, UIOverlay* overlay)
{
  obj->_cache = rhi->CreateBuffer();
  obj->_texture = rhi->CreateTexture();
  obj->_sampler = rhi->CreateSampler();
  obj->_font_set = rhi->CreateDescriptorSet();
  VkImageCreateInfo imgCi = {};
  VkSamplerCreateInfo samplerCi = {};
  VkImageViewCreateInfo viewCi = {};

  imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCi.arrayLayers = 1;
  imgCi.extent.depth = 1;
  imgCi.extent.width = static_cast<u32>(w);
  imgCi.extent.height = static_cast<u32>(h);
  imgCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCi.imageType = VK_IMAGE_TYPE_2D;
  imgCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCi.mipLevels = 1;
  imgCi.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCi.components = {};
  viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewCi.subresourceRange.baseArrayLayer = 0;
  viewCi.subresourceRange.baseMipLevel = 0;
  viewCi.subresourceRange.layerCount = 1;
  viewCi.subresourceRange.levelCount = 1;
  viewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;

  samplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.anisotropyEnable = VK_FALSE;
  samplerCi.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerCi.compareEnable = VK_FALSE;
  samplerCi.compareOp = VK_COMPARE_OP_LESS;
  samplerCi.magFilter = VK_FILTER_LINEAR;
  samplerCi.minFilter = VK_FILTER_LINEAR;
  samplerCi.maxAnisotropy = 16.0f;
  samplerCi.maxLod = 1.0f;
  samplerCi.minLod = 0.0f;
  samplerCi.mipLodBias = 0.0f;
  samplerCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCi.unnormalizedCoordinates = VK_FALSE;

  VkBufferCreateInfo bufferCi = {};
  bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCi.size = w * h * 4; // 4 component * width * height.
  bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  obj->_texture->Initialize(imgCi, viewCi);
  obj->_sampler->Initialize(samplerCi);
  obj->_cache->Initialize(bufferCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  obj->_cache->Map();

  obj->_font_set->Allocate(rhi->DescriptorPool(), overlay->GetMaterialLayout());
  VkDescriptorImageInfo img = { };
  img.sampler = obj->_sampler->Handle();
  img.imageView = obj->_texture->View();
  img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet writeSet = { };
  writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSet.descriptorCount = 1;
  writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSet.dstArrayElement = 0;
  writeSet.dstBinding = 0;
  writeSet.dstSet = 0;
  writeSet.pImageInfo = &img;

  obj->_font_set->Update(1, &writeSet);
}


void DestroyImageBuffers(NkObject* obj, VulkanRHI* rhi)
{
  rhi->GraphicsWaitIdle(DEFAULT_QUEUE_IDX);
  rhi->FreeTexture(obj->_texture);
  rhi->FreeDescriptorSet(obj->_font_set);
  obj->_cache->UnMap();
  rhi->FreeBuffer(obj->_cache);
  rhi->FreeSampler(obj->_sampler);
}


void    InitializeNkObject(NkObject* obj, VulkanRHI* rhi, UIOverlay* overlay)
{
  // TODO:
  nk_init_default(&obj->_ctx, 0); 
  nk_buffer_init_default(&obj->_cmds);
  nk_font_atlas_init_default(&obj->_atlas);
  nk_font_atlas_begin(&obj->_atlas);
  obj->_font = nk_font_atlas_add_default(&obj->_atlas, 13, 0);
  i32 w, h;
  const void* image = nk_font_atlas_bake(&obj->_atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
  InitImageBuffers(obj, w, h, rhi, overlay);
  UploadAtlas(obj, image, w, h, rhi);
  nk_font_atlas_end(&obj->_atlas, nk_handle_ptr(obj->_font_set), &obj->_null);
  nk_init_default(&obj->_ctx, &obj->_font->handle);
  R_DEBUG(rNotify, "GUI Nuklear initialized.\n");
}


void  CleanUpNkObject(NkObject* obj, VulkanRHI* rhi)
{
  // TODO:
  nk_font_atlas_clear(&obj->_atlas);
  nk_free(&obj->_ctx);
  nk_buffer_free(&obj->_cmds);
  DestroyImageBuffers(obj, rhi);
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
  CreateBuffers();
  // After initialization of our graphics gui pipeline, it's time to 
  // initialize nuklear.
  InitializeNkObject(gNkDevice(), rhi, this);

  // TODO: Fonts should be stashed and loaded on an atlas in gpu memory.
}


void UIOverlay::CleanUp()
{
  // Allow us to clean up and release our nk context and object.
  CleanUpNkObject(gNkDevice(), m_pRhi);  
  CleanUpDescriptorSetLayout();
  CleanUpBuffers();

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
    blendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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

    VkDescriptorSetLayout dSetLayouts[2] = {
      GlobalSetLayoutKey->Layout(),
      m_pDescLayout->Layout()
    };

    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.pPushConstantRanges = nullptr;
    layoutCI.pushConstantRangeCount = 0;
    layoutCI.setLayoutCount = 2;
    layoutCI.pSetLayouts = dSetLayouts;
   
    pipeline->Initialize(pipeCI, layoutCI);

    m_pRhi->FreeShader(vert);
    m_pRhi->FreeShader(frag);
  }
}


void UIOverlay::BuildCmdBuffers(CmdList<UiRenderCmd>& cmdList, GlobalDescriptor* global)
{
  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.height = (r32)m_pRhi->SwapchainObject()->SwapchainExtent().height;
  viewport.width = (r32)m_pRhi->SwapchainObject()->SwapchainExtent().width;

  // TODO(): This needs to be programmable now. Not hardcoded this way...
  std::string str = std::to_string(SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime)) + " fps       ";
  std::string engine = RTEXT("Recluse Engine v0.0.2a");
  std::string device = m_pRhi->DeviceName();
  NkObject* nk = gNkDevice();
  nk_begin(&nk->_ctx, RTEXT("Copyright (c) 2018 Recluse Project. All rights reserved"), nk_rect(100.0f, 100.0f, 500.0f, 100.0f), NK_WINDOW_BORDER | NK_WINDOW_TITLE);
    struct nk_command_buffer* cmd_buf = nk_window_get_canvas(&nk->_ctx);
    nk_draw_text(cmd_buf, nk_rect(100.0f + 30.0f, 100.0f + 30.0f, 150.0f, 20.0f), engine.c_str(), 
        engine.size(), &nk->_font->handle, nk_rgba(0, 0, 0, 0), nk_rgb(255, 255, 255));
    nk_draw_text(cmd_buf, nk_rect(100.0f + 30.0f, 100.0f + 45.0f, 300.0f, 20.0f), device.c_str(),
      device.size(), &nk->_font->handle, nk_rgba(0, 0, 0, 0), nk_rgb(255, 255, 255));
    nk_draw_text(cmd_buf, nk_rect(100.0f + 30.0f, 100.0f + 60.0f, 150.0f, 20.0f), str.c_str(),
      str.size(), &nk->_font->handle, nk_rgba(0, 0, 0, 0), nk_rgb(255, 255, 255));
  nk_end(&nk->_ctx);

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
#if 1
  {
    struct nk_convert_config cfg = { };

    cfg.line_AA = NK_ANTI_ALIASING_ON;
    cfg.shape_AA = NK_ANTI_ALIASING_ON;
    cfg.vertex_layout = vertLayout;
    cfg.vertex_size = sizeof(UIVertex);
    cfg.vertex_alignment = NK_ALIGNOF(UIVertex);
    cfg.global_alpha = 1.0f;
    cfg.circle_segment_count = 22;
    cfg.arc_segment_count = 22;
    cfg.curve_segment_count = 22;

    {
      struct nk_buffer vbuf, ebuf;
      nk_buffer_init_fixed(&vbuf, m_vertStagingBuffer->Mapped(), MAX_VERTEX_MEMORY);
      nk_buffer_init_fixed(&ebuf, m_indicesStagingBuffer->Mapped(), MAX_ELEMENT_MEMORY);
      // TODO(): canvas needs to be defined by the ui instead.
      nk_convert(&nk->_ctx, &nk->_cmds, &vbuf, &ebuf, &cfg);
    }
  }

  // Flush memory changes to let host cache flush to gpu. Makes memory visible to gpu.
  {
    std::array<VkMappedMemoryRange, 2> memRanges;
    memRanges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memRanges[0].memory = m_vertStagingBuffer->Memory();
    memRanges[0].size = m_vertStagingBuffer->MemorySize();
    memRanges[0].offset = 0;
    memRanges[0].pNext = nullptr;

    memRanges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memRanges[1].memory = m_indicesStagingBuffer->Memory();
    memRanges[1].size = m_indicesStagingBuffer->MemorySize();
    memRanges[1].offset = 0;
    memRanges[1].pNext = nullptr;

    LogicalDevice* dev = m_pRhi->LogicDevice();
    dev->FlushMappedMemoryRanges(static_cast<u32>(memRanges.size()), memRanges.data());
  }

  // Stream buffers.
  StreamBuffers();
#endif

  VkBuffer vert = m_vertBuffer->NativeBuffer();
  VkBuffer indx = m_indicesBuffer->NativeBuffer();
  VkDeviceSize offsets[] = { 0 };
  VkDescriptorSet sets[] = { global->Set()->Handle(), nk->_font_set->Handle() };
  // Unmap vertices and index buffers, then perform drawing here.
  cmdBuffer->Begin(beginInfo);  
    // When we render with secondary command buffers, we use VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS.
    cmdBuffer->BeginRenderPass(renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipeline->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipeline->Layout(), 0, 2, sets, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vert, offsets);
      cmdBuffer->BindIndexBuffer(indx, 0, VK_INDEX_TYPE_UINT16);
      const struct nk_draw_command* cmd;
#if 1
      u32 vertOffset = 0;
      VkRect2D scissor = { };
      nk_draw_foreach(cmd, &nk->_ctx, &nk->_cmds) {
        if (!cmd->elem_count) continue;
        scissor.offset.x = R_Max((i32)cmd->clip_rect.x, 0);
        scissor.offset.y = R_Max((i32)cmd->clip_rect.y, 0);
        scissor.extent.width = (u32)cmd->clip_rect.w;
        scissor.extent.height = (u32)cmd->clip_rect.h;
        cmdBuffer->SetScissor(0, 1, &scissor);

        if (cmd->texture.ptr) {
          DescriptorSet* set = static_cast<DescriptorSet*>(cmd->texture.ptr);
          sets[1] = set->Handle();
          cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipeline->Layout(), 0, 2, sets, 0, nullptr);
        }
        cmdBuffer->DrawIndexed(cmd->elem_count, 1, vertOffset, 0, 0);
        vertOffset += cmd->elem_count;
      }
#endif
      nk_clear(&nk->_ctx);
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


void UIOverlay::CreateBuffers()
{
  m_vertBuffer = m_pRhi->CreateBuffer();
  m_vertStagingBuffer = m_pRhi->CreateBuffer();
  m_indicesBuffer = m_pRhi->CreateBuffer();
  m_indicesStagingBuffer = m_pRhi->CreateBuffer();

  {
    VkBufferCreateInfo stagingBufferCi = { };
    stagingBufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferCi.size = MAX_VERTEX_MEMORY;
    stagingBufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stagingBufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    m_vertStagingBuffer->Initialize(stagingBufferCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    m_vertStagingBuffer->Map();
  }

  {
    VkBufferCreateInfo vertBufferCi = { };
    vertBufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertBufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vertBufferCi.size = MAX_VERTEX_MEMORY;
    vertBufferCi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    m_vertBuffer->Initialize(vertBufferCi, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  {
    VkBufferCreateInfo stagingBufferCi = { };
    stagingBufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferCi.size = MAX_ELEMENT_MEMORY;
    stagingBufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stagingBufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    m_indicesStagingBuffer->Initialize(stagingBufferCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    m_indicesStagingBuffer->Map();
  }

  {
    VkBufferCreateInfo indicesBufferCi = {};
    indicesBufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indicesBufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    indicesBufferCi.size = MAX_ELEMENT_MEMORY;
    indicesBufferCi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    m_indicesBuffer->Initialize(indicesBufferCi, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }
}


void UIOverlay::CleanUpBuffers()
{
  m_vertStagingBuffer->UnMap();
  m_indicesStagingBuffer->UnMap();

  if (m_vertBuffer) {
    m_pRhi->FreeBuffer(m_vertStagingBuffer);
    m_pRhi->FreeBuffer(m_vertBuffer);
    m_vertBuffer = nullptr;
    m_vertStagingBuffer = nullptr;
  }

  if (m_indicesBuffer) {
    m_pRhi->FreeBuffer(m_indicesStagingBuffer);
    m_pRhi->FreeBuffer(m_indicesBuffer);
    m_indicesBuffer = nullptr;
    m_indicesStagingBuffer = nullptr;
  }
}


void UIOverlay::StreamBuffers()
{
  CommandBuffer cmdBuffer;
  cmdBuffer.SetOwner(m_pRhi->LogicDevice()->Native());
  cmdBuffer.Allocate(m_pRhi->TransferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmdBuffer.Begin(beginInfo);
    VkBufferCopy region = {};
    region.size = MAX_VERTEX_MEMORY;
    region.srcOffset = 0;
    region.dstOffset = 0;
    cmdBuffer.CopyBuffer(m_vertStagingBuffer->NativeBuffer(),
      m_vertBuffer->NativeBuffer(),
      1,
      &region);
    VkBufferCopy regionIndices = {};
    regionIndices.size = MAX_ELEMENT_MEMORY;
    regionIndices.srcOffset = 0;
    regionIndices.dstOffset = 0;
    cmdBuffer.CopyBuffer(m_indicesStagingBuffer->NativeBuffer(),
      m_indicesBuffer->NativeBuffer(),
      1,
      &regionIndices);
  cmdBuffer.End();

  VkCommandBuffer cmd = cmdBuffer.Handle();
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  m_pRhi->TransferSubmit(DEFAULT_QUEUE_IDX, 1, &submitInfo);
  m_pRhi->TransferWaitIdle(DEFAULT_QUEUE_IDX);

  cmdBuffer.Free();
}
} // Recluse