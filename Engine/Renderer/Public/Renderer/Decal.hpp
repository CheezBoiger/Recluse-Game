// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Vertex.hpp"

#include "CmdList.hpp"
#include "RenderCmd.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"

namespace Recluse {


class Texture2D;
class GraphicsPipeline;
class RenderPass;
class Semaphore;
class CommandBuffer;
class VulkanRHI;
class VertexBuffer;

// Maximum decals that can be tagged into the world.
#define DECAL_MAX         1024


// Decal manager engine. this engine keeps track, and updates,
// decals within the game world. Decals are stored per instance.
class DecalEngine {
  static const u32 kMaxDecalCount = DECAL_MAX;
  static const u32 kBoundingVertexIndex = 0;
  static const u32 kInstanceVertexIndex = 1;
public:

  DecalEngine() 
    : m_decalCount(0)
    , m_pipeline(nullptr)
    , m_renderPass(nullptr) { }


  static u32 GetMaxDecalCount() { return kMaxDecalCount; }

  void        Initialize(VulkanRHI* rhi);
  void        CleanUp(VulkanRHI* rhi);

  // Get decal data information.
  DecalPerInstanceInfo&      GetDecalInfo(size_t idx) { return m_decals[idx]; }

  void      PushDecal(DecalPerInstanceInfo& decal);

  // Decals are set as the next subpass within the offscreen cmdbuffer;
  void        BuildDecals(CommandBuffer* offscreenCmdBuffer);
  
  void        ClearDecalBuffer();
  void        PushDecal(const DecalRenderCmd& cmd);
  
private:

  void        CreateRenderPass(VulkanRHI* pRhi);
  void        CreatePipeline(VulkanRHI* pRhi);
  void        CreateBoundingBox();

  CmdList<DecalRenderCmd>             m_decalCmds;
  std::vector<DecalPerInstanceInfo>   m_decals;
  u32                                 m_decalCount;
  GraphicsPipeline*                   m_pipeline;
  RenderPass*                         m_renderPass;
  VertexBuffer                        m_boundingBox;
};
} // Recluse