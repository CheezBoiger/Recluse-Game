// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Decal.hpp"
#include "RendererData.hpp"
#include "VertexDescription.hpp"
#include "Vertex.hpp"
#include "RendererData.hpp"

#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"

#include "Core/Exception.hpp"

namespace Recluse {


std::array<Vector4, 36> bboxPositions = 
{
    // front
    Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
    Vector4(1.0f, -1.0f, 1.0f, 1.0f),
    Vector4(1.0f, 1.0f, 1.0f, 1.0f),
    Vector4(1.0f, 1.0f, 1.0f, 1.0f),
    Vector4(-1.0f, 1.0f, 1.0f, 1.0f),
    Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
    // Back
    Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(1.0f, -1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
    // up
    Vector4(1.0f, 1.0f, 1.0f, 1.0f),
    Vector4(1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, 1.0f, 1.0f, 1.0f),
    Vector4(1.0f, 1.0f, 1.0f, 1.0f),
    // Down
    Vector4(1.0f, -1.0f, 1.0f, 1.0f),
    Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
    Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
    Vector4(1.0f, -1.0f, -1.0f, 1.0f),
    Vector4(1.0f, -1.0f, 1.0f, 1.0f),
    // right
    Vector4(1.0f, -1.0f, 1.0f, 1.0f),
    Vector4(1.0f, -1.0f, -1.0f, 1.0f),
    Vector4(1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(1.0f, 1.0f, 1.0f, 1.0f),
    Vector4(1.0f, -1.0f, 1.0f, 1.0f),
    // Left
    Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
    Vector4(-1.0f, 1.0f, 1.0f, 1.0f),
    Vector4(-1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, 1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
    Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
};


std::array<u16, 36> bboxIndices = 
{
  0, 1, 2,
  3, 4, 5,
  6, 7, 8,
  9, 10, 11,
  12, 13, 14,
  15, 16, 17,
  18, 19, 20,
  21, 22, 23,
  24, 25, 26,
  27, 28, 29,
  30, 31, 32,
  33, 34, 35
};


void DecalEngine::initialize(VulkanRHI* pRhi)
{
  createRenderPass(pRhi);
  createBoundingBox(pRhi);
}


void DecalEngine::createRenderPass(VulkanRHI* pRhi)
{
  m_renderPass = pRhi->createRenderPass();

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

  std::array<VkAttachmentDescription, 4> attachmentDescs;
  std::array<VkAttachmentReference, 4> attachmentRefs;

  attachmentDescs[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  attachmentDescs[0].format = gbuffer_AlbedoAttachKey->getFormat();
  
}


void DecalEngine::createPipeline(VulkanRHI* pRhi)
{
  m_pipeline = pRhi->createGraphicsPipeline();
 
  VkGraphicsPipelineCreateInfo pipelineInfo = { };
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;  


  VkPipelineLayoutCreateInfo pipelineLayout = { };
  pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;


  Shader* pVShader = pRhi->createShader();
  Shader* pPShader = pRhi->createShader();
  
  RendererPass::loadShader("Decal.vert.spv", pVShader);
  RendererPass::loadShader("Decal.frag.spv", pPShader);
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

  freeBoundingBox(pRhi);
}


void DecalEngine::freeBoundingBox(VulkanRHI* pRhi)
{
  if ( m_gpuBBoxIndices ) 
  {
    m_gpuBBoxIndices->cleanUp( pRhi );
    delete m_gpuBBoxIndices;
    m_gpuBBoxIndices = nullptr;
  }

  if ( m_gpuBBoxVertices )
  {
    m_gpuBBoxVertices->cleanUp( pRhi );
    delete m_gpuBBoxVertices;
    m_gpuBBoxVertices = nullptr;
  }
}


void DecalEngine::createBoundingBox(VulkanRHI* pRhi)
{
  m_gpuBBoxVertices = new VertexBuffer();
  m_gpuBBoxIndices = new IndexBuffer();
  
  m_gpuBBoxVertices->initialize(pRhi, bboxPositions.size(), sizeof(Vector4), bboxPositions.data());
  m_gpuBBoxIndices->initialize(pRhi, bboxIndices.size(), sizeof(u16), bboxPositions.data());

  
}


void DecalEngine::buildDecals(CommandBuffer* offscreenCmdBuffer)
{
  R_ASSERT(offscreenCmdBuffer, "Offscreen command buffer is null prior to building decals.");
  R_ASSERT(offscreenCmdBuffer->recording(), "Offscreen command buffer not in recording state.");


}
} // Recluse