// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "UIDescriptor.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/Texture.hpp"
#include "TextureType.hpp"

#include "RHI/DescriptorSet.hpp"
#include "UIOverlay.hpp"
#include "Renderer.hpp"

namespace Recluse {


void UIDescriptor::Initialize(VulkanRHI* pRhi)
{
  DescriptorSetLayout* layout = gRenderer().Overlay()->GetMaterialLayout();
  
}


void UIDescriptor::Update(VulkanRHI* pRhi)
{
}


void UIDescriptor::CleanUp(VulkanRHI* pRhi)
{
}
} // Recluse