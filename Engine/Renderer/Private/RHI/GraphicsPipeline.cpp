// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GraphicsPipeline.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


GraphicsPipeline::~GraphicsPipeline()
{
  if (mPipeline) {
    R_DEBUG(rWarning, "Graphics pipeline CleanUp was not called prior to its deletion!\n");
    R_ASSERT(false, "");
  }

  if (mLayout) {
    R_DEBUG(rWarning, "pipeline layout CleanUp was not called prior to its deletion!\n");
    R_ASSERT(false, "");
  }
}


void GraphicsPipeline::initialize(VkGraphicsPipelineCreateInfo& info,
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


void GraphicsPipeline::cleanUp()
{
  if (mPipeline) {
    vkDestroyPipeline(mOwner, mPipeline, nullptr);
    mPipeline = VK_NULL_HANDLE;
  }

  if (mLayout) {
    vkDestroyPipelineLayout(mOwner, mLayout, nullptr);
    mLayout = VK_NULL_HANDLE;
  }
}
} // Recluse