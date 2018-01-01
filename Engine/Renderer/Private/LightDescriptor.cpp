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

#include <array>


namespace Recluse {

LightDescriptor::LightDescriptor()
  : m_pShadowMap(nullptr)
  , m_pShadowSampler(nullptr)
  , m_pRhi(nullptr)
  , m_pLightDescriptorSet(nullptr)
  , m_pLightViewDescriptorSet(nullptr)
  , m_pLightBuffer(nullptr)
  , m_pLightViewBuffer(nullptr)
  , m_pFrameBuffer(nullptr)
{
  m_Lights._PrimaryLight._Enable = false;
  m_Lights._PrimaryLight._Pad[0] = 0;
  m_Lights._PrimaryLight._Pad[1] = 0;
  //mLights.primaryLight.pad[2] = 0;
  for (size_t i = 0; i < 128; ++i) {
    m_Lights._PointLights[i]._Position = Vector4();
    m_Lights._PointLights[i]._Color = Vector4();
    m_Lights._PointLights[i]._Range = 0.0f;
    m_Lights._PointLights[i]._Intensity = 1.0f;
    m_Lights._PointLights[i]._Enable = false;
    m_Lights._PointLights[i]._Pad = 0;
    //mLights.pointLights[i].pad[2] = 0.0f;
  }

  for (size_t i = 0; i < 32; ++i) {
    m_Lights._DirectionalLights[i]._Direction = Vector4();
    m_Lights._DirectionalLights[i]._Enable = false;
    m_Lights._DirectionalLights[i]._Intensity = 1.0f;
    m_Lights._DirectionalLights[i]._Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  }
}


LightDescriptor::~LightDescriptor()
{
  if (m_pLightBuffer) {
    R_DEBUG(rWarning, "Light buffer was not cleaned up!\n");
  }

  if (m_pLightDescriptorSet) {
    R_DEBUG(rWarning, "Light Material descriptor set was not properly cleaned up!\n");
  }
  
  if (m_pShadowMap) {
    R_DEBUG(rWarning, "Light Shadow Map texture was not properly cleaned up!\n");
  }

  if (m_pShadowSampler) {
    R_DEBUG(rWarning, "Light Shadow Map sampler was not properly cleaned up!\n");
  }

  if (m_pFrameBuffer) {
    R_DEBUG(rWarning, "Light framebuffer was not properly cleaned up!\n");
  }
  
  if (m_pLightViewBuffer) {
    R_DEBUG(rWarning, "Light view buffer was not properly cleaned up!\n");
  }
}


void LightDescriptor::Initialize()
{
  if (!m_pRhi) {
    R_DEBUG(rError, "RHI owner not set for light material upon initialization!\n");
    return;
  }

  // This class has already been initialized.
  if (m_pLightBuffer || m_pLightDescriptorSet)  {
    R_DEBUG(rNotify, "This light buffer is already initialized! Skipping...\n");
    return;
  }

  m_pLightBuffer = m_pRhi->CreateBuffer();
  VkBufferCreateInfo bufferCI = {};
  VkDeviceSize dSize = sizeof(LightBuffer);
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  m_pLightBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Create our shadow map texture.
  if (!m_pShadowMap) {

    // TODO():
    m_pShadowMap = m_pRhi->CreateTexture();

    VkImageCreateInfo ImageCi = {};
    ImageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCi.arrayLayers = 1;
    ImageCi.extent.width = 1024;
    ImageCi.extent.height = 1024;
    ImageCi.extent.depth = 1;
    ImageCi.format = VK_FORMAT_R8G8B8A8_UNORM;
    ImageCi.imageType = VK_IMAGE_TYPE_2D;
    ImageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCi.mipLevels = 1;
    ImageCi.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCi.usage =  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ImageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkImageViewCreateInfo ViewCi = { };
    ViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ViewCi.components = { };
    ViewCi.format = ImageCi.format;
    ViewCi.subresourceRange = { };
    ViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ViewCi.subresourceRange.baseArrayLayer = 0;
    ViewCi.subresourceRange.baseMipLevel = 0;
    ViewCi.subresourceRange.layerCount = 1;
    ViewCi.subresourceRange.levelCount = 1;
    ViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    
    m_pShadowMap->Initialize(ImageCi, ViewCi);
  }

  if (!m_pShadowSampler) {
    // TODO():
    m_pShadowSampler = m_pRhi->CreateSampler();
    VkSamplerCreateInfo SamplerCi = { };
    SamplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCi.addressModeV = SamplerCi.addressModeU;
    SamplerCi.addressModeW = SamplerCi.addressModeV;
    SamplerCi.anisotropyEnable = VK_FALSE;
    SamplerCi.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
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

  InitializeNativeLights();
  InitializePrimaryShadow();
}


void LightDescriptor::CleanUp()
{
  if (m_pShadowMap) {
    m_pRhi->FreeTexture(m_pShadowMap);
    m_pShadowMap = nullptr;
  }

  if (m_pShadowSampler) {
    m_pRhi->FreeSampler(m_pShadowSampler);
    m_pShadowSampler = nullptr;
  }

  // TODO
  if (m_pLightDescriptorSet) {
    m_pRhi->FreeDescriptorSet(m_pLightDescriptorSet);
    m_pLightDescriptorSet = nullptr;
  }

  if (m_pLightBuffer) {
    m_pRhi->FreeBuffer(m_pLightBuffer);
    m_pLightBuffer = nullptr;
  }

  if (m_pFrameBuffer) {
    m_pRhi->FreeFrameBuffer(m_pFrameBuffer);
    m_pFrameBuffer = nullptr;
  }

  if (m_pLightViewBuffer) {
    m_pRhi->FreeBuffer(m_pLightViewBuffer);
    m_pLightViewBuffer = nullptr;
  }
}


void LightDescriptor::Update()
{
  Vector3 Eye = Vector3(
    m_Lights._PrimaryLight._Direction.x, 
    m_Lights._PrimaryLight._Direction.y, 
    m_Lights._PrimaryLight._Direction.z
  );

  // Pass as one matrix.
  Matrix4 view = Matrix4::LookAt(Eye, Vector3::ZERO, Vector3::UP);
  Matrix4 proj = Matrix4::Ortho(1024.0f, 1024.0f, 0.0001f, 1000.0f);
  m_PrimaryLightSpace._ViewProj = view * proj;


  m_pLightBuffer->Map();
    memcpy(m_pLightBuffer->Mapped(), &m_Lights, sizeof(LightBuffer));
  m_pLightBuffer->UnMap();

  if (PrimaryShadowEnabled() && m_pLightViewBuffer) {
    m_pLightViewBuffer->Map();
      memcpy(m_pLightViewBuffer->Mapped(), &m_PrimaryLightSpace, sizeof(LightViewSpace));
    m_pLightViewBuffer->UnMap();
  }
}


void LightDescriptor::InitializeNativeLights()
{
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout(LightSetLayoutStr);
  m_pLightDescriptorSet = m_pRhi->CreateDescriptorSet();
  m_pLightDescriptorSet->Allocate(m_pRhi->DescriptorPool(), pbrLayout);

  VkDescriptorBufferInfo lightBufferInfo = {};
  lightBufferInfo.buffer = m_pLightBuffer->NativeBuffer();
  lightBufferInfo.offset = 0;
  lightBufferInfo.range = sizeof(LightBuffer);

  // TODO(): Once we create our shadow map, we will add it here.
  // This will pass the rendered shadow map to the pbr pipeline.
  VkDescriptorImageInfo globalShadowInfo = {};
  globalShadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalShadowInfo.imageView = m_pShadowMap->View();
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


void LightDescriptor::InitializePrimaryShadow()
{
  // TODO(): Create DescriptorSet and Framebuffer for shadow pass.
}
} // Recluse