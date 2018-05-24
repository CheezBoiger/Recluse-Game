// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MaterialDescriptor.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "TextureType.hpp"

#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"

#include "Core/Exception.hpp"

#include <array>


namespace Recluse {


MaterialDescriptor::MaterialDescriptor()
  : m_pAlbedo(nullptr)
  , m_pRoughnessMetallic(nullptr)
  , m_pNormal(nullptr)
  , m_pAo(nullptr)
  , m_pEmissive(nullptr)
  , m_pSampler(nullptr)
  , m_pBuffer(nullptr)
  , m_bNeedsUpdate(true)
{ 
  m_MaterialData._Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  m_MaterialData._Opacity = 1.0f;
  m_MaterialData._HasAlbedo = false;
  m_MaterialData._HasAO = false;
  m_MaterialData._HasEmissive = false;
  m_MaterialData._HasMetallic = false;
  m_MaterialData._HasNormal = false;
  m_MaterialData._emissiveFactor = 0.0f;
  m_MaterialData._HasRoughness = false;
  m_MaterialData._IsTransparent = false;
  m_MaterialData._metalFactor = 0.0f;
  m_MaterialData._roughFactor = 1.0f;
  m_MaterialData._Pad = 0;

  m_materialSet = nullptr;
}


MaterialDescriptor::~MaterialDescriptor()
{
  if (m_pBuffer) {
    R_DEBUG(rWarning, "MaterialDescriptor buffer was not properly cleaned up!\n");
  }
}


void MaterialDescriptor::Initialize()
{
  if (m_pBuffer) return;

  m_pBuffer = m_pRhi->CreateBuffer();
  VkDeviceSize memSize = sizeof(MaterialBuffer);
  VkBufferCreateInfo bufferCi = { };
  bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCi.size = memSize;
  
  m_pBuffer->Initialize(bufferCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  m_pBuffer->Map();

  m_materialSet = m_pRhi->CreateDescriptorSet();

  DescriptorSetLayout* MaterialLayout = MaterialSetLayoutKey;

  m_materialSet->Allocate(m_pRhi->DescriptorPool(), MaterialLayout);
}


void MaterialDescriptor::Update()
{
  if ((m_bNeedsUpdate & MATERIAL_DESCRIPTOR_UPDATE)) {
    R_DEBUG(rNotify, "Updating material Descriptor.\n");
    Sampler* sampler = DefaultSamplerKey;
    if (m_pSampler) sampler = m_pSampler->Handle();

    Texture* defaultTexture = DefaultTextureKey;
    std::array<VkWriteDescriptorSet, 6> MaterialWriteSets;
    VkDescriptorImageInfo albedoInfo = {};
    albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (Albedo()) {
      albedoInfo.imageView = Albedo()->Handle()->View();
    }
    else {
      albedoInfo.imageView = defaultTexture->View();
    }
    albedoInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo roughMetalInfo = {};
    roughMetalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (RoughnessMetallic()) {
      roughMetalInfo.imageView = RoughnessMetallic()->Handle()->View();
    }
    else {
      roughMetalInfo.imageView = defaultTexture->View();
    }
    roughMetalInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo normalInfo = {};
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (Normal()) {
      normalInfo.imageView = Normal()->Handle()->View();
    }
    else {
      normalInfo.imageView = defaultTexture->View();
    }
    normalInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo aoInfo = {};
    aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (Ao()) {
      aoInfo.imageView = Ao()->Handle()->View();
    }
    else {
      aoInfo.imageView = defaultTexture->View();
    }
    aoInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo emissiveInfo = {};
    emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (Emissive()) {
      emissiveInfo.imageView = Emissive()->Handle()->View();
    }
    else {
      emissiveInfo.imageView = defaultTexture->View();
    }
    emissiveInfo.sampler = sampler->Handle();

    VkDescriptorBufferInfo matBufferInfo = { };
    matBufferInfo.buffer = m_pBuffer->NativeBuffer();
    matBufferInfo.offset = 0;
    matBufferInfo.range = sizeof(MaterialBuffer);

    MaterialWriteSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[0].dstBinding = 1;
    MaterialWriteSets[0].descriptorCount = 1;
    MaterialWriteSets[0].dstArrayElement = 0;
    MaterialWriteSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[0].pImageInfo = &albedoInfo;
    MaterialWriteSets[0].pNext = nullptr;

    MaterialWriteSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[1].dstBinding = 2;
    MaterialWriteSets[1].descriptorCount = 1;
    MaterialWriteSets[1].dstArrayElement = 0;
    MaterialWriteSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[1].pImageInfo = &roughMetalInfo;
    MaterialWriteSets[1].pNext = nullptr;

    MaterialWriteSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[2].dstBinding = 3;
    MaterialWriteSets[2].descriptorCount = 1;
    MaterialWriteSets[2].dstArrayElement = 0;
    MaterialWriteSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[2].pImageInfo = &normalInfo;
    MaterialWriteSets[2].pNext = nullptr;

    MaterialWriteSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[3].dstBinding = 4;
    MaterialWriteSets[3].descriptorCount = 1;
    MaterialWriteSets[3].dstArrayElement = 0;
    MaterialWriteSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[3].pImageInfo = &aoInfo;
    MaterialWriteSets[3].pNext = nullptr;

    MaterialWriteSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[4].dstBinding = 5;
    MaterialWriteSets[4].descriptorCount = 1;
    MaterialWriteSets[4].dstArrayElement = 0;
    MaterialWriteSets[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[4].pImageInfo = &emissiveInfo;
    MaterialWriteSets[4].pNext = nullptr;

    MaterialWriteSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[5].dstBinding = 0;
    MaterialWriteSets[5].descriptorCount = 1;
    MaterialWriteSets[5].dstArrayElement = 0;
    MaterialWriteSets[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    MaterialWriteSets[5].pBufferInfo = &matBufferInfo;
    MaterialWriteSets[5].pNext = nullptr;

    m_materialSet->Update(static_cast<u32>(MaterialWriteSets.size()), MaterialWriteSets.data());
    //SwapDescriptorSet();
  }
  
  if ((m_bNeedsUpdate & MATERIAL_BUFFER_UPDATE)) {
    R_ASSERT(m_pBuffer->Mapped(), "Material buffer was not mapped!");
    memcpy(m_pBuffer->Mapped(), &m_MaterialData, sizeof(MaterialBuffer));
  }

  m_bNeedsUpdate = 0;
}


void MaterialDescriptor::CleanUp()
{

  if (m_materialSet) {
    m_pRhi->FreeDescriptorSet(m_materialSet);
    m_materialSet = nullptr;
  }
  if (m_pBuffer)  {
    m_pBuffer->UnMap();
    m_pRhi->FreeBuffer(m_pBuffer);
    m_pBuffer  = nullptr;
  }
}
} // Recluse