// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "RenderCmd.hpp"
#include "CmdList.hpp"

namespace Recluse {


class TextureCube;
class ComputePipeline;
class VulkanRHI;
class CommandBuffer;
class DescriptorSet;
class DescriptorSetLayout;
class Texture;
class FrameBuffer;
class RenderPass;
class GraphicsPipeline;
class GlobalDescriptor;

class BakeIBL {
public:
  BakeIBL();
  ~BakeIBL();

  void                  Initialize(VulkanRHI* pRhi);
  void                  CleanUp(VulkanRHI* pRhi);

  void                  RenderPrefilterSpec(CommandBuffer* pCmd, CmdList<MeshRenderCmd>* pCmdList);
  void                  RenderIrrDiff(CommandBuffer* pCmd, CmdList<MeshRenderCmd>* pCmdList);
  void                  RenderGenBRDF(CommandBuffer* pCmd, GlobalDescriptor* pGlobal, Texture* target);
  void                  UpdateTargetBRDF(Texture* target);
private:


  void                  SetUpDescriptorSetLayouts(VulkanRHI* pRhi);
  void                  SetUpComputePipelines(VulkanRHI* pRhi);

  void                  CleanUpDescriptorSetLayouts(VulkanRHI* pRhi);
  void                  CleanUpComputePipelines(VulkanRHI* pRhi);

  ComputePipeline*      m_pPipePrefilterSpecIBL;
  ComputePipeline*      m_pPipeGenBRDF;
  ComputePipeline*      m_pPipeIrradianceDiffIBL;
  DescriptorSetLayout*  m_pLayoutSpec;
  DescriptorSetLayout*  m_pLayoutBRDF;
  DescriptorSetLayout*  m_pLayoutDiff;
  DescriptorSet*        m_pSpecSet;
  DescriptorSet*        m_pDiffSet;
  DescriptorSet*        m_pBRDFSet;
};
} // Recluse