// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"


namespace Recluse {


// LogicalDevice object, which holds the handle of the native
// device. This object is used to query data and objects from the 
// native logical device.
class LogicalDevice {
public:
  LogicalDevice()
    : handle(VK_NULL_HANDLE) { }


  b8                    Initialize(const VkPhysicalDevice physical, const VkDeviceCreateInfo& info);
  void                  CleanUp();

  VkMemoryRequirements  GetImageMemoryRequirements(const VkImage& image);
  VkMemoryRequirements  GetBufferMemoryRequirements(const VkBuffer& buffer);
  VkDevice              Handle() const { return handle; }

private:
  VkDevice              handle;
};
} // Recluse