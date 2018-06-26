// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Decal.hpp"
#include "RendererData.hpp"
#include "VertexDescription.hpp"
#include "Vertex.hpp"

#include "RHI/VulkanRHI.hpp"

#include "Core/Exception.hpp"

namespace Recluse {


void DecalEngine::Initialize(VulkanRHI* pRhi)
{
  CreateRenderPass(pRhi);
}


void DecalEngine::CreateRenderPass(VulkanRHI* pRhi)
{
  m_renderPass = pRhi->CreateRenderPass();

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
}


void DecalEngine::CreatePipeline(VulkanRHI* pRhi)
{
  m_pipeline = pRhi->CreateGraphicsPipeline();
 
  VkGraphicsPipelineCreateInfo pipelineInfo = { };
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;  
}


void DecalEngine::CleanUp(VulkanRHI* pRhi)
{
  if (m_renderPass) {
    pRhi->FreeRenderPass(m_renderPass);
    m_renderPass = nullptr;
  }

  if (m_pipeline) {
    pRhi->FreeGraphicsPipeline(m_pipeline);
    m_pipeline = nullptr;
  }
}


void DecalEngine::CreateBoundingBox()
{
  
}
} // Recluse