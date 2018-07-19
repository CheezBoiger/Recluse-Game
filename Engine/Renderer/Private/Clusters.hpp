// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "LightBVH.hpp"

namespace Recluse {


class ComputePipeline;
class Texture;
class DescriptorSetLayout;
class VulkanRHI;


// Clusterer deals with computing clusters and assigning light indices to each cluster
// in the scene.
class Clusterer {
public:
  void                  Initialize(VulkanRHI* pRhi);
  void                  CleanUp(VulkanRHI* pRhi);

  Texture*              GetClusterIds() { return m_clusterIds3D; }
  Texture*              GetClusterIndices() { return m_clusterIndices1D; }

  DescriptorSetLayout*  GetDescriptorSetLayout() { return m_pSetLayout; }

private:
  ComputePipeline*      m_lightAssignmentPipe;
  ComputePipeline*      m_clusterGenPipe;

  DescriptorSetLayout*  m_pSetLayout;
  // 3D texture containing cluster ids.
  Texture*              m_clusterIds3D;

  // 1D texture containing cluster indices.
  Texture*              m_clusterIndices1D;
};
} // Recluse