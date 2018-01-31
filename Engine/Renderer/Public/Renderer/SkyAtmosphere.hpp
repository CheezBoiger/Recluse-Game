// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Renderer/Renderer.hpp"

#include <array>

namespace Recluse {


class Texture;
class Semaphore;
class GraphicsPipeline;
class DescriptorSet;
class DescriptorSetLayout;
class Buffer;
class FrameBuffer;


// Sky, which renders the, well, sky, onto a cubemap. This cubemap is then 
// used to sample to the final render texture.
class Sky {
public:
  static const std::string  kVertStr;
  static const std::string  kFragStr;
  static const u32          kTextureSize;

  Sky() 
    : m_pCubeMap(nullptr)
    , m_pAtmosphereSema(nullptr)
    , m_pPipeline(nullptr)
    , m_pCmdBuffer(nullptr)
    , m_pSampler(nullptr)
    , m_pFrameBuffer(nullptr)
    , m_bDirty(false)
     { }

  ~Sky();

  void                    Initialize();
  void                    CleanUp();
  void                    MarkDirty() { m_bDirty = true; }
  void                    MarkClean() { m_bDirty = false; }


  Semaphore*              SignalSemaphore() { return m_pAtmosphereSema; }
  Texture*                GetCubeMap() { return m_pCubeMap; }
  Sampler*                GetSampler() { return m_pSampler; }

  i32                     NeedsRendering() { return m_bDirty; }
  CommandBuffer*          CmdBuffer() { return m_pCmdBuffer; }

private:
  void                    CreateRenderAttachment(VulkanRHI* rhi);
  void                    CreateCubeMap(VulkanRHI* rhi);
  void                    CreateCommandBuffer(VulkanRHI* rhi);
  void                    BuildCmdBuffer(VulkanRHI* rhi);
  void                    CreateGraphicsPipeline(VulkanRHI* rhi);
  void                    CreateFrameBuffer(VulkanRHI* rhi);
  
  // RenderTextures that will be used as attachments,
  // and will be loaded onto a cubemap for the skybox.
  Texture*                m_RenderTexture;
  GraphicsPipeline*       m_pPipeline;
  Texture*                m_pCubeMap;
  Sampler*                m_pSampler;
  Semaphore*              m_pAtmosphereSema;
  CommandBuffer*          m_pCmdBuffer;
  FrameBuffer*            m_pFrameBuffer;
  i32                     m_bDirty;

  struct ViewerBlock {
    Matrix4             _InvView;
    Matrix4             _InvProj;
  } m_ViewerConsts;
};
} // Recluse 