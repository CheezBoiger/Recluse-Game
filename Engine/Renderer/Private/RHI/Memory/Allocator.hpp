// Copyright (c) 2019 Recluse Project. All rights reserved.
#pragma once

#include "../VulkanConfigs.hpp"

#include <vector>


namespace Recluse {


struct VulkanMemBlockNode {
  VkDeviceSize _sz;
  VkDeviceSize _offset;
  VulkanMemBlockNode* _pPrev;
  VulkanMemBlockNode* _pNext;
  u64 _id       : 32,
      _free     : 32;
};


struct VulkanAllocation {
  u64 _poolId         : 32,
      _blockId        : 32;
  VkDeviceMemory _deviceMemory;
  VkDeviceSize _offset;
  VkDeviceSize _sz;
  b8* _pData;
};


class VulkanMemoryPool {
public:
  VulkanMemoryPool();

  ~VulkanMemoryPool();

  void init(VkDevice device,
            const VkPhysicalDeviceMemoryProperties* pMemoryProperties, 
            u32 id, 
            u32 memoryTypeBits, 
            const VkDeviceSize sz);

  void cleanUp(VkDevice device);

  b32 allocate(u32 sz, u32 align, VulkanAllocation* pOutput);
  void free(VulkanAllocation* pIn);

private:
  u32 m_id;
  u32 m_nextBlockId;
  u32 m_memTypeIndex;
  b32 m_hostVisible;
  VkDeviceMemory m_rawMem;
  VkDeviceSize m_memSz;
  VkDeviceSize m_memAllocated;
  VulkanMemBlockNode* m_pHead;
  b8* m_pRawDat;
};


class VulkanMemoryAllocatorManager {
public:
  static u32 numberOfAllocations;
  VulkanMemoryAllocatorManager();
  ~VulkanMemoryAllocatorManager();


  void init(VkDevice device, const VkPhysicalDeviceMemoryProperties* memProperties);
  void cleanUp(VkDevice device);

  VulkanAllocation allocate(u32 sz, u32 align, u32 memoryTypeBits);
  void free(const VulkanAllocation& alloc);

  void emptyGarbage();

private:
  u32 m_nextPoolId;
  u32 m_garbageIndex;
  u32 m_deviceLocalMemoryMB;
  u32 m_hostVisibleMemoryMB;

  std::vector<VulkanMemoryPool*>  m_pools;
  std::vector<std::vector<VulkanAllocation>> m_frameGarbage;
};
} // Recluse