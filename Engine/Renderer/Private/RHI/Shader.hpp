// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"


namespace Recluse {


class Shader : public VulkanHandle {
public:
  Shader()
    : mModule(VK_NULL_HANDLE) { }

  ~Shader();

  B32                  initialize(const std::string& binaryPath);
  void                cleanUp();
  VkShaderModule      getHandle() { return mModule; }

private:
  VkShaderModule      mModule;
};
} // Recluse