// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/ComputePipeline.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void ComputePipeline::Initialize(VkComputePipelineCreateInfo& info,
    const VkPipelineLayoutCreateInfo& layout)
{
  if (vkCreatePipelineLayout(mOwner, &layout, nullptr, &mLayout) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create compute pipeline layout!\n");
    return;
  }

  info.layout = mLayout;

  if (vkCreateComputePipelines(mOwner, VK_NULL_HANDLE, 1, &info, nullptr, &mPipeline) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create compute pipeline!\n");
  }
}


ComputePipeline::~ComputePipeline()
{
  if (mLayout) {
    R_DEBUG(rWarning, "Compute pipeline layout was not properly cleaned up, prior to ComputePipeline destruction!\n");
  }

  if (mPipeline) {
    R_DEBUG(rWarning, "Compute pipeline was not properly cleaned up, prior to ComputePipeline destruction!\n");
  }
}


void ComputePipeline::CleanUp()
{
  if (mLayout) {
    vkDestroyPipelineLayout(mOwner, mLayout, nullptr);
    mLayout = nullptr;
  }

  if (mPipeline) {
    vkDestroyPipeline(mOwner, mPipeline, nullptr);
    mPipeline = nullptr;
  }
}
} // Recluse