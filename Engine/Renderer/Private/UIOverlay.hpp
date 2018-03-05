// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"

namespace Recluse {

class VulkanRHI;
class CommandBuffer;
class GraphicsPipeline;
class FrameBuffer;
class Texture;
class Renderer;
class Semaphore;
class CmdList;

// User Interface overlay, used to render out images, text, and/or
// streams through screen space.
class UIOverlay {
public:

  void                        Initialize(VulkanRHI* rhi);
  void                        CleanUp();
  void                        Render();

  void                        BuildCmdBuffers(CmdList* cmdList);

  Semaphore*                  Signal() { return m_pSemaphore; }
private:
  void                        InitializeFrameBuffer();
  void                        SetUpGraphicsPipeline();

  VulkanRHI*                  m_pRhi;
  Texture*                    m_pColorTarget;
  Texture*                    m_pDepthTarget;
  Semaphore*                  m_pSemaphore;
  std::vector<CommandBuffer*> m_CmdBuffers;
  FrameBuffer*                m_pFrameBuffer;
  GraphicsPipeline*           m_pGraphicsPipeline;
  friend Renderer;
};
} // Recluse