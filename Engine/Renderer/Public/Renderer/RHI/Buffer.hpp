// Copyright (c) 2017 Recluse Project.
#pragma once

#include "Renderer/RHI/VulkanConfigs.hpp"


namespace Recluse {



class Buffer : public VulkanHandle {
public:
  Buffer();
  ~Buffer();

  void              CleanUp();
  void              Initialize(VkDevice device, VkBufferCreateInfo& info);

  VkBuffer          Handle() { return mBuffer; }
  VkDeviceMemory    Memory() { return mMemory; }

private:
  VkBuffer          mBuffer;
  VkDeviceMemory    mMemory;
};
} // Recluse