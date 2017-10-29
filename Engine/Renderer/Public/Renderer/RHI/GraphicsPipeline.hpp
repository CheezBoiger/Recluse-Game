// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "VulkanConfigs.hpp"
#include "LogicalDevice.hpp"

#include "Core/Types.hpp"


namespace Recluse {


class GraphicsPipeline : public VulkanHandle {
public:
  GraphicsPipeline()
    : mPipeline(VK_NULL_HANDLE)
    , mLayout(VK_NULL_HANDLE) { }

  // Graphics Pipeline initialization call. This will effectively 
  // initialize the pipeline, along with setting the pipeline layout.
  void              Initialize(VkGraphicsPipelineCreateInfo& info,
                      const VkPipelineLayoutCreateInfo& layout);

  // Clean up the pipeline object, as well as layout.
  void              CleanUp();

  VkPipeline        Pipeline() { return mPipeline; }
  VkPipelineLayout  Layout() { return mLayout; }

private:
  VkPipeline        mPipeline;
  VkPipelineLayout  mLayout;
};
} // Recluse