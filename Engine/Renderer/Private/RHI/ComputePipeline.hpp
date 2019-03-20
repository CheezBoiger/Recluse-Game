// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"


namespace Recluse {


class ComputePipeline : public VulkanHandle {
public:
  
  ComputePipeline()
    : mPipeline(VK_NULL_HANDLE)
    , mLayout(VK_NULL_HANDLE) { }

  ~ComputePipeline();

  void              initialize(VkComputePipelineCreateInfo& info, 
                      const VkPipelineLayoutCreateInfo& layout);
  void              cleanUp();

  VkPipeline        Pipeline() const { return mPipeline; }
  VkPipelineLayout  getLayout() const { return mLayout; }


private:
  VkPipeline        mPipeline;
  VkPipelineLayout  mLayout;
};
} // Recluse