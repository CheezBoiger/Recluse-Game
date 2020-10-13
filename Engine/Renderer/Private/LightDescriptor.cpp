// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "LightDescriptor.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "TextureType.hpp"
#include "GlobalDescriptor.hpp"
#include "MeshDescriptor.hpp"
#include "MaterialDescriptor.hpp"
#include "Mesh.hpp"
#include "Renderer.hpp"
#include "Material.hpp"
#include "MeshData.hpp"
#include "Vertex.hpp"
#include "VertexDescription.hpp"

#include "Core/Utility/Profile.hpp"

#include "Filesystem/Filesystem.hpp"

#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Framebuffer.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Shader.hpp"

#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Time.hpp"

#include <array>


namespace Recluse {


std::vector<GraphicsPipeline*> ShadowMapSystem::k_pSkinnedPipeline;
std::vector<GraphicsPipeline*> ShadowMapSystem::k_pSkinnedMorphPipeline;
std::vector<GraphicsPipeline*> ShadowMapSystem::k_pStaticMorphPipeline;
std::vector<GraphicsPipeline*> ShadowMapSystem::k_pStaticPipeline;

std::vector<GraphicsPipeline*>  ShadowMapSystem::k_pSkinnedPipelineOpaque;
std::vector<GraphicsPipeline*>  ShadowMapSystem::k_pSkinnedMorphPipelineOpaque;
std::vector<GraphicsPipeline*>  ShadowMapSystem::k_pStaticMorphPipelineOpaque;
std::vector<GraphicsPipeline*>  ShadowMapSystem::k_pStaticPipelineOpaque;

GraphicsPipeline* ShadowMapSystem::k_pStaticSkinnedMorphPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticStaticPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticStaticMorphPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticSkinnedPipeline = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticSkinnedPipelineOpaque = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticSkinnedMorphPipelineOpaque = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticStaticPipelineOpaque = nullptr;
GraphicsPipeline* ShadowMapSystem::k_pStaticStaticMorphPipelineOpaque = nullptr;
RenderPass*       ShadowMapSystem::k_pDynamicRenderPass = nullptr;
RenderPass*       ShadowMapSystem::k_pStaticRenderPass = nullptr;
RenderPass*       ShadowMapSystem::k_pCascadeRenderPass = nullptr;
const U32         ShadowMapSystem::kTotalCascades = 4u;
const U32         ShadowMapSystem::kMaxShadowDim = 4196u;


U32 LightBuffer::maxNumDirectionalLights()
{
  return MAX_DIRECTIONAL_LIGHTS;
}


U32 LightBuffer::maxNumPointLights()
{
  return MAX_POINT_LIGHTS;
}


U32 LightBuffer::maxNumSpotLights()
{
  return MAX_SPOT_LIGHTS;
}


void initializeShadowMapRenderPass(RenderPass* renderPass, VkFormat format, VkSampleCountFlagBits samples);
void initializeShadowMapFrameBuffer(FrameBuffer* frameBuffer, RenderPass* renderPass, Texture* texture, VkImageView view);
void initializeShadowPipeline(VulkanRHI* pRhi, GraphicsPipeline* pipeline,
  RenderPass* pRenderPass,
  U32 width,
  U32 height,
  std::vector<VkVertexInputBindingDescription>& bindings,
  std::vector<VkVertexInputAttributeDescription>& attributes,
  B32 skinned,
  B32 morphTargets = false,
  B32 opaque = false,
  B32 subpassIdx = 0);


ShadowMapSystem::ShadowMapSystem()
  : m_pStaticMap(nullptr)
  //, m_pDynamicMap(nullptr)
  , m_pStaticFrameBuffer(nullptr)
  //, m_pDynamicFrameBuffer(nullptr)
  , m_pOmniMapArray(nullptr)
  , m_pSpotLightMapArray(nullptr)
  , m_pStaticOmniMapArray(nullptr)
  , m_pStaticSpotLightMapArray(nullptr)
#if 0
  , m_pDynamicRenderPass(nullptr)
  , m_pSkinnedPipeline(nullptr)
  , m_pStaticSkinnedPipeline(nullptr)
  , m_pStaticSkinnedMorphPipeline(nullptr)
  , m_pStaticStaticPipeline(nullptr)
  , m_pStaticStaticMorphPipeline(nullptr)
  , m_pSkinnedMorphPipeline(nullptr)
  , m_pStaticMorphPipeline(nullptr)
  , m_pStaticPipeline(nullptr)
  , m_pStaticRenderPass(nullptr)
#endif 
  , m_staticMapNeedsUpdate(true)
  , m_rShadowViewportDim(15.0f)
  , m_rShadowLightSz(1.0f)
  , m_rSoftShadowNear(0.135f)
  , m_staticShadowViewportDim(100.0f)
  , m_numCascadeShadowMaps(kTotalCascades)
{ 
}


void ShadowMapSystem::initializeShadowPipelines(VulkanRHI* pRhi, U32 numCascades)
{
  if (!k_pStaticRenderPass) {
    k_pStaticRenderPass = pRhi->createRenderPass();
    initializeShadowMapRenderPass(k_pStaticRenderPass, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
  }

  if (!k_pDynamicRenderPass) {
    k_pDynamicRenderPass = pRhi->createRenderPass();
    initializeShadowMapRenderPass(k_pDynamicRenderPass, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
  }

  {
    VkRenderPassCreateInfo renderPass = {};
    std::vector<VkAttachmentDescription> attachments(numCascades);
    std::vector<VkSubpassDescription> subpasses(numCascades);
    std::vector<VkSubpassDependency>   dependencies(numCascades);
    std::vector<VkAttachmentReference> references(numCascades);

    for (U32 i = 0; i < subpasses.size(); ++i) {
      VkAttachmentReference depth = {};
      depth.attachment = i;
      depth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      references[i] = depth;
      subpasses[i] = {};
      subpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpasses[i].pColorAttachments = nullptr;
      subpasses[i].pDepthStencilAttachment = &references[i];
    }

    for (U32 i = 0; i < dependencies.size(); ++i) {
      dependencies[i] = {};
      dependencies[i].srcSubpass = i - 1;
      if (i == 0) dependencies[i].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[i].dstSubpass = (i == 0) ? 0 : i;
      dependencies[i].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependencies[i].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependencies[i].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependencies[i].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    for (U32 i = 0; i < attachments.size(); ++i) {
      attachments[i] = {};
      attachments[i].format = VK_FORMAT_D32_SFLOAT;
      attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      // TODO(): We want to make our shadowmap shader readonly!! this will require 
      // Some fixes to the PBR shader pipes that read this rendertexture.
      attachments[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
    }

    renderPass.pSubpasses = subpasses.data();
    renderPass.subpassCount = static_cast<U32>(subpasses.size());
    renderPass.dependencyCount = static_cast<U32>(dependencies.size());
    renderPass.pDependencies = dependencies.data();
    renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPass.pAttachments = attachments.data();
    renderPass.attachmentCount = static_cast<U32>(attachments.size());

    k_pCascadeRenderPass = pRhi->createRenderPass();
    k_pCascadeRenderPass->initialize(renderPass);
  }
  k_pSkinnedPipeline.resize(numCascades);
  k_pSkinnedPipelineOpaque.resize(numCascades);
  k_pSkinnedMorphPipeline.resize(numCascades);
  k_pSkinnedMorphPipelineOpaque.resize(numCascades);
  k_pStaticMorphPipeline.resize(numCascades);
  k_pStaticMorphPipelineOpaque.resize(numCascades);
  k_pStaticPipeline.resize(numCascades);
  k_pStaticPipelineOpaque.resize(numCascades);

  for (U32 i = 0; i < numCascades; ++i) {
    {
      std::vector<VkVertexInputBindingDescription> bindings = { SkinnedVertexDescription::GetBindingDescription() };
      k_pSkinnedPipeline[i] = pRhi->createGraphicsPipeline();
      k_pSkinnedPipelineOpaque[i] = pRhi->createGraphicsPipeline();
      initializeShadowPipeline(pRhi, k_pSkinnedPipeline[i], k_pCascadeRenderPass, 1, 1,
        bindings, SkinnedVertexDescription::GetVertexAttributes(), true, false, false, i);
      initializeShadowPipeline(pRhi, k_pSkinnedPipelineOpaque[i], k_pCascadeRenderPass, 1, 1,
        bindings, SkinnedVertexDescription::GetVertexAttributes(), true, false, true, i);
    }
    {
      k_pSkinnedMorphPipeline[i] = pRhi->createGraphicsPipeline();
      k_pSkinnedMorphPipelineOpaque[i] = pRhi->createGraphicsPipeline();
      initializeShadowPipeline(pRhi, k_pSkinnedMorphPipeline[i], k_pCascadeRenderPass, 1, 1,
        MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription()),
        MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes()),
        true, true, false, i);
      initializeShadowPipeline(pRhi, k_pSkinnedMorphPipelineOpaque[i], k_pCascadeRenderPass, 1, 1,
        MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription()),
        MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes()),
        true, true, true, i);

    }
    {
      std::vector<VkVertexInputBindingDescription> bindings = { StaticVertexDescription::GetBindingDescription() };
      k_pStaticPipeline[i] = pRhi->createGraphicsPipeline();
      k_pStaticPipelineOpaque[i] = pRhi->createGraphicsPipeline();
      initializeShadowPipeline(pRhi, k_pStaticPipeline[i], k_pCascadeRenderPass, 1, 1,
        bindings, StaticVertexDescription::GetVertexAttributes(), false, false, false, i);
      initializeShadowPipeline(pRhi, k_pStaticPipelineOpaque[i], k_pCascadeRenderPass, 1, 1,
        bindings, StaticVertexDescription::GetVertexAttributes(), false, false, true, i);
    }
    {
      k_pStaticMorphPipeline[i] = pRhi->createGraphicsPipeline();
      k_pStaticMorphPipelineOpaque[i] = pRhi->createGraphicsPipeline();
      initializeShadowPipeline(pRhi, k_pStaticMorphPipeline[i], k_pCascadeRenderPass, 1, 1,
        MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription()),
        MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes()),
        false, true, false, i);
      initializeShadowPipeline(pRhi, k_pStaticMorphPipelineOpaque[i], k_pCascadeRenderPass, 1, 1,
        MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription()),
        MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes()),
        false, true, true, i);
    }
  }
  if (!k_pStaticSkinnedPipeline) {
    std::vector<VkVertexInputBindingDescription> bindings = { SkinnedVertexDescription::GetBindingDescription() };
    k_pStaticSkinnedPipeline = pRhi->createGraphicsPipeline();
    initializeShadowPipeline(pRhi, k_pStaticSkinnedPipeline, k_pStaticRenderPass, 1, 1,
      bindings, SkinnedVertexDescription::GetVertexAttributes(), true);
  }

  if (!k_pStaticSkinnedMorphPipeline) {
    k_pStaticSkinnedMorphPipeline = pRhi->createGraphicsPipeline();
    initializeShadowPipeline(pRhi, k_pStaticSkinnedMorphPipeline, k_pStaticRenderPass, 1, 1,
      MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription()),
      MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes()), true, true);
  }

  if (!k_pStaticStaticPipeline) {
    std::vector<VkVertexInputBindingDescription> bindings = { StaticVertexDescription::GetBindingDescription() };
    k_pStaticStaticPipeline = pRhi->createGraphicsPipeline();
    initializeShadowPipeline(pRhi, k_pStaticStaticPipeline, k_pStaticRenderPass, 1, 1,
      bindings, StaticVertexDescription::GetVertexAttributes(), false);
  }

  if (!k_pStaticStaticMorphPipeline) {
    k_pStaticStaticMorphPipeline = pRhi->createGraphicsPipeline();
    initializeShadowPipeline(pRhi, k_pStaticStaticMorphPipeline, k_pStaticRenderPass, 1, 1,
      MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription()),
      MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes()),
      false, true);
  }
}


