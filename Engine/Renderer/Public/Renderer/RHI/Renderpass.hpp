// Copyright (c) 2017 Recluse Project.
#pragma once

#include "VulkanConfigs.hpp"


namespace Recluse {


class RenderPass {
public:

  RenderPass();
  ~RenderPass();

  void          CleanUp();
  void          Initialize(const VkRenderPassCreateInfo& info);

  VkRenderPass  Handle() { return mRenderPass; }

private:
  VkRenderPass  mRenderPass;
};
} // Recluse