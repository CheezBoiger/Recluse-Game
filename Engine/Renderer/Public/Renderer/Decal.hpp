// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Vertex.hpp"

#include "CmdList.hpp"
#include "RenderCmd.hpp"

namespace Recluse {


class Texture2D;
class GraphicsPipeline;
class RenderPass;
class Semaphore;
class CommandBuffer;
class VulkanRHI;

// Maximum decals that can be tagged into the world.
#define DECAL_MAX         1024


// Decal manager engine. this engine keeps track, and updates,
// decals within the game world. Decals are stored per instance.
class DecalEngine {
  static const u32 kMaxDecalCount = DECAL_MAX;
public:
  DecalEngine() { }


  static u32 GetMaxDecalCount() { return kMaxDecalCount; }

  void        Initialize(VulkanRHI* rhi);
  void        CleanUp(VulkanRHI* rhi);

  // Get decal data information.
  DecalPerInstanceInfo&      GetDecalInfo(size_t idx) { return m_decals[idx]; }

  void      PushDecal(DecalPerInstanceInfo& decal);

  // Decals are set as the next subpass within the offscreen cmdbuffer;
  void        BuildDecals(CommandBuffer* offscreenCmdBuffer);

  void        ClearDecalBuffer();
  
private:
  std::vector<DecalPerInstanceInfo>   m_decals;
  u32                                 m_decalCount;
  GraphicsPipeline*                   m_pipeline;
  RenderPass*                         m_renderPass;
};
} // Recluse