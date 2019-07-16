// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Decal.hpp"
#include "RendererData.hpp"
#include "VertexDescription.hpp"
#include "Vertex.hpp"

#include "RHI/VulkanRHI.hpp"

#include "Core/Exception.hpp"

namespace Recluse {


void DecalEngine::initialize(VulkanRHI* pRhi)
{
  CreateRenderPass(pRhi);
}


void DecalEngine::CreateRenderPass(VulkanRHI* pRhi)
{
  m_renderPass = pRhi->createRenderPass();

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
}


void DecalEngine::CreatePipeline(VulkanRHI* pRhi)
{
  m_pipeline = pRhi->createGraphicsPipeline();
 
  VkGraphicsPipelineCreateInfo pipelineInfo = { };
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;  
}


void DecalEngine::cleanUp(VulkanRHI* pRhi)
{
  if (m_renderPass) {
    pRhi->freeRenderPass(m_renderPass);
    m_renderPass = nullptr;
  }

  if (m_pipeline) {
    pRhi->freeGraphicsPipeline(m_pipeline);
    m_pipeline = nullptr;
  }
}


void DecalEngine::CreateBoundingBox()
{
  
}


void DecalEngine::BuildDecals(CommandBuffer* offscreenCmdBuffer)
{
  R_ASSERT(offscreenCmdBuffer, "Offscreen command buffer is null prior to building decals.");
  R_ASSERT(offscreenCmdBuffer->recording(), "Offscreen command buffer not in recording state.");


}
} // Recluse