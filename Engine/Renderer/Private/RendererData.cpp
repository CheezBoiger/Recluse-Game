// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererData.hpp"
#include "VertexDescription.hpp"
#include "Resources.hpp"
#include "Core/Logging/Log.hpp"
#include "Renderer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Shader.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Framebuffer.hpp"


namespace Recluse {

std::string ShadersPath               = "Shaders";

std::string DefaultTextureStr         = "DefaultTexture";
std::string DefaultSamplerStr         = "DefaultSampler";

std::string ShadowMapPipelineStr      = "ShadowMapPipeline";
std::string ShadowMapVertFileStr      = "ShadowMapping.vert.spv";
std::string ShadowMapFragFileStr      = "ShadowMapping.frag.spv";

std::string PBRPipelineStr              = "PBRPipeline";
std::string PBRStaticPipelineStr        = "PBRStaticPipeline";
std::string PBRLayoutStr                = "PBRLayout";
std::string PBRColorAttachStr           = "PBRColor";
std::string PBRNormalAttachStr          = "PBRNormal";
std::string PBRDepthAttachStr           = "PBRDepth";
std::string PBRSamplerStr               = "PBRSampler";
std::string PBRFrameBufferStr           = "PBRFrameBuffer";
std::string PBRGlobalMatLayoutStr       = "PBRGlobalMaterialLayout";
std::string PBRLightMatLayoutStr        = "PBRLightMaterialLayout";
std::string PBRObjMatLayoutStr          = "PBRObjectMaterialLayout";
std::string PBRVertFileStr              = "PBRPass.vert.spv";
std::string PBRStaticVertFileStr        = "StaticPBRPass.vert.spv";
std::string PBRFragFileStr              = "PBRPass.frag.spv";

std::string ScaledSamplerStr            = "ScaledSampler";
std::string RenderTarget2xScaledStr     = "RenderTarget2x";
std::string RenderTarget4xScaledStr     = "RenderTarget4x";
std::string RenderTarget8xScaledStr     = "RenderTarget8x";
std::string FrameBuffer2xStr            = "FrameBuffer2x";
std::string FrameBuffer4xStr            = "FrameBuffer4x";
std::string FrameBuffer8xStr            = "FrameBuffer8x";
std::string GlowPipelineStr             = "GlowPipelineStr";
std::string DownscaleBlurPipeline2xStr  = "DownscaleBlurPipeline2x";
std::string DownscaleBlurPipeline4xStr  = "DownscaleBlurPipeline4x";
std::string DownscaleBlurPipeline8xStr  = "DownscaleBlurPipeline8x";
std::string DownscaleBlurLayoutStr      = "DownscaleBlurLayout";
std::string DownscaleBlurDescriptorSet  = "DownscaleBlurDescriptorSet";
std::string DownscaleBlurVertFileStr    = "DownscaleBlurPass.vert.spv";
std::string DownscaleBlurFragFileStr    = "DownscaleBlurPass.frag.spv";

std::string RenderTargetVelocityStr     = "VelocityMap";

std::string HDRGammaPipelineStr         = "HDRGammaPipeline";
std::string HDRGammaColorAttachStr      = "HDRGammaColor";
std::string HDRGammaFrameBufferStr      = "HDRGammaFrameBuffer";
std::string HDRGammaSamplerStr          = "HDRGammaSampler";
std::string HDRGammaDescSetStr          = "HDRGammaSet";
std::string HDRGammaDescSetLayoutStr    = "HDRGammaSetLayout";
std::string HDRGammaVertFileStr         = "HDRGammaPass.vert.spv";
std::string HDRGammaFragFileStr         = "HDRGammaPass.frag.spv";

std::string FinalPipelineStr            = "FinalPipeline";
std::string FinalDescSetStr             = "FinalSet";
std::string FinalDescSetLayoutStr       = "FinalSetLayout";
std::string FinalVertFileStr            = "FinalPass.vert.spv";
std::string FinalFragFileStr            = "FinalPass.frag.spv";


namespace RendererPass {


void SetUpPBRForwardPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  // PbrForward Pipeline Creation.
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  GraphicsPipeline* PbrForwardPipeline = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* PbrStaticPipeline = Rhi->CreateGraphicsPipeline();
  FrameBuffer* PbrFrameBuffer = gResources().GetFrameBuffer(PBRFrameBufferStr);
  Shader* VertPBR = Rhi->CreateShader();
  Shader* FragPBR = Rhi->CreateShader();

