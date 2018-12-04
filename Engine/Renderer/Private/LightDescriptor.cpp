// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "LightDescriptor.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "TextureType.hpp"
#include "GlobalDescriptor.hpp"
#include "MeshDescriptor.hpp"
#include "MaterialDescriptor.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "MeshData.hpp"
#include "Vertex.hpp"
#include "VertexDescription.hpp"

#include "Core/Utility/Profile.hpp"

#include "Filesystem/Filesystem.hpp"

#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Framebuffer.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Shader.hpp"

#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Time.hpp"

#include <array>


namespace Recluse {


GraphicsPipeline* ShadowMapSystem::k_pSkinnedPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pSkinnedMorphPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticMorphPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticSkinnedPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticSkinnedMorphPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticStaticPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticStaticMorphPipeline = nullptr;
RenderPass*       ShadowMapSystem::k_pDynamicRenderPass = nullptr;
RenderPass*       ShadowMapSystem::k_pStaticRenderPass = nullptr;

u32 LightBuffer::MaxNumDirectionalLights()
{
  return MAX_DIRECTIONAL_LIGHTS;
}


u32 LightBuffer::MaxNumPointLights()
{
  return MAX_POINT_LIGHTS;
}


void InitializeShadowMapRenderPass(RenderPass* renderPass, VkFormat format, VkSampleCountFlagBits samples);
void InitializeShadowPipeline(VulkanRHI* pRhi, GraphicsPipeline* pipeline,
  RenderPass* pRenderPass,
  u32 width,
  u32 height,
  std::vector<VkVertexInputBindingDescription>& bindings,
  std::vector<VkVertexInputAttributeDescription>& attributes,
  b32 skinned,
  b32 morphTargets = false);


void ShadowMapSystem::InitializeShadowPipelines(VulkanRHI* pRhi)
{
  if (!k_pStaticRenderPass) {
    k_pStaticRenderPass = pRhi->CreateRenderPass();
    InitializeShadowMapRenderPass(k_pStaticRenderPass, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
  }

  if (!k_pDynamicRenderPass) {
    k_pDynamicRenderPass = pRhi->CreateRenderPass();
    InitializeShadowMapRenderPass(k_pDynamicRenderPass, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
  }

  if (!k_pSkinnedPipeline) {
    std::vector<VkVertexInputBindingDescription> bindings = { SkinnedVertexDescription::GetBindingDescription() };
    k_pSkinnedPipeline = pRhi->CreateGraphicsPipeline();
    InitializeShadowPipeline(pRhi, k_pSkinnedPipeline, k_pDynamicRenderPass, 1, 1,
      bindings, SkinnedVertexDescription::GetVertexAttributes(), true);
  }

  if (!k_pSkinnedMorphPipeline) {
    k_pSkinnedMorphPipeline = pRhi->CreateGraphicsPipeline();
    InitializeShadowPipeline(pRhi, k_pSkinnedMorphPipeline, k_pDynamicRenderPass, 1, 1,
      MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription()),
      MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes()),
      true, true);
  }

  if (!k_pStaticSkinnedPipeline) {
    std::vector<VkVertexInputBindingDescription> bindings = { SkinnedVertexDescription::GetBindingDescription() };
    k_pStaticSkinnedPipeline = pRhi->CreateGraphicsPipeline();
    InitializeShadowPipeline(pRhi, k_pStaticSkinnedPipeline, k_pStaticRenderPass, 1, 1,
      bindings, SkinnedVertexDescription::GetVertexAttributes(), true);
  }

  if (!k_pStaticSkinnedMorphPipeline) {
    k_pStaticSkinnedMorphPipeline = pRhi->CreateGraphicsPipeline();
    InitializeShadowPipeline(pRhi, k_pStaticSkinnedMorphPipeline, k_pStaticRenderPass, 1, 1,
      MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription()),
      MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes()), true, true);
  }

  if (!k_pStaticPipeline) {
    std::vector<VkVertexInputBindingDescription> bindings = { StaticVertexDescription::GetBindingDescription() };
    k_pStaticPipeline = pRhi->CreateGraphicsPipeline();
    InitializeShadowPipeline(pRhi, k_pStaticPipeline, k_pDynamicRenderPass, 1, 1,
      bindings, StaticVertexDescription::GetVertexAttributes(), false);
  }

  if (!k_pStaticMorphPipeline) {
    k_pStaticMorphPipeline = pRhi->CreateGraphicsPipeline();
    InitializeShadowPipeline(pRhi, k_pStaticMorphPipeline, k_pDynamicRenderPass, 1, 1,
      MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription()),
      MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes()),
      false, true);
  }

  if (!k_pStaticStaticPipeline) {
    std::vector<VkVertexInputBindingDescription> bindings = { StaticVertexDescription::GetBindingDescription() };
    k_pStaticStaticPipeline = pRhi->CreateGraphicsPipeline();
    InitializeShadowPipeline(pRhi, k_pStaticStaticPipeline, k_pStaticRenderPass, 1, 1,
      bindings, StaticVertexDescription::GetVertexAttributes(), false);
  }

  if (!k_pStaticStaticMorphPipeline) {
    k_pStaticStaticMorphPipeline = pRhi->CreateGraphicsPipeline();
    InitializeShadowPipeline(pRhi, k_pStaticStaticMorphPipeline, k_pStaticRenderPass, 1, 1,
      MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription()),
      MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes()),
      false, true);
  }
}


void ShadowMapSystem::CleanUpShadowPipelines(VulkanRHI* pRhi)
{
  if (k_pStaticRenderPass) {
    pRhi->FreeRenderPass(k_pStaticRenderPass);
    k_pStaticRenderPass = nullptr;
  }

  if (k_pDynamicRenderPass) {
    pRhi->FreeRenderPass(k_pDynamicRenderPass);
    k_pDynamicRenderPass = nullptr;
  }

  if (k_pSkinnedPipeline) {
    pRhi->FreeGraphicsPipeline(k_pSkinnedPipeline);
    k_pSkinnedPipeline = nullptr;
  }

  if (k_pSkinnedMorphPipeline) {
    pRhi->FreeGraphicsPipeline(k_pSkinnedMorphPipeline);
    k_pSkinnedMorphPipeline = nullptr;
  }

  if (k_pStaticSkinnedPipeline) {
    pRhi->FreeGraphicsPipeline(k_pStaticSkinnedPipeline);
    k_pStaticSkinnedPipeline = nullptr;
  }

  if (k_pStaticSkinnedMorphPipeline) {
    pRhi->FreeGraphicsPipeline(k_pStaticSkinnedMorphPipeline);
    k_pStaticSkinnedMorphPipeline = nullptr;
  }

  if (k_pStaticStaticPipeline) {
    pRhi->FreeGraphicsPipeline(k_pStaticStaticPipeline);
    k_pStaticStaticPipeline = nullptr;
  }

  if (k_pStaticStaticMorphPipeline) {
    pRhi->FreeGraphicsPipeline(k_pStaticStaticMorphPipeline);
    k_pStaticStaticMorphPipeline = nullptr;
  }

  if (k_pStaticPipeline) {
    pRhi->FreeGraphicsPipeline(k_pStaticPipeline);
    k_pStaticPipeline = nullptr;
  }

  if (k_pStaticMorphPipeline) {
    pRhi->FreeGraphicsPipeline(k_pStaticMorphPipeline);
    k_pStaticMorphPipeline = nullptr;
  }
}


