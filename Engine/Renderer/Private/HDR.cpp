// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "HDR.hpp"

#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"

#include "RHI/DescriptorSet.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"

#include <array>


namespace Recluse {



HDR::HDR()
  : m_pSet(nullptr)
  , m_pLayout(nullptr)
{
  m_config._allowChromaticAberration.x = 1.0;
  m_config._k.x = -0.02f;
  m_config._kcube.x = 0.02f;
  m_config._bEnable.x = 0.0f;
  m_config._bEnable.y = 0.0f;
  m_config._bEnable.z = 0.0f;
  m_config._bEnable.w = 0.0f;
  m_config._interleavedVideoShakeInterval = 0.0f; // Every x seconds by default. This disables shaking.

  m_brightFilterParams.bloomScale16x = 1.0f;
  m_brightFilterParams.bloomScale8x = 1.0f;
  m_brightFilterParams.bloomScale4x = 1.0f;
  m_brightFilterParams.bloomScale2x = 1.0f;
    
  m_brightFilterParams.bloomStrength16x = 1.0f; 
  m_brightFilterParams.bloomStrength8x = 1.0f;
  m_brightFilterParams.bloomStrength4x = 1.0f;
  m_brightFilterParams.bloomStrength2x = 1.0f;
}



HDR::~HDR()
{
}


void HDR::initialize(VulkanRHI* pRhi)
{
  {
    std::array<VkDescriptorSetLayoutBinding, 1> bindings;
    bindings[0] = { };
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCi = { };
    layoutCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCi.bindingCount = static_cast<U32>(bindings.size());
    layoutCi.pBindings = bindings.data();
    m_pLayout = pRhi->createDescriptorSetLayout();
    m_pLayout->initialize(layoutCi);
  }

  {
    VkBufferCreateInfo bufferCi = { };
    bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCi.size = VkDeviceSize(sizeof(ConfigHDR));
    m_pBuffer = pRhi->createBuffer();
    m_pBuffer->initialize(pRhi->logicDevice()->getNative(), 
                          bufferCi, 
                          PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
  }


  m_pSet = pRhi->createDescriptorSet();
  m_pSet->allocate(pRhi->descriptorPool(), m_pLayout);

  VkDescriptorBufferInfo bufferInfo = { };
  bufferInfo.buffer = m_pBuffer->getNativeBuffer();
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(ConfigHDR);
  
  std::array<VkWriteDescriptorSet, 1> writeSets;
  writeSets[0] = { };
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].dstBinding = 0;
  writeSets[0].pBufferInfo = &bufferInfo;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

  m_pSet->update(static_cast<U32>(writeSets.size()), writeSets.data());
}


void HDR::cleanUp(VulkanRHI* pRhi)
{
  if (m_pBuffer) {
    pRhi->freeBuffer(m_pBuffer);
    m_pBuffer = nullptr;
  }

  if (m_pSet) {
    pRhi->freeDescriptorSet(m_pSet);
    m_pSet = nullptr;
  }

  if (m_pLayout) {
    pRhi->freeDescriptorSetLayout(m_pLayout);
    m_pLayout = nullptr;
  }
}


void HDR::UpdateToGPU(VulkanRHI* pRhi)
{
  R_ASSERT(m_pBuffer->getMapped(), "HDR Realtime settings are not mapped.");
  memcpy(m_pBuffer->getMapped(), &m_config, sizeof(ConfigHDR));

  VkMappedMemoryRange memRange = { };
  memRange.memory = m_pBuffer->getMemory();
  memRange.offset = m_pBuffer->getMemoryOffset();
  memRange.size = m_pBuffer->getMemorySize();
  memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  pRhi->logicDevice()->FlushMappedMemoryRanges(1, &memRange);
}
} // Recluse