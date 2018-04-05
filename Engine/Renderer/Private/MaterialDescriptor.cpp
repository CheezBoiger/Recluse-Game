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
  m_MaterialData._Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  m_MaterialData._Opacity = 1.0f;
  m_MaterialData._HasAlbedo = false;
  m_MaterialData._HasAO = false;
  m_MaterialData._HasEmissive = false;
  m_MaterialData._HasMetallic = false;
  m_MaterialData._HasNormal = false;
  m_MaterialData._BaseEmissive = 0.0f;
  m_MaterialData._HasRoughness = false;
  m_MaterialData._IsTransparent = false;
  m_MaterialData._metalFactor = 0.0f;
  m_MaterialData._roughFactor = 1.0f;
  m_MaterialData._Pad = 0;
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
}


void MaterialDescriptor::Update()
{
  if (m_bNeedsUpdate) {
    m_pBuffer->Map();
      memcpy(m_pBuffer->Mapped(), &m_MaterialData, sizeof(MaterialBuffer));
    m_pBuffer->UnMap();
    m_bNeedsUpdate = false;
  }
}


void MaterialDescriptor::CleanUp()
{
  if (m_pBuffer)  {
    m_pRhi->FreeBuffer(m_pBuffer);
    m_pBuffer  = nullptr;
  }
}
} // Recluse