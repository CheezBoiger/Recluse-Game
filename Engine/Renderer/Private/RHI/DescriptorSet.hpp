// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"


namespace Recluse {


class DescriptorSetLayout : public VulkanHandle {
public:
  DescriptorSetLayout()
    : mLayout(VK_NULL_HANDLE) { }

  ~DescriptorSetLayout();
  void                      Initialize(const VkDescriptorSetLayoutCreateInfo& info);
  void                      CleanUp();

  VkDescriptorSetLayout     Layout() const { return mLayout; }
private:
  VkDescriptorSetLayout     mLayout;
};


class DescriptorSet : public VulkanHandle {
public:
  DescriptorSet()
    : mDescriptorSet(VK_NULL_HANDLE)
    , mPoolOwner(VK_NULL_HANDLE) { }

  ~DescriptorSet();
  void                  Allocate(const VkDescriptorPool& pool, const DescriptorSetLayout* layout);
  void                  Free();

  // Updates the descriptor set with the given info. This also automatically sets the
  // dstSet parameter with each decriptor set write info.
  void                  Update(u32 count, VkWriteDescriptorSet* writeDescriptorSets);
 
  VkDescriptorSet       Handle() const { return mDescriptorSet; }
  VkDescriptorPool      PoolOwner() const { return mPoolOwner; }

private:
  VkDescriptorSet       mDescriptorSet;
  VkDescriptorPool      mPoolOwner;
};
} // Recluse