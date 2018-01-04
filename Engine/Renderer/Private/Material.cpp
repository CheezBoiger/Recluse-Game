// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Material.hpp"
#include "Resources.hpp"
#include "Renderer.hpp"
#include "RendererData.hpp"
#include "TextureType.hpp"

#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"

#include "Core/Exception.hpp"

#include <array>


namespace Recluse {


Material::Material()
  : mAlbedo(nullptr)
  , mMetallic(nullptr)
  , mRoughness(nullptr)
  , mNormal(nullptr)
  , mAo(nullptr)
  , mEmissive(nullptr)
  , mSampler(nullptr)
{ 
  m_MaterialData._Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  m_MaterialData._LodBias = 0.0f;
  m_MaterialData._Transparency = 1.0f;
  m_MaterialData._HasAlbedo = false;
  m_MaterialData._HasAO = false;
  m_MaterialData._HasEmissive = false;
  m_MaterialData._HasMetallic = false;
  m_MaterialData._HasNormal = false;
  m_MaterialData._BaseEmissive = 0.0f;
  m_MaterialData._HasRoughness = false;
  m_MaterialData._IsTransparent = false;
  m_MaterialData._BaseMetal = 0.0f;
  m_MaterialData._BaseRough = 1.0f;
}


void Material::Initialize(Renderer* renderer)
{
  m_pRenderer = renderer;
  m_pBuffer = renderer->RHI()->CreateBuffer();
  VkDeviceSize memSize = sizeof(MaterialBuffer);
  VkBufferCreateInfo bufferCi = { };
  bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCi.size = memSize;
  
  m_pBuffer->Initialize(bufferCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}


void Material::Update()
{
  m_pBuffer->Map();
    memcpy(m_pBuffer->Mapped(), &m_MaterialData, sizeof(MaterialBuffer));
  m_pBuffer->UnMap();
}


void Material::CleanUp()
{
  if (m_pBuffer)  {
    m_pRenderer->RHI()->FreeBuffer(m_pBuffer);
    m_pBuffer  = nullptr;
  }
}
} // Recluse