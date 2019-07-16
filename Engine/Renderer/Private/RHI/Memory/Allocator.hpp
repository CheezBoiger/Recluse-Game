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
  u64 _id       : 32,
      _free     : 32;
};


struct VulkanAllocation {
  u64 _memId         : 16,
      _poolId         : 16,
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

  b32 init(VkDevice device,
           u32 id,
           u32 memoryTypeBits, 
           PhysicalDeviceMemoryUsage usage,
           const VkDeviceSize sz);

  void cleanUp(VkDevice device);
  u32 getMemoryTypeIndex() const { return m_memTypeIndex; }

  b32 allocate(u32 sz, 
               u32 align, 
               VulkanAllocationType allocType, 
               VkDeviceSize granularity,
               VulkanAllocation* pOutput);
  void free(VulkanAllocation* pIn);
  b32 isHostVisible() const;
  VkDeviceSize getNumAllocated() const { return m_memAllocated; }

private:
  u32 m_id;
  u32 m_nextBlockId;
  u32 m_memTypeIndex;
  PhysicalDeviceMemoryUsage m_usage;
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


  void init(VulkanRHI* pRhi, 
            const VkPhysicalDeviceProperties* props, 
            const VkPhysicalDeviceMemoryProperties* memProperties);
  void cleanUp(VulkanRHI* pRhi);
  void update(VulkanRHI* pRhi);

  VulkanAllocation allocate(VkDevice device,
                            u32 sz, 
                            u32 align, 
                            u32 memoryTypeBits,
                            PhysicalDeviceMemoryUsage usage,
                            VulkanAllocationType allocType,
                            u32 memoryBankIdx = 0);
  void free(const VulkanAllocation& alloc);

  void emptyGarbage(VulkanRHI* pRhi);

  VkDeviceSize getMaxDeviceLocalMemBytes() { return m_maxDeviceLocalMemBytes; }

private:
  u32 m_nextPoolId;
  u32 m_garbageIndex;
  u32 m_bufferCount;
  VkDeviceSize m_deviceLocalMemoryMB;
  VkDeviceSize m_hostVisibleMemoryMB;
  VkDeviceSize m_bufferImageGranularity;
  VkDeviceSize m_maxDeviceLocalMemBytes;

  std::array<std::vector<VulkanMemoryPool*>, VK_MAX_MEMORY_TYPES> m_pools;
  std::vector<std::vector<VulkanAllocation>> m_frameGarbage;
};
} // Recluse