ShadowMapSystem::~ShadowMapSystem()
{
  DEBUG_OP(
    if (m_pDynamicFrameBuffer) {
      R_DEBUG(rWarning, "Dynamic Frame Buffer not destroyed prior to destruct call.");
    }
    if (m_pStaticFrameBuffer) {
      R_DEBUG(rWarning, "Static frame buffer not destroyed prior to destruct call.");
    }
    if (m_pStaticMap) {
      R_DEBUG(rWarning, "Static map not destroyed prior to destruct call.");
    }
    if (m_pDynamicMap) {
      R_DEBUG(rWarning, "Dynamic map not destroyed prior to destruct call.");
    }
    if (m_pLightViewDescriptorSet) {
      R_DEBUG(rWarning, "Light view descriptor set not destroyed prior to destruct call.");
    }
    if (m_pLightViewBuffer) {
      R_DEBUG(rWarning, "Light view buffer not destroyed prior to destruct call.");
    }

    if ( m_pStaticLightViewBuffer ) {
      R_DEBUG(rWarning, "Static Light View Buffer not destroyed prior to destruct call.");
    }
  );
}


void ShadowMapSystem::Initialize(VulkanRHI* pRhi, 
  GraphicsQuality dynamicShadowDetail, GraphicsQuality staticShadowDetail,
  b32 staticSoftShadows, b32 dynamicSoftShadows)
{
  // ShadowMap is a depth image.
  VkImageCreateInfo ImageCi = {};
  ImageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImageCi.arrayLayers = 1;
  ImageCi.extent.width = 256 << dynamicShadowDetail;
  ImageCi.extent.height = 256 << dynamicShadowDetail;
  ImageCi.extent.depth = 1;
  ImageCi.format = VK_FORMAT_D32_SFLOAT;
  ImageCi.imageType = VK_IMAGE_TYPE_2D;
  ImageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ImageCi.mipLevels = 1;
  ImageCi.samples = VK_SAMPLE_COUNT_1_BIT;
  ImageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImageCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  ImageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImageViewCreateInfo ViewCi = {};
  ViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ViewCi.components = {};
  ViewCi.format = ImageCi.format;
  ViewCi.subresourceRange = {};
  ViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  ViewCi.subresourceRange.baseArrayLayer = 0;
  ViewCi.subresourceRange.baseMipLevel = 0;
  ViewCi.subresourceRange.layerCount = 1;
  ViewCi.subresourceRange.levelCount = 1;
  ViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;

  // Light view buffer creation.
  m_pLightViewBuffer = pRhi->CreateBuffer();
  m_pStaticLightViewBuffer = pRhi->CreateBuffer();
  VkBufferCreateInfo bufferCI = {};
  VkDeviceSize dSize = sizeof(LightViewSpace);
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCI.size = dSize;
  m_pLightViewBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_pStaticLightViewBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_pLightViewBuffer->Map();
  m_pStaticLightViewBuffer->Map();

  if (!m_pDynamicMap) {
    m_pDynamicMap = pRhi->CreateTexture();
    RDEBUG_SET_VULKAN_NAME(m_pDynamicMap, "Dynamic Shadowmap.");
    R_DEBUG(rNotify, "Dynamic Shadow map size: ");
    R_DEBUG(rNormal, std::to_string(ImageCi.extent.width) + "x" + std::to_string(ImageCi.extent.height) + "\n");
    m_pDynamicMap->Initialize(ImageCi, ViewCi);
  }
  if (!m_pStaticMap) {
    m_pStaticMap = pRhi->CreateTexture();
    RDEBUG_SET_VULKAN_NAME(m_pStaticMap, "Static Shadowmap.");
    ImageCi.extent.width = 256 << staticShadowDetail;
    ImageCi.extent.height = 256 << staticShadowDetail;
    R_DEBUG(rNotify, "Static Shadow map size: ");
    R_DEBUG(rNormal, std::to_string(ImageCi.extent.width) + "x" + std::to_string(ImageCi.extent.height) + "\n");
    m_pStaticMap->Initialize(ImageCi, ViewCi);
  }

  InitializeShadowMap(pRhi);
  
  EnableStaticMapSoftShadows(staticSoftShadows);
  EnableDynamicMapSoftShadows(dynamicSoftShadows);
}


void ShadowMapSystem::EnableDynamicMapSoftShadows(b32 enable)
{
  m_viewSpace._shadowTechnique = Vector4(r32(enable), r32(enable), r32(enable), r32(enable));
}

void ShadowMapSystem::EnableStaticMapSoftShadows(b32 enable)
{
  m_staticViewSpace._shadowTechnique = Vector4(r32(enable), r32(enable), r32(enable), r32(enable));
}


