// Copyright (c) 2017 Recluse Project.
#include "RHI/GraphicsPipeline.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


void GraphicsPipeline::Initialize(const VkGraphicsPipelineCreateInfo& info,
  const VkPipelineLayoutCreateInfo& layout)
{
  if (vkCreateGraphicsPipelines(mOwner, VK_NULL_HANDLE, 1, &info, nullptr, &mPipeline) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create pipeline!\n");
    return;
  }

  if (vkCreatePipelineLayout(mOwner, &layout, nullptr, &mLayout) != VK_SUCCESS) {
    R_DEBUG("ERROR: Failed to create pipeline layout!");
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