// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RHI/GraphicsPipeline.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void GraphicsPipeline::Initialize(VkGraphicsPipelineCreateInfo& info,
  const VkPipelineLayoutCreateInfo& layout)
{
  if (vkCreatePipelineLayout(mOwner, &layout, nullptr, &mLayout) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create pipeline layout!");
  }

  info.layout = mLayout;

  if (vkCreateGraphicsPipelines(mOwner, VK_NULL_HANDLE, 1, &info, nullptr, &mPipeline) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create pipeline!");
    return;
  }
}


void GraphicsPipeline::CleanUp()
{
  if (mPipeline && mLayout) {
    vkDestroyPipeline(mOwner, mPipeline, nullptr);
    vkDestroyPipelineLayout(mOwner, mLayout, nullptr);
  }
}
} // Recluse