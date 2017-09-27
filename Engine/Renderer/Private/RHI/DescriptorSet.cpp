// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/DescriptorSet.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void DescriptorSetLayout::Initialize(const VkDescriptorSetLayoutCreateInfo& info)
{
  if (vkCreateDescriptorSetLayout(mOwner, &info, nullptr, &mLayout) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create descriptor layout! Aborting descriptor set allocation.\n");
    return;
  }
}


void DescriptorSetLayout::CleanUp()
{
  if (mLayout) {
    vkDestroyDescriptorSetLayout(mOwner, mLayout, nullptr);
    mLayout = VK_NULL_HANDLE;
  }
}


void DescriptorSet::Allocate(const VkDescriptorPool& pool, const DescriptorSetLayout& layout)
{
  mPoolOwner = pool;
  VkDescriptorSetLayout layoutRef = layout.Layout();
  VkDescriptorSetAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.pNext = nullptr;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layoutRef;

  if (vkAllocateDescriptorSets(mOwner, &allocInfo, &mDescriptorSet) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to allocate descriptor set!\n");
  }
}


void DescriptorSet::Free()
{
  if (mDescriptorSet) {
    vkFreeDescriptorSets(mOwner, mPoolOwner, 1, &mDescriptorSet);
    mDescriptorSet = VK_NULL_HANDLE;
  }
}
} // Recluse