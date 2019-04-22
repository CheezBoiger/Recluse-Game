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
{
  m_Global._ScreenSize[0] = 1;
  m_Global._ScreenSize[1] = 1;
  m_Global._Gamma = 2.2f;
  m_Global._BloomEnabled = true;
  m_Global._Exposure = 1.0f;
  m_Global._EnableShadows = false;
  m_Global._EnableAA = false;
  m_Global._fDeltaTime = 0.0f;
  m_Global._fEngineTime = 0.0f;
  m_Global._zNear = 0.1f;
  m_Global._zFar = 1.0f;

  // Testing params.
  m_Global._vSunBrightness = 0.01f;
  m_Global._vSunDir = Vector3(-1.0f, 0.0f, 0.0f).normalize();
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
  for (u32 i = 0; i < m_pDescriptorSets.size(); ++i) {
    if (m_pGlobalBuffers[i]) {
      R_DEBUG(rWarning, "Global buffer was not cleaned up!\n");
    }
    if (m_pDescriptorSets[i]) {
      R_DEBUG(rWarning, "Global material was not properly cleaned up!\n");
    }
  }
  );
}


void GlobalDescriptor::initialize(VulkanRHI* pRhi)
{
  R_ASSERT(pRhi, "No RHI owner set in this Global MaterialDescriptor upon initialization!\n");

  VkDeviceSize dSize = sizeof(GlobalBuffer);
  VkBufferCreateInfo bufferCI = {};
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  DescriptorSetLayout* pbrLayout = GlobalSetLayoutKey;

  m_pGlobalBuffers.resize(pRhi->bufferingCount());
  m_pDescriptorSets.resize(pRhi->bufferingCount());
  for (u32 i = 0; i < m_pGlobalBuffers.size(); ++i) {
    m_pGlobalBuffers[i] = pRhi->createBuffer();

    m_pGlobalBuffers[i]->initialize(bufferCI, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
    m_pGlobalBuffers[i]->Map();


    m_pDescriptorSets[i] = pRhi->createDescriptorSet();
    m_pDescriptorSets[i]->allocate(pRhi->descriptorPool(), pbrLayout);

    VkDescriptorBufferInfo globalBufferInfo = {};
    globalBufferInfo.buffer = m_pGlobalBuffers[i]->NativeBuffer();
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

    m_pDescriptorSets[i]->update(static_cast<u32>(writeSets.size()), writeSets.data());
  }
}


void GlobalDescriptor::cleanUp(VulkanRHI* pRhi)
{
  // TODO
  for (u32 i = 0; i < m_pGlobalBuffers.size(); ++i) {
    if (m_pDescriptorSets[i]) {

      pRhi->freeDescriptorSet(m_pDescriptorSets[i]);
      m_pDescriptorSets[i] = nullptr;
    }

    if (m_pGlobalBuffers[i]) {
      m_pGlobalBuffers[i]->UnMap();
      pRhi->freeBuffer(m_pGlobalBuffers[i]);
      m_pGlobalBuffers[i] = nullptr;
    }
  }
}


void GlobalDescriptor::update(VulkanRHI* pRhi, u32 frameIndex)
{
  u32 currFrame = frameIndex;
  R_ASSERT(m_pGlobalBuffers[currFrame]->Mapped(), "Global data was not mapped!");
  memcpy(m_pGlobalBuffers[currFrame]->Mapped(), &m_Global, sizeof(GlobalBuffer));
  
  VkMappedMemoryRange range = { };
  range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  range.memory = m_pGlobalBuffers[currFrame]->Memory();
  range.size = m_pGlobalBuffers[currFrame]->MemorySize();
  pRhi->logicDevice()->FlushMappedMemoryRanges(1, &range);
}
} // Recluse