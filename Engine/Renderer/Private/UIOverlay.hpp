// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {

class VulkanRHI;
class CommandBuffer;
class Renderer;

// User Interface overlay, used to render out images, text, and/or
// streams through screen space.
class UIOverlay {
public:

  void                        Initialize(VulkanRHI* rhi);
  void                        CleanUp();
  void                        Render();

private:
  VulkanRHI*                  mRhiRef;
  std::vector<CommandBuffer*> cmdBuffers;
  friend Renderer;
};
} // Recluse