void ShadowMapSystem::cleanUpShadowPipelines(VulkanRHI* pRhi)
{
  if (k_pStaticRenderPass) {
    pRhi->freeRenderPass(k_pStaticRenderPass);
    k_pStaticRenderPass = nullptr;
  }

  if (k_pCascadeRenderPass) {
    pRhi->freeRenderPass(k_pCascadeRenderPass);
    k_pCascadeRenderPass = nullptr;
  }

  if (k_pDynamicRenderPass) {
    pRhi->freeRenderPass(k_pDynamicRenderPass);
    k_pDynamicRenderPass = nullptr;
  }

  for (U32 i = 0; i < k_pSkinnedPipeline.size(); ++i) {
    if (k_pSkinnedPipeline[i]) {
      pRhi->freeGraphicsPipeline(k_pSkinnedPipeline[i]);
      k_pSkinnedPipeline[i] = nullptr;
    }

    if (k_pSkinnedPipelineOpaque[i]) {
      pRhi->freeGraphicsPipeline(k_pSkinnedPipelineOpaque[i]);
      k_pSkinnedPipelineOpaque[i] = nullptr;
    }

    if (k_pSkinnedMorphPipeline[i]) {
      pRhi->freeGraphicsPipeline(k_pSkinnedMorphPipeline[i]);
      k_pSkinnedMorphPipeline[i] = nullptr;
    }

    if (k_pSkinnedMorphPipelineOpaque[i]) {
      pRhi->freeGraphicsPipeline(k_pSkinnedMorphPipelineOpaque[i]);
      k_pSkinnedMorphPipelineOpaque[i] = nullptr;
    }

    if (k_pStaticPipeline[i]) {
      pRhi->freeGraphicsPipeline(k_pStaticPipeline[i]);
      k_pStaticPipeline[i] = nullptr;
    }

    if (k_pStaticPipelineOpaque[i]) {
      pRhi->freeGraphicsPipeline(k_pStaticPipelineOpaque[i]);
      k_pStaticPipelineOpaque[i] = nullptr;
    }

    if (k_pStaticMorphPipeline[i]) {
      pRhi->freeGraphicsPipeline(k_pStaticMorphPipeline[i]);
      k_pStaticMorphPipeline[i] = nullptr;
    }

    if (k_pStaticMorphPipelineOpaque[i]) {
      pRhi->freeGraphicsPipeline(k_pStaticMorphPipelineOpaque[i]);
      k_pStaticMorphPipelineOpaque[i] = nullptr;
    }
  }

  if (k_pStaticSkinnedPipeline) {
    pRhi->freeGraphicsPipeline(k_pStaticSkinnedPipeline);
    k_pStaticSkinnedPipeline = nullptr;
  }

  if (k_pStaticSkinnedMorphPipeline) {
    pRhi->freeGraphicsPipeline(k_pStaticSkinnedMorphPipeline);
    k_pStaticSkinnedMorphPipeline = nullptr;
  }

  if (k_pStaticStaticPipeline) {
    pRhi->freeGraphicsPipeline(k_pStaticStaticPipeline);
    k_pStaticStaticPipeline = nullptr;
  }

  if (k_pStaticStaticMorphPipeline) {
    pRhi->freeGraphicsPipeline(k_pStaticStaticMorphPipeline);
    k_pStaticStaticMorphPipeline = nullptr;
  }
}


ShadowMapSystem::~ShadowMapSystem()
{
  DEBUG_OP(
/*
    if (m_pDynamicFrameBuffer) {
      R_DEBUG(rWarning, "Dynamic Frame Buffer not destroyed prior to destruct call.");
    }
*/
    if (m_pStaticFrameBuffer) {
      R_DEBUG(rWarning, "Static frame buffer not destroyed prior to destruct call.");
    }
    if (m_pStaticMap) {
      R_DEBUG(rWarning, "Static map not destroyed prior to destruct call.");
    }
/*
    if (m_pDynamicMap) {
      R_DEBUG(rWarning, "Dynamic map not destroyed prior to destruct call.");
    }
*/
    for (U32 i = 0; i < m_pLightViewDescriptorSets.size(); ++i) {
      if (m_pLightViewDescriptorSets[i]) {
        R_DEBUG(rWarning, "Light view descriptor set not destroyed prior to destruct call.");
      }
      if (m_pLightViewBuffers[i]) {
        R_DEBUG(rWarning, "Light view buffer not destroyed prior to destruct call.");
      }

      if ( m_pStaticLightViewBuffers[i] ) {
        R_DEBUG(rWarning, "Static Light View Buffer not destroyed prior to destruct call.");
      }
    }
  );
}


void ShadowMapSystem::initializeSpotLightShadowMapArray(VulkanRHI* pRhi, U32 layers, U32 resolution)
{
  if (layers > MAX_SPOT_LIGHTS) {
    R_DEBUG(rWarning, "SpotLight layers init is greater than number of spotlights allowed! Defaulting to Max Spot Light count for layers\n");
    layers = MAX_SPOT_LIGHTS;
  }

  {
    VkImageCreateInfo imageCi = { };
    imageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCi.arrayLayers = layers;
    imageCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCi.mipLevels = 1;
    imageCi.format = VK_FORMAT_D32_SFLOAT;
    imageCi.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCi.imageType = VK_IMAGE_TYPE_2D;
    imageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCi.extent.depth = 1;
    imageCi.extent.height = resolution;
    imageCi.extent.width = resolution;
 
    VkImageViewCreateInfo viewCi = { };
    viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCi.format = VK_FORMAT_D32_SFLOAT;
    viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewCi.subresourceRange.baseArrayLayer = 0;
    viewCi.subresourceRange.baseMipLevel = 0;
    viewCi.subresourceRange.layerCount = layers;
    viewCi.subresourceRange.levelCount = 1;
    viewCi.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;  
  
    if ( !m_pSpotLightMapArray ) {
      m_pSpotLightMapArray = pRhi->createTexture();
      m_pSpotLightMapArray->initialize(imageCi, viewCi);
    }
  
    if ( !m_pStaticSpotLightMapArray ) {
      m_pStaticSpotLightMapArray = pRhi->createTexture();
      m_pStaticSpotLightMapArray->initialize(imageCi, viewCi);
    }
  }
  {
    DescriptorSetLayout* pLightSpaceLayout = LightViewDescriptorSetLayoutKey;
    VkImageViewCreateInfo viewCi = { };
    viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCi.image = m_pSpotLightMapArray->getImage();
    viewCi.subresourceRange.baseMipLevel = 0;
    viewCi.subresourceRange.layerCount = 1;
    viewCi.subresourceRange.levelCount = 1;
    viewCi.format = VK_FORMAT_D32_SFLOAT;
    
    // This may require us to have to lower the allowed spotlights rendering.
    m_spotLightShadowMaps.resize(layers);
    for ( U32 i = 0; i < m_spotLightShadowMaps.size(); ++i ) {
      ShadowMapLayer& layerView = m_spotLightShadowMaps[i];
      viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      viewCi.subresourceRange.baseArrayLayer = i;
      layerView._view = pRhi->createImageView(viewCi);
    }
  }
}


void ShadowMapSystem::cleanUpSpotLightShadowMapArray(VulkanRHI* pRhi)
{
  for ( U32 i = 0; i < m_spotLightShadowMaps.size(); ++i ) {
    ShadowMapLayer& layerView = m_spotLightShadowMaps[i];
    pRhi->freeImageView(layerView._view);
    layerView._view = nullptr;
  }

  if (m_pSpotLightMapArray) {
    pRhi->freeTexture(m_pSpotLightMapArray);
    m_pSpotLightMapArray = nullptr;
  }
  if (m_pStaticSpotLightMapArray) {
    pRhi->freeTexture(m_pStaticSpotLightMapArray);
    m_pStaticSpotLightMapArray = nullptr;
  }
}


