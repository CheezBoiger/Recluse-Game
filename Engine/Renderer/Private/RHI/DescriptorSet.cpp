// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "DescriptorSet.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"


namespace Recluse {


void DescriptorSetLayout::initialize(const VkDescriptorSetLayoutCreateInfo& info)
{
  if (vkCreateDescriptorSetLayout(mOwner, &info, nullptr, &mLayout) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create descriptor layout! Aborting descriptor set allocation.\n");
    return;
  }
}


DescriptorSetLayout::~DescriptorSetLayout()
{
  if (mLayout) {
    R_DEBUG(rWarning, "Descriptor set layout was not cleaned up prior to deletion!\n");
  }
}


DescriptorSet::~DescriptorSet()
{
  if (mDescriptorSet) {
    R_DEBUG(rWarning, "Descriptor set was not cleaned up prior to deletion!\n");
  }
}


void DescriptorSetLayout::cleanUp()
{
  if (mLayout) {
    vkDestroyDescriptorSetLayout(mOwner, mLayout, nullptr);
    mLayout = VK_NULL_HANDLE;
  }
}


void DescriptorSet::allocate(const VkDescriptorPool& pool, const DescriptorSetLayout* layout)
{
  mPoolOwner = pool;
  VkDescriptorSetLayout layoutRef = layout->getLayout();
  VkDescriptorSetAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.pNext = nullptr;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layoutRef;

  VkResult result = vkAllocateDescriptorSets(mOwner, &allocInfo, &mDescriptorSet);
  if (result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to allocate descriptor set!\n");
    switch (result) {
      case VK_ERROR_FRAGMENTED_POOL:
        Log(rError) << "Descriptor pool is fragmented! Need to create your own new descriptor pool!\n"; break;
      case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        Log(rError) << "Can not allocate descriptor set. GPU out of memory!\n"; break;
      case VK_ERROR_OUT_OF_HOST_MEMORY:
        Log(rError) << "Can not allocate descriptor set. CPU is out of memory!\n"; break;
      default:
        Log(rError) << "Unknown descriptor set allocation failure!\n"; break;
    }
  }
}


void DescriptorSet::free()
{
  if (mDescriptorSet) {
    vkFreeDescriptorSets(mOwner, mPoolOwner, 1, &mDescriptorSet);
    mDescriptorSet = VK_NULL_HANDLE;
  }
}


void DescriptorSet::update(u32 count, VkWriteDescriptorSet* writeDescriptorSets)
{
  for (u32 i = 0; i < count; ++i) {
    writeDescriptorSets[i].dstSet = getHandle();
  }
  vkUpdateDescriptorSets(mOwner, count, writeDescriptorSets, 0, nullptr);
}
} // Recluse