void InitializeShadowPipeline(VulkanRHI* pRhi, GraphicsPipeline* pipeline, 
  RenderPass* pRenderPass,
  u32 width,
  u32 height, 
  std::vector<VkVertexInputBindingDescription>& bindings, 
  std::vector<VkVertexInputAttributeDescription>& attributes,
  b32 skinned,
  b32 morphTargets)
{
  VkPipelineInputAssemblyStateCreateInfo assemblyCI = {};
  assemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  assemblyCI.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.height = static_cast<r32>(height);
  viewport.width = static_cast<r32>(width);

  VkRect2D scissor = {};
  scissor.extent = { width, height };
  scissor.offset = { 0, 0 };

  VkPipelineViewportStateCreateInfo viewportCI = {};
  viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportCI.viewportCount = 1;
  viewportCI.pViewports = &viewport;
  viewportCI.scissorCount = 1;
  viewportCI.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizerCI = CreateRasterInfo(
    VK_POLYGON_MODE_FILL,
    VK_FALSE,
    SHADOW_CULL_MODE,
    SHADOW_WINDING_ORDER,
    1.0f,
    VK_FALSE,
    VK_FALSE
  );

  VkPipelineMultisampleStateCreateInfo msCI = {};
  msCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  msCI.sampleShadingEnable = VK_FALSE;
  msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  msCI.minSampleShading = 1.0f;
  msCI.pSampleMask = nullptr;
  msCI.alphaToOneEnable = VK_FALSE;
  msCI.alphaToCoverageEnable = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depthStencilCI = {};
  depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilCI.depthTestEnable = VK_TRUE;
  depthStencilCI.depthWriteEnable = VK_TRUE;
  depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilCI.depthBoundsTestEnable = pRhi->DepthBoundsAllowed();
  depthStencilCI.minDepthBounds = 0.0f;
  depthStencilCI.maxDepthBounds = 1.0f;
  depthStencilCI.stencilTestEnable = VK_FALSE;
  depthStencilCI.back.compareMask = 0xff;
  depthStencilCI.back.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilCI.back.passOp = VK_STENCIL_OP_REPLACE;
  depthStencilCI.back.failOp = VK_STENCIL_OP_ZERO;
  depthStencilCI.front = {};

  std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachments;
  colorBlendAttachments[0] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[1] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[2] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[3] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  VkPipelineColorBlendStateCreateInfo colorBlendCI = CreateBlendStateInfo(
    static_cast<u32>(colorBlendAttachments.size()),
    colorBlendAttachments.data(),
    VK_FALSE,
    VK_LOGIC_OP_COPY
  );

  VkDynamicState dynamicStates[2] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicCI = {};
  dynamicCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicCI.dynamicStateCount = 2;
  dynamicCI.pDynamicStates = dynamicStates;

  VkPipelineVertexInputStateCreateInfo vertexCI = {};
  vertexCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexCI.vertexBindingDescriptionCount = static_cast<u32>(bindings.size());
  vertexCI.pVertexBindingDescriptions = bindings.data();
  vertexCI.vertexAttributeDescriptionCount = static_cast<u32>(attributes.size());
  vertexCI.pVertexAttributeDescriptions = attributes.data();

  VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = {};
  GraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  GraphicsPipelineInfo.pColorBlendState = &colorBlendCI;
  GraphicsPipelineInfo.pDepthStencilState = &depthStencilCI;
  GraphicsPipelineInfo.pInputAssemblyState = &assemblyCI;
  GraphicsPipelineInfo.pRasterizationState = &rasterizerCI;
  GraphicsPipelineInfo.pMultisampleState = &msCI;
  GraphicsPipelineInfo.pVertexInputState = &vertexCI;
  GraphicsPipelineInfo.pViewportState = &viewportCI;
  GraphicsPipelineInfo.pTessellationState = nullptr;
  GraphicsPipelineInfo.pDynamicState = &dynamicCI;
  GraphicsPipelineInfo.subpass = 0;
  GraphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  // TODO(): Initialize shadow map pipeline.
  VkPipelineLayoutCreateInfo PipeLayout = {};
  std::array<VkDescriptorSetLayout, 4> DescLayouts;
  DescLayouts[0] = MeshSetLayoutKey->Layout();
  DescLayouts[1] = LightViewDescriptorSetLayoutKey->Layout();
  DescLayouts[2] = MaterialSetLayoutKey->Layout();
  DescLayouts[3] = BonesSetLayoutKey->Layout();

  PipeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipeLayout.pushConstantRangeCount = 0;
  PipeLayout.pPushConstantRanges = nullptr;
  PipeLayout.setLayoutCount = static_cast<u32>((skinned ? DescLayouts.size() : DescLayouts.size() - 1));
  PipeLayout.pSetLayouts = DescLayouts.data();

  colorBlendCI.attachmentCount = 0;
  GraphicsPipelineInfo.renderPass = pRenderPass->Handle();

  Shader* SmVert = pRhi->CreateShader();
  Shader* SmFrag = pRhi->CreateShader();

  RendererPass::LoadShader((skinned ? 
    (morphTargets ? "DynamicShadowMapping_MorphTargets.vert.spv" : DynamicShadowMapVertFileStr) :  
    (morphTargets ? "ShadowMapping_MorphTargets.vert.spv" : ShadowMapVertFileStr)), SmVert);
  RendererPass::LoadShader(ShadowMapFragFileStr, SmFrag);

  std::array<VkPipelineShaderStageCreateInfo, 2> Shaders;
  Shaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shaders[0].flags = 0;
  Shaders[0].pName = kDefaultShaderEntryPointStr;
  Shaders[0].pNext = nullptr;
  Shaders[0].pSpecializationInfo = nullptr;
  Shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  Shaders[0].module = SmVert->Handle();

  Shaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shaders[1].flags = 0;
  Shaders[1].pName = kDefaultShaderEntryPointStr;
  Shaders[1].pNext = nullptr;
  Shaders[1].pSpecializationInfo = nullptr;
  Shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  Shaders[1].module = SmFrag->Handle();

  GraphicsPipelineInfo.pStages = Shaders.data();
  GraphicsPipelineInfo.stageCount = static_cast<u32>(Shaders.size());

  pipeline->Initialize(GraphicsPipelineInfo, PipeLayout);
  pRhi->FreeShader(SmVert);
  pRhi->FreeShader(SmFrag);
}


void InitializeShadowMapRenderPass(RenderPass* renderPass, VkFormat format, VkSampleCountFlagBits samples)
{
  std::array<VkAttachmentDescription, 1> attachmentDescriptions;

  // Actual depth buffer to write onto.
  attachmentDescriptions[0] = CreateAttachmentDescription(
    format,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    samples
  );

  std::array<VkSubpassDependency, 2> dependencies;
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

  VkAttachmentReference depthRef = {};
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthRef.attachment = 0;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 0;
  subpass.pColorAttachments = nullptr;
  subpass.pDepthStencilAttachment = &depthRef;
  subpass.flags = 0;


  VkRenderPassCreateInfo renderPassCi = CreateRenderPassInfo(
    static_cast<u32>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    2,
    dependencies.data(),
    1,
    &subpass
  );

  renderPass->Initialize(renderPassCi);
}


void InitializeShadowMapFrameBuffer(FrameBuffer* frameBuffer, RenderPass* renderPass, Texture* texture)
{
  std::array<VkImageView, 1> attachments;
  attachments[0] = texture->View();

  VkFramebufferCreateInfo frameBufferCi = CreateFrameBufferInfo(
    texture->Width(),
    texture->Height(),
    nullptr,
    static_cast<u32>(attachments.size()),
    attachments.data(),
    1
  );

  frameBuffer->Finalize(frameBufferCi, renderPass);
}


