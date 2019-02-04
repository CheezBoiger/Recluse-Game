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
class Buffer;
class Renderer;
class DescriptorSetLayout;
class GlobalDescriptor;
class Semaphore;


// User Interface overlay, used to render out images, text, and/or
// streams through screen space.
class UIOverlay {
public:
  UIOverlay() 
    : m_pGraphicsPipeline(VK_NULL_HANDLE)
    , m_renderPass(nullptr)
    , m_pDescLayout(VK_NULL_HANDLE)
    , m_vertBuffer(nullptr)
    , m_indicesBuffer(nullptr)
    , m_vertStagingBuffer(nullptr)
    , m_indicesStagingBuffer(nullptr)
    , m_mainBuffer(0) { }

  void                        Initialize(VulkanRHI* rhi);
  void                        CleanUp();
  void                        Render();

  // Build the cmd buffers. Cmdlist must be a list of UI compatible objects.
  void                        BuildCmdBuffers(GlobalDescriptor* global, u32 frameIndex);

  Semaphore*                  Signal(u32 idx) { return m_pSemaphores[idx]; }

  DescriptorSetLayout*        GetMaterialLayout() { return m_pDescLayout; }

  CommandBuffer*              GetCommandBuffer(u32 idx) { return m_CmdBuffers[idx]; }

  RenderPass*                 GetRenderPass() { return m_renderPass; }

  BufferUI*                   GetUIBuffer() { return &m_mainBuffer; }

  void                        ClearUiBuffers();

private:
  void                        InitializeRenderPass();
  void                        CreateBuffers();
  void                        SetUpGraphicsPipeline();
  void                        CreateDescriptorSetLayout();
  void                        CleanUpDescriptorSetLayout();
  void                        CleanUpBuffers();
  void                        StreamBuffers();

  VulkanRHI*                  m_pRhi;
  std::vector<Semaphore*>     m_pSemaphores;
  std::vector<CommandBuffer*> m_CmdBuffers;
  Buffer*                     m_vertStagingBuffer;
  Buffer*                     m_indicesStagingBuffer;
  Buffer*                     m_vertBuffer;
  Buffer*                     m_indicesBuffer;
  RenderPass*                 m_renderPass;
  GraphicsPipeline*           m_pGraphicsPipeline;
  DescriptorSetLayout*        m_pDescLayout;
  BufferUI                    m_mainBuffer;
  friend class Renderer;
};
} // Recluse