// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GraphicsPipeline.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


GraphicsPipeline::~GraphicsPipeline()
{
  if (mPipeline) {
    R_DEBUG(rWarning, "Graphics pipeline CleanUp was not called prior to its deletion!\n");
  }
}


void GraphicsPipeline::Initialize(VkGraphicsPipelineCreateInfo& info,
  const VkPipelineLayoutCreateInfo& layout)
{
  VkResult result = vkCreatePipelineLayout(mOwner, &layout, nullptr, &mLayout);
  if (result != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create pipeline layout!\n");
  }

  info.layout = mLayout;

  if (vkCreateGraphicsPipelines(mOwner, VK_NULL_HANDLE, 1, &info, nullptr, &mPipeline) != VK_SUCCESS) {
    R_DEBUG(rError, "Failed to create pipeline!\n");
    R_ASSERT(false, "");
    return;
  }
}


void GraphicsPipeline::CleanUp()
{
  if (mPipeline && mLayout) {
    vkDestroyPipeline(mOwner, mPipeline, nullptr);
    vkDestroyPipelineLayout(mOwner, mLayout, nullptr);
    mPipeline = VK_NULL_HANDLE;
    mLayout = VK_NULL_HANDLE;
  }
}
} // Recluse