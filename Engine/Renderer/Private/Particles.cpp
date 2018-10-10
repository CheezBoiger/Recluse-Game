// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Particles.hpp"
#include "RendererData.hpp"
#include "GlobalDescriptor.hpp"
#include "TextureType.hpp"
#include "Core/Logging/Log.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/Shader.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Framebuffer.hpp"
#include "RHI/ComputePipeline.hpp"
#include "RHI/GraphicsPipeline.hpp"

#include "Core/Exception.hpp"

#include <random>
#include <algorithm>
#include <vector>
#include <array>

namespace Recluse {


VkVertexInputBindingDescription GetParticleBindingDescription()
{
  // Instance data is used for this pipeline.
  VkVertexInputBindingDescription description = { };
  description.stride = sizeof(Particle);
  description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  description.binding = 0;
  return description;
}

VkVertexInputBindingDescription GetParticleTrailBindingDescription()
{
  // Instance data is used for this pipeline.
  VkVertexInputBindingDescription description = {};
  description.stride = sizeof(ParticleTrailPosition);
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
  _particleConfig._maxParticles = static_cast<r32>(initialParticleCount);
  _particleConfig._angleThreshold = 0.0f;
  _particleConfig._fadeAt = 0.0f;
  _particleConfig._fadeThreshold = 1.0f;
  _particleConfig._isWorldSpace = 0.0f;
  _particleConfig._lifeTimeScale = 1.0f;
  _particleConfig._particleMaxAlive = 2.0f;
  _particleConfig._rate = 1.0f;

  m_particleBuffer = pRhi->CreateBuffer();
  m_particleConfigBuffer = pRhi->CreateBuffer();

  {
    VkBufferCreateInfo gpuBufferCi = { };
    gpuBufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    gpuBufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    gpuBufferCi.size = VkDeviceSize(sizeof(Particle) * static_cast<u32>(_particleConfig._maxParticles));
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


void ParticleSystem::UpdateGpuParticles(VulkanRHI* pRhi)
{
  Buffer staging;
  staging.SetOwner(pRhi->LogicDevice()->Native());

  // TODO(): Randomizing stuff, so we need to figure out how to check when a particle is dead,
  // and reupdate after.
  std::vector<Particle> particles(_particleConfig._maxParticles);
  std::random_device dev;
  std::mt19937 twist(dev());
  std::uniform_real_distribution<r32> uni(-0.3f, 0.3f);
  r32 grad = 10.0f;
  for (size_t i = 0; i < particles.size(); ++i) {
    particles[i]._velocity = Vector4(uni(twist), uni(twist), uni(twist), 0.0f);
    particles[i]._life = grad;
    particles[i]._color = Vector4(0.0f, 0.0f, 0.0f, 0.6f);
    particles[i]._sz = 0.3f;
    grad = grad - 1.0f / static_cast<r32>(particles.size());
  }

  {
    VkBufferCreateInfo stagingCI = {};
    stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingCI.size = sizeof(Particle) * particles.size();
    stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    staging.Initialize(stagingCI, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    staging.Map();
    memcpy(staging.Mapped(), particles.data(), (size_t)stagingCI.size);
    staging.UnMap();
  }

  CommandBuffer cmdBuffer;
  cmdBuffer.SetOwner(pRhi->LogicDevice()->Native());
  cmdBuffer.Allocate(pRhi->TransferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmdBuffer.Begin(beginInfo);
  VkBufferCopy region = {};
  region.size = sizeof(Particle) * _particleConfig._maxParticles;
  region.srcOffset = 0;
  region.dstOffset = 0;
  cmdBuffer.CopyBuffer(staging.NativeBuffer(),
    m_particleBuffer->NativeBuffer(),
    1,
    &region);
  cmdBuffer.End();

  VkCommandBuffer cmd = cmdBuffer.Handle();
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  pRhi->TransferSubmit(DEFAULT_QUEUE_IDX, 1, &submitInfo);
  pRhi->TransferWaitIdle(DEFAULT_QUEUE_IDX);

  staging.CleanUp();
}


void ParticleSystem::UpdateDescriptor()
{
  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = m_particleConfigBuffer->NativeBuffer();
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(ParticleSystemConfig);

  VkDescriptorBufferInfo storage = { };
  storage.buffer = m_particleBuffer->NativeBuffer();
  storage.offset = 0;
  storage.range = VkDeviceSize(sizeof(Particle) * _particleConfig._maxParticles);

  Texture* textureImg = DefaultTextureKey;
  Sampler* sampler = DefaultSampler2DKey;
  if (_texture) textureImg = _texture->Handle();
  if (_sampler) sampler = _sampler->Handle();
  VkDescriptorImageInfo imgInfo = {};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgInfo.imageView = textureImg->View();
  imgInfo.sampler = sampler->Handle();

  // TODO():
  std::array<VkWriteDescriptorSet, 3> writes;
  writes[0] = {};
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].pBufferInfo = &bufferInfo;
  writes[0].dstBinding = 0;

  writes[1] = {};
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writes[1].pImageInfo = &imgInfo;  
  writes[1].dstBinding = 1;
  writes[1].descriptorCount = 1;

  writes[2] = { };
  writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[2].pBufferInfo = &storage;
  writes[2].descriptorCount = 1;
  writes[2].dstBinding = 2;

  m_pDescriptorSet->Update(static_cast<u32>(writes.size()), writes.data());
}


void ParticleSystem::Update(VulkanRHI* pRhi)
{
  if (m_updateBits & PARTICLE_DESCRIPTOR_UPDATE_BIT) {
    UpdateDescriptor();
  }

  if (m_updateBits & PARTICLE_VERTEX_BUFFER_UPDATE_BIT) {
    UpdateGpuParticles(pRhi);
  }

  if (m_updateBits & PARTICLE_CONFIG_BUFFER_UPDATE_BIT) {
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
  assemblyCi.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  
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
  depthStencilCi.maxDepthBounds = 1.0f;
  depthStencilCi.minDepthBounds = 0.0f;
  depthStencilCi.depthWriteEnable = VK_TRUE;
  depthStencilCi.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilCi.stencilTestEnable = VK_FALSE;
  
  VkPipelineColorBlendStateCreateInfo colorBlendCi = { };
  colorBlendCi.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  std::array<VkPipelineColorBlendAttachmentState, 6> colorBlendAttachments;
  colorBlendAttachments[0] = CreateColorBlendAttachmentState(
    VK_TRUE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[1] = CreateColorBlendAttachmentState(
    VK_TRUE,
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

  colorBlendAttachments[4] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[5] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );
  colorBlendCi.attachmentCount = static_cast<u32>(colorBlendAttachments.size());
  colorBlendCi.pAttachments = colorBlendAttachments.data();
  colorBlendCi.logicOpEnable = VK_FALSE;
  colorBlendCi.logicOp = VK_LOGIC_OP_COPY;

  VkPipelineRasterizationStateCreateInfo rasterCi = { };
  rasterCi.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; 
  rasterCi.cullMode = VK_CULL_MODE_NONE;
  rasterCi.rasterizerDiscardEnable = VK_FALSE;
  rasterCi.depthBiasClamp = 0.0f;
  rasterCi.depthBiasConstantFactor = 0.0f;
  rasterCi.depthBiasEnable = VK_FALSE;
  rasterCi.depthBiasSlopeFactor = 0.0f;
  rasterCi.depthClampEnable = VK_FALSE;
  rasterCi.lineWidth = 1.0f;
  rasterCi.frontFace = VK_FRONT_FACE_CLOCKWISE;

  VkPipelineDynamicStateCreateInfo dynamicCi = { };
  dynamicCi.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT };
  dynamicCi.dynamicStateCount = 1;
  dynamicCi.pDynamicStates = states;
  
  VkPipelineMultisampleStateCreateInfo multisampleCi = { };
  multisampleCi.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleCi.alphaToCoverageEnable = VK_FALSE;
  multisampleCi.alphaToOneEnable = VK_FALSE;
  multisampleCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleCi.sampleShadingEnable = VK_FALSE;

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
  shaderStages[1].module = fragShader.Handle();
  shaderStages[1].pName = kDefaultShaderEntryPointStr;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

  shaderStages[2] = { };
  shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[2].module = geomShader.Handle();
  shaderStages[2].pName = kDefaultShaderEntryPointStr;
  shaderStages[2].stage = VK_SHADER_STAGE_GEOMETRY_BIT;

  graphicsCi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphicsCi.pColorBlendState = &colorBlendCi;
  graphicsCi.pDepthStencilState = &depthStencilCi;
  graphicsCi.pInputAssemblyState = &assemblyCi;
  graphicsCi.pRasterizationState = &rasterCi;
  graphicsCi.pDynamicState = &dynamicCi;
  graphicsCi.pMultisampleState = &multisampleCi;
  graphicsCi.pVertexInputState = &vertexCi;
  graphicsCi.pTessellationState = nullptr;
  graphicsCi.pViewportState = &viewportCi;
  graphicsCi.renderPass = pRenderPass->Handle();
  graphicsCi.basePipelineHandle = VK_NULL_HANDLE;
  graphicsCi.basePipelineIndex = -1;
  graphicsCi.stageCount = static_cast<u32>(shaderStages.size());
  graphicsCi.pStages = shaderStages.data();

  VkPipelineLayoutCreateInfo pipelineLayoutCi = { };
  pipelineLayoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCi.setLayoutCount = 2;
  VkDescriptorSetLayout layouts[] = { 
    GlobalSetLayoutKey->Layout(),
    pParticleConfigSetLayout->Layout()
  };
  pipelineLayoutCi.pSetLayouts = layouts;
  
  pipeline->Initialize(graphicsCi, pipelineLayoutCi);

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
    std::array<VkDescriptorSetLayoutBinding, 3> bindings;

    bindings[0] = { };
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT 
    | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

    bindings[1] = { };
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].pImmutableSamplers = nullptr;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[2] = { };
    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].pImmutableSamplers = nullptr;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    dsLayoutCi.bindingCount = static_cast<u32>( bindings.size() );
    dsLayoutCi.pBindings = bindings.data();

    m_pParticleDescriptorSetLayout->Initialize( dsLayoutCi );
  }

  InitializeRenderPass(pRhi);

  // Particle Renderer Pipeline.
  m_pParticleRender = GenerateParticleRendererPipeline(pRhi, m_pParticleDescriptorSetLayout, m_pRenderPass);

  // Particle Compute Pipeline.
  {
    Shader* compShader = pRhi->CreateShader();
    RendererPass::LoadShader("Particles.comp.spv", compShader);
    m_pParticleCompute = pRhi->CreateComputePipeline();
    VkPipelineShaderStageCreateInfo shaderCi = { };
    shaderCi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderCi.module = compShader->Handle();
    shaderCi.pName = kDefaultShaderEntryPointStr;
    shaderCi.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    VkComputePipelineCreateInfo pipeCi = { };
    pipeCi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeCi.stage = shaderCi;
    pipeCi.basePipelineHandle = VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo layoutCi = { };
    layoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkDescriptorSetLayout layouts[] = {
      GlobalSetLayoutKey->Layout(),
      m_pParticleDescriptorSetLayout->Layout()
    };
    layoutCi.setLayoutCount = 2;
    layoutCi.pSetLayouts = layouts;
    
    m_pParticleCompute->Initialize(pipeCi, layoutCi);
    pRhi->FreeShader(compShader);
  }
  R_DEBUG(rNotify, "Particle engine initialized.\n");
}


void ParticleEngine::InitializeRenderPass(VulkanRHI* pRhi)
{
  std::array<VkAttachmentDescription, 7> attachmentDescriptions;
  // final color.
  attachmentDescriptions[0] = {};
  attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[0].format = pbr_FinalTextureKey->Format();
  attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  // brightness output.
  attachmentDescriptions[1] = {};
  attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[1].format = pbr_BrightTextureKey->Format();
  attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  attachmentDescriptions[2] = {};
  attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[2].format = gbuffer_AlbedoAttachKey->Format();
  attachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  attachmentDescriptions[3] = {};
  attachmentDescriptions[3].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[3].format = gbuffer_NormalAttachKey->Format();
  attachmentDescriptions[3].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[3].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentDescriptions[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  attachmentDescriptions[4] = {};
  attachmentDescriptions[4].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[4].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[4].format = gbuffer_PositionAttachKey->Format();
  attachmentDescriptions[4].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[4].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentDescriptions[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  attachmentDescriptions[5] = {};
  attachmentDescriptions[5].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[5].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[5].format = gbuffer_EmissionAttachKey->Format();
  attachmentDescriptions[5].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[5].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[5].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentDescriptions[5].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[5].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  // depth load, from total scene. This is after deferred and forward pipeline.
  attachmentDescriptions[6] = {};
  attachmentDescriptions[6].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[6].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[6].format = gbuffer_DepthAttachKey->Format();
  attachmentDescriptions[6].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[6].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[6].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentDescriptions[6].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[6].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  std::array<VkSubpassDependency, 2> dependencies;
  dependencies[0] = {};
  dependencies[0] = CreateSubPassDependency(
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_DEPENDENCY_BY_REGION_BIT);
  dependencies[1] = {};
  dependencies[1] = CreateSubPassDependency(
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VK_DEPENDENCY_BY_REGION_BIT);

  VkSubpassDescription subpassDescription = {};
  std::array<VkAttachmentReference, 6> colorReferences;
  VkAttachmentReference depthStencilReference;

  colorReferences[0] = {};
  colorReferences[0].attachment = 0;
  colorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorReferences[1] = {};
  colorReferences[1].attachment = 1;
  colorReferences[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorReferences[2] = {};
  colorReferences[2].attachment = 2;
  colorReferences[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorReferences[3] = {};
  colorReferences[3].attachment = 3;
  colorReferences[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorReferences[4] = {};
  colorReferences[4].attachment = 4;
  colorReferences[4].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorReferences[5] = {};
  colorReferences[5].attachment = 5;
  colorReferences[5].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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


void ParticleEngine::CleanUp(VulkanRHI* pRhi)
{
  if ( m_pParticleDescriptorSetLayout ) {
    pRhi->FreeDescriptorSetLayout( m_pParticleDescriptorSetLayout );
    m_pParticleDescriptorSetLayout = nullptr;
  }

  if ( m_pParticleCompute ) {
    pRhi->FreeComputePipeline( m_pParticleCompute );
    m_pParticleCompute = nullptr;
  }

  if ( m_pFrameBuffer ) {
    pRhi->FreeFrameBuffer( m_pFrameBuffer );
    m_pFrameBuffer = nullptr;
  }

  CleanUpPipeline(pRhi);

  R_DEBUG(rNotify, "Particle engine cleaned up.\n");
}


void ParticleEngine::CleanUpPipeline(VulkanRHI* pRhi)
{
  if (m_pParticleRender) {
    pRhi->FreeGraphicsPipeline(m_pParticleRender);
    m_pParticleRender = nullptr;
  }

  if (m_pRenderPass) {
    pRhi->FreeRenderPass(m_pRenderPass);
    m_pRenderPass = nullptr;
  }
}


void ParticleEngine::InitializePipeline(VulkanRHI* pRhi)
{
  InitializeRenderPass(pRhi);
  m_pParticleRender = GenerateParticleRendererPipeline(pRhi, m_pParticleDescriptorSetLayout, m_pRenderPass);

}


ParticleEngine::~ParticleEngine()
{
  DEBUG_OP(
  if ( m_pParticleDescriptorSetLayout ) {
    R_DEBUG(rError, "Particle descriptor set layout not properly cleaned up!\n");
  });
}


void ParticleEngine::GenerateParticleComputeCommands(VulkanRHI* pRhi, CommandBuffer* cmdBuffer, GlobalDescriptor* global, 
  CmdList<ParticleSystem*>& particleList)
{
  if (particleList.Size() == 0) return;

  cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_pParticleCompute->Pipeline());

  for (size_t i = 0; i < particleList.Size(); ++i) {
    ParticleSystem* system = particleList[i];
    VkDescriptorSet sets[] = {
      global->Set()->Handle(),
      system->GetSet()->Handle()
    };
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, m_pParticleCompute->Layout(),
    0, 2, sets, 0, nullptr);
    cmdBuffer->Dispatch(1, 1, 1);
  }
}


void ParticleEngine::GenerateParticleRenderCommands(VulkanRHI* pRhi, CommandBuffer* cmdBuffer, 
  GlobalDescriptor* global, CmdList<ParticleSystem*>& particleList)
{
  if (particleList.Size() == 0) return;
  VkExtent2D extent = pRhi->SwapchainObject()->SwapchainExtent();
  VkRenderPassBeginInfo renderPassInfo = { };
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.framebuffer = pbr_forwardFrameBuffer->Handle();
  renderPassInfo.renderPass = m_pRenderPass->Handle();
  renderPassInfo.renderArea.extent = extent;
  renderPassInfo.renderArea.offset = { 0, 0 };
  std::array<VkClearValue, 7> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[4].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[5].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[6].depthStencil = { 1.0f, 0 };
  renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  cmdBuffer->BeginRenderPass(renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);  
  cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pParticleRender->Pipeline());
  VkViewport viewport = { };
  viewport.width = (r32)extent.width;
  viewport.height = (r32)extent.height;
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.minDepth = 0.0f;
  cmdBuffer->SetViewPorts(0, 1, &viewport);
  for (size_t i = 0; i < particleList.Size(); ++i) {
    ParticleSystem* system = particleList[i];
    VkDeviceSize offset[] = { 0 };
    VkDescriptorSet sets[] = { 
      global->Set()->Handle(),
      system->GetSet()->Handle()
    };
    VkBuffer nativeBuffer = system->GetParticleBuffer()->NativeBuffer();
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pParticleRender->Layout(),
      0, 2, sets, 0, nullptr);
    cmdBuffer->BindVertexBuffers(0, 1, &nativeBuffer, offset);
    cmdBuffer->Draw(system->_particleConfig._maxParticles, 1, 0, 0);
  }
  cmdBuffer->EndRenderPass();
}
} // Recluse