void ShadowMapSystem::InitializeShadowMap(VulkanRHI* pRhi)
{
  DescriptorSetLayout* viewLayout = LightViewDescriptorSetLayoutKey;

  DEBUG_OP(
    if (!k_pStaticRenderPass || !k_pDynamicRenderPass) {
      R_DEBUG(rError, "No shadow render passes where initialized!\n");
    }
  );
  if (!m_pLightViewDescriptorSet) {
    m_pLightViewDescriptorSet = pRhi->CreateDescriptorSet();
    m_pLightViewDescriptorSet->Allocate(pRhi->DescriptorPool(), viewLayout);
  }

  if ( !m_pStaticLightViewDescriptorSet ) {
    m_pStaticLightViewDescriptorSet = pRhi->CreateDescriptorSet();
    m_pStaticLightViewDescriptorSet->Allocate(pRhi->DescriptorPool(), viewLayout);
  }

  VkDescriptorBufferInfo viewBuf = {};
  viewBuf.buffer = m_pLightViewBuffer->NativeBuffer();
  viewBuf.offset = 0;
  viewBuf.range = sizeof(LightViewSpace);

  VkDescriptorBufferInfo staticViewBuf = {};
  staticViewBuf.buffer = m_pStaticLightViewBuffer->NativeBuffer();
  staticViewBuf.offset = 0;
  staticViewBuf.range = sizeof(LightViewSpace);

  // TODO(): Once we create our shadow map, we will add it here.
  // This will pass the rendered shadow map to the pbr pipeline.
  VkDescriptorImageInfo globalShadowInfo = {};
  globalShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  globalShadowInfo.imageView = m_pDynamicMap->View();
  globalShadowInfo.sampler = _pSampler->Handle();

  VkDescriptorImageInfo staticShadowInfo = { };
  staticShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  staticShadowInfo.imageView = m_pStaticMap->View();
  staticShadowInfo.sampler = _pSampler->Handle();

  std::array<VkWriteDescriptorSet, 2> writes;
  writes[0] = {};
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].dstArrayElement = 0;
  writes[0].dstBinding = 0;
  writes[0].dstSet = nullptr;
  writes[0].pImageInfo = nullptr;
  writes[0].pBufferInfo = &viewBuf;

  writes[1] = {};
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].descriptorCount = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writes[1].dstArrayElement = 0;
  writes[1].pNext = nullptr;
  writes[1].pImageInfo = &globalShadowInfo;
  writes[1].dstBinding = 1;

  m_pLightViewDescriptorSet->Update(static_cast<u32>(writes.size()), writes.data());

  writes[1].pImageInfo = &staticShadowInfo;
  writes[0].pBufferInfo = &staticViewBuf;
  m_pStaticLightViewDescriptorSet->Update(static_cast<u32>(writes.size()), writes.data());

  if (!m_pDynamicFrameBuffer) {
    m_pDynamicFrameBuffer = pRhi->CreateFrameBuffer();
    InitializeShadowMapFrameBuffer(m_pDynamicFrameBuffer, k_pDynamicRenderPass, m_pDynamicMap);
  }

  if (!m_pStaticFrameBuffer) {
    m_pStaticFrameBuffer = pRhi->CreateFrameBuffer();
    InitializeShadowMapFrameBuffer(m_pStaticFrameBuffer, k_pStaticRenderPass, m_pStaticMap);
  }
  
  m_staticMapNeedsUpdate = true;
}


void ShadowMapSystem::GenerateDynamicShadowCmds(CommandBuffer* pCmdBuffer, CmdList<PrimitiveRenderCmd>& dynamicCmds)
{
  R_TIMED_PROFILE_RENDERER();

  GraphicsPipeline* staticPipeline = k_pStaticPipeline;
  GraphicsPipeline* dynamicPipeline = k_pSkinnedPipeline;
  GraphicsPipeline* staticMorphPipeline = k_pStaticMorphPipeline;
  GraphicsPipeline* dynamicMorphPipeline = k_pSkinnedMorphPipeline;
  DescriptorSet*    lightViewSet = m_pLightViewDescriptorSet;

  VkRenderPassBeginInfo renderPass = {};
  renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPass.framebuffer = m_pDynamicFrameBuffer->Handle();
  renderPass.renderPass = m_pDynamicFrameBuffer->RenderPassRef()->Handle();
  renderPass.renderArea.extent = { m_pDynamicFrameBuffer->Width(), m_pDynamicFrameBuffer->Height() };
  renderPass.renderArea.offset = { 0, 0 };
  VkClearValue depthValue = {};
  depthValue.depthStencil = { 1.0f, 0 };
  renderPass.clearValueCount = 1;
  renderPass.pClearValues = &depthValue;

  VkViewport viewport = {};
  viewport.height = (r32)m_pDynamicFrameBuffer->Height();
  viewport.width = (r32)m_pDynamicFrameBuffer->Width();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  VkRect2D scissor = {};
  scissor.extent = { m_pDynamicFrameBuffer->Width(), m_pDynamicFrameBuffer->Height() };
  scissor.offset = { 0, 0 };

  auto render = [&](PrimitiveRenderCmd& renderCmd) -> void {
    if (!renderCmd._pMeshDesc) return;
    if (!(renderCmd._config & CMD_RENDERABLE_BIT) || !(renderCmd._config & CMD_SHADOWS_BIT)) return;
    R_ASSERT(renderCmd._pMeshData, "Null data was passed to renderer.");
    MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
    b32 skinned = (renderCmd._config & CMD_SKINNED_BIT);
    VkDescriptorSet descriptorSets[4];
    descriptorSets[0] = pMeshDesc->CurrMeshSet()->Handle();
    descriptorSets[1] = lightViewSet->Handle();
    descriptorSets[3] = skinned ? renderCmd._pJointDesc->CurrJointSet()->Handle() : VK_NULL_HANDLE;

    GraphicsPipeline* pipeline = skinned ? dynamicPipeline : staticPipeline;
    MeshData* mesh = renderCmd._pMeshData;
    VertexBuffer* vertex = mesh->VertexData();
    IndexBuffer* index = mesh->IndexData();
    VkBuffer buf = vertex->Handle()->NativeBuffer();
    VkDeviceSize offset[] = { 0 };
    pCmdBuffer->BindVertexBuffers(0, 1, &buf, offset);
    if ( renderCmd._config & CMD_MORPH_BIT ) {
      pipeline = skinned ? dynamicMorphPipeline : staticMorphPipeline;
      R_ASSERT(renderCmd._pMorph0, "morph0 is null");
      R_ASSERT(renderCmd._pMorph1, "morph1 is null.");
      VkBuffer morph0 = renderCmd._pMorph0->VertexData()->Handle()->NativeBuffer();
      VkBuffer morph1 = renderCmd._pMorph1->VertexData()->Handle()->NativeBuffer();
      pCmdBuffer->BindVertexBuffers(1, 1, &morph0, offset);
      pCmdBuffer->BindVertexBuffers(2, 1, &morph1, offset);
    }

    pCmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Pipeline());
    pCmdBuffer->SetViewPorts(0, 1, &viewport);
    pCmdBuffer->SetScissor(0, 1, &scissor);

    if (index) {
      VkBuffer ind = index->Handle()->NativeBuffer();
      pCmdBuffer->BindIndexBuffer(ind, 0, GetNativeIndexType(index->GetSizeType()));
    }

    Primitive* primitive = renderCmd._pPrimitive;
    descriptorSets[2] = primitive->_pMat->Native()->CurrMaterialSet()->Handle();
    pCmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Layout(), 0, skinned ? 4 : 3, descriptorSets, 0, nullptr);
    if (index) {
      pCmdBuffer->DrawIndexed(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0, 0);
    }
    else {
      pCmdBuffer->Draw(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0);
    }
  };

  pCmdBuffer->BeginRenderPass(renderPass, VK_SUBPASS_CONTENTS_INLINE);
  for (size_t i = 0; i < dynamicCmds.Size(); ++i) {
    PrimitiveRenderCmd& renderCmd = dynamicCmds[i];
    render(renderCmd);
  }
  pCmdBuffer->EndRenderPass();

  if (dynamicCmds.Size() == 0) {
    R_DEBUG(rNotify, "Empty dynamic map cmd buffer updated.\n");
    return;
  }
  R_DEBUG(rNotify, "Updated dynamic map cmd buffer.\n");
}


