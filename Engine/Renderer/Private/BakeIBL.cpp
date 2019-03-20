// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "BakeIBL.hpp"
#include "RendererData.hpp"
#include "GlobalDescriptor.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Texture.hpp"
#include "RHI/ComputePipeline.hpp"
#include "RHI/Shader.hpp"

#include <array>

namespace Recluse {



BakeIBL::BakeIBL()
  : m_pPipePrefilterSpecIBL(VK_NULL_HANDLE)
  , m_pPipeGenBRDF(VK_NULL_HANDLE)
  , m_pPipeIrradianceDiffIBL(VK_NULL_HANDLE)
  , m_pLayoutSpec(VK_NULL_HANDLE)
  , m_pLayoutBRDF(VK_NULL_HANDLE)
  , m_pLayoutDiff(VK_NULL_HANDLE)
  , m_pSpecSet(VK_NULL_HANDLE)
  , m_pDiffSet(VK_NULL_HANDLE)
  , m_pBRDFSet(VK_NULL_HANDLE)
{
}


BakeIBL::~BakeIBL()
{
}


void BakeIBL::initialize(VulkanRHI* pRhi)
{
  setUpDescriptorSetLayouts(pRhi);
  SetUpComputePipelines(pRhi);
}


void BakeIBL::SetUpComputePipelines(VulkanRHI* pRhi)
{
  {
    m_pPipeGenBRDF = pRhi->createComputePipeline();
    Shader shader; shader.SetOwner(pRhi->logicDevice()->getNative());
    RendererPass::LoadShader("GenerateBRDFLUT.comp.spv", &shader);

    VkPipelineShaderStageCreateInfo shaderCi = { };
    shaderCi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderCi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderCi.module = shader.getHandle();
    shaderCi.pName = kDefaultShaderEntryPointStr;

    VkComputePipelineCreateInfo computeInfo = { };
    computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO; 
    computeInfo.basePipelineHandle = VK_NULL_HANDLE;
    computeInfo.stage = shaderCi;

    VkPipelineLayoutCreateInfo layoutInfo = { };
    VkDescriptorSetLayout layouts[] = { GlobalSetLayoutKey->getLayout(), m_pLayoutBRDF->getLayout() };
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 2;
    layoutInfo.pSetLayouts = layouts;
    
    m_pPipeGenBRDF->initialize(computeInfo, layoutInfo);
    shader.cleanUp();
  }
}


void BakeIBL::setUpDescriptorSetLayouts(VulkanRHI* pRhi)
{
  {
    m_pLayoutBRDF = pRhi->createDescriptorSetLayout();
    VkDescriptorSetLayoutBinding bind = { };
    bind.binding = 0;
    bind.descriptorCount = 1;
    bind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCi = { };
    layoutCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCi.bindingCount = 1;
    layoutCi.pBindings = &bind;
    m_pLayoutBRDF->initialize(layoutCi);

    m_pBRDFSet = pRhi->createDescriptorSet();
    m_pBRDFSet->allocate(pRhi->descriptorPool(), m_pLayoutBRDF);
  }
}


void BakeIBL::cleanUp(VulkanRHI* pRhi)
{
  cleanUpDescriptorSetLayouts(pRhi);
  CleanUpComputePipelines(pRhi);
}


void BakeIBL::cleanUpDescriptorSetLayouts(VulkanRHI* pRhi)
{
  if (m_pLayoutBRDF) {
    pRhi->freeDescriptorSetLayout(m_pLayoutBRDF);
    m_pLayoutBRDF = nullptr;
  }

  if (m_pBRDFSet) {
    pRhi->freeDescriptorSet(m_pBRDFSet);
    m_pBRDFSet = nullptr;
  }
}


void BakeIBL::CleanUpComputePipelines(VulkanRHI* pRhi)
{
  if (m_pPipeGenBRDF) {
    pRhi->freeComputePipeline(m_pPipeGenBRDF);
    m_pPipeGenBRDF = nullptr;
  }
}


void BakeIBL::RenderGenBRDF(CommandBuffer* pCmd, GlobalDescriptor* pGlobal, Texture* target, u32 frameIndex)
{
  if (!pCmd || !target) return;

  VkDescriptorSet sets[] = { pGlobal->getDescriptorSet(frameIndex)->getHandle(), m_pBRDFSet->getHandle() };
  pCmd->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_pPipeGenBRDF->Pipeline());
  pCmd->BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, m_pPipeGenBRDF->getLayout(),
    0, 2, sets, 0, nullptr);
  pCmd->Dispatch((target->getWidth() / 16) + 1, (target->getHeight() / 16) + 1, 1);
}


void BakeIBL::UpdateTargetBRDF(Texture* target)
{
  if (!target) { return; }
  std::array<VkWriteDescriptorSet, 1> writes;
  VkDescriptorImageInfo imgInfo = { };
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imgInfo.imageView = target->getView();
  imgInfo.sampler = nullptr;

  writes[0] = { };
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[0].dstBinding = 0;
  writes[0].dstArrayElement = 0;
  writes[0].pImageInfo = &imgInfo;
  m_pBRDFSet->update(static_cast<u32>(writes.size()), writes.data());
}
} // Recluse