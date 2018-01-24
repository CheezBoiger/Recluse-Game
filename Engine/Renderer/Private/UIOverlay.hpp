// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "RHI/VulkanConfigs.hpp"

namespace Recluse {

class VulkanRHI;
class CommandBuffer;
class FrameBuffer;
class Texture;
class Renderer;
class Semaphore;

// User Interface overlay, used to render out images, text, and/or
// streams through screen space.
class UIOverlay {
public:

  void                        Initialize(VulkanRHI* rhi);
  void                        CleanUp();
  void                        Render();

  void                        BuildCmdBuffers();

  Semaphore*                  Signal() { return m_pSemaphore; }
private:
  VulkanRHI*                  m_pRhiRef;
  Texture*                    m_pColorTarget;
  Texture*                    m_pDepthTarget;
  Semaphore*                  m_pSemaphore;
  std::vector<CommandBuffer*> m_CmdBuffers;
  std::vector<VkFramebuffer>  m_FrameBuffers;
  friend Renderer;
};
} // Recluse