void ShadowMapSystem::initializeCascadeShadowMap(Renderer* pRenderer, U32 resolution)
{
  /*
  m_pCascadeDescriptorSet = pRhi->CreateDescriptorSet();
  m_pCascadeLightViewBuffer = pRhi->CreateBuffer();
  {
    VkBufferCreateInfo bCi = { };
    bCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bCi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bCi.size = VkDeviceSize(sizeof(LightViewCascadeSpace));
    m_pCascadeLightViewBuffer->initialize(bCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    m_pCascadeLightViewBuffer->Map();
  }
  {
    m_pCascadeDescriptorSet->allocate(pRhi->DescriptorPool(), LightViewDescriptorSetLayoutKey);
    VkDescriptorBufferInfo b = { };
    b.buffer = m_pCascadeLightViewBuffer->NativeBuffer();
    b.offset = 0;
    b.range = VkDeviceSize(sizeof(LightViewCascadeSpace));
    std::array<VkWriteDescriptorSet, 2> writes;
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    m_pCascadeDescriptorSet->Update(writes.size(), writes.data());
  }
  */
  
  U32 sDim = resolution;
  VkImageCreateInfo ImageCi = {};
  ImageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImageCi.arrayLayers = m_numCascadeShadowMaps;
  ImageCi.extent.width = sDim;
  ImageCi.extent.height = sDim;
  ImageCi.extent.depth = 1;
  ImageCi.format = VK_FORMAT_D32_SFLOAT;
  ImageCi.imageType = VK_IMAGE_TYPE_2D;
  ImageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ImageCi.mipLevels = 1;
  ImageCi.samples = VK_SAMPLE_COUNT_1_BIT;
  ImageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImageCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  ImageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImageViewCreateInfo ViewCi = {};
  ViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ViewCi.components = {};
  ViewCi.format = ImageCi.format;
  ViewCi.subresourceRange = {};
  ViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  ViewCi.subresourceRange.baseArrayLayer = 0;
  ViewCi.subresourceRange.baseMipLevel = 0;
  ViewCi.subresourceRange.layerCount = m_numCascadeShadowMaps;
  ViewCi.subresourceRange.levelCount = 1;
  ViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

  m_pCascadeShadowMapD.resize(pRenderer->getResourceBufferCount());
  m_cascades.resize(pRenderer->getResourceBufferCount());
  m_pCascadeFrameBuffers.resize(pRenderer->getResourceBufferCount());
  VulkanRHI* pRhi = pRenderer->getRHI();

  for (U32 i = 0; i < m_pCascadeShadowMapD.size(); ++i) {
  m_pCascadeShadowMapD[i] = pRhi->createTexture();
    RDEBUG_SET_VULKAN_NAME(m_pCascadeShadowMapD[i], "Cascade Shadow Map");
    m_pCascadeShadowMapD[i]->initialize(ImageCi, ViewCi);
  }

  ViewCi.subresourceRange.layerCount = 1;
  for(U32 i = 0; i < m_cascades.size(); ++i) {
    std::vector<Cascade>& cascade = m_cascades[i];
    cascade.resize(m_numCascadeShadowMaps);

    VkFramebufferCreateInfo fCi = {};
    fCi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fCi.layers = 1;
    fCi.width = sDim;
    fCi.height = sDim;
    fCi.attachmentCount = m_numCascadeShadowMaps;

    std::vector<VkImageView> cascadeViews(m_numCascadeShadowMaps);
    for (U32 j = 0; j < cascade.size(); ++j) {
      ViewCi.subresourceRange.baseArrayLayer = j;
      ViewCi.image = m_pCascadeShadowMapD[i]->getImage();
      cascade[j]._view = pRhi->createImageView(ViewCi);
      cascadeViews[j] = cascade[j]._view->getHandle();
    }

    fCi.pAttachments = cascadeViews.data();
    fCi.attachmentCount = static_cast<U32>(cascadeViews.size());
    m_pCascadeFrameBuffers[i] = pRhi->createFrameBuffer();
    m_pCascadeFrameBuffers[i]->Finalize(fCi, k_pCascadeRenderPass);
  }

  if (resolution == 1u) {
    for (U32 i = 0; i < m_pCascadeShadowMapD.size(); ++i) {
      CommandBuffer cmd;
      cmd.SetOwner(pRhi->logicDevice()->getNative());
      cmd.allocate(pRhi->getGraphicsCmdPool(i, 0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
      VkCommandBufferBeginInfo b = {};
      b.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      b.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      cmd.begin(b);

      VkImageMemoryBarrier imgBarrier = { };
      imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imgBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      imgBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      imgBarrier.image = m_pCascadeShadowMapD[i]->getImage();
      imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imgBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      imgBarrier.subresourceRange.baseArrayLayer = 0;
      imgBarrier.subresourceRange.baseMipLevel = 0;
      imgBarrier.subresourceRange.layerCount = m_pCascadeShadowMapD[i]->getArrayLayers();
      imgBarrier.subresourceRange.levelCount = 1;

      cmd.pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          0, 0, nullptr, 0, nullptr, 
                          1, & imgBarrier);
      cmd.end();

      VkSubmitInfo s = { };
      s.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      s.commandBufferCount = 1;
      VkCommandBuffer hh[] = {cmd.getHandle()}; 
      s.pCommandBuffers = hh;

      pRhi->graphicsSubmit(0, 1, &s);
      pRhi->graphicsWaitIdle(0);
      cmd.free();
    }
  }
}


void ShadowMapSystem::initializeShadowMapD(Renderer* pRenderer, U32 resolution)
{
  U32 dDim = resolution;
  // ShadowMap is a depth image.
  VkImageCreateInfo ImageCi = {};
  ImageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImageCi.arrayLayers = 1;
  ImageCi.extent.width = dDim;
  ImageCi.extent.height = dDim;
  ImageCi.extent.depth = 1;
  ImageCi.format = VK_FORMAT_D32_SFLOAT;
  ImageCi.imageType = VK_IMAGE_TYPE_2D;
  ImageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ImageCi.mipLevels = 1;
  ImageCi.samples = VK_SAMPLE_COUNT_1_BIT;
  ImageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImageCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  ImageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImageViewCreateInfo ViewCi = {};
  ViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ViewCi.components = {};
  ViewCi.format = ImageCi.format;
  ViewCi.subresourceRange = {};
  ViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  ViewCi.subresourceRange.baseArrayLayer = 0;
  ViewCi.subresourceRange.baseMipLevel = 0;
  ViewCi.subresourceRange.layerCount = 1;
  ViewCi.subresourceRange.levelCount = 1;
  ViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;

  // Light view buffer creation.
  VulkanRHI* pRhi = pRenderer->getRHI();
  VkBufferCreateInfo bufferCI = {};
  VkDeviceSize dSize = sizeof(LightViewCascadeSpace);
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCI.size = dSize;
  m_pLightViewBuffers.resize(pRenderer->getResourceBufferCount());
  m_pStaticLightViewBuffers.resize(pRenderer->getResourceBufferCount());
  for (U32 i = 0; i < m_pLightViewBuffers.size(); ++i) {
    m_pLightViewBuffers[i] = pRhi->createBuffer();
    m_pStaticLightViewBuffers[i] = pRhi->createBuffer();
    m_pLightViewBuffers[i]->initialize(pRhi->logicDevice()->getNative(),
                                       bufferCI, 
                                       PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
    m_pStaticLightViewBuffers[i]->initialize(pRhi->logicDevice()->getNative(),
                                             bufferCI, 
                                             PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
  }
/*
  if (!m_pDynamicMap) {
    m_pDynamicMap = pRhi->CreateTexture();
    RDEBUG_SET_VULKAN_NAME(m_pDynamicMap, "Dynamic Shadowmap.");
    R_DEBUG(rNotify, "Dynamic Shadow map size: ");
    R_DEBUG(rNormal, std::to_string(ImageCi.extent.width) + "x" + std::to_string(ImageCi.extent.height) + "\n");
    m_pDynamicMap->initialize(ImageCi, ViewCi);
  }
*/
  if (!m_pStaticMap) {
    m_pStaticMap = pRhi->createTexture();
    RDEBUG_SET_VULKAN_NAME(m_pStaticMap, "Static Shadowmap.");
    U32 sDim = resolution;
    ImageCi.extent.width = sDim;
    ImageCi.extent.height = sDim;
    R_DEBUG(rNotify, "Static Shadow map size: ");
    R_DEBUG(rNormal, std::to_string(ImageCi.extent.width) + "x" + std::to_string(ImageCi.extent.height) + "\n");
    m_pStaticMap->initialize(ImageCi, ViewCi);
  }
}


void ShadowMapSystem::initialize(Renderer* pRenderer, const GraphicsConfigParams* params)
{
  m_shadowQuality = params->_Shadows;

  if ( params->_numberCascadeShadowMaps > kTotalCascades ) 
  {
    m_numCascadeShadowMaps = kTotalCascades;
  } 
  else 
  {
    m_numCascadeShadowMaps = params->_numberCascadeShadowMaps;
  }

  VulkanRHI* pRhi = pRenderer->getRHI();
  initializeShadowMapD(pRenderer, params->_cascadeShadowMapRes);
  initializeCascadeShadowMap(pRenderer, params->_cascadeShadowMapRes);
  initializeShadowMapDescriptors(pRenderer);  
  initializeSpotLightShadowMapArray(pRhi, params->_shadowMapArrayRes);
  
  enableStaticMapSoftShadows(params->_enableSoftShadows);
  enableDynamicMapSoftShadows(params->_enableSoftShadows);
}


void ShadowMapSystem::transitionEmptyShadowMap(CommandBuffer* cmdBuffer, U32 resourceIndex)
{
  Texture* pCascadeShadowMap = m_pCascadeShadowMapD[resourceIndex];

  VkImageMemoryBarrier imgBarrier = { };
  imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  imgBarrier.image = pCascadeShadowMap->getImage();
  imgBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  imgBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  imgBarrier.pNext = nullptr;

  imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  imgBarrier.subresourceRange.baseArrayLayer = 0;
  imgBarrier.subresourceRange.baseMipLevel = 0;
  imgBarrier.subresourceRange.layerCount = m_numCascadeShadowMaps;
  imgBarrier.subresourceRange.levelCount = 1;

  cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                              0, 
                              0, nullptr, 
                              0, nullptr, 
                              1, &imgBarrier);
}


void ShadowMapSystem::enableDynamicMapSoftShadows(B32 enable)
{
  m_viewSpace._shadowTechnique = Vector4(R32(enable), R32(enable), R32(enable), R32(enable));
  m_cascadeViewSpace._shadowTechnique = Vector4(R32(enable), R32(enable), R32(enable), R32(enable));
}

void ShadowMapSystem::enableStaticMapSoftShadows(B32 enable)
{
  m_staticViewSpace._shadowTechnique = Vector4(R32(enable), R32(enable), R32(enable), R32(enable));
}


void initializeShadowPipeline(VulkanRHI* pRhi, GraphicsPipeline* pipeline, 
  RenderPass* pRenderPass,
  U32 width,
  U32 height, 
  std::vector<VkVertexInputBindingDescription>& bindings, 
  std::vector<VkVertexInputAttributeDescription>& attributes,
  B32 skinned,
  B32 morphTargets,
  B32 opaque,
  U32 subpassIdx)
{
  VkPipelineInputAssemblyStateCreateInfo assemblyCI = {};
  assemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  assemblyCI.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.height = static_cast<R32>(height);
  viewport.width = static_cast<R32>(width);

  VkRect2D scissor = {};
  scissor.extent = { width, height };
  scissor.offset = { 0, 0 };

  VkPipelineViewportStateCreateInfo viewportCI = {};
  viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportCI.viewportCount = 1;
  viewportCI.pViewports = &viewport;
  viewportCI.scissorCount = 1;
  viewportCI.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizerCI = CreateRasterInfo(
    VK_POLYGON_MODE_FILL,
    VK_FALSE,
    SHADOW_CULL_MODE,
    SHADOW_WINDING_ORDER,
    1.0f,
    VK_FALSE,
    VK_FALSE
  );

  VkPipelineMultisampleStateCreateInfo msCI = {};
  msCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  msCI.sampleShadingEnable = VK_FALSE;
  msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  msCI.minSampleShading = 1.0f;
  msCI.pSampleMask = nullptr;
  msCI.alphaToOneEnable = VK_FALSE;
  msCI.alphaToCoverageEnable = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depthStencilCI = {};
  depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilCI.depthTestEnable = VK_TRUE;
  depthStencilCI.depthWriteEnable = VK_TRUE;
  depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilCI.depthBoundsTestEnable = pRhi->depthBoundsAllowed();
  depthStencilCI.minDepthBounds = 0.0f;
  depthStencilCI.maxDepthBounds = 1.0f;
  depthStencilCI.stencilTestEnable = VK_FALSE;
  depthStencilCI.back.compareMask = 0xff;
  depthStencilCI.back.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilCI.back.passOp = VK_STENCIL_OP_REPLACE;
  depthStencilCI.back.failOp = VK_STENCIL_OP_ZERO;
  depthStencilCI.front = {};

  std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachments;
  colorBlendAttachments[0] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[1] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[2] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[3] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  VkPipelineColorBlendStateCreateInfo colorBlendCI = CreateBlendStateInfo(
    static_cast<U32>(colorBlendAttachments.size()),
    colorBlendAttachments.data(),
    VK_FALSE,
    VK_LOGIC_OP_COPY
  );

  VkDynamicState dynamicStates[2] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicCI = {};
  dynamicCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicCI.dynamicStateCount = 2;
  dynamicCI.pDynamicStates = dynamicStates;

  VkPipelineVertexInputStateCreateInfo vertexCI = {};
  vertexCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexCI.vertexBindingDescriptionCount = static_cast<U32>(bindings.size());
  vertexCI.pVertexBindingDescriptions = bindings.data();
  vertexCI.vertexAttributeDescriptionCount = static_cast<U32>(attributes.size());
  vertexCI.pVertexAttributeDescriptions = attributes.data();

  VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = {};
  GraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  GraphicsPipelineInfo.pColorBlendState = &colorBlendCI;
  GraphicsPipelineInfo.pDepthStencilState = &depthStencilCI;
  GraphicsPipelineInfo.pInputAssemblyState = &assemblyCI;
  GraphicsPipelineInfo.pRasterizationState = &rasterizerCI;
  GraphicsPipelineInfo.pMultisampleState = &msCI;
  GraphicsPipelineInfo.pVertexInputState = &vertexCI;
  GraphicsPipelineInfo.pViewportState = &viewportCI;
  GraphicsPipelineInfo.pTessellationState = nullptr;
  GraphicsPipelineInfo.pDynamicState = &dynamicCI;
  GraphicsPipelineInfo.subpass = 0;
  GraphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  // TODO(): initialize shadow map pipeline.
  VkPipelineLayoutCreateInfo PipeLayout = {};
  std::array<VkDescriptorSetLayout, 3> DescLayouts;
  DescLayouts[0] = MeshSetLayoutKey->getLayout();
  DescLayouts[1] = MaterialSetLayoutKey->getLayout();
  DescLayouts[2] = BonesSetLayoutKey->getLayout();

  VkPushConstantRange pc = { };
  pc.offset = 0;
  pc.size = VkDeviceSize(sizeof(Matrix4));
  pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  PipeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipeLayout.pushConstantRangeCount = 1;
  PipeLayout.pPushConstantRanges = &pc;
  PipeLayout.setLayoutCount = static_cast<U32>((skinned ? DescLayouts.size() : DescLayouts.size() - 1));
  PipeLayout.pSetLayouts = DescLayouts.data();

  colorBlendCI.attachmentCount = 0;
  GraphicsPipelineInfo.renderPass = pRenderPass->getHandle();
  GraphicsPipelineInfo.subpass = subpassIdx;
  Shader* SmVert = pRhi->createShader();
  Shader* SmFrag = pRhi->createShader();

  RendererPass::loadShader((skinned ? 
    (morphTargets ? "DynamicDepth_MorphTargets.vert.spv" : DynamicShadowMapVertFileStr) :  
    (morphTargets ? "Depth_MorphTargets.vert.spv" : ShadowMapVertFileStr)), SmVert);
  RendererPass::loadShader(opaque ? 
    (ShadowMapFragOpaqueFileStr) : (ShadowMapFragFileStr), SmFrag);

  std::array<VkPipelineShaderStageCreateInfo, 2> Shaders;
  Shaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shaders[0].flags = 0;
  Shaders[0].pName = kDefaultShaderEntryPointStr;
  Shaders[0].pNext = nullptr;
  Shaders[0].pSpecializationInfo = nullptr;
  Shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  Shaders[0].module = SmVert->getHandle();

  Shaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shaders[1].flags = 0;
  Shaders[1].pName = kDefaultShaderEntryPointStr;
  Shaders[1].pNext = nullptr;
  Shaders[1].pSpecializationInfo = nullptr;
  Shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  Shaders[1].module = SmFrag->getHandle();

  GraphicsPipelineInfo.pStages = Shaders.data();
  GraphicsPipelineInfo.stageCount = static_cast<U32>(Shaders.size());

  pipeline->initialize(GraphicsPipelineInfo, PipeLayout);
  pRhi->freeShader(SmVert);
  pRhi->freeShader(SmFrag);
}


void initializeShadowMapRenderPass(RenderPass* renderPass, VkFormat format, VkSampleCountFlagBits samples)
{
  std::array<VkAttachmentDescription, 1> attachmentDescriptions;

  // Actual depth buffer to write onto.
  attachmentDescriptions[0] = CreateAttachmentDescription(
    format,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    samples
  );

  std::array<VkSubpassDependency, 2> dependencies;
  dependencies[0] = CreateSubPassDependency(
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  dependencies[1] = CreateSubPassDependency(
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  VkAttachmentReference depthRef = {};
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthRef.attachment = 0;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 0;
  subpass.pColorAttachments = nullptr;
  subpass.pDepthStencilAttachment = &depthRef;
  subpass.flags = 0;


  VkRenderPassCreateInfo renderPassCi = CreateRenderPassInfo(
    static_cast<U32>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    2,
    dependencies.data(),
    1,
    &subpass
  );

  renderPass->initialize(renderPassCi);
}


void initializeShadowMapFrameBuffer(FrameBuffer* frameBuffer, RenderPass* renderPass, Texture* texture, VkImageView view)
{
  std::array<VkImageView, 1> attachments;
  attachments[0] = view;

  VkFramebufferCreateInfo frameBufferCi = CreateFrameBufferInfo(
    texture->getWidth(),
    texture->getHeight(),
    nullptr,
    static_cast<U32>(attachments.size()),
    attachments.data(),
    1
  );

  frameBuffer->Finalize(frameBufferCi, renderPass);
}


void ShadowMapSystem::initializeShadowMapDescriptors(Renderer* pRenderer)
{
  DescriptorSetLayout* viewLayout = LightViewDescriptorSetLayoutKey;
  VulkanRHI* pRhi = pRenderer->getRHI();

  DEBUG_OP(
    if (!k_pStaticRenderPass || !k_pDynamicRenderPass) {
      R_DEBUG(rError, "No shadow render passes where initialized!\n");
    }
  );

  m_pLightViewDescriptorSets.resize(pRenderer->getResourceBufferCount());
  m_pStaticLightViewDescriptorSets.resize(pRenderer->getResourceBufferCount());

  for (U32 i = 0;i < m_pLightViewDescriptorSets.size(); ++i) {
    if (!m_pLightViewDescriptorSets[i]) {
      m_pLightViewDescriptorSets[i] = pRhi->createDescriptorSet();
      m_pLightViewDescriptorSets[i]->allocate(pRhi->descriptorPool(), viewLayout);
    }

    if ( !m_pStaticLightViewDescriptorSets[i] ) {
      m_pStaticLightViewDescriptorSets[i] = pRhi->createDescriptorSet();
      m_pStaticLightViewDescriptorSets[i]->allocate(pRhi->descriptorPool(), viewLayout);
    }

    VkDescriptorBufferInfo viewBuf = {};
    viewBuf.buffer = m_pLightViewBuffers[i]->getNativeBuffer();
    viewBuf.offset = 0;
    viewBuf.range = sizeof(LightViewCascadeSpace);

    VkDescriptorBufferInfo staticViewBuf = {};
    staticViewBuf.buffer = m_pStaticLightViewBuffers[i]->getNativeBuffer();
    staticViewBuf.offset = 0;
    staticViewBuf.range = sizeof(LightViewSpace);

    // TODO(): Once we create our shadow map, we will add it here.
    // This will pass the rendered shadow map to the pbr pipeline.
    VkDescriptorImageInfo globalShadowInfo = {};
    globalShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    globalShadowInfo.imageView = m_pCascadeShadowMapD[i]->getView();
    globalShadowInfo.sampler = _pSampler->getHandle(); 

    VkDescriptorImageInfo staticShadowInfo = { };
    staticShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    staticShadowInfo.imageView = m_pStaticMap->getView();
    staticShadowInfo.sampler = _pSampler->getHandle();

    std::array<VkWriteDescriptorSet, 2> writes;
    writes[0] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].dstArrayElement = 0;
    writes[0].dstBinding = 0;
    writes[0].dstSet = nullptr;
    writes[0].pImageInfo = nullptr;
    writes[0].pBufferInfo = &viewBuf;

    writes[1] = {};
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].dstArrayElement = 0;
    writes[1].pNext = nullptr;
    writes[1].pImageInfo = &globalShadowInfo;
    writes[1].dstBinding = 1;

    m_pLightViewDescriptorSets[i]->update(static_cast<U32>(writes.size()), writes.data());

    writes[1].pImageInfo = &staticShadowInfo;
    writes[0].pBufferInfo = &staticViewBuf;
    m_pStaticLightViewDescriptorSets[i]->update(static_cast<U32>(writes.size()), writes.data());
  }
/*
  if (!m_pDynamicFrameBuffer) {
    m_pDynamicFrameBuffer = pRhi->createFrameBuffer();
    initializeShadowMapFrameBuffer(m_pDynamicFrameBuffer, k_pDynamicRenderPass, m_pDynamicMap, m_pDynamicMap->getView());
  }
*/
  if (!m_pStaticFrameBuffer) {
    m_pStaticFrameBuffer = pRhi->createFrameBuffer();
    initializeShadowMapFrameBuffer(m_pStaticFrameBuffer, k_pStaticRenderPass, m_pStaticMap, m_pStaticMap->getView());
  }
  
  m_staticMapNeedsUpdate = true;
}


void ShadowMapSystem::generateDynamicShadowCmds(CommandBuffer* pCmdBuffer, CmdList<PrimitiveRenderCmd>& dynamicCmds, U32 resourceIndex)
{
  R_TIMED_PROFILE_RENDERER();

  auto& staticPipeline = k_pStaticPipeline;
  auto& dynamicPipeline = k_pSkinnedPipeline;
  auto& staticMorphPipeline = k_pStaticMorphPipeline;
  auto& dynamicMorphPipeline = k_pSkinnedMorphPipeline;
  auto& staticPipelineOpaque = k_pStaticPipelineOpaque;
  auto& dynamicPipelineOpaque = k_pSkinnedPipelineOpaque;
  auto& staticMorphPipelineOpaque = k_pStaticMorphPipelineOpaque;
  auto& dynamicMorphPipelineOpaque = k_pSkinnedMorphPipelineOpaque;
  DescriptorSet*    lightViewSet = m_pLightViewDescriptorSets[resourceIndex];
  FrameBuffer*      pFb = m_pCascadeFrameBuffers[resourceIndex];
  RenderPass*       pRp = k_pCascadeRenderPass;

  VkRenderPassBeginInfo renderPass = {};
  renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  //renderPass.framebuffer = m_cascades[0]._framebuffer->getHandle();
  //renderPass.renderPass = m_cascades[0]._framebuffer->RenderPassRef()->getHandle();
  renderPass.renderArea.extent = { pFb->getWidth(), pFb->getHeight() };
  renderPass.renderArea.offset = { 0, 0 };
  renderPass.renderPass = pRp->getHandle();
  renderPass.framebuffer = pFb->getHandle();

  VkClearValue depthValues[kTotalCascades];

  for (U32 i = 0; i < m_numCascadeShadowMaps; ++i) {
    depthValues[i].depthStencil = { 1.0f, 0 };
  }
  renderPass.clearValueCount = m_numCascadeShadowMaps;
  renderPass.pClearValues = depthValues;

  VkViewport viewport = {};
  viewport.height = (R32)pFb->getHeight();
  viewport.width = (R32)pFb->getWidth();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  VkRect2D scissor = {};
  scissor.extent = { pFb->getWidth(), pFb->getHeight() };
  scissor.offset = { 0, 0 };

  auto render = [&](PrimitiveRenderCmd& renderCmd, const Matrix4& lightVP, U32 subpassIdx) -> void {
    if (!renderCmd._pMeshDesc) return;
    if (!(renderCmd._config & CMD_RENDERABLE_BIT) || !(renderCmd._config & CMD_SHADOWS_BIT)) return;
    R_ASSERT(renderCmd._pMeshData, "Null data was passed to renderer.");
    MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
    B32 skinned = (renderCmd._config & CMD_SKINNED_BIT);
    B32 opaque = !(renderCmd._config & (CMD_TRANSPARENT_BIT | CMD_TRANSLUCENT_BIT));
    VkDescriptorSet descriptorSets[3];
    descriptorSets[0] = pMeshDesc->getCurrMeshSet(resourceIndex)->getHandle();
    descriptorSets[2] = skinned ? renderCmd._pJointDesc->getCurrJointSet(resourceIndex)->getHandle() : VK_NULL_HANDLE;

    GraphicsPipeline* pipeline = skinned ? 
      (opaque ? dynamicPipelineOpaque[subpassIdx] : dynamicPipeline[subpassIdx]) : 
      (opaque ? staticPipelineOpaque[subpassIdx] : staticPipeline[subpassIdx]);
    MeshData* mesh = renderCmd._pMeshData;
    VertexBuffer* vertex = mesh->getVertexData();
    IndexBuffer* index = mesh->getIndexData();
    VkBuffer buf = vertex->getHandle()->getNativeBuffer();
    VkDeviceSize offset[] = { 0 };
    pCmdBuffer->bindVertexBuffers(0, 1, &buf, offset);
    if ( renderCmd._config & CMD_MORPH_BIT ) {
      pipeline = skinned ? 
        (opaque ? dynamicMorphPipelineOpaque[subpassIdx] : dynamicMorphPipeline[subpassIdx]) : 
        (opaque ? staticMorphPipeline[subpassIdx] : staticMorphPipeline[subpassIdx]);
      R_ASSERT(renderCmd._pMorph0, "morph0 is null");
      R_ASSERT(renderCmd._pMorph1, "morph1 is null.");
      VkBuffer morph0 = renderCmd._pMorph0->getVertexData()->getHandle()->getNativeBuffer();
      VkBuffer morph1 = renderCmd._pMorph1->getVertexData()->getHandle()->getNativeBuffer();
      pCmdBuffer->bindVertexBuffers(1, 1, &morph0, offset);
      pCmdBuffer->bindVertexBuffers(2, 1, &morph1, offset);
    }

    pCmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getNative());
    pCmdBuffer->pushConstants(pipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4), &lightVP);
    pCmdBuffer->setViewPorts(0, 1, &viewport);
    pCmdBuffer->setScissor(0, 1, &scissor);

    if (index) {
      VkBuffer ind = index->getHandle()->getNativeBuffer();
      pCmdBuffer->bindIndexBuffer(ind, 0, getNativeIndexType(index->GetSizeType()));
    }

    Primitive* primitive = renderCmd._pPrimitive;
    descriptorSets[1] = primitive->_pMat->getNative()->CurrMaterialSet()->getHandle();
    pCmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, skinned ? 3 : 2, descriptorSets, 0, nullptr);
    if (index) {
      pCmdBuffer->drawIndexed(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0, 0);
    }
    else {
      pCmdBuffer->draw(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0);
    }
  };
