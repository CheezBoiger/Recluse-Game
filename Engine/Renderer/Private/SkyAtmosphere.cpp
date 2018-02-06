// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "SkyAtmosphere.hpp"

#include "Core/Logging/Log.hpp"

#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Framebuffer.hpp"
#include "RHI/Shader.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"

#include "RendererData.hpp"
#include "RenderQuad.hpp"
#include "VertexDescription.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


const std::string Sky::kAtmVertStr = "Atmosphere.vert.spv";
const std::string Sky::kAtmFragStr = "Atmosphere.frag.spv";
const std::string Sky::kSkyVertStr = "Sky.vert.spv";
const std::string Sky::kSkyFragStr = "Sky.frag.spv";
const u32         Sky::kTextureSize = 512;
std::array<Vector4, 36> Sky::kSkyBoxVertices = { 
  // Front
  Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
  Vector4( 1.0f, -1.0f, 1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, 1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, 1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, 1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
  // Back
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  // Up
  Vector4( 1.0f,  1.0f,  1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f,  1.0f, 1.0f),
  Vector4( 1.0f,  1.0f,  1.0f, 1.0f),
  // Down
  Vector4( 1.0f, -1.0f,  1.0f, 1.0f),
  Vector4(-1.0f, -1.0f,  1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4( 1.0f, -1.0f, -1.0f, 1.0f),
  Vector4( 1.0f, -1.0f,  1.0f, 1.0f),
  // Right
  Vector4( 1.0f, -1.0f,  1.0f, 1.0f),
  Vector4( 1.0f, -1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f,  1.0f, 1.0f),
  Vector4( 1.0f, -1.0f,  1.0f, 1.0f),
  // Left
  Vector4(-1.0f, -1.0f,  1.0f, 1.0f),
  Vector4(-1.0f,  1.0f,  1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f,  1.0f, 1.0f),
};


std::array<u32, 36> Sky::kSkyboxIndices = {
  0, 1, 2,
  3, 4, 5,
  6, 7, 8,
  9, 10, 11,
  12, 13, 14,
  15, 16, 17,
  18, 19, 20,
  21, 22, 23,
  24, 25, 26,
  27, 28, 29,
  30, 31, 32,
  33, 34, 35
};


void Sky::Initialize()
{
  VulkanRHI* pRhi = gRenderer().RHI();

  CreateRenderAttachment(pRhi);
  CreateFrameBuffer(pRhi);
  CreateCubeMap(pRhi);
  CreateGraphicsPipeline(pRhi);
  CreateCommandBuffer(pRhi);
  BuildCmdBuffer(pRhi);

  m_pAtmosphereSema = pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCi = { };
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_pAtmosphereSema->Initialize(semaCi);

  m_SkyboxVertBuf.Initialize(pRhi, static_cast<u32>(kSkyBoxVertices.size()), 
    sizeof(Vector4), kSkyBoxVertices.data());
  m_SkyboxIndBuf.Initialize(pRhi, static_cast<u32>(kSkyboxIndices.size()), sizeof(u32), kSkyboxIndices.data());
}


Sky::~Sky()
{
  if (m_pCubeMap) {
    R_DEBUG(rError, "Skybox cube map was not properly cleaned up prior to class descruction!\n");
  }

  if (m_RenderTexture) {
    R_DEBUG(rError, "Sky texture was not cleaned up prior to class destruction!\n");
  }

  if (m_pFrameBuffer) {
    R_DEBUG(rError, "Sky frame buffer not cleaned up prior to class descruction!\n");
  }
}


void Sky::CreateCommandBuffer(VulkanRHI* rhi)
{
  m_pCmdBuffer = rhi->CreateCommandBuffer();
  m_pCmdBuffer->Allocate(rhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}


void Sky::CreateRenderAttachment(VulkanRHI* rhi)
{
  VkExtent2D extent = rhi->SwapchainObject()->SwapchainExtent();
  VkImageCreateInfo imgCi = { };
  imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCi.imageType = VK_IMAGE_TYPE_2D;
  imgCi.mipLevels = 1;
  imgCi.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCi.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  imgCi.extent.width = kTextureSize;
  imgCi.extent.height = kTextureSize;
  imgCi.extent.depth = 1;
  imgCi.arrayLayers = 1;
  
  VkImageViewCreateInfo viewCi = { };
  viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewCi.subresourceRange.baseArrayLayer = 0;
  viewCi.subresourceRange.baseMipLevel = 0;
  viewCi.subresourceRange.levelCount = 1;
  viewCi.subresourceRange.layerCount = 1;
  viewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;

  m_RenderTexture = rhi->CreateTexture();
  m_RenderTexture->Initialize(imgCi, viewCi);
} 


void Sky::CreateCubeMap(VulkanRHI* rhi)
{
  VkExtent2D extent = rhi->SwapchainObject()->SwapchainExtent();
  VkImageCreateInfo imgCi = {};
  imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCi.imageType = VK_IMAGE_TYPE_2D;
  imgCi.mipLevels = 1;
  imgCi.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imgCi.extent.width = kTextureSize;
  imgCi.extent.height = kTextureSize;
  imgCi.extent.depth = 1;
  imgCi.arrayLayers = 6;
  imgCi.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

  VkImageViewCreateInfo viewCi = {};
  viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewCi.subresourceRange.baseArrayLayer = 0;
  viewCi.subresourceRange.baseMipLevel = 0;
  viewCi.subresourceRange.levelCount = 1;
  viewCi.subresourceRange.layerCount = 6;
  viewCi.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

  m_pCubeMap = rhi->CreateTexture();
  m_pCubeMap->Initialize(imgCi, viewCi);

  // Now we create the samplerCube.
  m_pSampler = rhi->CreateSampler();
  VkSamplerCreateInfo samplerCi = { };
  samplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.anisotropyEnable = VK_TRUE;
  samplerCi.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerCi.compareEnable = VK_FALSE;
  samplerCi.compareOp = VK_COMPARE_OP_NEVER;
  samplerCi.magFilter = VK_FILTER_LINEAR;
  samplerCi.minFilter = VK_FILTER_LINEAR;
  samplerCi.maxAnisotropy = 16.0f;
  samplerCi.maxLod = 1.0f;
  samplerCi.minLod = 0.0f;
  samplerCi.mipLodBias = 0.0f;
  samplerCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCi.unnormalizedCoordinates = VK_FALSE;
  m_pSampler->Initialize(samplerCi);
}


void Sky::CreateFrameBuffer(VulkanRHI* rhi)
{
  m_pFrameBuffer = rhi->CreateFrameBuffer();
  VkImageView attachment = m_RenderTexture->View();

  VkAttachmentDescription attachDesc = CreateAttachmentDescription(
    m_RenderTexture->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    m_RenderTexture->Samples()
  );

  VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
  
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

  VkSubpassDescription subpassDesc = { };
  subpassDesc.colorAttachmentCount = 1;
  subpassDesc.pColorAttachments = &colorRef;
  subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  
  VkRenderPassCreateInfo renderpassCi = { };
  renderpassCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpassCi.attachmentCount = 1;
  renderpassCi.pAttachments = &attachDesc;
  renderpassCi.dependencyCount = static_cast<u32>(dependencies.size());
  renderpassCi.pDependencies = dependencies.data();
  renderpassCi.subpassCount = 1;
  renderpassCi.pSubpasses = &subpassDesc;
  
  VkFramebufferCreateInfo framebufferCi = { };
  framebufferCi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferCi.height = kTextureSize;
  framebufferCi.width = kTextureSize;
  framebufferCi.attachmentCount = 1;
  framebufferCi.layers = 1;
  framebufferCi.pAttachments = &attachment;

  m_pFrameBuffer->Finalize(framebufferCi, renderpassCi);

  // Create a renderpass for the pbr overlay.
  Texture* pbrColor = gResources().GetRenderTexture(PBRColorAttachStr);
  Texture* pbrNormal = gResources().GetRenderTexture(PBRNormalAttachStr);
  Texture* pbrPosition = gResources().GetRenderTexture(PBRPositionAttachStr);
  Texture* pbrRoughMetal = gResources().GetRenderTexture(PBRRoughMetalAttachStr);
  Texture* pbrDepth = gResources().GetRenderTexture(PBRDepthAttachStr);
  Texture* RTBright = gResources().GetRenderTexture(RenderTargetBrightStr);

  std::array<VkAttachmentDescription, 6> attachmentDescriptions;
  VkSubpassDependency dependenciesNative[2];

  attachmentDescriptions[0] = CreateAttachmentDescription(
    pbrColor->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    pbrColor->Samples()
  );

  attachmentDescriptions[1] = CreateAttachmentDescription(
    pbrNormal->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    pbrNormal->Samples()
  );

  attachmentDescriptions[2] = CreateAttachmentDescription(
    RTBright->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    RTBright->Samples()
  );

  attachmentDescriptions[3] = CreateAttachmentDescription(
    pbrPosition->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    pbrPosition->Samples()
  );

  attachmentDescriptions[4] = CreateAttachmentDescription(
    pbrRoughMetal->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    pbrRoughMetal->Samples()
  );

  attachmentDescriptions[5] = CreateAttachmentDescription(
    pbrDepth->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    pbrDepth->Samples()
  );

  dependenciesNative[0] = CreateSubPassDependency(
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  dependenciesNative[1] = CreateSubPassDependency(
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  std::array<VkAttachmentReference, 5> attachmentColors;
  VkAttachmentReference attachmentDepthRef = { static_cast<u32>(attachmentColors.size()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
  attachmentColors[0].attachment = 0;
  attachmentColors[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[1].attachment = 1;
  attachmentColors[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[2].attachment = 2;
  attachmentColors[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[3].attachment = 3;
  attachmentColors[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[4].attachment = 4;
  attachmentColors[4].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<u32>(attachmentColors.size());
  subpass.pColorAttachments = attachmentColors.data();
  subpass.pDepthStencilAttachment = &attachmentDepthRef;

  VkRenderPassCreateInfo renderpassCI = CreateRenderPassInfo(
    static_cast<u32>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    2,
    dependenciesNative,
    1,
    &subpass
  );

  VkResult result = 
    vkCreateRenderPass(rhi->LogicDevice()->Native(), &renderpassCI, nullptr, &m_SkyboxRenderPass); 

  if (result != VK_SUCCESS) {
    Log(rError) << "Failed to create native renderpass for skybox!\n";
  }
}


void Sky::CreateGraphicsPipeline(VulkanRHI* rhi)
{
  VkGraphicsPipelineCreateInfo gpCi = { };
  gpCi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  
  VkPipelineInputAssemblyStateCreateInfo assemblyCi = { };
  assemblyCi.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assemblyCi.primitiveRestartEnable = VK_FALSE;
  assemblyCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  
  auto bindings = QuadVertexDescription::GetBindingDescription();
  auto attributes = QuadVertexDescription::GetVertexAttributes();

  VkPipelineVertexInputStateCreateInfo vertexCi = { };
  vertexCi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexCi.vertexAttributeDescriptionCount = static_cast<u32>(attributes.size());
  vertexCi.vertexBindingDescriptionCount = 1;
  vertexCi.pVertexAttributeDescriptions = attributes.data();
  vertexCi.pVertexBindingDescriptions = &bindings;

  VkRect2D scissor = { };
  scissor.extent = { 1024, 1024 };
  scissor.offset = { 0, 0 };

  VkViewport viewport = { };
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = 1024.0f;
  viewport.height = 1024.0f;
  viewport.maxDepth = 1.0f;
  viewport.minDepth = 0.0f;

  VkPipelineViewportStateCreateInfo viewportCi = { };
  viewportCi.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportCi.scissorCount = 1;
  viewportCi.viewportCount = 1;
  viewportCi.pScissors = &scissor;
  viewportCi.pViewports = &viewport;
  
  VkPipelineRasterizationStateCreateInfo rasterCi = { };
  rasterCi.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterCi.cullMode = VK_CULL_MODE_NONE;
  rasterCi.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterCi.lineWidth = 1.0f;
  rasterCi.polygonMode = VK_POLYGON_MODE_FILL;
  rasterCi.rasterizerDiscardEnable = VK_FALSE;
  rasterCi.depthBiasEnable = VK_FALSE;
  rasterCi.depthClampEnable = VK_FALSE;
  rasterCi.depthBiasSlopeFactor = 0.0f;
  rasterCi.depthBiasConstantFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampleCi = { };
  multisampleCi.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleCi.sampleShadingEnable = VK_FALSE;
  multisampleCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleCi.minSampleShading = 1.0f;
  multisampleCi.pSampleMask = nullptr;
  multisampleCi.alphaToOneEnable = VK_FALSE;
  multisampleCi.alphaToCoverageEnable = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depthCi = { };
  depthCi.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthCi.depthTestEnable = VK_FALSE;
  depthCi.depthWriteEnable = VK_FALSE;
  depthCi.depthCompareOp = VK_COMPARE_OP_LESS;
  depthCi.minDepthBounds = 0.0f;
  depthCi.maxDepthBounds = 1.0f;
  depthCi.stencilTestEnable = VK_FALSE;
  depthCi.back = { };
  depthCi.front = { };

  VkPipelineColorBlendAttachmentState colorBlendAttachment =
    CreateColorBlendAttachmentState(
      VK_TRUE,
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      VK_BLEND_FACTOR_SRC_ALPHA,
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD,
      VK_BLEND_FACTOR_ONE,
      VK_BLEND_FACTOR_ZERO,
      VK_BLEND_OP_ADD
    );

  VkPipelineColorBlendStateCreateInfo blendCi = { };
  blendCi.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blendCi.attachmentCount = 1;
  blendCi.pAttachments = &colorBlendAttachment;
  blendCi.logicOpEnable = VK_FALSE;
  blendCi.logicOp = VK_LOGIC_OP_COPY;

  Shader* vert = rhi->CreateShader();
  Shader* frag = rhi->CreateShader();
  
  RendererPass::LoadShader(kAtmVertStr, vert);
  RendererPass::LoadShader(kAtmFragStr, frag);

  gpCi.renderPass = m_pFrameBuffer->RenderPass();
  gpCi.pColorBlendState = &blendCi;
  gpCi.pDepthStencilState = &depthCi;
  gpCi.pDynamicState = nullptr;
  gpCi.pInputAssemblyState = &assemblyCi;
  gpCi.pMultisampleState = &multisampleCi;
  gpCi.pRasterizationState = &rasterCi;
  gpCi.pTessellationState = nullptr;
  gpCi.pVertexInputState = &vertexCi;
  gpCi.pViewportState = &viewportCi;
  gpCi.subpass = 0;
  gpCi.basePipelineHandle = VK_NULL_HANDLE;

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].module = vert->Handle();
  shaderStages[0].pName = kDefaultShaderEntryPointStr;
  shaderStages[0].pSpecializationInfo = nullptr;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].pNext = nullptr;
  shaderStages[0].flags = 0;

  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].module = frag->Handle();
  shaderStages[1].pName = kDefaultShaderEntryPointStr;
  shaderStages[1].pSpecializationInfo = nullptr;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].pNext = nullptr;
  shaderStages[1].flags = 0;

  gpCi.stageCount = static_cast<u32>(shaderStages.size());
  gpCi.pStages = shaderStages.data();

  VkPushConstantRange range = { };
  range.offset = 0;
  range.size = sizeof(Matrix4) * 2;
  range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  DescriptorSetLayout* globalLayout = gResources().GetDescriptorSetLayout(GlobalSetLayoutStr);
  VkDescriptorSetLayout global = globalLayout->Layout();

  VkPipelineLayoutCreateInfo layout = { };
  layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout.pushConstantRangeCount = 1;
  layout.pPushConstantRanges = &range;
  layout.setLayoutCount = 1;
  layout.pSetLayouts = &global;

  m_pPipeline = rhi->CreateGraphicsPipeline();
  m_pPipeline->Initialize(gpCi, layout);

  rhi->FreeShader(vert);
  rhi->FreeShader(frag);
}


void Sky::BuildCmdBuffer(VulkanRHI* rhi)
{
  if (m_pCmdBuffer) {
    rhi->GraphicsWaitIdle();
    m_pCmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  }
  
  VkCommandBufferBeginInfo begin = { };
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
 
  CommandBuffer* cmdBuffer = m_pCmdBuffer;
  cmdBuffer->Begin(begin);

    VkClearValue colorClear;
    colorClear.color = { 0.0f, 0.0f, 0.0f, 1.0f };
    VkRenderPassBeginInfo renderpassBegin = { };
    renderpassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpassBegin.framebuffer = m_pFrameBuffer->Handle();
    renderpassBegin.renderPass = m_pFrameBuffer->RenderPass();
    renderpassBegin.renderArea.extent = { kTextureSize, kTextureSize };
    renderpassBegin.renderArea.offset = { 0, 0 };
    renderpassBegin.clearValueCount = 1;
    renderpassBegin.pClearValues = &colorClear;

    // Get our view matrices.
    // Index                Face
    // 0                    POSITIVE_X
    // 1                    NEGATIVE_X
    // 2                    POSITIVE_Y
    // 3                    NEGATIVE_Y
    // 4                    POSITIVE_Z
    // 5                    NEGATIVE_Z
    std::array<Matrix4, 6> viewMatrices = {
      Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(90.0f), Vector3::UP), Radians(180.0f), Vector3::RIGHT),
      Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(-90.0f), Vector3::UP), Radians(180.0f), Vector3::RIGHT),
      Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(-90.0f), Vector3::RIGHT), Radians(180.0f), Vector3::UP),
      Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(90.0f), Vector3::RIGHT), Radians(180.0f), Vector3::UP),
      Matrix4::Rotate(Matrix4::Identity(), Radians(180.0f), Vector3::BACK),
      Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(180.0f), Vector3::UP), Radians(180.0f), Vector3::FRONT)
    };

    struct ViewerBlock {
      Matrix4             _InvView;
      Matrix4             _InvProj;
    } viewerConsts;

    viewerConsts._InvProj = Matrix4::Perspective(static_cast<r32>(CONST_PI_HALF), 
      1.0f, 0.1f, static_cast<r32>(kTextureSize)).Transpose();

    // can't be doing this...
    GlobalDescriptor* global = gRenderer().GlobalNative();
    RenderQuad*       quad = gRenderer().GetRenderQuad();

    VkDescriptorSet globalDesc = global->Set()->Handle();
    VkBuffer vertexbuf = quad->Quad()->Handle()->NativeBuffer();
    VkBuffer indexbuf = quad->Indices()->Handle()->NativeBuffer();
  
    VkDeviceSize offsets = { 0 };

    VkImageSubresourceRange subRange = { };
    subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subRange.baseMipLevel = 0;
    subRange.baseArrayLayer = 0;
    subRange.levelCount = 1;
    subRange.layerCount = 6;

    VkImageMemoryBarrier imgMemBarrier = { };
    imgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imgMemBarrier.subresourceRange = subRange;
    imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imgMemBarrier.srcAccessMask = 0;
    imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imgMemBarrier.image = m_pCubeMap->Image();

    // set the cubemap image layout for transfer from our framebuffer.
    cmdBuffer->PipelineBarrier(
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0, 
      0, nullptr,
      0, nullptr,
      1, &imgMemBarrier
    );

    // For each face of cube:
    //   Render 
    //   Copy to cubemap face.
    for (size_t face = 0; face < 6; ++face) {
      cmdBuffer->BeginRenderPass(renderpassBegin, VK_SUBPASS_CONTENTS_INLINE);
        cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->Pipeline());
        viewerConsts._InvView = viewMatrices[face].Transpose();
        cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->Layout(), 0, 1, &globalDesc, 0, nullptr);
        cmdBuffer->PushConstants(m_pPipeline->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ViewerBlock), &viewerConsts);
        
        cmdBuffer->BindVertexBuffers(0, 1, &vertexbuf, &offsets);
        cmdBuffer->BindIndexBuffer(indexbuf, 0, VK_INDEX_TYPE_UINT32);
        cmdBuffer->DrawIndexed(quad->Indices()->IndexCount(), 1, 0, 0, 0);
      cmdBuffer->EndRenderPass();
    
      // TODO(): Perform copy to cubemap, here.
      // Barriers will be needed.
      subRange.baseArrayLayer = 0;
      subRange.baseMipLevel = 0;
      subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      subRange.layerCount = 1;
      subRange.levelCount = 1;

      imgMemBarrier.image = m_RenderTexture->Image();
      imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imgMemBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      imgMemBarrier.subresourceRange = subRange;

      // transfer color attachment to transfer.
      cmdBuffer->PipelineBarrier(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imgMemBarrier
      );

      VkImageCopy imgCopy = { };
      imgCopy.srcOffset = { 0, 0, 0 };
      imgCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imgCopy.srcSubresource.baseArrayLayer = 0;
      imgCopy.srcSubresource.mipLevel = 0;
      imgCopy.srcSubresource.layerCount = 1;

      imgCopy.dstOffset = { 0, 0, 0 };
      imgCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imgCopy.dstSubresource.baseArrayLayer = static_cast<u32>(face);
      imgCopy.dstSubresource.layerCount = 1;
      imgCopy.dstSubresource.mipLevel = 0;

      imgCopy.extent.width = kTextureSize;
      imgCopy.extent.height = kTextureSize;
      imgCopy.extent.depth = 1;

      cmdBuffer->CopyImage(
        m_RenderTexture->Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        m_pCubeMap->Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        1, &imgCopy
      );

      imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      imgMemBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      cmdBuffer->PipelineBarrier(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 
        0, nullptr,
        0, nullptr,
        1, &imgMemBarrier
      );
    }

    subRange.baseMipLevel = 0;
    subRange.baseArrayLayer = 0;
    subRange.levelCount = 1;
    subRange.layerCount = 6;

    imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imgMemBarrier.image = m_pCubeMap->Image();
    imgMemBarrier.subresourceRange = subRange;
    
    cmdBuffer->PipelineBarrier(
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &imgMemBarrier
    );
    
  cmdBuffer->End();
}


