// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "LightDescriptor.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "TextureType.hpp"

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

u32 LightBuffer::MaxNumDirectionalLights()
{
  return MAX_DIRECTIONAL_LIGHTS;
}


u32 LightBuffer::MaxNumPointLights()
{
  return MAX_POINT_LIGHTS;
}


LightDescriptor::LightDescriptor()
  : m_pOpaqueShadowMap(nullptr)
  , m_pShadowSampler(nullptr)
  , m_pLightDescriptorSet(nullptr)
  , m_pLightViewDescriptorSet(nullptr)
  , m_pLightBuffer(nullptr)
  , m_pLightViewBuffer(nullptr)
  , m_pFrameBuffer(nullptr)
  , m_pRenderPass(nullptr)
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

  m_PrimaryLightSpace._lightSz = Vector4();
  m_PrimaryLightSpace._near = Vector4();
  m_PrimaryLightSpace._shadowTechnique = Vector4();
}


LightDescriptor::~LightDescriptor()
{
  if (m_pLightBuffer) {
    R_DEBUG(rWarning, "Light buffer was not cleaned up!\n");
  }

  if (m_pLightDescriptorSet) {
    R_DEBUG(rWarning, "Light MaterialDescriptor descriptor set was not properly cleaned up!\n");
  }
  
  if (m_pOpaqueShadowMap) {
    R_DEBUG(rWarning, "Light Shadow Map texture was not properly cleaned up!\n");
  }

  if (m_pShadowSampler) {
    R_DEBUG(rWarning, "Light Shadow Map sampler was not properly cleaned up!\n");
  }

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
}


void LightDescriptor::Initialize(VulkanRHI* pRhi, ShadowDetail shadowDetail)
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
  InitializePrimaryShadow(pRhi);

  if (shadowDetail < SHADOWS_MEDIUM) {
    m_PrimaryLightSpace._shadowTechnique = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
  } else {
    m_PrimaryLightSpace._shadowTechnique = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
  }
}


void LightDescriptor::CleanUp(VulkanRHI* pRhi)
{
  if (m_pOpaqueShadowMap) {
    pRhi->FreeTexture(m_pOpaqueShadowMap);
    m_pOpaqueShadowMap = nullptr;
  }

  if (m_pShadowSampler) {
    pRhi->FreeSampler(m_pShadowSampler);
    m_pShadowSampler = nullptr;
  }

  // TODO
  if (m_pLightDescriptorSet) {
    pRhi->FreeDescriptorSet(m_pLightDescriptorSet);
    m_pLightDescriptorSet = nullptr;
  }

  if (m_pLightViewDescriptorSet) {
    pRhi->FreeDescriptorSet(m_pLightViewDescriptorSet);
    m_pLightViewDescriptorSet = nullptr;
  }

  if (m_pLightBuffer) {
    m_pLightBuffer->UnMap();
    pRhi->FreeBuffer(m_pLightBuffer);
    m_pLightBuffer = nullptr;
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
}


void LightDescriptor::Update(VulkanRHI* pRhi)
{
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

  R_ASSERT(m_pLightBuffer->Mapped(), "Light buffer was not mapped!");
  memcpy(m_pLightBuffer->Mapped(), &m_Lights, sizeof(LightBuffer));

  if (PrimaryShadowEnabled() && m_pLightViewBuffer) {
    R_ASSERT(m_pLightViewBuffer->Mapped(), "Light view buffer was not mapped!");
    memcpy(m_pLightViewBuffer->Mapped(), &m_PrimaryLightSpace, sizeof(LightViewSpace));

    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pLightViewBuffer->Memory();
    range.size = m_pLightViewBuffer->MemorySize();
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  VkMappedMemoryRange lightRange = { };
  lightRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  lightRange.memory = m_pLightBuffer->Memory();
  lightRange.size = m_pLightBuffer->MemorySize();
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

  // TODO(): Once we create our shadow map, we will add it here.
  // This will pass the rendered shadow map to the pbr pipeline.
  VkDescriptorImageInfo globalShadowInfo = {};
  globalShadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalShadowInfo.imageView = m_pOpaqueShadowMap->View();
  globalShadowInfo.sampler = m_pShadowSampler->Handle();

  std::array<VkWriteDescriptorSet, 2> writeSets;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pBufferInfo = &lightBufferInfo;
  writeSets[0].pNext = nullptr;
  writeSets[0].dstBinding = 0;

  writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[1].descriptorCount = 1;
  writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[1].dstArrayElement = 0;
  writeSets[1].pNext = nullptr;
  writeSets[1].pImageInfo = &globalShadowInfo;
  writeSets[1].dstBinding = 1;

  m_pLightDescriptorSet->Update(static_cast<u32>(writeSets.size()), writeSets.data());
}


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

  VkWriteDescriptorSet write = { };
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.dstArrayElement = 0;
  write.dstBinding = 0;
  write.dstSet = nullptr;
  write.pImageInfo = nullptr;
  write.pBufferInfo = &viewBuf;  
  
  m_pLightViewDescriptorSet->Update(1, &write);

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
} // Recluse