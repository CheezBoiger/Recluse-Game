// Copyright (c) 2017 Recluse Project.
#pragma once


#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"


namespace Recluse {


class RenderTarget : public VulkanHandle {
public:
  RenderTarget()
    : mImage(VK_NULL_HANDLE)
    , mView(VK_NULL_HANDLE)
    , mMemory(VK_NULL_HANDLE) { }

  VkImage         Image() { return mImage; }
  VkImageView     View() { return mView; }
  VkDeviceMemory  Memory() { return mMemory; }

private:
  VkImage         mImage;
  VkImageView     mView;
  VkDeviceMemory  mMemory;
};
} // Recluse