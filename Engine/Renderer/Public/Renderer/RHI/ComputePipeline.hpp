// Copyright (c) 2017 Recluse Project.
#pragma once


#include "Core/Types.hpp"
#include "VulkanConfigs.hpp"


namespace Recluse {


class ComputePipeline : public VulkanHandle {
public:
  
  ComputePipeline()
    : mPipeline(VK_NULL_HANDLE)
    , mLayout(VK_NULL_HANDLE) { }

  void              Initialize(const VkComputePipelineCreateInfo& info, 
                      const VkPipelineLayoutCreateInfo& layout);
  void              CleanUp();

  VkPipeline        Pipeline() { return mPipeline; }
  VkPipelineLayout  Layout() { return mLayout; }


private:
  VkPipeline        mPipeline;
  VkPipelineLayout  mLayout;
};
} // Recluse