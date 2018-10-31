// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "LightBVH.hpp"

namespace Recluse {


class ComputePipeline;
class Texture;
class DescriptorSetLayout;
class VulkanRHI;
class CommandBuffer;

struct LightBVH;

// Clusterer deals with computing clusters and assigning light indices to each cluster
// in the scene.
class Clusterer {
public:
  void                  Initialize(VulkanRHI* pRhi);
  void                  CleanUp(VulkanRHI* pRhi);
  void                  GenerateComputeBuildBVHCommands(CommandBuffer* pCmdBuffer, LightBVH* pBvH);
  Buffer*               GetClusterIds() { return m_clusterIds; }
  Buffer*               GetClusterIndices() { return m_clusterIndices; }

  DescriptorSetLayout*  GetDescriptorSetLayout() { return m_pSetLayout; }

private:
  // Light assignment builds the bvh hierarchy tree from cpu to gpu, which then is 
  // used by the lighting stage.
  ComputePipeline*      m_lightAssignmentPipe;
  ComputePipeline*      m_clusterGenPipe;

  DescriptorSetLayout*  m_pSetLayout;
  // 3D texture containing cluster ids.
  Buffer*               m_clusterIds;

  // 1D texture containing cluster indices.
  Buffer*               m_clusterIndices;
};
} // Recluse