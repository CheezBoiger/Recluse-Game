// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Clusters.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/ComputePipeline.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Commandbuffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void Clusterer::initialize(VulkanRHI* pRhi)
{
  m_clusters = pRhi->createBuffer();
}


void Clusterer::cleanUp(VulkanRHI* pRhi)
{
  pRhi->freeBuffer(m_clusters);
  m_clusters = VK_NULL_HANDLE;
}
} // Recluse