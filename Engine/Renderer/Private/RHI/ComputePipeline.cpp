// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/ComputePipeline.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void ComputePipeline::Initialize(VkComputePipelineCreateInfo& info,
    const VkPipelineLayoutCreateInfo& layout)
{
  if (vkCreatePipelineLayout(mOwner, &layout, nullptr, &mLayout) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create compute pipeline layout!");
    return;
  }

  info.layout = mLayout;

  if (vkCreateComputePipelines(mOwner, VK_NULL_HANDLE, 1, &info, nullptr, &mPipeline) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create compute pipeline!");
  }
}


void ComputePipeline::CleanUp()
{
  if (mLayout) {
    vkDestroyPipelineLayout(mOwner, mLayout, nullptr);
  }

  if (mPipeline) {
    vkDestroyPipeline(mOwner, mPipeline, nullptr);
  }
}
} // Recluse