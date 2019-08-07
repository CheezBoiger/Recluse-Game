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
    , m_vertStagingBuffer(nullptr)
    , m_indicesStagingBuffer(nullptr)
    , m_mainBuffer(0) { }

  void                        initialize(VulkanRHI* rhi);
  void                        cleanUp(VulkanRHI* pRhi);
  void                        render(VulkanRHI* pRhi);

  // Build the cmd buffers. Cmdlist must be a list of UI compatible objects.
  void                        BuildCmdBuffers(VulkanRHI* pRhi, GlobalDescriptor* global, u32 frameIndex);

  Semaphore*                  Signal(u32 idx) { return m_pSemaphores[idx]; }

  DescriptorSetLayout*        GetMaterialLayout() { return m_pDescLayout; }

  CommandBuffer*              GetCommandBuffer(u32 idx) { return m_CmdBuffers[idx]; }

  RenderPass*                 GetRenderPass() { return m_renderPass; }

  BufferUI*                   GetUIBuffer() { return &m_mainBuffer; }

  void                        ClearUiBuffers();

private:
  void                        initializeRenderPass(VulkanRHI* pRhi);
  void                        CreateBuffers(VulkanRHI* pRhi);
  void                        SetUpGraphicsPipeline(VulkanRHI* pRhi);
  void                        createDescriptorSetLayout(VulkanRHI* pRhi);
  void                        CleanUpDescriptorSetLayout(VulkanRHI* pRhi);
  void                        CleanUpBuffers(VulkanRHI* pRhi);
  void                        StreamBuffers(VulkanRHI* pRhi, u32 frameIndex);

  std::vector<Semaphore*>     m_pSemaphores;
  std::vector<CommandBuffer*> m_CmdBuffers;
  std::vector<Buffer*>        m_vertBuffers;
  std::vector<Buffer*>        m_indicesBuffers;
  Buffer*                     m_vertStagingBuffer;
  Buffer*                     m_indicesStagingBuffer;
  RenderPass*                 m_renderPass;
  GraphicsPipeline*           m_pGraphicsPipeline;
  DescriptorSetLayout*        m_pDescLayout;
  BufferUI                    m_mainBuffer;
  friend class Renderer;
};
} // Recluse