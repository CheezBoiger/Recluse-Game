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

  void initialize(Renderer* pRenderer);
  void cleanUp(Renderer* pRenderer);
  void render(Renderer* pRenderer);

  // Build the cmd buffers. Cmdlist must be a list of UI compatible objects.
  void                        buildCmdBuffers(Renderer* pRenderer, 
                                              GlobalDescriptor* global, 
                                              U32 frameIndex, 
                                              U32 resourceIndex);

  Semaphore*                  getSignal(U32 idx) { return m_pSemaphores[idx]; }

  DescriptorSetLayout*        getMaterialLayout() { return m_pDescLayout; }

  CommandBuffer*              gtCommandBuffer(U32 idx) { return m_CmdBuffers[idx]; }

  RenderPass*                 getRenderPass() { return m_renderPass; }

  BufferUI*                   getUIBuffer() { return &m_mainBuffer; }

  void                        clearUiBuffers();

private:
  void                        initializeRenderPass(VulkanRHI* pRhi);
  void                        CreateBuffers(Renderer* pRenderer);
  void                        SetUpGraphicsPipeline(VulkanRHI* pRhi);
  void                        createDescriptorSetLayout(VulkanRHI* pRhi);
  void                        CleanUpDescriptorSetLayout(VulkanRHI* pRhi);
  void                        CleanUpBuffers(VulkanRHI* pRhi);
  void                        StreamBuffers(VulkanRHI* pRhi, U32 frameIndex);

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