  gResources().RegisterGraphicsPipeline(PBRPipelineStr, PbrForwardPipeline);
  gResources().RegisterGraphicsPipeline(PBRStaticPipelineStr, PbrStaticPipeline);
  if (!VertPBR->Initialize(Filepath + "/" + ShadersPath + "/" + PBRVertFileStr)) {
    Log(rError) << "Could not find " + PBRVertFileStr + "!";
  }

  if (!FragPBR->Initialize(Filepath + "/" + ShadersPath + "/" + PBRFragFileStr)) {
    Log(rError) << "Could not find " + PBRFragFileStr + "!";
  }

  VkPipelineShaderStageCreateInfo PbrShaders[2];
  PbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[0].module = VertPBR->Handle();
  PbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  PbrShaders[0].pName = "main";
  PbrShaders[0].pNext = nullptr;
  PbrShaders[0].pSpecializationInfo = nullptr;
  PbrShaders[0].flags = 0;

  PbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[1].module = FragPBR->Handle();
  PbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  PbrShaders[1].pName = "main";
  PbrShaders[1].pNext = nullptr;
  PbrShaders[1].flags = 0;
  PbrShaders[1].pSpecializationInfo = nullptr;

  GraphicsInfo.renderPass = PbrFrameBuffer->RenderPass();
  GraphicsInfo.stageCount = 2;
  GraphicsInfo.pStages = PbrShaders;

  VkDescriptorSetLayout DLayouts[3];
  DLayouts[0] = gResources().GetDescriptorSetLayout(PBRGlobalMatLayoutStr)->Layout();
  DLayouts[1] = gResources().GetDescriptorSetLayout(PBRObjMatLayoutStr)->Layout();
  DLayouts[2] = gResources().GetDescriptorSetLayout(PBRLightMatLayoutStr)->Layout();

  VkPipelineLayoutCreateInfo PipelineLayout = {};
  PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipelineLayout.setLayoutCount = 3;
  PipelineLayout.pSetLayouts = DLayouts;
  PipelineLayout.pPushConstantRanges = 0;
  PipelineLayout.pushConstantRangeCount = 0;

  // Initialize pbr forward pipeline.
  PbrForwardPipeline->Initialize(GraphicsInfo, PipelineLayout);

  // Static pipeline creation.
  auto Bindings = StaticVertexDescription::GetBindingDescription();
  auto VertexAttribs = StaticVertexDescription::GetVertexAttributes();
  VkPipelineVertexInputStateCreateInfo Input = { };

  GraphicsInfo.pVertexInputState = &Input;
  Input.vertexAttributeDescriptionCount = static_cast<u32>(VertexAttribs.size());
  Input.vertexBindingDescriptionCount = 1;
  Input.pVertexBindingDescriptions = &Bindings;
  Input.pVertexAttributeDescriptions = VertexAttribs.data();
  Input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  Input.pNext = nullptr;

  Rhi->FreeShader(VertPBR);
  VertPBR = Rhi->CreateShader();
  if (!VertPBR->Initialize(Filepath + "/" + ShadersPath + "/" + PBRStaticVertFileStr)) {
    Log(rError) << "Could not find " + PBRStaticVertFileStr + "!";
  }
  
  PbrShaders[0].module = VertPBR->Handle();
  PbrStaticPipeline->Initialize(GraphicsInfo, PipelineLayout);
  
  Rhi->FreeShader(VertPBR);
  Rhi->FreeShader(FragPBR);
}


void SetUpHDRGammaPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  GraphicsPipeline* hdrPipeline = Rhi->CreateGraphicsPipeline();
  VkPipelineLayoutCreateInfo hdrLayout = {};
  VkDescriptorSetLayout hdrSetLayout = gResources().GetDescriptorSetLayout(HDRGammaDescSetLayoutStr)->Layout();

  Shader* HdrFrag = Rhi->CreateShader();
  Shader* HdrVert = Rhi->CreateShader();

