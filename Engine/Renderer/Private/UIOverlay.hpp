// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"

#include "RHI/VulkanConfigs.hpp"
#include "RenderCmd.hpp"
#include "CmdList.hpp"

namespace Recluse {

class VulkanRHI;
class CommandBuffer;
class GraphicsPipeline;
class FrameBuffer;
class RenderPass;
class Texture;
class Renderer;
class DescriptorSetLayout;
class Semaphore;


// User Interface overlay, used to render out images, text, and/or
// streams through screen space.
class UIOverlay {
public:
  UIOverlay() 
    : m_pGraphicsPipeline(VK_NULL_HANDLE)
    , m_pSemaphore(VK_NULL_HANDLE)
    , m_renderPass(nullptr)
    , m_pDescLayout(VK_NULL_HANDLE) { }

  void                        Initialize(VulkanRHI* rhi);
  void                        CleanUp();
  void                        Render();

  // Build the cmd buffers. Cmdlist must be a list of UI compatible objects.
  void                        BuildCmdBuffers(CmdList<UiRenderCmd>& cmdList);

  Semaphore*                  Signal() { return m_pSemaphore; }

  DescriptorSetLayout*        GetMaterialLayout() { return m_pDescLayout; }

  CommandBuffer*              GetCommandBuffer() { return m_CmdBuffer; }

  Semaphore*                  GetSemaphore() { return m_pSemaphore; }

  RenderPass*                 GetRenderPass() { return m_renderPass; }

private:
  void                        InitializeRenderPass();
  void                        SetUpGraphicsPipeline();
  void                        CreateDescriptorSetLayout();
  void                        CleanUpDescriptorSetLayout();

  VulkanRHI*                  m_pRhi;
  Semaphore*                  m_pSemaphore;
  CommandBuffer*              m_CmdBuffer;
  RenderPass*                 m_renderPass;
  GraphicsPipeline*           m_pGraphicsPipeline;
  DescriptorSetLayout*        m_pDescLayout;
  friend class Renderer;
};
} // Recluse