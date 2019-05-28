// Copyright (c) 2018 Recluse Project. All rights reserved.

#include "LightBVH.hpp"

#include "RHI/Buffer.hpp"
#include "RHI/Commandbuffer.hpp"


namespace Recluse {


LightHierarchy::LightHierarchy()
  : m_gpuBvh(nullptr)
  , m_gpuClusterGrid(nullptr)
  , m_gpuLightIndexList(nullptr)
  , m_descSet(nullptr)
  , m_pLightAssignment(nullptr)
{

}


void LightHierarchy::initialize(VulkanRHI* pRhi, u32 width, u32 height)
{
  m_gpuBvh = pRhi->createBuffer();
  m_gpuClusterGrid = pRhi->createBuffer();
  m_gpuLightIndexList = pRhi->createBuffer();

  {
    VkBufferCreateInfo bCI = {};
    bCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bCI.size = VkDeviceSize(sizeof(LightBVH));
    bCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_gpuBvh->initialize(bCI, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
  }
  {
    VkBufferCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = VkDeviceSize(0);
    ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_gpuClusterGrid->initialize(ci, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
  }
  {
    VkBufferCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = VkDeviceSize(0);
    ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_gpuLightIndexList->initialize(ci, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
  }
}


void LightHierarchy::cleanUp(VulkanRHI* pRhi)
{
  if (m_gpuBvh) {
    pRhi->freeBuffer(m_gpuBvh);
    m_gpuBvh = nullptr;
  }

  if (m_gpuClusterGrid) {
    pRhi->freeBuffer(m_gpuClusterGrid);
    m_gpuClusterGrid = nullptr;
  }

  if (m_gpuLightIndexList) {
    pRhi->freeBuffer(m_gpuLightIndexList);
    m_gpuLightIndexList = nullptr;
  }
}


void LightHierarchy::build(CommandBuffer* pCmdBuffer)
{
  
}
} // Recluse