  if (!HdrFrag->Initialize(Filepath + "/" + ShadersPath + "/" + HDRGammaFragFileStr)) {
    Log(rError) << "Could not find " + HDRGammaFragFileStr + "!";
  }

  if (!HdrVert->Initialize(Filepath + "/" + ShadersPath + "/" + HDRGammaVertFileStr)) {
    Log(rError) << "Could not find " + HDRGammaVertFileStr + "!";
  }

  FrameBuffer* hdrBuffer = gResources().GetFrameBuffer(HDRGammaFrameBufferStr);
  GraphicsInfo.renderPass = hdrBuffer->RenderPass();

  VkPipelineShaderStageCreateInfo ShaderModules[2];
  ShaderModules[0].flags = 0;
  ShaderModules[0].module = HdrVert->Handle();
  ShaderModules[0].pName = "main";
  ShaderModules[0].pNext = nullptr;
  ShaderModules[0].pSpecializationInfo = nullptr;
  ShaderModules[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  ShaderModules[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  ShaderModules[1].flags = 0;
  ShaderModules[1].module = HdrFrag->Handle();
  ShaderModules[1].pName = "main";
  ShaderModules[1].pNext = nullptr;
  ShaderModules[1].pSpecializationInfo = nullptr;
  ShaderModules[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  ShaderModules[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  GraphicsInfo.pStages = ShaderModules;
  GraphicsInfo.stageCount = 2;

  hdrLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  hdrLayout.setLayoutCount = 1;
  hdrLayout.pSetLayouts = &hdrSetLayout;

  hdrPipeline->Initialize(GraphicsInfo, hdrLayout);

  Rhi->FreeShader(HdrFrag);
  Rhi->FreeShader(HdrVert);
  gResources().RegisterGraphicsPipeline(HDRGammaPipelineStr, hdrPipeline);

}


void SetUpDownScalePass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;

  // TODO(): Glow and Downsampling graphics pipeline, which will be done right after pbr 
  // pass. 
  GraphicsPipeline* Downscale2x = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* Downscale4x = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* Downscale8x = Rhi->CreateGraphicsPipeline();
  FrameBuffer*      FrameBuffer2x = gResources().GetFrameBuffer(FrameBuffer2xStr);
  FrameBuffer*      FrameBuffer4x = gResources().GetFrameBuffer(FrameBuffer4xStr);
  FrameBuffer*      FrameBuffer8x = gResources().GetFrameBuffer(FrameBuffer8xStr);
  gResources().RegisterGraphicsPipeline(DownscaleBlurPipeline2xStr, Downscale2x);
  gResources().RegisterGraphicsPipeline(DownscaleBlurPipeline4xStr, Downscale4x);
  gResources().RegisterGraphicsPipeline(DownscaleBlurPipeline8xStr, Downscale8x);
  DescriptorSetLayout* DownscaleDescLayout = gResources().GetDescriptorSetLayout(DownscaleBlurLayoutStr);

  Shader* DbVert = Rhi->CreateShader();
  Shader* DbFrag = Rhi->CreateShader();
  if (!DbVert->Initialize(Filepath + "/" + ShadersPath + "/" + DownscaleBlurVertFileStr)) {
    Log(rError) << "Could not find " + DownscaleBlurVertFileStr + "!\n";
  }

  if (!DbFrag->Initialize(Filepath + "/" + ShadersPath + "/" + DownscaleBlurFragFileStr)) {
    Log(rError) << "Could not find " + DownscaleBlurFragFileStr + "!\n";
  }

  VkPushConstantRange PushConst = {};
  PushConst.offset = 0;
  PushConst.size = 4;
  PushConst.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayout DwnsclLayout[] = { DownscaleDescLayout->Layout() };
  VkPipelineLayoutCreateInfo DownscaleLayout = {};
  DownscaleLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  DownscaleLayout.pushConstantRangeCount = 1;
  DownscaleLayout.pPushConstantRanges = &PushConst;
  DownscaleLayout.setLayoutCount = 1;
  DownscaleLayout.pSetLayouts = DwnsclLayout;

  VkPipelineShaderStageCreateInfo ShaderModules[2];
  ShaderModules[0].flags = 0;
  ShaderModules[0].module = DbVert->Handle();
  ShaderModules[0].pName = "main";
  ShaderModules[0].pNext = nullptr;
  ShaderModules[0].pSpecializationInfo = nullptr;
  ShaderModules[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  ShaderModules[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  ShaderModules[1].flags = 0;
  ShaderModules[1].module = DbFrag->Handle();
  ShaderModules[1].pName = "main";
  ShaderModules[1].pNext = nullptr;
  ShaderModules[1].pSpecializationInfo = nullptr;
  ShaderModules[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  ShaderModules[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  GraphicsInfo.pStages = ShaderModules;
  GraphicsInfo.stageCount = 2;
  GraphicsInfo.renderPass = FrameBuffer2x->RenderPass();
  
  Downscale2x->Initialize(GraphicsInfo, DownscaleLayout);
  GraphicsInfo.renderPass = FrameBuffer4x->RenderPass();
  Downscale4x->Initialize(GraphicsInfo, DownscaleLayout);
  GraphicsInfo.renderPass = FrameBuffer8x->RenderPass();
  Downscale8x->Initialize(GraphicsInfo, DownscaleLayout);

  Rhi->FreeShader(DbVert);
  Rhi->FreeShader(DbFrag);
}


void SetUpFinalPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  Shader* quadVert = Rhi->CreateShader();
  Shader* quadFrag = Rhi->CreateShader();
  // TODO(): Need to structure all of this into more manageable modules.
  //
  GraphicsPipeline* quadPipeline = Rhi->CreateGraphicsPipeline();
  gResources().RegisterGraphicsPipeline(FinalPipelineStr, quadPipeline);

  if (!quadVert->Initialize(Filepath + "/" + ShadersPath + "/" + FinalVertFileStr)) {
    Log(rError) << "Could not find " + FinalVertFileStr + "!";
  }

  if (!quadFrag->Initialize(Filepath + "/" + ShadersPath + "/" + FinalFragFileStr)) {
    Log(rError) << "Could not find " + FinalFragFileStr + "!";
  }

  GraphicsInfo.renderPass = Rhi->SwapchainRenderPass();

  VkPipelineShaderStageCreateInfo FinalShaders[2];
  FinalShaders[0].flags = 0;
  FinalShaders[0].module = quadVert->Handle();
  FinalShaders[0].pName = "main";
  FinalShaders[0].pNext = nullptr;
  FinalShaders[0].pSpecializationInfo = nullptr;
  FinalShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  FinalShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  FinalShaders[1].flags = 0;
  FinalShaders[1].module = quadFrag->Handle();
  FinalShaders[1].pName = "main";
  FinalShaders[1].pNext = nullptr;
  FinalShaders[1].pSpecializationInfo = nullptr;
  FinalShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  FinalShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  GraphicsInfo.stageCount = 2;
  GraphicsInfo.pStages = FinalShaders;

  VkPipelineLayoutCreateInfo finalLayout = {};
  finalLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  finalLayout.setLayoutCount = 1;
  VkDescriptorSetLayout finalL = gResources().GetDescriptorSetLayout(FinalDescSetLayoutStr)->Layout();
  finalLayout.pSetLayouts = &finalL;
  finalLayout.pushConstantRangeCount = 0;
  finalLayout.pPushConstantRanges = nullptr;

  quadPipeline->Initialize(GraphicsInfo, finalLayout);

  Rhi->FreeShader(quadVert);
  Rhi->FreeShader(quadFrag);
}


void SetUpShadowPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  // ShadowMapping shader.
  // TODO(): Shadow mapping MUST be deferred until downsampling and glow buffers have finished!
  // This will prevent blurry shadows.
  Shader* SmVert = Rhi->CreateShader();
  Shader* SmFrag = Rhi->CreateShader();

  if (!SmVert->Initialize(Filepath + "/" + ShadersPath + "/" + ShadowMapVertFileStr)) {
    Log(rError) << "Could not find " + ShadowMapVertFileStr + "!\n";
  }

  if (!SmFrag->Initialize(Filepath + "/" + ShadersPath + "/" + ShadowMapFragFileStr)) {
    Log(rError) << "Could not find " + ShadowMapFragFileStr + "!\n";
  }

  Rhi->FreeShader(SmVert);
  Rhi->FreeShader(SmFrag);
}
} // RendererPass
} // Recluse