/*
  pCmdBuffer->BeginRenderPass(renderPass, VK_SUBPASS_CONTENTS_INLINE);
  for (size_t i = 0; i < dynamicCmds.Size(); ++i) {
    PrimitiveRenderCmd& renderCmd = dynamicCmds[i];
    render(renderCmd, m_viewSpace._ViewProj);
  }
  pCmdBuffer->EndRenderPass();
*/
  pCmdBuffer->beginRenderPass(renderPass, VK_SUBPASS_CONTENTS_INLINE);
  for (U32 i = 0; i < m_cascades[resourceIndex].size(); ++i) {
    //renderPass.framebuffer = m_cascades[resourceIndex][i]._framebuffer->getHandle();
    //renderPass.renderPass = m_cascades[resourceIndex][i]._framebuffer->RenderPassRef()->getHandle();
    //renderPass.renderArea.extent = {  m_cascades[resourceIndex][i]._framebuffer->getWidth(), 
    //                                  m_cascades[resourceIndex][i]._framebuffer->getHeight() };
    for (size_t j = 0; j < dynamicCmds.Size(); ++j) {
      PrimitiveRenderCmd& renderCmd = dynamicCmds[j];
      render(renderCmd, m_cascadeViewSpace._ViewProj[i], i);
    }

    if (i+1 < m_cascades[resourceIndex].size()) {
      pCmdBuffer->nextSubpass(VK_SUBPASS_CONTENTS_INLINE);
    }
  }
  pCmdBuffer->endRenderPass();

  if (dynamicCmds.Size() == 0) {
    R_DEBUG(rNotify, "Empty dynamic map cmd buffer updated.\n");
    return;
  }
  R_DEBUG(rNotify, "Updated dynamic map cmd buffer. frame index: " << resourceIndex << "\n");
}


