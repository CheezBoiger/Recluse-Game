// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "VulkanConfigs.hpp"
#include "Core/Types.hpp"
#include "Core/Utility/Image.hpp"


namespace Recluse {


class PhysicalDevice;
class VulkanRHI;
class CommandBuffer;

// Sampler Object, used for textures, cubemaps, and images.
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


// Texture object.
class Texture : public VulkanHandle {
public:
  Texture()
    : mView(VK_NULL_HANDLE)
    , mImage(VK_NULL_HANDLE)
    , mMemory(VK_NULL_HANDLE) { }

  
  void            Initialize(const VkImageCreateInfo& imageInfo, 
                    const VkImageViewCreateInfo& viewInfo);

  // Uploads the texture from cpu to gpu memory. CommandBuffer must already be
  // allocated before calling this function!
  void            Upload(VulkanRHI* rhi, Image& image);
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