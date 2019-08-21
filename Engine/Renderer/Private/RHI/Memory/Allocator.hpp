// Copyright (c) 2019 Recluse Project. All rights reserved.
// Based on  VkDoom3 Vulkan Allocation manager by Dustin H. Land.
// Thank you very much!
#pragma once

#include "../VulkanConfigs.hpp"

#include <vector>
#include <array>


namespace Recluse {


class VulkanRHI;


enum VulkanAllocationType {
  VULKAN_ALLOCATION_TYPE_FREE,
  VULKAN_ALLOCATION_TYPE_BUFFER,
  VULKAN_ALLOCATION_TYPE_IMAGE,
  VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR,
  VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL,
};


struct VulkanMemBlockNode {
  VkDeviceSize _sz;
  VkDeviceSize _offset;
  VulkanAllocationType _type;
  VulkanMemBlockNode* _pPrev;
  VulkanMemBlockNode* _pNext;
  U64 _id       : 32,
      _free     : 32;
};


struct VulkanAllocation {
  U64 _memId         : 16,
      _poolId         : 16,
      _blockId        : 32;
  VkDeviceMemory _deviceMemory;
  VkDeviceSize _offset;
  VkDeviceSize _sz;
  B8* _pData;
};


class VulkanMemoryPool {
public:
  VulkanMemoryPool();

  ~VulkanMemoryPool();

  B32 init(VkDevice device,
           U32 id,
           U32 memoryTypeBits, 
           PhysicalDeviceMemoryUsage usage,
           const VkDeviceSize sz);

  void cleanUp(VkDevice device);
  U32 getMemoryTypeIndex() const { return m_memTypeIndex; }

  B32 allocate(U32 sz, 
               U32 align, 
               VulkanAllocationType allocType, 
               VkDeviceSize granularity,
               VulkanAllocation* pOutput);
  void free(VulkanAllocation* pIn);
  B32 isHostVisible() const;
  VkDeviceSize getNumAllocated() const { return m_memAllocated; }

private:
  U32 m_id;
  U32 m_nextBlockId;
  U32 m_memTypeIndex;
  PhysicalDeviceMemoryUsage m_usage;
  VkDeviceMemory m_rawMem;
  VkDeviceSize m_memSz;
  VkDeviceSize m_memAllocated;
  VulkanMemBlockNode* m_pHead;
  B8* m_pRawDat;
};


// Memory Allocator.
class VulkanMemoryAllocatorManager {
public:

  static U32 numberOfAllocations;
  VulkanMemoryAllocatorManager();
  ~VulkanMemoryAllocatorManager();


  void init(VulkanRHI* pRhi, 
            const VkPhysicalDeviceProperties* props, 
            const VkPhysicalDeviceMemoryProperties* memProperties);
  void cleanUp(VulkanRHI* pRhi);
  void update(VulkanRHI* pRhi);

  VulkanAllocation allocate(VkDevice device,
                            U32 sz, 
                            U32 align, 
                            U32 memoryTypeBits,
                            PhysicalDeviceMemoryUsage usage,
                            VulkanAllocationType allocType,
                            U32 memoryBankIdx = 0);
  void free(const VulkanAllocation& alloc);

  void emptyGarbage(VulkanRHI* pRhi);

  VkDeviceSize getMaxDeviceLocalMemBytes() { return m_maxDeviceLocalMemBytes; }
  VkDeviceSize getDeviceLocalMemBytes() const { return m_deviceLocalMemoryBytes; }
private:
  U32 m_nextPoolId;
  U32 m_garbageIndex;
  U32 m_bufferCount;
  VkDeviceSize m_deviceLocalMemoryBytes;
  VkDeviceSize m_hostVisibleMemoryBytes;
  VkDeviceSize m_bufferImageGranularity;
  VkDeviceSize m_maxDeviceLocalMemBytes;

  std::array<std::vector<VulkanMemoryPool*>, VK_MAX_MEMORY_TYPES> m_pools;
  std::vector<std::vector<VulkanAllocation>> m_frameGarbage;
};
} // Recluse