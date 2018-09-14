// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Particles.hpp"
#include "RendererData.hpp"
#include "Core/Logging/Log.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Shader.hpp"

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
  {
    Shader vertShader; vertShader.SetOwner(pRhi->LogicDevice()->Native());
    Shader fragShader; fragShader.SetOwner(pRhi->LogicDevice()->Native());
    Shader geomShader; geomShader.SetOwner(pRhi->LogicDevice()->Native());
    RendererPass::LoadShader("Particles.vert.spv", &vertShader);
    RendererPass::LoadShader("Particles.frag.spv", &fragShader);
    RendererPass::LoadShader("Particles.geom.spv", &geomShader);

    // TODO():

    vertShader.CleanUp();
    fragShader.CleanUp();
    geomShader.CleanUp();
  }

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

  R_DEBUG(rNotify, "Particle engine cleaned up.\n");
}


ParticleEngine::~ParticleEngine()
{
  if ( m_pParticleDescriptorSetLayout ) {
    R_DEBUG(rError, "Particle descriptor set layout not properly cleaned up!\n");
  }
}
} // Recluse