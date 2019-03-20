// Copyright (c) 2019 Recluse Project. All rights reserved.
#include "Allocator.hpp"
#include "../VulkanConfigs.hpp"

#include "Core/Exception.hpp"

#define R_MEM_ALIGN(p, alignment) (( p ) + (( alignment ) - 1)) & (~(( alignment ) - 1))


namespace Recluse {


VulkanMemoryPool::VulkanMemoryPool()
  : m_nextBlockId(0)
  , m_memTypeIndex(0)
  , m_hostVisible(false)
  , m_rawMem(VK_NULL_HANDLE)
  , m_memSz(0)
  , m_memAllocated(0)
  , m_pHead(0)
  , m_pRawDat(nullptr)
{
}

VulkanMemoryPool::~VulkanMemoryPool()
{
  R_ASSERT(m_memAllocated == 0, "Vulkan Memory Pool leak prior to destruction!");
}


void VulkanMemoryPool::init(VkDevice device,
                            const VkPhysicalDeviceMemoryProperties* pMemoryProperties,
                            u32 id,
                            u32 memoryTypeBits,
                            const VkDeviceSize sz)
{
  m_id = id;
  m_hostVisible = ((memoryTypeBits & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? true : false);
  m_memSz = sz;

  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = sz;
  allocInfo.memoryTypeIndex = findMemoryProperties(pMemoryProperties, memoryTypeBits);

  vkAllocateMemory(device, &allocInfo, nullptr, &m_rawMem);
  if (m_hostVisible) {
    vkMapMemory(device, m_rawMem, 0, sz, 0, (void**)&m_pRawDat);
  }

  m_pHead = new VulkanMemBlockNode();
  m_pHead->_pPrev =  nullptr;
  m_pHead->_free = true;
  m_pHead->_offset = 0;
  m_pHead->_pNext = nullptr;
  m_pHead->_sz = m_memSz;
  m_pHead->_id = m_nextBlockId++;
}


void VulkanMemoryPool::cleanUp(VkDevice device)
{
  {
    VulkanMemBlockNode* pCurrent = m_pHead;
    VulkanMemBlockNode* pPrev = nullptr;
    while (pCurrent) {
      pPrev = pCurrent;
      pCurrent = pCurrent->_pNext;
      delete pPrev;
    }
  }

  if (m_rawMem) {
    if (m_hostVisible) {
      vkUnmapMemory(device, m_rawMem);
    }
    vkFreeMemory(device, m_rawMem, nullptr);
    m_rawMem = VK_NULL_HANDLE;
  } 
  
  m_memSz = 0;
}


b32 VulkanMemoryPool::allocate(u32 sz, u32 align, VulkanAllocation* pOutput)
{
  R_ASSERT(((align & (align - 1)) == 0), "align is not a power of 2!");

  VkDeviceSize freeSz = m_memSz - m_memAllocated;
  if (freeSz < sz) {
    R_DEBUG(rWarning, "OUT OF MEMORY.\n");
    return false;
  }

  VulkanMemBlockNode* node = new VulkanMemBlockNode();
  node->_free = true;
  node->_id = m_nextBlockId++;
  node->_offset = 0;

  VulkanMemBlockNode* pCurrent = m_pHead;
  while (pCurrent) {

  }
  return 0;
}

void VulkanMemoryPool::free(VulkanAllocation* pIn)
{
  
}


u32 VulkanMemoryAllocatorManager::numberOfAllocations = 0;


VulkanMemoryAllocatorManager::VulkanMemoryAllocatorManager()
  : m_nextPoolId(0)
  , m_garbageIndex(0)
  , m_deviceLocalMemoryMB(0)
  , m_hostVisibleMemoryMB(0)
{
}


VulkanMemoryAllocatorManager::~VulkanMemoryAllocatorManager()
{
}


void VulkanMemoryAllocatorManager::init(VkDevice device,
                                        const VkPhysicalDeviceMemoryProperties* pMemProperties)
{
}


void VulkanMemoryAllocatorManager::cleanUp(VkDevice device)
{

}
} // Recluse