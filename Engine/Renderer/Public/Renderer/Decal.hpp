// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Vertex.hpp"

#include "CmdList.hpp"
#include "RenderCmd.hpp"

namespace Recluse {


class Texture2D;
class Image;
class GraphicsPipeline;
class RenderPass;
class Semaphore;
class CommandBuffer;
class VulkanRHI;
class VertexBuffer;
class IndexBuffer;
class Texture;

// Maximum decals that can be tagged into the world.
#define DECAL_MAX         4196


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


  static u32 getMaxDecalCount() { return kMaxDecalCount; }

  void        initialize(VulkanRHI* rhi);
  void        cleanUp(VulkanRHI* rhi);

  // Decals are set as the next subpass within the offscreen cmdbuffer;
  void        buildDecals(CommandBuffer* offscreenCmdBuffer);
  void        updateTextureAtlas(const Image& img, u32 idx);
  
  void        clearDecalBuffer() { m_decalCmds.clear(); }
  void        pushDecal(const DecalRenderCmd& cmd) { m_decalCmds.pushBack( cmd ); }

  void  visualizeBoundingBoxes(b32 enable) { m_visualizeBBoxes = enable; }
  
private:

  void        createRenderPass(VulkanRHI* pRhi);
  void        createPipeline(VulkanRHI* pRhi);
  void        createBoundingBox(VulkanRHI* pRhi);

  void freeBoundingBox(VulkanRHI* pRhi);

  CmdList<DecalRenderCmd>             m_decalCmds;
  u32                                 m_decalCount;
  b32                                 m_visualizeBBoxes;
  GraphicsPipeline*                   m_pipeline;
  RenderPass*                         m_renderPass;
  VertexBuffer*                       m_gpuBBoxVertices;
  IndexBuffer*                        m_gpuBBoxIndices;
};


class InstancedDecalEngine : public DecalEngine
{
public:
  
};


class ClusterDecalEngine : public DecalEngine
{
public:


};
} // Recluse