void Sky::CleanUp()
{
  VulkanRHI* rhi = gRenderer().RHI();
  if (m_pCubeMap) {
    rhi->FreeTexture(m_pCubeMap);
    m_pCubeMap = nullptr;
    rhi->FreeSampler(m_pSampler);
    m_pSampler = nullptr;
  }

  if (m_pAtmosphereSema) {
    rhi->FreeVkSemaphore(m_pAtmosphereSema);
    m_pAtmosphereSema = nullptr;
  }

  if (m_pCmdBuffer) {
    rhi->FreeCommandBuffer(m_pCmdBuffer);
    m_pCmdBuffer = nullptr;
  }

  if (m_pFrameBuffer) {
    rhi->FreeFrameBuffer(m_pFrameBuffer);
    m_pFrameBuffer = nullptr;
  }

  if (m_pPipeline) {
    rhi->FreeGraphicsPipeline(m_pPipeline);
    m_pPipeline = nullptr;
  }

  if (m_RenderTexture) {
    rhi->FreeTexture(m_RenderTexture);
    m_RenderTexture = nullptr;
  }

  if (m_SkyboxRenderPass) {
    vkDestroyRenderPass(rhi->LogicDevice()->Native(), m_SkyboxRenderPass, nullptr);
    m_SkyboxRenderPass = nullptr;
  }


  m_SkyboxIndBuf.CleanUp();
  m_SkyboxVertBuf.CleanUp();
}
} // Recluse 