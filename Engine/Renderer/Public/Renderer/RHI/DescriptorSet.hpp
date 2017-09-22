// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"


namespace Recluse {


class DescriptorSet : public VulkanHandle {
public:
  DescriptorSet()
    : mDescriptorSet(VK_NULL_HANDLE)
    , mLayout(VK_NULL_HANDLE)
    , mPoolOwner(VK_NULL_HANDLE) { }


  void                  Allocate(const VkDescriptorPool& pool, const VkDescriptorSetLayoutCreateInfo& createInfo);
  void                  Free();

  VkDescriptorSet       Handle() const { return mDescriptorSet; }
  VkDescriptorSetLayout Layout() const { return mLayout; }
  VkDescriptorPool      PoolOwner() const { return mPoolOwner; }

private:
  VkDescriptorSet       mDescriptorSet;
  VkDescriptorSetLayout mLayout;
  VkDescriptorPool      mPoolOwner;
};
} // Recluse