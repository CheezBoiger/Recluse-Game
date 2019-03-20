// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "UIDescriptor.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/Texture.hpp"
#include "TextureType.hpp"

#include "RHI/DescriptorSet.hpp"
#include "UIOverlay.hpp"
#include "Renderer.hpp"
#include "RendererData.hpp"

#include <array>

namespace Recluse {


void UIDescriptor::initialize(VulkanRHI* pRhi)
{
  DescriptorSetLayout* layout = gRenderer().getOverlay()->GetMaterialLayout();
  
  m_pSet = pRhi->createDescriptorSet();
  m_pSet->allocate(pRhi->descriptorPool(), layout);
  update(pRhi);
}


void UIDescriptor::update(VulkanRHI* pRhi)
{
  VkDescriptorImageInfo imgInfo = { };
  Texture* pT = DefaultTextureKey;
  Sampler* pS = DefaultSampler2DKey;
  if (m_image) {
    pT = m_image->getHandle();
  }
  if (m_sampler) {
    pS = m_sampler;
  }
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgInfo.imageView = pT->getView();
  imgInfo.sampler = pS->getHandle();

  std::array<VkWriteDescriptorSet, 1> writes;
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].descriptorCount = 1;  
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writes[0].dstArrayElement = 0;
  writes[0].dstBinding = 0;
  writes[0].dstSet = nullptr;
  writes[0].pBufferInfo = nullptr;
  writes[0].pImageInfo = &imgInfo;

  m_pSet->update(1, writes.data());
}


void UIDescriptor::cleanUp(VulkanRHI* pRhi)
{
  if (m_pSet) {
    pRhi->freeDescriptorSet(m_pSet);
  }
}
} // Recluse