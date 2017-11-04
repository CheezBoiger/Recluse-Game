// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "RHI/VulkanRHI.hpp"


namespace Recluse {


class RendererData {
public:

  static std::string PBRPipelineStr;
  static std::string HDRGammaPipelineStr;
  static std::string FinalPipelineStr;
};
} // Recluse