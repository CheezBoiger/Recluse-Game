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
    , mMemory(VK_NULL_HANDLE)
    , mMipLevels(0)
    , mArrayLayers(0)
    , mFormat(VK_FORMAT_UNDEFINED)
    , mWidth(0)
    , mHeight(0)
    , mSamples(VK_SAMPLE_COUNT_1_BIT) { }

  
  void                  Initialize(const VkImageCreateInfo& imageInfo, 
                          VkImageViewCreateInfo& viewInfo, b8 stream = false);

  // Uploads the texture from cpu to gpu memory. Can be used to stream texture data for 
  // updates.
  void                  Upload(VulkanRHI* rhi, Image const& image);
  void                  CleanUp();

  VkImageView           View() { return mView; }
  VkImage               Image() { return mImage; }
  VkDeviceMemory        Memory() { return mMemory; }
  VkFormat              Format() const { return mFormat; }
  u32                   Width() const { return mWidth; }
  u32                   Height() const { return mHeight; }
  u32                   MipLevels() const { return mMipLevels; }
  u32                   ArrayLayers() const { return mArrayLayers; }
  VkSampleCountFlagBits Samples() const { return mSamples; }

private:
  VkImageView           mView;
  VkImage               mImage;
  VkDeviceMemory        mMemory;
  VkFormat              mFormat;
  VkSampleCountFlagBits mSamples;
  u32                   mWidth;
  u32                   mHeight;
  u32                   mMipLevels;
  u32                   mArrayLayers;
};
} // Recluse