void ShadowMapSystem::generateStaticShadowCmds(CommandBuffer* pCmdBuffer, CmdList<PrimitiveRenderCmd>& staticCmds, U32 resourceIndex)
{
  R_TIMED_PROFILE_RENDERER();

  if ( !m_staticMapNeedsUpdate ) { return; }
  
  GraphicsPipeline* staticPipeline = k_pStaticStaticPipeline;
  GraphicsPipeline* dynamicPipeline = k_pStaticSkinnedPipeline;
  GraphicsPipeline* staticMorphPipeline = k_pStaticStaticMorphPipeline;
  GraphicsPipeline* dynamicMorphPipeline = k_pStaticSkinnedMorphPipeline;
  DescriptorSet*    lightViewSet = m_pStaticLightViewDescriptorSets[resourceIndex];

  VkRenderPassBeginInfo renderPass = {};
  renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPass.framebuffer = m_pStaticFrameBuffer->getHandle();
  renderPass.renderPass = m_pStaticFrameBuffer->RenderPassRef()->getHandle();
  renderPass.renderArea.extent = { m_pStaticFrameBuffer->getWidth(), m_pStaticFrameBuffer->getHeight() };
  renderPass.renderArea.offset = { 0, 0 };
  VkClearValue depthValue = {};
  depthValue.depthStencil = { 1.0f, 0 };
  renderPass.clearValueCount = 1;
  renderPass.pClearValues = &depthValue;

  VkViewport viewport = {};
  viewport.height = (R32)m_pStaticFrameBuffer->getHeight();
  viewport.width = (R32)m_pStaticFrameBuffer->getWidth();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  VkRect2D scissor = {};
  scissor.extent = { m_pStaticFrameBuffer->getWidth(), m_pStaticFrameBuffer->getHeight() };
  scissor.offset = { 0, 0 };

  auto render = [&](PrimitiveRenderCmd& renderCmd, const Matrix4& lightVP) -> void {
    if (!renderCmd._pMeshDesc) return;
    if (!(renderCmd._config & CMD_RENDERABLE_BIT) || !(renderCmd._config & CMD_SHADOWS_BIT)) return;
    R_ASSERT(renderCmd._pMeshData, "Null data was passed to renderer.");
    MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
    B32 skinned = (renderCmd._config & CMD_SKINNED_BIT);
    VkDescriptorSet descriptorSets[4];
    descriptorSets[0] = pMeshDesc->getCurrMeshSet(resourceIndex)->getHandle();
    descriptorSets[2] = skinned ? renderCmd._pJointDesc->getCurrJointSet(resourceIndex)->getHandle() : VK_NULL_HANDLE;

    GraphicsPipeline* pipeline = skinned ? dynamicPipeline : staticPipeline;
    MeshData* mesh = renderCmd._pMeshData;
    VertexBuffer* vertex = mesh->getVertexData();
    IndexBuffer* index = mesh->getIndexData();
    VkBuffer buf = vertex->getHandle()->getNativeBuffer();
    VkDeviceSize offset[] = { 0 };
    pCmdBuffer->bindVertexBuffers(0, 1, &buf, offset);
    if (renderCmd._config & CMD_MORPH_BIT) {
      pipeline = skinned ? dynamicMorphPipeline : staticMorphPipeline;
      R_ASSERT(renderCmd._pMorph0, "morph0 is null");
      R_ASSERT(renderCmd._pMorph1, "morph1 is null.");
      VkBuffer morph0 = renderCmd._pMorph0->getVertexData()->getHandle()->getNativeBuffer();
      VkBuffer morph1 = renderCmd._pMorph1->getVertexData()->getHandle()->getNativeBuffer();
      pCmdBuffer->bindVertexBuffers(1, 1, &morph0, offset);
      pCmdBuffer->bindVertexBuffers(2, 1, &morph1, offset);
    }

    pCmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getNative());
    pCmdBuffer->pushConstants(pipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4), &lightVP);
    pCmdBuffer->setViewPorts(0, 1, &viewport);
    pCmdBuffer->setScissor(0, 1, &scissor);

    if (index) {
      VkBuffer ind = index->getHandle()->getNativeBuffer();
      pCmdBuffer->bindIndexBuffer(ind, 0, getNativeIndexType(index->GetSizeType()));
    }

    Primitive* primitive = renderCmd._pPrimitive;
    descriptorSets[1] = primitive->_pMat->getNative()->CurrMaterialSet()->getHandle();
    pCmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, skinned ? 3 : 2, descriptorSets, 0, nullptr);
    if (index) {
      pCmdBuffer->drawIndexed(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0, 0);
    }
    else {
      pCmdBuffer->draw(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0);
    }
  };

  pCmdBuffer->beginRenderPass(renderPass, VK_SUBPASS_CONTENTS_INLINE);
  for (size_t i = 0; i < staticCmds.Size(); ++i) {
    PrimitiveRenderCmd& renderCmd = staticCmds[i];
    render(renderCmd, m_staticViewSpace._ViewProj);
  }
  pCmdBuffer->endRenderPass();
  if (staticCmds.Size() == 0) {
    R_DEBUG(rNotify, "Empty static map cmd buffer updated.\n");
  }
  R_DEBUG(rNotify, "Static map command buffer updated with meshes.\n")
  m_staticMapNeedsUpdate = false;
}


