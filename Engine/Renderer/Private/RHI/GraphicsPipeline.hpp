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

  ~GraphicsPipeline();

  // Graphics Pipeline initialization call. This will effectively 
  // initialize the pipeline, along with setting the pipeline layout.
  void              initialize(VkGraphicsPipelineCreateInfo& info,
                      const VkPipelineLayoutCreateInfo& layout);

  // Clean up the pipeline object, as well as layout.
  void              cleanUp();

  VkPipeline        Pipeline() { return mPipeline; }
  VkPipelineLayout  getLayout() { return mLayout; }

private:
  VkPipeline        mPipeline;
  VkPipelineLayout  mLayout;
};
} // Recluse