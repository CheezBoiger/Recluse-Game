// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Renderer/Renderer.hpp"
#include "Core/Math/Vector4.hpp"
#include "RHI/VulkanConfigs.hpp"
#include <array>

namespace Recluse {


class Texture;
class Semaphore;
class GraphicsPipeline;
class DescriptorSet;
class DescriptorSetLayout;
class Buffer;
class FrameBuffer;
class RenderPass;
class VertexBuffer;
class IndexBuffer;


// Sky, which renders the, well, sky, onto a cubemap. This cubemap is then 
// used to sample to the final render texture.
class SkyRenderer {
public:
  static const std::string  kAtmVertStr;
  static const std::string  kAtmFragStr;
  static const std::string  kSkyVertStr;
  static const std::string  kSkyFragStr;
  static const u32          kTextureSize;
  static std::array<Vector4, 36> kSkyBoxVertices;
  static std::array<u16, 36>  kSkyboxIndices;
  static const Vector3      kDefaultAirColor;

  SkyRenderer() 
    : m_pCubeMap(nullptr)
    , m_pAtmosphereSema(nullptr)
    , m_pPipeline(nullptr)
    , m_pCmdBuffer(nullptr)
    , m_pSampler(nullptr)
    , m_pFrameBuffer(nullptr)
    , m_pRenderPass(nullptr)
    , m_RenderTexture(nullptr)
    , m_bDirty(false)
    , m_SkyboxRenderPass(nullptr)
     { }

  ~SkyRenderer();

  void                    Initialize();
  void                    CleanUp();
  void                    MarkDirty() { m_bDirty = true; }
  void                    MarkClean() { m_bDirty = false; }


  Semaphore*              SignalSemaphore() { return m_pAtmosphereSema; }
  Texture*                GetCubeMap() { return m_pCubeMap; }
  Sampler*                GetSampler() { return m_pSampler; }

  b32                     NeedsRendering() { return m_bDirty; }
  b32                     UsingPredefinedSkybox() const { return m_bUsingPredefined; }
  CommandBuffer*          CmdBuffer() { return m_pCmdBuffer; }

  // Somewhat of a hack... We can't clear out our attachments when rendering the skybox.
  // This is the renderpass to say NO to clearing out the pbr pass.
  RenderPass*             GetSkyboxRenderPass() { return m_SkyboxRenderPass; }

  VertexBuffer*           GetSkyboxVertexBuffer() { return &m_SkyboxVertBuf; }
  IndexBuffer*            GetSkyboxIndexBuffer() { return &m_SkyboxIndBuf; }

  Vector3                 GetAirColor() const { return m_vAirColor; }
  void                    SetAirColor(Vector3 color) { m_vAirColor = color; }

  void                    BuildCmdBuffer(VulkanRHI* rhi, CommandBuffer* pOut = nullptr);

private:
  void                    CreateRenderAttachment(VulkanRHI* rhi);
  void                    CreateCubeMap(VulkanRHI* rhi);
  void                    CreateCommandBuffer(VulkanRHI* rhi);
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
  RenderPass*             m_pRenderPass;
  RenderPass*             m_SkyboxRenderPass;
  VertexBuffer            m_SkyboxVertBuf;
  IndexBuffer             m_SkyboxIndBuf;
  b32                     m_bDirty;
  b32                     m_bUsingPredefined;
  Vector3                 m_vAirColor;
};
} // Recluse 