void ShadowMapSystem::GenerateStaticShadowCmds(CommandBuffer* pCmdBuffer, CmdList<PrimitiveRenderCmd>& staticCmds)
{
  R_TIMED_PROFILE_RENDERER();

  if ( !m_staticMapNeedsUpdate ) { return; }
  
  GraphicsPipeline* staticPipeline = k_pStaticStaticPipeline;
  GraphicsPipeline* dynamicPipeline = k_pStaticSkinnedPipeline;
  GraphicsPipeline* staticMorphPipeline = k_pStaticStaticMorphPipeline;
  GraphicsPipeline* dynamicMorphPipeline = k_pStaticSkinnedMorphPipeline;
  DescriptorSet*    lightViewSet = m_pStaticLightViewDescriptorSet;

  VkRenderPassBeginInfo renderPass = {};
  renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPass.framebuffer = m_pStaticFrameBuffer->Handle();
  renderPass.renderPass = m_pStaticFrameBuffer->RenderPassRef()->Handle();
  renderPass.renderArea.extent = { m_pStaticFrameBuffer->Width(), m_pStaticFrameBuffer->Height() };
  renderPass.renderArea.offset = { 0, 0 };
  VkClearValue depthValue = {};
  depthValue.depthStencil = { 1.0f, 0 };
  renderPass.clearValueCount = 1;
  renderPass.pClearValues = &depthValue;

  VkViewport viewport = {};
  viewport.height = (r32)m_pStaticFrameBuffer->Height();
  viewport.width = (r32)m_pStaticFrameBuffer->Width();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  VkRect2D scissor = {};
  scissor.extent = { m_pStaticFrameBuffer->Width(), m_pStaticFrameBuffer->Height() };
  scissor.offset = { 0, 0 };

  auto render = [&](PrimitiveRenderCmd& renderCmd) -> void {
    if (!renderCmd._pMeshDesc) return;
    if (!(renderCmd._config & CMD_RENDERABLE_BIT) || !(renderCmd._config & CMD_SHADOWS_BIT)) return;
    R_ASSERT(renderCmd._pMeshData, "Null data was passed to renderer.");
    MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
    b32 skinned = (renderCmd._config & CMD_SKINNED_BIT);
    VkDescriptorSet descriptorSets[4];
    descriptorSets[0] = pMeshDesc->CurrMeshSet()->Handle();
    descriptorSets[1] = lightViewSet->Handle();
    descriptorSets[3] = skinned ? renderCmd._pJointDesc->CurrJointSet()->Handle() : VK_NULL_HANDLE;

    GraphicsPipeline* pipeline = skinned ? dynamicPipeline : staticPipeline;
    MeshData* mesh = renderCmd._pMeshData;
    VertexBuffer* vertex = mesh->VertexData();
    IndexBuffer* index = mesh->IndexData();
    VkBuffer buf = vertex->Handle()->NativeBuffer();
    VkDeviceSize offset[] = { 0 };
    pCmdBuffer->BindVertexBuffers(0, 1, &buf, offset);
    if (renderCmd._config & CMD_MORPH_BIT) {
      pipeline = skinned ? dynamicMorphPipeline : staticMorphPipeline;
      R_ASSERT(renderCmd._pMorph0, "morph0 is null");
      R_ASSERT(renderCmd._pMorph1, "morph1 is null.");
      VkBuffer morph0 = renderCmd._pMorph0->VertexData()->Handle()->NativeBuffer();
      VkBuffer morph1 = renderCmd._pMorph1->VertexData()->Handle()->NativeBuffer();
      pCmdBuffer->BindVertexBuffers(1, 1, &morph0, offset);
      pCmdBuffer->BindVertexBuffers(2, 1, &morph1, offset);
    }

    pCmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Pipeline());
    pCmdBuffer->SetViewPorts(0, 1, &viewport);
    pCmdBuffer->SetScissor(0, 1, &scissor);

    if (index) {
      VkBuffer ind = index->Handle()->NativeBuffer();
      pCmdBuffer->BindIndexBuffer(ind, 0, GetNativeIndexType(index->GetSizeType()));
    }

    Primitive* primitive = renderCmd._pPrimitive;
    descriptorSets[2] = primitive->_pMat->Native()->CurrMaterialSet()->Handle();
    pCmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Layout(), 0, skinned ? 4 : 3, descriptorSets, 0, nullptr);
    if (index) {
      pCmdBuffer->DrawIndexed(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0, 0);
    }
    else {
      pCmdBuffer->Draw(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0);
    }
  };

  pCmdBuffer->BeginRenderPass(renderPass, VK_SUBPASS_CONTENTS_INLINE);
  for (size_t i = 0; i < staticCmds.Size(); ++i) {
    PrimitiveRenderCmd& renderCmd = staticCmds[i];
    render(renderCmd);
  }
  pCmdBuffer->EndRenderPass();
  if (staticCmds.Size() == 0) {
    R_DEBUG(rNotify, "Empty static map cmd buffer updated.\n");
    return;
  }
  R_DEBUG(rNotify, "Static map command buffer updated with meshes.\n")
  m_staticMapNeedsUpdate = false;
}


