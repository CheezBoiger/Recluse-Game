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
  m_config._k.x = -0.05f;
  m_config._kcube.x = 0.05f;
  m_config._interleavedVideo.x = 0.0f;
  m_config._interleavedVideo.y = 0.0f;
  m_config._interleavedVideoShakeInterval = 0.0f; // Every x seconds by default. This disables shaking.
}



HDR::~HDR()
{
}


void HDR::Initialize(VulkanRHI* pRhi)
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
    layoutCi.bindingCount = static_cast<u32>(bindings.size());
    layoutCi.pBindings = bindings.data();
    m_pLayout = pRhi->CreateDescriptorSetLayout();
    m_pLayout->Initialize(layoutCi);
  }

  {
    VkBufferCreateInfo bufferCi = { };
    bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCi.size = VkDeviceSize(sizeof(ConfigHDR));
    m_pBuffer = pRhi->CreateBuffer();
    m_pBuffer->Initialize(bufferCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    m_pBuffer->Map();
  }


  m_pSet = pRhi->CreateDescriptorSet();
  m_pSet->Allocate(pRhi->DescriptorPool(), m_pLayout);

  VkDescriptorBufferInfo bufferInfo = { };
  bufferInfo.buffer = m_pBuffer->NativeBuffer();
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

  m_pSet->Update(static_cast<u32>(writeSets.size()), writeSets.data());
}


void HDR::CleanUp(VulkanRHI* pRhi)
{
  if (m_pBuffer) {
    pRhi->FreeBuffer(m_pBuffer);
    m_pBuffer = nullptr;
  }

  if (m_pSet) {
    pRhi->FreeDescriptorSet(m_pSet);
    m_pSet = nullptr;
  }

  if (m_pLayout) {
    pRhi->FreeDescriptorSetLayout(m_pLayout);
    m_pLayout = nullptr;
  }
}


void HDR::UpdateToGPU(VulkanRHI* pRhi)
{
  R_ASSERT(m_pBuffer->Mapped(), "HDR Realtime settings are not mapped.");
  memcpy(m_pBuffer->Mapped(), &m_config, sizeof(ConfigHDR));

  VkMappedMemoryRange memRange = { };
  memRange.memory = m_pBuffer->Memory();
  memRange.offset = 0;
  memRange.size = VK_WHOLE_SIZE;
  memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &memRange);
}
} // Recluse