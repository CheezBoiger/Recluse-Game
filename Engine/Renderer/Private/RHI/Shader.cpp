// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Shader.hpp"

#include "Core/Exception.hpp"
#include <fstream>
#include <vector>

namespace Recluse {


b32 Shader::initialize(const std::string& binaryPath)
{
  if (binaryPath.empty()) return false;
  
  std::ifstream file(binaryPath, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    R_DEBUG(rError, "Could not find compiled shader file! Aborting shader creation.\n");
    return false;
  }
  
  size_t size = size_t(file.tellg());
  std::vector<char> buf(size);
  file.seekg(0);

  file.read(buf.data(), size);
  file.close();

  VkShaderModuleCreateInfo info = { };
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = buf.size();
  info.pCode = reinterpret_cast<const u32*>(buf.data());
  
  if (vkCreateShaderModule(mOwner, &info, nullptr, &mModule) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create our shader module!\n");
    return false;
  }

  return true;
}


Shader::~Shader()
{
  if (mModule) {
    R_DEBUG(rWarning, "Shader module not properly cleaned up prior to deletion!\n");
  }
}


void Shader::cleanUp()
{
  if (mModule) {
    vkDestroyShaderModule(mOwner, mModule, nullptr);
    mModule = VK_NULL_HANDLE;
  }
}
} // Recluse