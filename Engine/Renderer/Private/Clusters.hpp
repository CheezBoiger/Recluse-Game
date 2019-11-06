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


struct Cluster {
  Vector4 pmin;
  Vector4 pmax;
};

// Clusterer deals with computing clusters and assigning light indices to each cluster
// in the scene.
class Clusterer {
public:
  void                  initialize(VulkanRHI* pRhi);
  void                  cleanUp(VulkanRHI* pRhi);
  void                  generateComputeBuildBVHCommands(CommandBuffer* pCmdBuffer, LightBVH* pBvH);
  Buffer*               getClusterBuffer() { return m_clusters; }
  Buffer*               getLightList() { return m_lightList; }

private:

  // 3D texture containing clusters.
  Buffer*               m_clusters;

  // 1D Buffer containing light list.
  Buffer*               m_lightList;

  // Light grid.
  Buffer*               m_lightGrid;
};
} // Recluse