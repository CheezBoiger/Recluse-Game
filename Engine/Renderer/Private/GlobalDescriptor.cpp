// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GlobalDescriptor.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "TextureType.hpp"
#include "SkyAtmosphere.hpp"

#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"

#include "Core/Exception.hpp"

#include <array>


namespace Recluse {


GlobalDescriptor::GlobalDescriptor()
  : m_pGlobalBuffer(nullptr)
  , m_pDescriptorSet(nullptr)
{
  m_Global._ScreenSize[0] = 0;
  m_Global._ScreenSize[1] = 0;
  m_Global._Gamma = 2.2f;
  m_Global._BloomEnabled = true;
  m_Global._Exposure = 1.0f;
  m_Global._EnableShadows = false;
  m_Global._EnableAA = false;
  m_Global._fDeltaTime = 0.0f;
  m_Global._fEngineTime = 0.0f;

  // Testing params.
  m_Global._vSunBrightness = 0.01f;
  m_Global._vSunDir = Vector3(-1.0f, 0.0f, 0.0f).Normalize();
  m_Global._Mie = 0.01f;
  m_Global._Rayleigh = 1.6f;
  m_Global._MieDist = 0.997f;
  m_Global._fIntensity = 1.0f;
  m_Global._fMieStength = 0.1f;
  m_Global._fRayleighStength = 5.0f;
  m_Global._fScatterStrength = 1000.0f;
  m_Global._vAirColor = SkyRenderer::kDefaultAirColor;
  
}


GlobalDescriptor::~GlobalDescriptor()
{
  DEBUG_OP(
  if (m_pGlobalBuffer) {
    R_DEBUG(rWarning, "Global buffer was not cleaned up!\n");
  }
  if (m_pDescriptorSet) {
    R_DEBUG(rWarning, "Global material was not properly cleaned up!\n");
  }
  );
}


void GlobalDescriptor::Initialize(VulkanRHI* pRhi)
{
  R_ASSERT(pRhi, "No RHI owner set in this Global MaterialDescriptor upon initialization!\n");
  R_ASSERT(!m_pGlobalBuffer && !m_pDescriptorSet, "This global buffer is already intialized...\n");

  m_pGlobalBuffer = pRhi->CreateBuffer();
  VkDeviceSize dSize = sizeof(GlobalBuffer);
  VkBufferCreateInfo bufferCI = {};
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  m_pGlobalBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_pGlobalBuffer->Map();

  DescriptorSetLayout* pbrLayout = GlobalSetLayoutKey;

  m_pDescriptorSet = pRhi->CreateDescriptorSet();
  m_pDescriptorSet->Allocate(pRhi->DescriptorPool(), pbrLayout);

  VkDescriptorBufferInfo globalBufferInfo = {};
  globalBufferInfo.buffer = m_pGlobalBuffer->NativeBuffer();
  globalBufferInfo.offset = 0;
  globalBufferInfo.range = sizeof(GlobalBuffer);

  std::array<VkWriteDescriptorSet, 1> writeSets;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstBinding = 0;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].pNext = nullptr;
  writeSets[0].pBufferInfo = &globalBufferInfo;

  m_pDescriptorSet->Update(static_cast<u32>(writeSets.size()), writeSets.data());
}


void GlobalDescriptor::CleanUp(VulkanRHI* pRhi)
{
  // TODO
  if (m_pDescriptorSet) {

    pRhi->FreeDescriptorSet(m_pDescriptorSet);
    m_pDescriptorSet = nullptr;
  }

  if (m_pGlobalBuffer) {
    m_pGlobalBuffer->UnMap();
    pRhi->FreeBuffer(m_pGlobalBuffer);
    m_pGlobalBuffer = nullptr;
  }
}


void GlobalDescriptor::Update(VulkanRHI* pRhi)
{
  R_ASSERT(m_pGlobalBuffer->Mapped(), "Global data was not mapped!");
  memcpy(m_pGlobalBuffer->Mapped(), &m_Global, sizeof(GlobalBuffer));

  VkMappedMemoryRange range = { };
  range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  range.memory = m_pGlobalBuffer->Memory();
  range.size = m_pGlobalBuffer->MemorySize();
  pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
}
} // Recluse