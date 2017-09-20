// Copyright (c) 2017 Recluse Project.
#pragma once

#include "VulkanConfigs.hpp"
#include "Core/Types.hpp"


namespace Recluse {



// GPU friendly parameters. Sent over for the Renderer to determine the 
// Renderer configurations, along with information about textures, meshes,
// and whatnot.
class GpuParams {
public:



  struct {
    VkPresentModeKHR presentMode;
  } Swapchain;


  struct {
  } AntiAliasing;
};
} // Recluse 