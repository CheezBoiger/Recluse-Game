// Copyright (c) 2019 Recluse Project. All rights reserved.
#include "Allocator.hpp"
#include "../VulkanConfigs.hpp"
#include "../VulkanRHI.hpp"

#include "Core/Exception.hpp"

#define R_MEM_ALIGN(p, alignment) (( p ) + (( alignment ) - 1) + sizeof(size_t)) & (~(( alignment ) - 1))
#define R_MEM_1_KB (1024)
#define R_MEM_1_MB (1024 * 1024) 

namespace Recluse {


VulkanMemoryPool::VulkanMemoryPool()
  : m_nextBlockId(0)
  , m_memTypeIndex(0)
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


b32 VulkanMemoryPool::init(VkDevice device,
                           u32 id,
                           u32 memoryTypeIndex,
                           PhysicalDeviceMemoryUsage usage,
                           const VkDeviceSize sz)
{
  m_id = id;
  m_memSz = sz;
  m_usage = usage;

  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = sz;
  allocInfo.memoryTypeIndex = memoryTypeIndex;

  vkAllocateMemory(device, &allocInfo, nullptr, &m_rawMem);
  if (isHostVisible()) {
    vkMapMemory(device, m_rawMem, 0, sz, 0, (void**)&m_pRawDat);
  }

  m_pHead = new VulkanMemBlockNode();
  m_pHead->_pPrev =  nullptr;
  m_pHead->_free = true;
  m_pHead->_offset = 0;
  m_pHead->_pNext = nullptr;
  m_pHead->_sz = m_memSz;
  m_pHead->_id = m_nextBlockId++;

  return true;
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
    if (isHostVisible()) {
      vkUnmapMemory(device, m_rawMem);
    }
    vkFreeMemory(device, m_rawMem, nullptr);
    m_rawMem = VK_NULL_HANDLE;
  } 
  
  m_memSz = 0;
}


b32 VulkanMemoryPool::isHostVisible() const
{
  return (m_usage != PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY);
}


b32 VulkanMemoryPool::allocate(u32 sz, 
                               u32 align, 
                               VulkanAllocationType allocType, 
                               VulkanAllocation* pOutput)
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
  node->_sz = sz;
  node->_pNext = nullptr;
  node->_pPrev  = nullptr;

  VulkanMemBlockNode* pCurrent = m_pHead;
  u32 offset = 0;
  VkDeviceSize pt = reinterpret_cast<VkDeviceSize>(m_pRawDat);
  while (pCurrent->_pNext) {
    offset = R_MEM_ALIGN(pCurrent->_offset, align);
    pCurrent = pCurrent->_pNext;
    pt += pCurrent->_sz;
  }
  pCurrent->_pNext = node;
  node->_pPrev = pCurrent;

  pOutput->_blockId = node->_id;
  pOutput->_poolId = m_id;
  pOutput->_memId = m_memTypeIndex; 
  pOutput->_deviceMemory = m_rawMem;
  pOutput->_offset = node->_offset;
  pOutput->_pData = nullptr;
  pOutput->_poolId = m_id;
  pOutput->_sz = sz;

  return true;
}

void VulkanMemoryPool::free(VulkanAllocation* pIn)
{
  
}


u32 VulkanMemoryAllocatorManager::numberOfAllocations = 0;


VulkanMemoryAllocatorManager::VulkanMemoryAllocatorManager()
  : m_nextPoolId(0)
  , m_garbageIndex(0)
  , m_bufferCount(0)
  , m_deviceLocalMemoryMB(0)
  , m_hostVisibleMemoryMB(0)
  , m_bufferImageGranularity(0)
{
}


VulkanMemoryAllocatorManager::~VulkanMemoryAllocatorManager()
{
}


void VulkanMemoryAllocatorManager::init(VulkanRHI* pRhi,
                                        const VkPhysicalDeviceProperties* props,
                                        const VkPhysicalDeviceMemoryProperties* pMemProperties)
{
  m_deviceLocalMemoryMB = 128 * R_MEM_1_MB;
  m_deviceLocalMemoryMB = 64 * R_MEM_1_MB; 
  m_bufferImageGranularity = props->limits.bufferImageGranularity;
  update(pRhi);
}


void VulkanMemoryAllocatorManager::update(VulkanRHI* pRhi)
{
  emptyGarbage(pRhi);
  m_bufferCount = pRhi->bufferingCount();
  m_frameGarbage.resize(m_bufferCount);
}


VulkanAllocation VulkanMemoryAllocatorManager::allocate(VkDevice device,
                                                        u32 sz,
                                                        u32 align,
                                                        u32 memoryTypeBits,
                                                        PhysicalDeviceMemoryUsage usage,
                                                        VulkanAllocationType allocType,
                                                        u32 memoryBankIdx)
{
  VulkanAllocation allocation;
  u32 memoryIndex = VulkanRHI::gPhysicalDevice.findMemoryType(memoryTypeBits, usage);
  R_ASSERT(memoryIndex != 0xffffffff, "Unable to find memory index for allocation request.");
  auto& poolGroup = m_pools[memoryIndex];
  for (u32 i = 0; i < poolGroup.size(); ++i) {
    VulkanMemoryPool* pool = poolGroup[i];
    if (pool->getMemoryTypeIndex() != memoryIndex) {
      continue;
    }
    if (pool->allocate(sz, align, allocType, &allocation)) {
      return allocation;      
    }
  }
  // If no pool exists for the wanted memory type, create one!
  VkDeviceSize poolSz = ((usage == PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY) ? m_deviceLocalMemoryMB : m_hostVisibleMemoryMB);
  VulkanMemoryPool* newPool = new VulkanMemoryPool();
  VkPhysicalDeviceMemoryProperties memProps = VulkanRHI::gPhysicalDevice.getMemoryProperties();
  if (newPool->init(device, (poolGroup.size() + 1),  memoryIndex, usage, sz)) {
    poolGroup.push_back(newPool);
  }
  newPool->allocate(sz, align, allocType, &allocation);
  return allocation;
}


void VulkanMemoryAllocatorManager::free(const VulkanAllocation& alloc)
{
  m_frameGarbage[m_garbageIndex].push_back(alloc);
}


void VulkanMemoryAllocatorManager::emptyGarbage(VulkanRHI* pRhi)
{
  // TODO(): Need to fix this, take buffering count as a parameter instead!!
  if (m_bufferCount == 0) return;
  m_garbageIndex = (m_garbageIndex + 1) % m_bufferCount;
  auto& garbage = m_frameGarbage[m_garbageIndex];
  for (u32 i = 0; i < garbage.size(); ++i) {
    VulkanAllocation& alloc = garbage[i];
    VulkanMemoryPool* pool = m_pools[alloc._memId][alloc._poolId];
    pool->free(&alloc);
    if (pool->getNumAllocated() == 0) {
      // TODO(): 
    }
  }
  garbage.clear();
}


void VulkanMemoryAllocatorManager::cleanUp(VulkanRHI* pRhi)
{
  emptyGarbage(pRhi);
  for (u32 i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
    auto& pool = m_pools[i];
    for (i32 j = 0; j < pool.size(); ++j) {
      pool[j]->cleanUp(pRhi->logicDevice()->getNative());
      delete pool[j];
    }
    pool.clear();
  }
}
} // Recluse