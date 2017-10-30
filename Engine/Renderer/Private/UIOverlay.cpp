// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "UIOverlay.hpp"
#include "Core/Exception.hpp"
#include "Renderer.hpp"
#include "Resources.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"

namespace Recluse {

void UIOverlay::Render()
{
  // Ignore if no reference to the rhi.
  if (!mRhiRef) return;

  // Render the overlay.
}


void UIOverlay::Initialize(VulkanRHI* rhi)
{
  mRhiRef = rhi;

  if (!gResources().GetGraphicsPipeline("UIOverlayPipeline")) {
    GraphicsPipeline* pipeline = rhi->CreateGraphicsPipeline();
    gResources().RegisterGraphicsPipeline("UIOverlayPipeline", pipeline);
    VkGraphicsPipelineCreateInfo pipeCI = { };
    VkPipelineLayoutCreateInfo layoutCI = { };

    //pipeline->Initialize(pipeCI, layoutCI);
  }
}


void UIOverlay::CleanUp()
{
  GraphicsPipeline* pipeline = gResources().UnregisterGraphicsPipeline("UIOverlayPipeline");
  if (pipeline) {
    mRhiRef->FreeGraphicsPipeline(pipeline);
  }
}
} // Recluse