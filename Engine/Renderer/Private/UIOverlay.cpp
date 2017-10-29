// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "UIOverlay.hpp"
#include "Core/Exception.hpp"
#include "Renderer.hpp"

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

  
}


void UIOverlay::CleanUp()
{
}
} // Recluse