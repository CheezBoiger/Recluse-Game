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
  DescriptorSetLayout* pParticleConfigSetLayout, RenderPass* pEenderPass)
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