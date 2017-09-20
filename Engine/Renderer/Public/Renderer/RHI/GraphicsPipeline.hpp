// Copyright (c) 2017 Recluse Project.
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

  void              Initialize(const VkGraphicsPipelineCreateInfo& info,
                      const VkPipelineLayoutCreateInfo& layout);
  void              CleanUp();

  VkPipeline        Pipeline() { return mPipeline; }
  VkPipelineLayout  Layout() { return mLayout; }

private:
  VkPipeline        mPipeline;
  VkPipelineLayout  mLayout;
};
} // Recluse