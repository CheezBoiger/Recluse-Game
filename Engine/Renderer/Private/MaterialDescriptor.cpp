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
  , m_pAlbedoSampler(nullptr)
  , m_pNormalSampler(nullptr)
  , m_pAoSampler(nullptr)
  , m_pRoughMetalSampler(nullptr)
  , m_pEmissiveSampler(nullptr)
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
  m_MaterialData._AnisoSpec = Vector4(0.01f, 0.01f, 0.0f, 0.0f);
  m_MaterialData._offsetUV = Vector4(0.0f, 0.0f, 0.0f, 0.0f); 
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
  R_ASSERT(!m_pBuffer, "MaterialDescriptor buffer was not properly cleaned up!\n");
}


void MaterialDescriptor::initialize(VulkanRHI* pRhi)
{
  if (m_pBuffer) return;

  m_pBuffer = pRhi->createBuffer();
  VkDeviceSize memSize = sizeof(MaterialBuffer);
  VkBufferCreateInfo bufferCi = { };
  bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCi.size = memSize;
  
  m_pBuffer->initialize(bufferCi, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
  m_pBuffer->map();

  m_materialSet = pRhi->createDescriptorSet();

  DescriptorSetLayout* MaterialLayout = MaterialSetLayoutKey;

  m_materialSet->allocate(pRhi->descriptorPool(), MaterialLayout);
}


void MaterialDescriptor::update(VulkanRHI* pRhi)
{
#define CHECK_SAMPLER(pSampler) (pSampler) ? pSampler->getHandle()->getHandle() : DefaultSampler2DKey->getHandle()
  if ((m_bNeedsUpdate & MATERIAL_DESCRIPTOR_UPDATE_BIT)) {
    R_DEBUG(rNotify, "Updating material Descriptor.\n");

    Texture* defaultTexture = DefaultTextureKey;
    std::array<VkWriteDescriptorSet, 6> MaterialWriteSets;
    VkDescriptorImageInfo albedoInfo = {};
    albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (getAlbedo()) {
      albedoInfo.imageView = getAlbedo()->getHandle()->getView();
    }
    else {
      albedoInfo.imageView = defaultTexture->getView();
    }
    albedoInfo.sampler = CHECK_SAMPLER(m_pAlbedoSampler);

    VkDescriptorImageInfo roughMetalInfo = {};
    roughMetalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (getRoughnessMetallic()) {
      roughMetalInfo.imageView = getRoughnessMetallic()->getHandle()->getView();
    }
    else {
      roughMetalInfo.imageView = defaultTexture->getView();
    }
    roughMetalInfo.sampler = CHECK_SAMPLER(m_pRoughMetalSampler);

    VkDescriptorImageInfo normalInfo = {};
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (getNormal()) {
      normalInfo.imageView = getNormal()->getHandle()->getView();
    }
    else {
      normalInfo.imageView = defaultTexture->getView();
    }
    normalInfo.sampler = CHECK_SAMPLER(m_pNormalSampler);

    VkDescriptorImageInfo aoInfo = {};
    aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (getAo()) {
      aoInfo.imageView = getAo()->getHandle()->getView();
    }
    else {
      aoInfo.imageView = defaultTexture->getView();
    }
    aoInfo.sampler = CHECK_SAMPLER(m_pAoSampler);

    VkDescriptorImageInfo emissiveInfo = {};
    emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (getEmissive()) {
      emissiveInfo.imageView = getEmissive()->getHandle()->getView();
    }
    else {
      emissiveInfo.imageView = defaultTexture->getView();
    }
    emissiveInfo.sampler = CHECK_SAMPLER(m_pEmissiveSampler);

    VkDescriptorBufferInfo matBufferInfo = { };
    matBufferInfo.buffer = m_pBuffer->getNativeBuffer();
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

    m_materialSet->update(static_cast<U32>(MaterialWriteSets.size()), MaterialWriteSets.data());
    //SwapDescriptorSet();
  }
  
  if ((m_bNeedsUpdate & MATERIAL_BUFFER_UPDATE_BIT)) {
    R_ASSERT(m_pBuffer->getMapped(), "Material buffer was not mapped!");
    memcpy(m_pBuffer->getMapped(), &m_MaterialData, sizeof(MaterialBuffer));

    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pBuffer->getMemory();
    range.size = VK_WHOLE_SIZE;
    pRhi->logicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  m_bNeedsUpdate = 0;
}


void MaterialDescriptor::cleanUp(VulkanRHI* pRhi)
{

  if (m_materialSet) {
    pRhi->freeDescriptorSet(m_materialSet);
    m_materialSet = nullptr;
  }
  if (m_pBuffer)  {
    m_pBuffer->unmap();
    pRhi->freeBuffer(m_pBuffer);
    m_pBuffer  = nullptr;
  }
}
} // Recluse