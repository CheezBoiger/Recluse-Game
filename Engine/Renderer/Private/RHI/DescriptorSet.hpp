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
  void                      initialize(const VkDescriptorSetLayoutCreateInfo& info);
  void                      cleanUp();

  VkDescriptorSetLayout     getLayout() const { return mLayout; }
private:
  VkDescriptorSetLayout     mLayout;
};


class DescriptorSet : public VulkanHandle {
public:
  DescriptorSet()
    : mDescriptorSet(VK_NULL_HANDLE)
    , mPoolOwner(VK_NULL_HANDLE) { }

  ~DescriptorSet();
  void                  allocate(const VkDescriptorPool& pool, const DescriptorSetLayout* layout);
  void                  free();

  // Updates the descriptor set with the given info. This also automatically sets the
  // dstSet parameter with each decriptor set write info.
  void                  update(U32 count, VkWriteDescriptorSet* writeDescriptorSets);
 
  VkDescriptorSet       getHandle() const { return mDescriptorSet; }
  VkDescriptorPool      getPoolOwner() const { return mPoolOwner; }

private:
  VkDescriptorSet       mDescriptorSet;
  VkDescriptorPool      mPoolOwner;
};
} // Recluse