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
     { }

  ~Sky();

  void                    Initialize();
  void                    CleanUp();


  Semaphore*              SignalSemaphore() { return m_pAtmosphereSema; }
  Texture*                GetCubeMap() { return m_pCubeMap; }

  // Render command should be used sparingly, as it forces the device to wait.
  // Call this every time skybox needs to be updated.
  void                    Render(Semaphore* signal);

private:
  void                    CreateRenderAttachments(VulkanRHI* rhi);
  void                    CreateCubeMap(VulkanRHI* rhi);
  void                    CreateCommandBuffer(VulkanRHI* rhi);
  void                    BuildCmdBuffer(VulkanRHI* rhi);
  void                    CreateGraphicsPipeline(VulkanRHI* rhi);
  
  // RenderTextures that will be used as attachments,
  // and will be loaded onto a cubemap for the skybox.
  std::array<Texture*, 6> m_RenderTextures;
  GraphicsPipeline*       m_pPipeline;
  Texture*                m_pCubeMap;
  Sampler*                m_pSampler;
  Semaphore*              m_pAtmosphereSema;
  CommandBuffer*          m_pCmdBuffer;
};
} // Recluse 