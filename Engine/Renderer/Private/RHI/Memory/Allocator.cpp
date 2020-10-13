// Copyright (c) 2019 Recluse Project. All rights reserved.
#include "Allocator.hpp"
#include "../VulkanConfigs.hpp"
#include "../VulkanRHI.hpp"

#include "Core/Utility/Profile.hpp"
#include "Core/Exception.hpp"

#define R_MEM_ALIGN(p, alignment) (( p ) + (( alignment ) - 1)) & (~(( alignment ) - 1))
#define R_MEM_1_KB (1024)
#define R_MEM_1_MB (1024 * 1024) 
#define R_MEM_1_GB (1024 * 1024 * 1024)

namespace Recluse {


B32 isMemoryResourcesOnSeparatePages(VkDeviceSize rOffsetA, 
                                     VkDeviceSize rSizeA,
                                     VkDeviceSize rOffsetB,
                                     VkDeviceSize bufferImageGranularity)
{
  VkDeviceSize rResEndA = rOffsetA + rSizeA - 1;
  VkDeviceSize rResEndPageA = rResEndA & ~(bufferImageGranularity - 1);
  VkDeviceSize rResStartB = rOffsetB;
  VkDeviceSize rResStartPageB = rResStartB & ~(bufferImageGranularity - 1);
  return rResEndA < rResStartPageB;
}


B32 hasGranularityConflict(VulkanAllocationType type1, VulkanAllocationType type2)
{
  if (type1 > type2) {
    VulkanAllocationType t = type1;
    type1 = type2;
    type2 = t;
  }

  switch (type1) {
    case VULKAN_ALLOCATION_TYPE_FREE:
      return false;
    case VULKAN_ALLOCATION_TYPE_BUFFER:
      return type2 == VULKAN_ALLOCATION_TYPE_IMAGE ||
             type2 == VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL;
    case VULKAN_ALLOCATION_TYPE_IMAGE:
      return type2 == VULKAN_ALLOCATION_TYPE_IMAGE ||
             type2 == VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR ||
             type2 == VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL;
    case VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR:
      return type2 == VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL;
    case VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL:
      return false;
    default:
      R_ASSERT(false, "Vulkan Allocation type must have a proper value assigned!");
      return true;
  }
}


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


B32 VulkanMemoryPool::init(VkDevice device,
                           U32 id,
                           U32 memoryTypeIndex,
                           PhysicalDeviceMemoryUsage usage,
                           const VkDeviceSize sz)
{
  m_id = id;
  m_memSz = sz;
  m_usage = usage;
  m_memTypeIndex = memoryTypeIndex;

  VkMemoryAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = sz;
  allocInfo.memoryTypeIndex = memoryTypeIndex;

  VkResult Result = vkAllocateMemory(device, &allocInfo, nullptr, &m_rawMem);
  if (Result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create memory pool for given memory type!");
  }
  if (isHostVisible()) {
    vkMapMemory(device, m_rawMem, 0, sz, 0, (void**)&m_pRawDat);
  }

  m_pHead = new VulkanMemBlockNode();
  m_pHead->_pPrev =  nullptr;
  m_pHead->_free = true;
  m_pHead->_offset = 0;
  m_pHead->_pNext = nullptr;
  m_pHead->_sz = 0;
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


B32 VulkanMemoryPool::isHostVisible() const
{
  return (m_usage != PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY);
}


B32 VulkanMemoryPool::allocate(U32 sz, 
                               U32 align, 
                               VulkanAllocationType allocType, 
                               VkDeviceSize granularity,
                               VulkanAllocation* pOutput)
{
  R_ASSERT(((align & (align - 1)) == 0), "align is not a power of 2!");

  VkDeviceSize freeSz = m_memSz - m_memAllocated;
  if (freeSz < sz) {
    return false;
  }

  VulkanMemBlockNode* pCurrent = m_pHead;
  VkDeviceSize offset = 0;
  VkDeviceSize padding = 0;
  VkDeviceSize alignedSize = 0;
  VkDeviceSize offsetting = 0;

  VulkanMemBlockNode* pPrev = nullptr;
  for (pCurrent = m_pHead; 
       pCurrent != nullptr; 
       pPrev = pCurrent, pCurrent = pCurrent->_pNext) 
  {
    if ((pCurrent->_type == VULKAN_ALLOCATION_TYPE_FREE) &&
        (pCurrent->_sz >= sz))
      break;

    VkDeviceSize offsetTemp = pCurrent->_sz == 0 ? 0 : 
                                                   R_MEM_ALIGN(pCurrent->_sz, align);

    if (pPrev && 
        granularity > 1 && 
        !isMemoryResourcesOnSeparatePages(pPrev->_offset, 
                                          pPrev->_sz, 
                                          offsetTemp, 
                                          granularity)) 
    {
      if (hasGranularityConflict(pPrev->_type, allocType)) 
      {
        offsetTemp = R_MEM_ALIGN(offsetTemp, granularity);
      }
    }

    //padding = offset - pCurrent->_offset;
    //alignedSize = padding + sz;

    offset += offsetTemp;
#if 0
    if (alignedSize > pCurrent->_sz) {
      continue;
    }  
    if (alignedSize + m_memAllocated >= m_memSz) {
      continue;
    }
#endif
  }

  VulkanMemBlockNode* node = nullptr; 

    // Check if the chosen offset and sz is out of bounds.
    if ((offset + R_MEM_ALIGN(sz, align)) >= m_memSz) {
        return false;
    }

  if (pCurrent && pCurrent->_type == VULKAN_ALLOCATION_TYPE_FREE) {
    // free block, we can use!
    //R_DEBUG(Verbosity::rNotify, "Freed block!\n");
    if (pCurrent->_sz >= sz) {
      VkDeviceSize szDiff = pCurrent->_sz - sz;
      if (szDiff == 0) {
        node = pCurrent;
      } else {
        node = new VulkanMemBlockNode();
        pCurrent->_offset = offset + R_MEM_ALIGN(sz, align);
        pCurrent->_sz = szDiff;
        node->_pNext = pCurrent;
        pCurrent->_pPrev = node;
      }
    }
  } else {
    node = new VulkanMemBlockNode();
  }

  node->_free = false;
  node->_id = m_nextBlockId++;
  node->_sz = sz;
  node->_type = allocType;
  node->_offset = offset;

  pPrev->_pNext = node;
  node->_pPrev = pPrev;

  pOutput->_blockId = node->_id;
  pOutput->_poolId = m_id;
  pOutput->_memId = m_memTypeIndex; 
  pOutput->_deviceMemory = m_rawMem;
  pOutput->_offset = offset;
  pOutput->_pData = reinterpret_cast<B8*>((uintptr_t)m_pRawDat + offset);
  pOutput->_poolId = m_id;
  pOutput->_sz = sz;
  m_memAllocated += sz;
  return true;
}

void VulkanMemoryPool::free(VulkanAllocation* pIn)
{
  VulkanMemBlockNode* pPrev = nullptr;
  for (VulkanMemBlockNode* pNode = m_pHead;
       pNode != nullptr;
       pNode = pNode->_pNext,
       pPrev = pNode->_pPrev) 
  {
    // Block id matches, need to mark as free.
    if (pNode->_id == pIn->_blockId) 
    {
      pNode->_type = VULKAN_ALLOCATION_TYPE_FREE;
      m_memAllocated -= pNode->_sz;
      break;
    }
  }
}


U32 VulkanMemoryAllocatorManager::numberOfAllocations = 0;


VulkanMemoryAllocatorManager::VulkanMemoryAllocatorManager()
  : m_nextPoolId(0)
  , m_garbageIndex(0)
  , m_bufferCount(0)
  , m_deviceLocalMemoryBytes(0)
  , m_hostVisibleMemoryBytes(0)
  , m_bufferImageGranularity(0)
{
}


VulkanMemoryAllocatorManager::~VulkanMemoryAllocatorManager()
{
}


void VulkanMemoryAllocatorManager::init(VulkanRHI* pRhi,
                                        U32 resourceIndex,
                                        U32 resourceCount)
{
  m_deviceLocalMemoryBytes = 1 * R_MEM_1_GB;
  m_hostVisibleMemoryBytes = 140 * R_MEM_1_MB;
  m_bufferImageGranularity = pRhi->PhysicalDeviceLimits().bufferImageGranularity;

  for (U32 i = 0; i < pRhi->getMemoryProperties().memoryHeapCount; ++i) {
    VkMemoryHeap heap = pRhi->getMemoryProperties().memoryHeaps[i];
    m_maxDeviceLocalMemBytes = heap.size;
    break;
  }

  update(pRhi, resourceIndex, resourceCount);
}


void VulkanMemoryAllocatorManager::update(VulkanRHI* pRhi, U32 currentResourceIndex, U32 resourceCount)
{
  m_bufferCount = resourceCount;
  m_garbageIndex = currentResourceIndex;

  emptyGarbage( pRhi );

  if (m_frameGarbage.size() != m_bufferCount)
    m_frameGarbage.resize( m_bufferCount );
}


B32 VulkanMemoryAllocatorManager::allocate(VkDevice device,
                                           U32 sz,
                                           U32 align,
                                           U32 memoryTypeBits,
                                           PhysicalDeviceMemoryUsage usage,
                                           VulkanAllocationType allocType,
                                           VulkanAllocation* out,
                                           U32 memoryBankIdx)
{
  R_TIMED_PROFILE_RENDERER();

  U32 memoryIndex = VulkanRHI::gPhysicalDevice.findMemoryType(memoryTypeBits, usage);
  R_ASSERT(memoryIndex != 0xffffffff, "Unable to find memory index for allocation request.");

  auto& poolGroup = m_pools[memoryIndex];
  for (U32 i = 0; i < poolGroup.size(); ++i) {
    VulkanMemoryPool* pool = poolGroup[i];
    if (pool->getMemoryTypeIndex() != memoryIndex) {
      continue;
    }
    if (pool->allocate(sz, align, allocType, m_bufferImageGranularity, out)) {
      numberOfAllocations++;
      return true;      
    } else {
        // Possible out of memory?
        emptyGarbage(nullptr);
        if (pool->allocate(sz, align, allocType, m_bufferImageGranularity, out)) {
            numberOfAllocations++;
            return true;
        }
        R_DEBUG(rWarning, "OUT OF MEMORY, attempting to create new heap.\n")
    }
  }
  // If no pool exists for the wanted memory type, create one!
  VkDeviceSize poolSz = ((usage == PHYSICAL_DEVICE_MEMORY_USAGE_GPU_ONLY) ? 
                          m_deviceLocalMemoryBytes : m_hostVisibleMemoryBytes);
  VulkanMemoryPool* newPool = new VulkanMemoryPool();
  VkPhysicalDeviceMemoryProperties memProps = VulkanRHI::gPhysicalDevice.getMemoryProperties();
  if (newPool->init(device, 
                    poolGroup.size(),  
                    memoryIndex, 
                    usage, 
                    poolSz)) {
    poolGroup.push_back(newPool);
  }

  B32 result = newPool->allocate(sz, 
                                 align, 
                                 allocType, 
                                 m_bufferImageGranularity,
                                 out); 
  if (result) 
    numberOfAllocations++;

  return result;
}


void VulkanMemoryAllocatorManager::free(const VulkanAllocation& alloc)
{
  numberOfAllocations--;
  m_frameGarbage[m_garbageIndex].push_back(alloc);
}


void VulkanMemoryAllocatorManager::emptyGarbage(VulkanRHI* pRhi)
{
  // TODO(): Need to fix this, take buffering count as a parameter instead!!
  if ( m_bufferCount == 0 || 
      ( m_garbageIndex >= m_frameGarbage.size() ) ) return;

  auto& garbage = m_frameGarbage[m_garbageIndex];
  for (U32 i = 0; i < garbage.size(); ++i) {
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
  for (U32 i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
    auto& pool = m_pools[i];
    for (I32 j = 0; j < pool.size(); ++j) {
      pool[j]->cleanUp(pRhi->logicDevice()->getNative());
      delete pool[j];
    }
    pool.clear();
  }
}
} // Recluse