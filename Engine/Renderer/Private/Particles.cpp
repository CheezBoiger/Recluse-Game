// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Particles.hpp"
#include "RendererData.hpp"
#include "TextureType.hpp"
#include "Core/Logging/Log.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/Shader.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Framebuffer.hpp"

#include "Core/Exception.hpp"

#include <vector>
#include <array>

namespace Recluse {


VkVertexInputBindingDescription GetParticleBindingDescription()
{
  // Instance data is used for this pipeline.
  VkVertexInputBindingDescription description = { };
  description.stride = sizeof(Particle);
  description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
  description.binding = 0;
  return description;
}


std::vector<VkVertexInputAttributeDescription> GetParticleAttributeDescription()
{
  u32 offset = 0;
  std::vector<VkVertexInputAttributeDescription> description(7);
  // Position
  description[0] = { };
  description[0].binding = 0;
  description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  description[0].location = 0;
  description[0].offset = offset;
  offset += sizeof(Vector4);
  // Velocity
  description[1] = { };
  description[1].binding = 0;
  description[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  description[1].location = 1;
  description[1].offset = offset;
  offset += sizeof(Vector4);
  // Color
  description[2] = { };
  description[2].binding = 0;
  description[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  description[2].location = 2;
  description[2].offset = offset;
  offset += sizeof(Vector4);
  // Angle
  description[3] = { };
  description[3].binding = 0;
  description[3].format = VK_FORMAT_R32_SFLOAT;
  description[3].location = 3;
  description[3].offset = offset;
  offset += sizeof(r32);
  // Size
  description[4] = { };
  description[4].binding = 0;
  description[4].format = VK_FORMAT_R32_SFLOAT;
  description[4].location = 4;
  description[4].offset = offset;
  offset += sizeof(r32);
  // Weight
  description[5] = { };
  description[5].binding = 0;
  description[5].format = VK_FORMAT_R32_SFLOAT;
  description[5].location = 5;
  description[5].offset = offset;
  offset += sizeof(r32);
  // Life
  description[6] = { };
  description[6].binding = 0;
  description[6].format = VK_FORMAT_R32_SFLOAT;
  description[6].location = 6;
  description[6].offset = offset;
  
  return description;
}


void ParticleSystem::Initialize(VulkanRHI* pRhi, 
  DescriptorSetLayout* particleLayout, u32 initialParticleCount)
{
  _particleConfig._maxParticles = initialParticleCount;

  m_particleBuffer = pRhi->CreateBuffer();
  m_particleConfigBuffer = pRhi->CreateBuffer();

  {
    VkBufferCreateInfo gpuBufferCi = { };
    gpuBufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    gpuBufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    gpuBufferCi.size = sizeof(Particle) * _particleConfig._maxParticles;
    gpuBufferCi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
      | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
      | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    m_particleBuffer->Initialize(gpuBufferCi, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  {
    VkBufferCreateInfo bufferCi = { };
    bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCi.size = VkDeviceSize(sizeof(ParticleSystemConfig));
    bufferCi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    m_particleConfigBuffer->Initialize(bufferCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    m_particleConfigBuffer->Map();
  }

  m_pDescriptorSet = pRhi->CreateDescriptorSet();
  m_pDescriptorSet->Allocate(pRhi->DescriptorPool(), particleLayout);
}


void ParticleSystem::UpdateDescriptor()
{
  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = m_particleBuffer->NativeBuffer();
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(ParticleSystemConfig);

  VkDescriptorImageInfo imgInfo = {};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgInfo.imageView = _texture->Handle()->View();
  imgInfo.sampler = _sampler->Handle()->Handle();

  // TODO():
  std::array<VkWriteDescriptorSet, 2> writes;
  writes[0] = {};
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

  writes[1] = {};
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

  m_pDescriptorSet->Update(static_cast<u32>(writes.size()), writes.data());
}


void ParticleSystem::Update(VulkanRHI* pRhi)
{
  if (m_updateBits & PARTICLE_DESCRIPTOR_UPDATE_BIT) {
    UpdateDescriptor();
  }

  if (m_updateBits & PARTICLE_BUFFER_UPDATE_BIT) {
    R_ASSERT(m_particleConfigBuffer, "Particle config not mapped.");
    memcpy(m_particleConfigBuffer->Mapped(), &_particleConfig, sizeof(ParticleSystemConfig));
    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.offset = 0;
    range.memory = m_particleConfigBuffer->Memory();
    range.size = m_particleConfigBuffer->MemorySize();
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  m_updateBits = 0;
}


void ParticleSystem::CleanUp(VulkanRHI* pRhi)
{
  if ( m_particleBuffer ) {
    pRhi->FreeBuffer( m_particleBuffer );
    m_particleBuffer = nullptr;
  }

  if ( m_particleConfigBuffer ) {
    pRhi->FreeBuffer( m_particleConfigBuffer );
    m_particleConfigBuffer = nullptr;
  }

  if ( m_pDescriptorSet ) {
    pRhi->FreeDescriptorSet( m_pDescriptorSet );
    m_pDescriptorSet = nullptr;
  }
}


GraphicsPipeline* GenerateParticleRendererPipeline(VulkanRHI* pRhi, 
  DescriptorSetLayout* pParticleConfigSetLayout, RenderPass* pRenderPass)
{
  GraphicsPipeline* pipeline = pRhi->CreateGraphicsPipeline();
  Shader vertShader; vertShader.SetOwner(pRhi->LogicDevice()->Native());
  Shader fragShader; fragShader.SetOwner(pRhi->LogicDevice()->Native());
  Shader geomShader; geomShader.SetOwner(pRhi->LogicDevice()->Native());
  RendererPass::LoadShader("Particles.vert.spv", &vertShader);
  RendererPass::LoadShader("Particles.frag.spv", &fragShader);
  RendererPass::LoadShader("Particles.geom.spv", &geomShader);

  // TODO():
  VkGraphicsPipelineCreateInfo graphicsCi = {};
  VkPipelineInputAssemblyStateCreateInfo assemblyCi = { };
  assemblyCi.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assemblyCi.primitiveRestartEnable = VK_FALSE;
  assemblyCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  
  VkPipelineVertexInputStateCreateInfo vertexCi = { };
  vertexCi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  auto attribs = GetParticleAttributeDescription();
  auto binding = GetParticleBindingDescription();
  vertexCi.vertexAttributeDescriptionCount = static_cast<u32>(attribs.size());
  vertexCi.vertexBindingDescriptionCount = 1;
  vertexCi.pVertexBindingDescriptions = &binding;
  vertexCi.pVertexAttributeDescriptions = attribs.data();

  VkPipelineDepthStencilStateCreateInfo depthStencilCi = { };
  depthStencilCi.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilCi.depthBoundsTestEnable = VK_FALSE;
  depthStencilCi.depthTestEnable = VK_TRUE;
  
  VkPipelineColorBlendStateCreateInfo colorBlendCi = { };
  colorBlendCi.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

  VkPipelineRasterizationStateCreateInfo rasterCi = { };
  rasterCi.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; 

  VkPipelineDynamicStateCreateInfo dynamicCi = { };
  dynamicCi.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

  VkPipelineMultisampleStateCreateInfo multisampleCi = { };
  multisampleCi.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

  VkPipelineTessellationStateCreateInfo tessStateCi = { }; 
  tessStateCi.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
  
  VkPipelineViewportStateCreateInfo viewportCi = { };
  viewportCi.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; 
  VkViewport viewport = { };
  VkExtent2D extent = pRhi->SwapchainObject()->SwapchainExtent();
  viewport.width = static_cast<r32>(extent.width);
  viewport.height = static_cast<r32>(extent.height);
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;  

  VkRect2D scissor = { };
  scissor.extent = extent;
  scissor.offset = { 0, 0 };
  viewportCi.pScissors = &scissor;
  viewportCi.pViewports = &viewport;
  viewportCi.scissorCount = 1;
  viewportCi.viewportCount = 1;

  std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages;
  shaderStages[0] = { };
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].module = vertShader.Handle();
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].pName = kDefaultShaderEntryPointStr;
  
  shaderStages[1] = { };
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].module = geomShader.Handle();
  shaderStages[1].pName = kDefaultShaderEntryPointStr;
  shaderStages[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;

  shaderStages[2] = { };
  shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[2].module = fragShader.Handle();
  shaderStages[2].pName = kDefaultShaderEntryPointStr;
  shaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;


  graphicsCi.pColorBlendState = &colorBlendCi;
  graphicsCi.pDepthStencilState = &depthStencilCi;
  graphicsCi.pInputAssemblyState = &assemblyCi;
  graphicsCi.pRasterizationState = &rasterCi;
  graphicsCi.pDynamicState = &dynamicCi;
  graphicsCi.pMultisampleState = &multisampleCi;
  graphicsCi.pVertexInputState = &vertexCi;
  graphicsCi.pTessellationState = &tessStateCi;
  graphicsCi.pViewportState = &viewportCi;
  graphicsCi.renderPass = pRenderPass->Handle();
  graphicsCi.basePipelineHandle = VK_NULL_HANDLE;
  graphicsCi.basePipelineIndex = -1;
  graphicsCi.stageCount = shaderStages.size();
  graphicsCi.pStages = shaderStages.data();

  VkPipelineLayoutCreateInfo pipelineLayoutCi = { };
  pipelineLayoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  
  vertShader.CleanUp();
  fragShader.CleanUp();
  geomShader.CleanUp();
  return pipeline;
}


void ParticleEngine::Initialize(VulkanRHI* pRhi)
{ 
  {
    m_pParticleDescriptorSetLayout = pRhi->CreateDescriptorSetLayout();
    VkDescriptorSetLayoutCreateInfo dsLayoutCi = { };
    dsLayoutCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayoutBinding, 2> bindings;

    bindings[0] = { };
    bindings[0].descriptorCount = 1;
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1] = { };
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].pImmutableSamplers = nullptr;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    dsLayoutCi.bindingCount = static_cast<u32>( bindings.size() );
    dsLayoutCi.pBindings = bindings.data();

    m_pParticleDescriptorSetLayout->Initialize( dsLayoutCi );

    {   
      std::array<VkAttachmentDescription, 3> attachmentDescriptions;
      // final color.
      attachmentDescriptions[0] = { };
      attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
      attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

      // brightness output.
      attachmentDescriptions[1] = { };
      attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachmentDescriptions[1].format = VK_FORMAT_R8G8B8A8_UNORM;
      attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

      // depth load, from total scene. This is after deferred and forward pipeline.
      attachmentDescriptions[2] = { };
      attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachmentDescriptions[2].format = VK_FORMAT_D32_SFLOAT;
      attachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

      std::array<VkSubpassDependency, 2> dependencies;
      dependencies[0] = { };
      dependencies[0]  = CreateSubPassDependency(
        VK_SUBPASS_EXTERNAL,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_DEPENDENCY_BY_REGION_BIT);
      dependencies[1] = { };
      dependencies[1] = CreateSubPassDependency(
        0,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_SUBPASS_EXTERNAL,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_DEPENDENCY_BY_REGION_BIT);

      VkSubpassDescription subpassDescription = { };
      std::array<VkAttachmentReference, 2> colorReferences;
      VkAttachmentReference depthStencilReference;

      colorReferences[0] = { };
      colorReferences[0].attachment = 0;
      colorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorReferences[1] = { };
      colorReferences[1].attachment = 1;
      colorReferences[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      depthStencilReference.attachment = static_cast<u32>(colorReferences.size());
      depthStencilReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      subpassDescription.colorAttachmentCount = static_cast<u32>(colorReferences.size());
      subpassDescription.inputAttachmentCount = 0;
      subpassDescription.pColorAttachments = colorReferences.data();
      subpassDescription.pDepthStencilAttachment = &depthStencilReference;
      subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpassDescription.preserveAttachmentCount = 0;
      
      VkRenderPassCreateInfo renderPassCi = {};
      renderPassCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassCi.attachmentCount = static_cast<u32>(attachmentDescriptions.size());
      renderPassCi.pAttachments = attachmentDescriptions.data();
      renderPassCi.dependencyCount = static_cast<u32>(dependencies.size());
      renderPassCi.pDependencies = dependencies.data();
      renderPassCi.pSubpasses = &subpassDescription;
      renderPassCi.subpassCount = 1u;

      m_pRenderPass = pRhi->CreateRenderPass();
      m_pRenderPass->Initialize(renderPassCi);
    }
  }

  // Particle Renderer Pipeline.
  m_pParticleRender = GenerateParticleRendererPipeline(pRhi, m_pParticleDescriptorSetLayout, m_pRenderPass);

  // Particle Compute Pipeline.
  {
    Shader* compShader = pRhi->CreateShader();
    RendererPass::LoadShader("Particles.comp.spv", compShader);
    
    // TODO(): 

    pRhi->FreeShader(compShader);
  }
  R_DEBUG(rNotify, "Particle engine initialized.\n");
}


void ParticleEngine::CleanUp(VulkanRHI* pRhi)
{
  if ( m_pParticleDescriptorSetLayout ) {
    pRhi->FreeDescriptorSetLayout( m_pParticleDescriptorSetLayout );
    m_pParticleDescriptorSetLayout = nullptr;
  }

  if ( m_pParticleRender ) {
    pRhi->FreeGraphicsPipeline( m_pParticleRender );
    m_pParticleRender = nullptr;
  }

  if ( m_pParticleCompute ) {
    pRhi->FreeComputePipeline( m_pParticleCompute );
    m_pParticleCompute = nullptr;
  }

  if ( m_pRenderPass ) {
    pRhi->FreeRenderPass( m_pRenderPass );
    m_pRenderPass = nullptr;
  }

  if ( m_pFrameBuffer ) {
    pRhi->FreeFrameBuffer( m_pFrameBuffer );
    m_pFrameBuffer = nullptr;
  }

  R_DEBUG(rNotify, "Particle engine cleaned up.\n");
}


ParticleEngine::~ParticleEngine()
{
  DEBUG_OP(
  if ( m_pParticleDescriptorSetLayout ) {
    R_DEBUG(rError, "Particle descriptor set layout not properly cleaned up!\n");
  });
}
} // Recluse