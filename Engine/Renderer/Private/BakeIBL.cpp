// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "BakeIBL.hpp"
#include "RendererData.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Texture.hpp"


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


void BakeIBL::Initialize(VulkanRHI* pRhi)
{
}


void BakeIBL::CleanUp(VulkanRHI* pRhi)
{
}
} // Recluse