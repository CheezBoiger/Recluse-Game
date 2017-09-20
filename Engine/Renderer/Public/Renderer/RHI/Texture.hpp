// Copyright (c) 2017 Recluse Project.
#pragma once


#include "VulkanConfigs.hpp"
#include "Core/Types.hpp"


namespace Recluse {


class Sampler : public VulkanHandle {
public:
  Sampler()
    : mSampler(VK_NULL_HANDLE) { }


  void        Initialize(VkSamplerCreateInfo& info);
  void        CleanUp();

  VkSampler   Handle() { return mSampler; }
private:
  VkSampler   mSampler;
};


class Texture : public VulkanHandle {
public:
  Texture()
    : mView(VK_NULL_HANDLE)
    , mImage(VK_NULL_HANDLE)
    , mMemory(VK_NULL_HANDLE) { }

  
  void            Initialize(const VkImageCreateInfo& image, const VkImageViewCreateInfo& view,
                    const VkMemoryAllocateInfo& memory);
  void            CleanUp();

  VkImageView     View() { return mView; }
  VkImage         Image() { return mImage; }
  VkDeviceMemory  Memory() { return mMemory; }

private:
  VkImageView     mView;
  VkImage         mImage;
  VkDeviceMemory  mMemory;
};
} // Recluse