void ShadowMapSystem::update(VulkanRHI* pRhi, 
                             GlobalBuffer* gBuffer, 
                             LightBuffer* buffer, 
                             I32 idx, 
                             U32 resourceIndex)
{
  DirectionalLight* light = nullptr;
  if (idx <= -1) {
    light = &buffer->_PrimaryLight; 
  } else {
    light = &buffer->_DirectionalLights[idx];
  }

  R32 cx = gBuffer->_View[0][2];
  R32 cy = gBuffer->_View[1][2];
  R32 cz = gBuffer->_View[2][2];
  Vector3 camDir = Vector3(cx, cy, cz);

  Vector3 viewerPos = Vector3(
    gBuffer->_CameraPos.x, 
    gBuffer->_CameraPos.y, 
    gBuffer->_CameraPos.z
  ); 

  // TODO(): The shadow map needs to follow the player...
  Vector3 Eye = Vector3(
    light->_Direction.x,
    light->_Direction.y,
    light->_Direction.z
  );

  Eye *= 1024.0f;
  Eye -= viewerPos;
  // Pass as one matrix.
  Matrix4 view = Matrix4::lookAt(-Eye, viewerPos, Vector3::UP);
  // TODO(): This may need to be adjustable depending on scale.
  Matrix4 proj = Matrix4::ortho(
    m_rShadowViewportDim,
    m_rShadowViewportDim,
    1.0f,
    8000.0f
  );
  m_viewSpace._ViewProj = view * proj;
  R32 lightSz = m_rShadowLightSz / m_rShadowViewportDim;

  //m_cascadeViewSpace._lightSz = Vector4(lightSz, lightSz, lightSz, lightSz);
  //m_cascadeViewSpace._near = Vector4(0.135f, 0.0f, 0.1f, 0.1f);
  R32 cShadowDim = m_rShadowViewportDim;
  R32 rr = 5.0f;
  R32 e = 2.0f;
  for (size_t i = 0; i < m_numCascadeShadowMaps; ++i) {
    Vector3 extent = viewerPos + camDir * e;
    Eye = Vector3(
      light->_Direction.x,
      light->_Direction.y,
      light->_Direction.z
    );
    Eye *= 1024.0f;
    Eye -= extent;

    Matrix4& mat = m_cascadeViewSpace._ViewProj[i];
    Matrix4 p = Matrix4::ortho(cShadowDim, cShadowDim, 1.0f, 8000.0f);
    Matrix4 v = Matrix4::lookAt(-Eye, extent, Vector3::UP);
    m_cascadeViewSpace._split[i] = rr;
    m_cascadeViewSpace._lightSz[i] = m_rShadowLightSz;
    m_cascadeViewSpace._near[i] = m_rSoftShadowNear;
    rr *= 4.5f;
    e += 1.0f;
    mat = v * p;
    cShadowDim *= 3.5f;
  }

  {
    R_ASSERT(m_pLightViewBuffers[resourceIndex]->getMapped(), "Light view buffer was not mapped!");
    memcpy(m_pLightViewBuffers[resourceIndex]->getMapped(), &m_cascadeViewSpace, sizeof(LightViewCascadeSpace));

    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pLightViewBuffers[resourceIndex]->getMemory();
    range.size =   m_pLightViewBuffers[resourceIndex]->getMemorySize();
    range.offset = m_pLightViewBuffers[resourceIndex]->getMemoryOffset();
    pRhi->logicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  if ( m_staticMapNeedsUpdate ) {
    viewerPos = m_staticViewerPos;
    Eye = Vector3(
      light->_Direction.x,
      light->_Direction.y,
      light->_Direction.z
    );
    proj = Matrix4::ortho(
      m_staticShadowViewportDim,
      m_staticShadowViewportDim,
      1.0f,
      8000.0f
    );

    Eye *= 1024.0f;
    Eye -= viewerPos;
    // Pass as one matrix.
    view = Matrix4::lookAt(-Eye, viewerPos, Vector3::UP);
    m_staticViewSpace._ViewProj = view * proj;
    m_staticViewSpace._near = Vector4(0.135f, 0.0f, 0.1f, 0.1f);
    m_staticViewSpace._lightSz.x = 15.0f / m_staticShadowViewportDim;//15.0f / m_staticShadowViewportHeight;
    R_ASSERT(m_pStaticLightViewBuffers[resourceIndex]->getMapped(), "Light view buffer was not mapped!");
    memcpy(m_pStaticLightViewBuffers[resourceIndex]->getMapped(), &m_staticViewSpace, sizeof(LightViewSpace));

    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pStaticLightViewBuffers[resourceIndex]->getMemory();
    range.size =   m_pStaticLightViewBuffers[resourceIndex]->getMemorySize();
    range.offset = m_pStaticLightViewBuffers[resourceIndex]->getMemoryOffset();
    pRhi->logicDevice()->FlushMappedMemoryRanges(1, &range);
  }
}


void ShadowMapSystem::cleanUpShadowMapCascades(VulkanRHI* pRhi)
{
  for (U32 i = 0; i < m_pCascadeFrameBuffers.size(); ++i) {
    if (m_pCascadeFrameBuffers[i]) {
      pRhi->freeFrameBuffer(m_pCascadeFrameBuffers[i]);
      m_pCascadeFrameBuffers[i] = nullptr;
    }
  }

  for (U32 i = 0; i < m_pCascadeShadowMapD.size(); ++i) {
    if ( m_pCascadeShadowMapD[i] ) {
      pRhi->freeTexture( m_pCascadeShadowMapD[i] );
      m_pCascadeShadowMapD[i] = nullptr;
    }
  }

  for (size_t i = 0; i < m_cascades.size(); ++i) {
    std::vector<Cascade>& cascade = m_cascades[i];
    for (size_t j = 0; j < cascade.size(); ++j) {
      if ( cascade[j]._view ) { 
        pRhi->freeImageView(cascade[j]._view);
        cascade[j]._view = nullptr;
      }
    }
  }
}


void LightDescriptor::checkBuffering(Renderer* pRenderer, const GraphicsConfigParams* params)
{
  if (pRenderer->getResourceBufferCount() != m_pLightBuffers.size()) {
    cleanUp(pRenderer->getRHI());
    initialize(pRenderer, params);
    
  }
}


void ShadowMapSystem::cleanUp(VulkanRHI* pRhi)
{
  for (U32 i = 0; i < m_pLightViewBuffers.size(); ++i) {
    if (m_pLightViewBuffers[i]) {
      pRhi->freeBuffer(m_pLightViewBuffers[i]);
      m_pLightViewBuffers[i] = nullptr;
    }

    if ( m_pStaticLightViewBuffers[i] ) {
      pRhi->freeBuffer( m_pStaticLightViewBuffers[i] );
      m_pStaticLightViewBuffers[i] = nullptr;
    }
  }
/*
  if (m_pDynamicMap) {
    pRhi->FreeTexture(m_pDynamicMap);
    m_pDynamicMap = nullptr;
  }
*/
  if (m_pStaticMap) {
    pRhi->freeTexture(m_pStaticMap);
    m_pStaticMap = nullptr;
  }
 
  for (U32 i = 0; i < m_pLightViewDescriptorSets.size(); ++i) {
    if (m_pLightViewDescriptorSets[i]) {
      pRhi->freeDescriptorSet(m_pLightViewDescriptorSets[i]);
      m_pLightViewDescriptorSets[i] = nullptr;
    }
    if (m_pStaticLightViewDescriptorSets[i]) {
      pRhi->freeDescriptorSet(m_pStaticLightViewDescriptorSets[i]);
      m_pStaticLightViewDescriptorSets[i] = nullptr;
    }
  }
/*
  if (m_pDynamicFrameBuffer) {
    pRhi->FreeFrameBuffer(m_pDynamicFrameBuffer);
    m_pDynamicFrameBuffer = nullptr;
  }
*/
  if (m_pStaticFrameBuffer) {
    pRhi->freeFrameBuffer(m_pStaticFrameBuffer);
    m_pStaticFrameBuffer = nullptr;
  }

  cleanUpShadowMapCascades(pRhi);
  cleanUpSpotLightShadowMapArray(pRhi);
}


LightDescriptor::LightDescriptor()
  : m_pShadowSampler(nullptr)
#if 0
  , m_pLightViewDescriptorSet(nullptr)
  , m_pOpaqueShadowMap(nullptr)
  , m_pLightViewBuffer(nullptr)
  , m_pFrameBuffer(nullptr)
  , m_pRenderPass(nullptr)
#endif
  , m_PrimaryShadowEnable(true)
  , m_rShadowViewportHeight(40.0f)
  , m_rShadowViewportWidth(40.0f)
{
  m_Lights._PrimaryLight._Enable = false;
  m_Lights._PrimaryLight._Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  m_Lights._PrimaryLight._Pad[0] = 0;
  m_Lights._PrimaryLight._Pad[1] = 0;

  for (size_t i = 0; i < LightBuffer::maxNumPointLights(); ++i) {
    m_Lights._PointLights[i]._Position = Vector4();
    m_Lights._PointLights[i]._Color = Vector4();
    m_Lights._PointLights[i]._Range = 0.0f;
    m_Lights._PointLights[i]._Intensity = 1.0f;
    m_Lights._PointLights[i]._Enable = false;
    m_Lights._PointLights[i]._shadowIndex = 0;
  }

  for (size_t i = 0; i < LightBuffer::maxNumDirectionalLights(); ++i) {
    m_Lights._DirectionalLights[i]._Direction = Vector4();
    m_Lights._DirectionalLights[i]._Enable = false;
    m_Lights._DirectionalLights[i]._Intensity = 1.0f;
    m_Lights._DirectionalLights[i]._Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    m_Lights._DirectionalLights[i]._Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  }

  for (size_t i = 0; i < LightBuffer::maxNumSpotLights(); ++i) {
    m_Lights._SpotLights[i]._Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    m_Lights._SpotLights[i]._Enable = false;
    m_Lights._SpotLights[i]._InnerCutOff = 1.0f;
    m_Lights._SpotLights[i]._OuterCutOff = 1.0f;
    m_Lights._SpotLights[i]._Position = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    m_Lights._SpotLights[i]._Direction = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    m_Lights._SpotLights[i]._goboIndex = 0;
    m_Lights._SpotLights[i]._shadowIndex = 0;
    m_Lights._SpotLights[i]._pad = Vector2(-1.0f, 0.0f);
  }

#if 0
  m_PrimaryLightSpace._lightSz = Vector4();
  m_PrimaryLightSpace._near = Vector4();
  m_PrimaryLightSpace._shadowTechnique = Vector4();
#endif 
}


LightDescriptor::~LightDescriptor()
{
  DEBUG_OP(
    for (U32 i = 0; i < m_pLightBuffers.size(); ++i) {
      if (m_pLightBuffers[i]) {
        R_DEBUG(rWarning, "Light buffer was not cleaned up!\n");
      }

      if (m_pLightDescriptorSets[i]) {
        R_DEBUG(rWarning, "Light MaterialDescriptor descriptor set was not properly cleaned up!\n");
      }
    }
    );
#if 0
  if (m_pOpaqueShadowMap) {
    R_DEBUG(rWarning, "Light Shadow Map texture was not properly cleaned up!\n");
  }
#endif
  DEBUG_OP(
    if (m_pShadowSampler) {
      R_DEBUG(rWarning, "Light Shadow Map sampler was not properly cleaned up!\n");
    }
  );
#if 0
  R_ASSERT(!m_pRenderPass, "Render pass was not properly discarded from light descriptor!");

  if (m_pFrameBuffer) {
    R_DEBUG(rWarning, "Light framebuffer was not properly cleaned up!\n");
  }
  
  if (m_pLightViewBuffer) {
    R_DEBUG(rWarning, "Light view buffer was not properly cleaned up!\n");
  }

  if (m_pLightViewDescriptorSet) {
    R_DEBUG(rWarning, "Light view descriptor set was not properly cleaned up!\n");
  }
#endif
}


void LightDescriptor::initialize(Renderer* pRenderer, const GraphicsConfigParams* params)
{
  R_ASSERT(pRenderer, "RHI owner not set for light material upon initialization!\n");
  m_pLightBuffers.resize(pRenderer->getResourceBufferCount());
 
  VulkanRHI* pRhi = pRenderer->getRHI();
  for (U32 i = 0; i < m_pLightBuffers.size(); ++i) {
    m_pLightBuffers[i] = pRhi->createBuffer();
    VkBufferCreateInfo bufferCI = {};
    VkDeviceSize dSize = sizeof(LightBuffer);
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCI.size = dSize;
    bufferCI.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    m_pLightBuffers[i]->initialize(pRhi->logicDevice()->getNative(),
                                   bufferCI, 
                                   PHYSICAL_DEVICE_MEMORY_USAGE_CPU_ONLY);
  }
#if 0
  // Light view buffer creation.
  m_pLightViewBuffer = pRhi->CreateBuffer();
  dSize = sizeof(LightViewSpace);
  bufferCI.size = dSize;
  m_pLightViewBuffer->initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_pLightViewBuffer->Map();

  // Create our shadow map texture.
  if (!m_pOpaqueShadowMap) {
    // TODO():
    m_pOpaqueShadowMap = pRhi->createTexture();

    // ShadowMap is a depth image.
    VkImageCreateInfo ImageCi = {};
    ImageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCi.arrayLayers = 1;
    ImageCi.extent.width = 512 << shadowDetail;
    ImageCi.extent.height = 512 << shadowDetail;
    ImageCi.extent.depth = 1;
    ImageCi.format = VK_FORMAT_D32_SFLOAT;
    ImageCi.imageType = VK_IMAGE_TYPE_2D;
    ImageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCi.mipLevels = 1;
    ImageCi.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCi.usage =  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ImageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkImageViewCreateInfo ViewCi = { };
    ViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ViewCi.components = { };
    ViewCi.format = ImageCi.format;
    ViewCi.subresourceRange = { }; 
    ViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT ;
    ViewCi.subresourceRange.baseArrayLayer = 0;
    ViewCi.subresourceRange.baseMipLevel = 0;
    ViewCi.subresourceRange.layerCount = 1;
    ViewCi.subresourceRange.levelCount = 1;
    ViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    
    m_pOpaqueShadowMap->initialize(ImageCi, ViewCi);
  }
#endif
  if (!m_pShadowSampler) {
    // TODO():
    m_pShadowSampler = pRhi->createSampler();
    VkSamplerCreateInfo SamplerCi = { };
    SamplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    SamplerCi.addressModeV = SamplerCi.addressModeU;
    SamplerCi.addressModeW = SamplerCi.addressModeV;
    SamplerCi.anisotropyEnable = VK_FALSE;
    SamplerCi.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    SamplerCi.compareEnable = VK_FALSE;
    SamplerCi.magFilter = VK_FILTER_NEAREST;
    SamplerCi.minFilter = VK_FILTER_NEAREST;
    SamplerCi.maxAnisotropy = 1.0f;
    SamplerCi.maxLod = 1.0f;
    SamplerCi.minLod = 0.0f;
    SamplerCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    SamplerCi.unnormalizedCoordinates = VK_FALSE;

    m_pShadowSampler->initialize(SamplerCi);
  }

  initializeNativeLights(pRenderer);

#if 0
  InitializePrimaryShadow(pRhi);

  if (shadowDetail < SHADOWS_MEDIUM) {
    m_PrimaryLightSpace._shadowTechnique = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
  }
  else {
    m_PrimaryLightSpace._shadowTechnique = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
  }
#endif 

  m_primaryMapSystem._pSampler = m_pShadowSampler;
  m_primaryMapSystem.initialize(pRenderer, params);
}


void LightDescriptor::cleanUp(VulkanRHI* pRhi)
{
#if 0
  if (m_pOpaqueShadowMap) {
    pRhi->FreeTexture(m_pOpaqueShadowMap);
    m_pOpaqueShadowMap = nullptr;
  }

  if (m_pRenderPass) {
    pRhi->FreeRenderPass(m_pRenderPass);
    m_pRenderPass = nullptr;
  }

  if (m_pFrameBuffer) {
    pRhi->FreeFrameBuffer(m_pFrameBuffer);
    m_pFrameBuffer = nullptr;
  }

  if (m_pLightViewBuffer) {
    m_pLightViewBuffer->UnMap();
    pRhi->FreeBuffer(m_pLightViewBuffer);
    m_pLightViewBuffer = nullptr;
  }

  if (m_pLightViewDescriptorSet) {
    pRhi->FreeDescriptorSet(m_pLightViewDescriptorSet);
    m_pLightViewDescriptorSet = nullptr;
  }
#endif
  if (m_pShadowSampler) {
    pRhi->freeSampler(m_pShadowSampler);
    m_pShadowSampler = nullptr;
  }

  // TODO
  for (U32 i = 0; i < m_pLightBuffers.size(); ++i) {
    if (m_pLightDescriptorSets[i]) {
      pRhi->freeDescriptorSet(m_pLightDescriptorSets[i]);
      m_pLightDescriptorSets[i] = nullptr;
    }

    if (m_pLightBuffers[i]) {
      pRhi->freeBuffer(m_pLightBuffers[i]);
      m_pLightBuffers[i] = nullptr;
    }
  }

  m_primaryMapSystem.cleanUp(pRhi);
}


void LightDescriptor::update(Renderer* pRenderer, GlobalBuffer* gBuffer, U32 resourceIndex)
{
  VulkanRHI* pRhi = pRenderer->getRHI();
#if 0
  // TODO(): The shadow map needs to follow the player...
  Vector3 Eye = Vector3(
    m_Lights._PrimaryLight._Direction.x, 
    m_Lights._PrimaryLight._Direction.y, 
    m_Lights._PrimaryLight._Direction.z
  );
  Eye *= 1024.0f;
  Eye -= m_vViewerPos;
  // Pass as one matrix.
  Matrix4 view = Matrix4::lookAt(-Eye, m_vViewerPos, Vector3::UP);
  // TODO(): This may need to be adjustable depending on scale.
  Matrix4 proj = Matrix4::ortho(
    m_rShadowViewportWidth, 
    m_rShadowViewportHeight, 
    1.0f, 
    8000.0f
  );
  m_PrimaryLightSpace._ViewProj = view * proj;
  R32 lightSz = 5.0f / m_rShadowViewportHeight;
  m_PrimaryLightSpace._lightSz = Vector4(lightSz, lightSz, lightSz, lightSz);
  m_PrimaryLightSpace._near = Vector4(0.135f, 0.0f, 0.1f, 0.1f);

#endif

#if 0
  if (isPrimaryShadowEnabled() && m_pLightViewBuffer) {
#else
  if ((isPrimaryShadowEnabled() || m_primaryMapSystem.staticMapNeedsUpdate()) && m_primaryMapSystem.getShadowMapViewDescriptor(resourceIndex)) {
#endif
    m_primaryMapSystem.update(pRhi, gBuffer, &m_Lights, -1, resourceIndex);
#if 0
    R_ASSERT(m_pLightViewBuffer->Mapped(), "Light view buffer was not mapped!");
    memcpy(m_pLightViewBuffer->Mapped(), &m_PrimaryLightSpace, sizeof(LightViewSpace));

    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pLightViewBuffer->Memory();
    range.size = m_pLightViewBuffer->MemorySize();
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
#endif
  }

  R_ASSERT(m_pLightBuffers[resourceIndex]->getMapped(), "Light buffer was not mapped!");
  memcpy(m_pLightBuffers[resourceIndex]->getMapped(), &m_Lights, sizeof(LightBuffer));

  VkMappedMemoryRange lightRange = { };
  lightRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  lightRange.memory = m_pLightBuffers[resourceIndex]->getMemory();
  lightRange.size =   m_pLightBuffers[resourceIndex]->getMemorySize();
  lightRange.offset = m_pLightBuffers[resourceIndex]->getMemoryOffset();
  pRhi->logicDevice()->FlushMappedMemoryRanges(1, &lightRange);
}


void LightDescriptor::initializeNativeLights(Renderer* pRenderer)
{
  VulkanRHI* pRhi = pRenderer->getRHI();
  DescriptorSetLayout* pbrLayout = LightSetLayoutKey;
  m_pLightDescriptorSets.resize(pRenderer->getResourceBufferCount());

  for (U32 i = 0; i < m_pLightDescriptorSets.size(); ++i) {
    m_pLightDescriptorSets[i] = pRhi->createDescriptorSet();
    m_pLightDescriptorSets[i]->allocate(pRhi->descriptorPool(), pbrLayout);

    VkDescriptorBufferInfo lightBufferInfo = {};
    lightBufferInfo.buffer = m_pLightBuffers[i]->getNativeBuffer();
    lightBufferInfo.offset = 0;
    lightBufferInfo.range = sizeof(LightBuffer);

    std::array<VkWriteDescriptorSet, 1> writeSets;
    writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[0].descriptorCount = 1;
    writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeSets[0].dstArrayElement = 0;
    writeSets[0].pBufferInfo = &lightBufferInfo;
    writeSets[0].pNext = nullptr;
    writeSets[0].dstBinding = 0;

    m_pLightDescriptorSets[i]->update(static_cast<U32>(writeSets.size()), writeSets.data());
  }
}


#if 0
void LightDescriptor::InitializePrimaryShadow(VulkanRHI* pRhi)
{
  // TODO(): Create DescriptorSet and Framebuffer for shadow pass.
  if (m_pFrameBuffer) return;

  DescriptorSetLayout* viewLayout = LightViewDescriptorSetLayoutKey;
  m_pLightViewDescriptorSet = pRhi->createDescriptorSet();
  m_pLightViewDescriptorSet->allocate(pRhi->DescriptorPool(), viewLayout);

  VkDescriptorBufferInfo viewBuf = { };
  viewBuf.buffer = m_pLightViewBuffer->NativeBuffer();
  viewBuf.offset = 0;
  viewBuf.range = sizeof(LightViewSpace);

  // TODO(): Once we create our shadow map, we will add it here.
  // This will pass the rendered shadow map to the pbr pipeline.
  VkDescriptorImageInfo globalShadowInfo = {};
  globalShadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalShadowInfo.imageView = m_pOpaqueShadowMap->getView();
  globalShadowInfo.sampler = m_pShadowSampler->getHandle();

  std::array<VkWriteDescriptorSet, 2> writes;
  writes[0] = {};
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].dstArrayElement = 0;
  writes[0].dstBinding = 0;
  writes[0].dstSet = nullptr;
  writes[0].pImageInfo = nullptr;
  writes[0].pBufferInfo = &viewBuf;

  writes[1] = {};
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].descriptorCount = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writes[1].dstArrayElement = 0;
  writes[1].pNext = nullptr;
  writes[1].pImageInfo = &globalShadowInfo;
  writes[1].dstBinding = 1;
  
  m_pLightViewDescriptorSet->update(writes.size(), writes.data());

  m_pFrameBuffer = pRhi->createFrameBuffer();
  m_pRenderPass = pRhi->CreateRenderPass();
  std::array<VkAttachmentDescription, 1> attachmentDescriptions;

  // Actual depth buffer to write onto.
  attachmentDescriptions[0] = CreateAttachmentDescription(
    m_pOpaqueShadowMap->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    m_pOpaqueShadowMap->Samples()
  );

  std::array<VkSubpassDependency, 2> dependencies;
  dependencies[0] = CreateSubPassDependency(
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  dependencies[1] = CreateSubPassDependency(
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  VkAttachmentReference depthRef =  {};
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthRef.attachment = 0;

  VkSubpassDescription subpass = { };
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 0;
  subpass.pColorAttachments = nullptr;
  subpass.pDepthStencilAttachment = &depthRef;
  subpass.flags = 0;

  VkRenderPassCreateInfo renderPassCi = CreateRenderPassInfo(
    static_cast<U32>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    2,
    dependencies.data(),
    1,
    &subpass
  );

  std::array<VkImageView, 1> attachments;
  attachments[0] = m_pOpaqueShadowMap->getView();
  
  VkFramebufferCreateInfo frameBufferCi = CreateFrameBufferInfo(
    m_pOpaqueShadowMap->getWidth(),
    m_pOpaqueShadowMap->getHeight(),
    nullptr,
    static_cast<U32>(attachments.size()),
    attachments.data(),
    1
  );

  m_pRenderPass->initialize(renderPassCi);
  m_pFrameBuffer->Finalize(frameBufferCi, m_pRenderPass);
}
#endif
} // Recluse