void ShadowMapSystem::Update(VulkanRHI* pRhi, GlobalBuffer* gBuffer, LightBuffer* buffer, i32 idx)
{
  DirectionalLight* light = nullptr;
  if (idx <= -1) {
    light = &buffer->_PrimaryLight; 
  } else {
    light = &buffer->_DirectionalLights[idx];
  }

  Vector3 viewerPos = Vector3(
    gBuffer->_CameraPos.x, 
    gBuffer->_CameraPos.y, 
    gBuffer->_CameraPos.z
  );

  // TODO(): The shadow map needs to follow the player...
  Vector3 Eye = Vector3(
    light->_Direction.x,
    light->_Direction.y,
    light->_Direction.z
  );

  Eye *= 1024.0f;
  Eye -= viewerPos;
  // Pass as one matrix.
  Matrix4 view = Matrix4::LookAt(-Eye, viewerPos, Vector3::UP);
  // TODO(): This may need to be adjustable depending on scale.
  Matrix4 proj = Matrix4::Ortho(
    m_rShadowViewportWidth,
    m_rShadowViewportHeight,
    1.0f,
    8000.0f
  );
  m_viewSpace._ViewProj = view * proj;
  r32 lightSz = 5.0f / m_rShadowViewportHeight;
  m_viewSpace._lightSz = Vector4(lightSz, lightSz, lightSz, lightSz);
  m_viewSpace._near = Vector4(0.135f, 0.0f, 0.1f, 0.1f);

  {
    R_ASSERT(m_pLightViewBuffer->Mapped(), "Light view buffer was not mapped!");
    memcpy(m_pLightViewBuffer->Mapped(), &m_viewSpace, sizeof(LightViewSpace));

    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pLightViewBuffer->Memory();
    range.size = VK_WHOLE_SIZE;
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  if ( m_staticMapNeedsUpdate ) {
    viewerPos = m_staticViewerPos;
    Eye = Vector3(
      light->_Direction.x,
      light->_Direction.y,
      light->_Direction.z
    );
    proj = Matrix4::Ortho(
      m_staticShadowViewportWidth,
      m_staticShadowViewportHeight,
      1.0f,
      8000.0f
    );

    Eye *= 1024.0f;
    Eye -= viewerPos;
    // Pass as one matrix.
    view = Matrix4::LookAt(-Eye, viewerPos, Vector3::UP);
    m_staticViewSpace._ViewProj = view * proj;
    m_staticViewSpace._near = Vector4(0.135f, 0.0f, 0.1f, 0.1f);
    m_staticViewSpace._lightSz.x = 5.0f / m_rShadowViewportHeight;//15.0f / m_staticShadowViewportHeight;
    R_ASSERT(m_pStaticLightViewBuffer->Mapped(), "Light view buffer was not mapped!");
    memcpy(m_pStaticLightViewBuffer->Mapped(), &m_staticViewSpace, sizeof(LightViewSpace));

    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pStaticLightViewBuffer->Memory();
    range.size = VK_WHOLE_SIZE;
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }
}


void ShadowMapSystem::CleanUp(VulkanRHI* pRhi)
{
  if (m_pLightViewBuffer) {
    pRhi->FreeBuffer(m_pLightViewBuffer);
    m_pLightViewBuffer = nullptr;
  }

  if ( m_pStaticLightViewBuffer ) {
    pRhi->FreeBuffer( m_pStaticLightViewBuffer );
    m_pStaticLightViewBuffer = nullptr;
  }

  if (m_pDynamicMap) {
    pRhi->FreeTexture(m_pDynamicMap);
    m_pDynamicMap = nullptr;
  }

  if (m_pStaticMap) {
    pRhi->FreeTexture(m_pStaticMap);
    m_pStaticMap = nullptr;
  }
 
  if (m_pLightViewDescriptorSet) {
    pRhi->FreeDescriptorSet(m_pLightViewDescriptorSet);
    m_pLightViewDescriptorSet = nullptr;
  }

  if (m_pDynamicFrameBuffer) {
    pRhi->FreeFrameBuffer(m_pDynamicFrameBuffer);
    m_pDynamicFrameBuffer = nullptr;
  }

  if (m_pStaticFrameBuffer) {
    pRhi->FreeFrameBuffer(m_pStaticFrameBuffer);
    m_pStaticFrameBuffer = nullptr;
  }
}


LightDescriptor::LightDescriptor()
  : m_pShadowSampler(nullptr)
  , m_pLightDescriptorSet(nullptr)
  , m_pLightBuffer(nullptr)
#if 0
  , m_pLightViewDescriptorSet(nullptr)
  , m_pOpaqueShadowMap(nullptr)
  , m_pLightViewBuffer(nullptr)
  , m_pFrameBuffer(nullptr)
  , m_pRenderPass(nullptr)
#endif
  , m_PrimaryShadowEnable(true)
  , m_rShadowViewportHeight(40.0f)
  , m_rShadowViewportWidth(40.0f)
{
  m_Lights._PrimaryLight._Enable = false;
  m_Lights._PrimaryLight._Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  m_Lights._PrimaryLight._Pad[0] = 0;
  m_Lights._PrimaryLight._Pad[1] = 0;

  for (size_t i = 0; i < LightBuffer::MaxNumPointLights(); ++i) {
    m_Lights._PointLights[i]._Position = Vector4();
    m_Lights._PointLights[i]._Color = Vector4();
    m_Lights._PointLights[i]._Range = 0.0f;
    m_Lights._PointLights[i]._Intensity = 1.0f;
    m_Lights._PointLights[i]._Enable = false;
    m_Lights._PointLights[i]._Pad = 0;
  }

  for (size_t i = 0; i < LightBuffer::MaxNumDirectionalLights(); ++i) {
    m_Lights._DirectionalLights[i]._Direction = Vector4();
    m_Lights._DirectionalLights[i]._Enable = false;
    m_Lights._DirectionalLights[i]._Intensity = 1.0f;
    m_Lights._DirectionalLights[i]._Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    m_Lights._DirectionalLights[i]._Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  }
#if 0
  m_PrimaryLightSpace._lightSz = Vector4();
  m_PrimaryLightSpace._near = Vector4();
  m_PrimaryLightSpace._shadowTechnique = Vector4();
#endif 
}


LightDescriptor::~LightDescriptor()
{
  DEBUG_OP(
    if (m_pLightBuffer) {
      R_DEBUG(rWarning, "Light buffer was not cleaned up!\n");
    }

    if (m_pLightDescriptorSet) {
      R_DEBUG(rWarning, "Light MaterialDescriptor descriptor set was not properly cleaned up!\n");
    }
    );
#if 0
  if (m_pOpaqueShadowMap) {
    R_DEBUG(rWarning, "Light Shadow Map texture was not properly cleaned up!\n");
  }
#endif
  DEBUG_OP(
    if (m_pShadowSampler) {
      R_DEBUG(rWarning, "Light Shadow Map sampler was not properly cleaned up!\n");
    }
  );
#if 0
  R_ASSERT(!m_pRenderPass, "Render pass was not properly discarded from light descriptor!");

  if (m_pFrameBuffer) {
    R_DEBUG(rWarning, "Light framebuffer was not properly cleaned up!\n");
  }
  
  if (m_pLightViewBuffer) {
    R_DEBUG(rWarning, "Light view buffer was not properly cleaned up!\n");
  }

  if (m_pLightViewDescriptorSet) {
    R_DEBUG(rWarning, "Light view descriptor set was not properly cleaned up!\n");
  }
#endif
}


void LightDescriptor::Initialize(VulkanRHI* pRhi, GraphicsQuality shadowDetail, b32 enableSoftShadows)
{
  R_ASSERT(pRhi, "RHI owner not set for light material upon initialization!\n");
  // This class has already been initialized.
  R_ASSERT(!m_pLightBuffer && !m_pLightDescriptorSet, "This light buffer is already initialized! Skipping...\n");

  m_pLightBuffer = pRhi->CreateBuffer();
  VkBufferCreateInfo bufferCI = {};
  VkDeviceSize dSize = sizeof(LightBuffer);
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  m_pLightBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_pLightBuffer->Map();
#if 0
  // Light view buffer creation.
  m_pLightViewBuffer = pRhi->CreateBuffer();
  dSize = sizeof(LightViewSpace);
  bufferCI.size = dSize;
  m_pLightViewBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_pLightViewBuffer->Map();

  // Create our shadow map texture.
  if (!m_pOpaqueShadowMap) {
    // TODO():
    m_pOpaqueShadowMap = pRhi->CreateTexture();

    // ShadowMap is a depth image.
    VkImageCreateInfo ImageCi = {};
    ImageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCi.arrayLayers = 1;
    ImageCi.extent.width = 512 << shadowDetail;
    ImageCi.extent.height = 512 << shadowDetail;
    ImageCi.extent.depth = 1;
    ImageCi.format = VK_FORMAT_D32_SFLOAT;
    ImageCi.imageType = VK_IMAGE_TYPE_2D;
    ImageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCi.mipLevels = 1;
    ImageCi.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCi.usage =  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ImageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkImageViewCreateInfo ViewCi = { };
    ViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ViewCi.components = { };
    ViewCi.format = ImageCi.format;
    ViewCi.subresourceRange = { }; 
    ViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT ;
    ViewCi.subresourceRange.baseArrayLayer = 0;
    ViewCi.subresourceRange.baseMipLevel = 0;
    ViewCi.subresourceRange.layerCount = 1;
    ViewCi.subresourceRange.levelCount = 1;
    ViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    
    m_pOpaqueShadowMap->Initialize(ImageCi, ViewCi);
  }
#endif
  if (!m_pShadowSampler) {
    // TODO():
    m_pShadowSampler = pRhi->CreateSampler();
    VkSamplerCreateInfo SamplerCi = { };
    SamplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    SamplerCi.addressModeV = SamplerCi.addressModeU;
    SamplerCi.addressModeW = SamplerCi.addressModeV;
    SamplerCi.anisotropyEnable = VK_FALSE;
    SamplerCi.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    SamplerCi.compareEnable = VK_FALSE;
    SamplerCi.magFilter = VK_FILTER_LINEAR;
    SamplerCi.minFilter = VK_FILTER_LINEAR;
    SamplerCi.maxAnisotropy = 1.0f;
    SamplerCi.maxLod = 1.0f;
    SamplerCi.minLod = 0.0f;
    SamplerCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCi.unnormalizedCoordinates = VK_FALSE;

    m_pShadowSampler->Initialize(SamplerCi);
  }

  InitializeNativeLights(pRhi);

#if 0
  InitializePrimaryShadow(pRhi);

  if (shadowDetail < SHADOWS_MEDIUM) {
    m_PrimaryLightSpace._shadowTechnique = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
  }
  else {
    m_PrimaryLightSpace._shadowTechnique = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
  }
#endif 

  m_primaryMapSystem._pSampler = m_pShadowSampler;
  m_primaryMapSystem.Initialize(pRhi, shadowDetail, shadowDetail, enableSoftShadows, enableSoftShadows);
}


void LightDescriptor::CleanUp(VulkanRHI* pRhi)
{
#if 0
  if (m_pOpaqueShadowMap) {
    pRhi->FreeTexture(m_pOpaqueShadowMap);
    m_pOpaqueShadowMap = nullptr;
  }

  if (m_pRenderPass) {
    pRhi->FreeRenderPass(m_pRenderPass);
    m_pRenderPass = nullptr;
  }

  if (m_pFrameBuffer) {
    pRhi->FreeFrameBuffer(m_pFrameBuffer);
    m_pFrameBuffer = nullptr;
  }

  if (m_pLightViewBuffer) {
    m_pLightViewBuffer->UnMap();
    pRhi->FreeBuffer(m_pLightViewBuffer);
    m_pLightViewBuffer = nullptr;
  }

  if (m_pLightViewDescriptorSet) {
    pRhi->FreeDescriptorSet(m_pLightViewDescriptorSet);
    m_pLightViewDescriptorSet = nullptr;
  }
#endif
  if (m_pShadowSampler) {
    pRhi->FreeSampler(m_pShadowSampler);
    m_pShadowSampler = nullptr;
  }

  // TODO
  if (m_pLightDescriptorSet) {
    pRhi->FreeDescriptorSet(m_pLightDescriptorSet);
    m_pLightDescriptorSet = nullptr;
  }

  if (m_pLightBuffer) {
    m_pLightBuffer->UnMap();
    pRhi->FreeBuffer(m_pLightBuffer);
    m_pLightBuffer = nullptr;
  }

  m_primaryMapSystem.CleanUp(pRhi);
}


void LightDescriptor::Update(VulkanRHI* pRhi, GlobalBuffer* gBuffer)
{
#if 0
  // TODO(): The shadow map needs to follow the player...
  Vector3 Eye = Vector3(
    m_Lights._PrimaryLight._Direction.x, 
    m_Lights._PrimaryLight._Direction.y, 
    m_Lights._PrimaryLight._Direction.z
  );
  Eye *= 1024.0f;
  Eye -= m_vViewerPos;
  // Pass as one matrix.
  Matrix4 view = Matrix4::LookAt(-Eye, m_vViewerPos, Vector3::UP);
  // TODO(): This may need to be adjustable depending on scale.
  Matrix4 proj = Matrix4::Ortho(
    m_rShadowViewportWidth, 
    m_rShadowViewportHeight, 
    1.0f, 
    8000.0f
  );
  m_PrimaryLightSpace._ViewProj = view * proj;
  r32 lightSz = 5.0f / m_rShadowViewportHeight;
  m_PrimaryLightSpace._lightSz = Vector4(lightSz, lightSz, lightSz, lightSz);
  m_PrimaryLightSpace._near = Vector4(0.135f, 0.0f, 0.1f, 0.1f);

#endif

#if 0
  if (PrimaryShadowEnabled() && m_pLightViewBuffer) {
#else
  if ((PrimaryShadowEnabled() || m_primaryMapSystem.StaticMapNeedsUpdate()) && m_primaryMapSystem.ShadowMapViewDescriptor()) {
#endif
    m_primaryMapSystem.Update(pRhi, gBuffer, &m_Lights);
#if 0
    R_ASSERT(m_pLightViewBuffer->Mapped(), "Light view buffer was not mapped!");
    memcpy(m_pLightViewBuffer->Mapped(), &m_PrimaryLightSpace, sizeof(LightViewSpace));

    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pLightViewBuffer->Memory();
    range.size = m_pLightViewBuffer->MemorySize();
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
#endif
  }

  R_ASSERT(m_pLightBuffer->Mapped(), "Light buffer was not mapped!");
  memcpy(m_pLightBuffer->Mapped(), &m_Lights, sizeof(LightBuffer));

  VkMappedMemoryRange lightRange = { };
  lightRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  lightRange.memory = m_pLightBuffer->Memory();
  lightRange.size = VK_WHOLE_SIZE;
  pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &lightRange);
}


void LightDescriptor::InitializeNativeLights(VulkanRHI* pRhi)
{
  DescriptorSetLayout* pbrLayout = LightSetLayoutKey;
  m_pLightDescriptorSet = pRhi->CreateDescriptorSet();
  m_pLightDescriptorSet->Allocate(pRhi->DescriptorPool(), pbrLayout);

  VkDescriptorBufferInfo lightBufferInfo = {};
  lightBufferInfo.buffer = m_pLightBuffer->NativeBuffer();
  lightBufferInfo.offset = 0;
  lightBufferInfo.range = sizeof(LightBuffer);

  std::array<VkWriteDescriptorSet, 1> writeSets;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pBufferInfo = &lightBufferInfo;
  writeSets[0].pNext = nullptr;
  writeSets[0].dstBinding = 0;

  m_pLightDescriptorSet->Update(static_cast<u32>(writeSets.size()), writeSets.data());
}


#if 0
void LightDescriptor::InitializePrimaryShadow(VulkanRHI* pRhi)
{
  // TODO(): Create DescriptorSet and Framebuffer for shadow pass.
  if (m_pFrameBuffer) return;

  DescriptorSetLayout* viewLayout = LightViewDescriptorSetLayoutKey;
  m_pLightViewDescriptorSet = pRhi->CreateDescriptorSet();
  m_pLightViewDescriptorSet->Allocate(pRhi->DescriptorPool(), viewLayout);

  VkDescriptorBufferInfo viewBuf = { };
  viewBuf.buffer = m_pLightViewBuffer->NativeBuffer();
  viewBuf.offset = 0;
  viewBuf.range = sizeof(LightViewSpace);

  // TODO(): Once we create our shadow map, we will add it here.
  // This will pass the rendered shadow map to the pbr pipeline.
  VkDescriptorImageInfo globalShadowInfo = {};
  globalShadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalShadowInfo.imageView = m_pOpaqueShadowMap->View();
  globalShadowInfo.sampler = m_pShadowSampler->Handle();

  std::array<VkWriteDescriptorSet, 2> writes;
  writes[0] = {};
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].dstArrayElement = 0;
  writes[0].dstBinding = 0;
  writes[0].dstSet = nullptr;
  writes[0].pImageInfo = nullptr;
  writes[0].pBufferInfo = &viewBuf;

  writes[1] = {};
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].descriptorCount = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writes[1].dstArrayElement = 0;
  writes[1].pNext = nullptr;
  writes[1].pImageInfo = &globalShadowInfo;
  writes[1].dstBinding = 1;
  
  m_pLightViewDescriptorSet->Update(writes.size(), writes.data());

  m_pFrameBuffer = pRhi->CreateFrameBuffer();
  m_pRenderPass = pRhi->CreateRenderPass();
  std::array<VkAttachmentDescription, 1> attachmentDescriptions;

  // Actual depth buffer to write onto.
  attachmentDescriptions[0] = CreateAttachmentDescription(
    m_pOpaqueShadowMap->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    m_pOpaqueShadowMap->Samples()
  );

  std::array<VkSubpassDependency, 2> dependencies;
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

  VkAttachmentReference depthRef =  {};
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthRef.attachment = 0;

  VkSubpassDescription subpass = { };
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 0;
  subpass.pColorAttachments = nullptr;
  subpass.pDepthStencilAttachment = &depthRef;
  subpass.flags = 0;

  VkRenderPassCreateInfo renderPassCi = CreateRenderPassInfo(
    static_cast<u32>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    2,
    dependencies.data(),
    1,
    &subpass
  );

  std::array<VkImageView, 1> attachments;
  attachments[0] = m_pOpaqueShadowMap->View();
  
  VkFramebufferCreateInfo frameBufferCi = CreateFrameBufferInfo(
    m_pOpaqueShadowMap->Width(),
    m_pOpaqueShadowMap->Height(),
    nullptr,
    static_cast<u32>(attachments.size()),
    attachments.data(),
    1
  );

  m_pRenderPass->Initialize(renderPassCi);
  m_pFrameBuffer->Finalize(frameBufferCi, m_pRenderPass);
}
#endif
} // Recluse