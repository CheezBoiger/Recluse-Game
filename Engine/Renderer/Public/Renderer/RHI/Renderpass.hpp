// Copyright (c) 2017 Recluse Project. All rights reserved.
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