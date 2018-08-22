// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "UIDescriptor.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/Texture.hpp"
#include "TextureType.hpp"

#include "RHI/DescriptorSet.hpp"
#include "UIOverlay.hpp"
#include "Renderer.hpp"

#include <array>

namespace Recluse {


void UIDescriptor::Initialize(VulkanRHI* pRhi)
{
  DescriptorSetLayout* layout = gRenderer().Overlay()->GetMaterialLayout();
  
  m_pSet = pRhi->CreateDescriptorSet();
  m_pSet->Allocate(pRhi->DescriptorPool(), layout);
  std::array<VkWriteDescriptorSet, 1> writes;
}


void UIDescriptor::Update(VulkanRHI* pRhi)
{
}


void UIDescriptor::CleanUp(VulkanRHI* pRhi)
{
  if (m_pSet) {
    pRhi->FreeDescriptorSet(m_pSet);
  }
}
} // Recluse