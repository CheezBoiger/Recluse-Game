// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Vertex.hpp"
#include "RHI/VulkanConfigs.hpp"


namespace Recluse {


struct SkinnedVertexDescription {
  static VkVertexInputBindingDescription                GetBindingDescription();
  static std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();
};


struct QuadVertexDescription {
  static VkVertexInputBindingDescription                GetBindingDescription();
  static std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();
};


struct StaticVertexDescription {
  static VkVertexInputBindingDescription                GetBindingDescription();
  static std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();
};


struct UIVertexDescription {
  static VkVertexInputBindingDescription                GetBindingDescription();
  static std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();
};
} // Recluse