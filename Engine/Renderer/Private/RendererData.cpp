// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererData.hpp"
#include "VertexDescription.hpp"
#include "Resources.hpp"
#include "Core/Logging/Log.hpp"
#include "Renderer.hpp"
#include "SkyAtmosphere.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Shader.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Framebuffer.hpp"

#include <array>

#include "Filesystem/Filesystem.hpp"

namespace Recluse {

std::string ShadersPath               = "Shaders";

std::string DefaultTextureStr         = "DefaultTexture";
std::string DefaultSamplerStr         = "DefaultSampler";

std::string ShadowMapPipelineStr              = "ShadowMapPipeline";
std::string DynamicShadowMapPipelineStr       = "DynamicShadowMapPipeline";
std::string DynamicShadowMapVertFileStr       = "DynamicShadowMapping.vert.spv";
std::string ShadowMapVertFileStr              = "ShadowMapping.vert.spv";
std::string ShadowMapFragFileStr              = "ShadowMapping.frag.spv";
std::string LightViewDescriptorSetLayoutStr   = "LightViewDescriptorLayout";

std::string gbuffer_PipelineStr               = "GBufferPipeline";
std::string gbuffer_StaticPipelineStr         = "StaticGBufferPipeline";
std::string gbuffer_LayoutStr                 = "GBufferLayout";
std::string gbuffer_AlbedoAttachStr           = "AlbedoColor";
std::string gbuffer_NormalAttachStr           = "NormalColor";
std::string gbuffer_PositionAttachStr         = "PositionColor";
std::string gbuffer_RoughMetalAttachStr       = "RoughMetalColor";
std::string gbuffer_EmissionAttachStr         = "EmissionColor";
std::string gbuffer_DepthAttachStr            = "GBufferDepth";
std::string gbuffer_SamplerStr                = "GBufferSampler";
std::string gbuffer_FrameBufferStr            = "GBufferFrameBuffer";
std::string gbuffer_VertFileStr               = "GBuffer.vert.spv";
std::string gbuffer_StaticVertFileStr         = "StaticGBuffer.vert.spv";
std::string gbuffer_FragFileStr               = "GBuffer.frag.spv";

std::string pbr_PipelineStr                   = "PBRPipeline";
std::string pbr_FrameBufferStr                = "PBRFrameBuffer";
std::string pbr_DescLayoutStr                 = "PBRDescLayout";
std::string pbr_DescSetStr                    = "PBRDescSet";
std::string pbr_FinalTextureStr               = "RenderTargetColor";
std::string pbr_BrightTextureStr              = "RenderTargetBright";
std::string pbr_VertStr                       = "PBR.vert.spv";
std::string pbr_FragStr                       = "PBR.frag.spv";

std::string RenderTargetBlurHoriz4xStr  = "RTBlurHoriz4xTemp";
std::string FrameBuffer4xHorizStr       = "FrameBufferHoriz4xStr";
std::string MeshSetLayoutStr            = "MeshDescriptorSetLayout";
std::string MaterialSetLayoutStr        = "MaterialDescriptorSetLayout";
std::string BonesSetLayoutStr           = "BonesDescriptorSetLayout";
std::string GlobalSetLayoutStr          = "GlobalDescriptorSetLayout";
std::string LightSetLayoutStr           = "LightDescriptorSetLayout";

std::string SkyboxPipelineStr           = "SkyboxPipeline";
std::string SkyboxDescriptorSetStr      = "SkyboxSet";
std::string SkyboxSetLayoutStr          = "SkyboxLayout";

std::string ScaledSamplerStr            = "ScaledSampler";
std::string RenderTarget2xHorizStr      = "RenderTarget2x";
std::string RenderTarget2xFinalStr      = "RenderTarget2xFinal";
std::string RenderTarget4xScaledStr     = "RenderTarget4x";
std::string RenderTarget4xFinalStr      = "RenderTarget4xFinal";
std::string RenderTarget8xScaledStr     = "RenderTarget8x";
std::string RenderTarget8xFinalStr      = "RenderTarget8xFinal";
std::string RenderTarget16xScaledStr    = "RenderTarget16x";
std::string RenderTarget16xFinalStr     = "RenderTarget16xFinal";
std::string FrameBuffer2xHorizStr       = "FrameBuffer2x";
std::string FrameBuffer2xFinalStr       = "FrameBuffer2xFinal";
std::string FrameBuffer4xFinalStr       = "FrameBuffer4xFinal";
std::string FrameBuffer4xStr            = "FrameBuffer4x";
std::string FrameBuffer8xStr            = "FrameBuffer8x";
std::string FrameBuffer8xFinalStr       = "FrameBuffer8xFinal";
std::string FrameBuffer16xStr           = "FrameBuffer16x";
std::string FrameBuffer16xFinalStr      = "FrameBuffer16xFinal";
std::string GlowPipelineStr             = "GlowPipelineStr";
std::string GlowFragFileStr             = "GlowPass.frag.spv";
std::string RenderTargetGlowStr         = "RenderTargetGlow";
std::string FrameBufferGlowStr          = "FrameBufferGlow";
std::string GlowDescriptorSetLayoutStr  = "GlowDescriptorSetLayout";
std::string GlowDescriptorSetStr        = "GlowDescriptorSet";
std::string DownscaleBlurPipeline2xStr  = "DownscaleBlurPipeline2x";
std::string DownscaleBlurPipeline4xStr  = "DownscaleBlurPipeline4x";
std::string DownscaleBlurPipeline8xStr  = "DownscaleBlurPipeline8x";
std::string DownscaleBlurPipeline16xStr = "DownscaleBlurPipeline16x";
std::string DownscaleBlurLayoutStr      = "DownscaleBlurLayout";
std::string DownscaleBlurDescriptorSet2x          = "DownscaleBlurDescriptorSet2x";
std::string DownscaleBlurDescriptorSet2xFinalStr  = "DownscaleFinal2x";
std::string DownscaleBlurDescriptorSet4x          = "DownscaleBlurDescriptorSet4x";
std::string DownscaleBlurDescriptorSet4xFinalStr  = "DownscaleFinal4x";
std::string DownscaleBlurDescriptorSet8x          = "DownscaleBlurDescriptorSet8x";
std::string DownscaleBlurDescriptorSet8xFinalStr  = "DownscaleFinal8x";
std::string DownscaleBlurDescriptorSet16x         = "DownscaleBlurDescriptorSet16x";
std::string DownscaleBlurDescriptorSet16xFinalStr = "DownScaleFinal16x";
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

// Default entry point on shaders.
char const* kDefaultShaderEntryPointStr = "main";


namespace RendererPass {


void LoadShader(std::string Filename, Shader* S)
{
  if (!S) { Log(rError) << "Shader module is null! Can not load a shader!\n"; }
  std::string Filepath = gFilesystem().CurrentAppDirectory();
  if (!S->Initialize(Filepath
      + "/" + ShadersPath + "/" + Filename)) {
    Log(rError) << "Could not find " + Filename + "!";
  }
}


void SetUpGBufferPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  // PbrForward Pipeline Creation.
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  GraphicsPipeline* GBufferPipeline = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* GBufferStaticPipeline = Rhi->CreateGraphicsPipeline();
  FrameBuffer* GBufferFrameBuffer = gResources().GetFrameBuffer(gbuffer_FrameBufferStr);
  Shader* VertGBuffer = Rhi->CreateShader();
  Shader* FragGBuffer = Rhi->CreateShader();

  gResources().RegisterGraphicsPipeline(gbuffer_PipelineStr, GBufferPipeline);
  gResources().RegisterGraphicsPipeline(gbuffer_StaticPipelineStr, GBufferStaticPipeline);

  LoadShader(gbuffer_VertFileStr, VertGBuffer);
  LoadShader(gbuffer_FragFileStr, FragGBuffer);

  VkPipelineShaderStageCreateInfo PbrShaders[2];
  PbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[0].module = VertGBuffer->Handle();
  PbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  PbrShaders[0].pName = kDefaultShaderEntryPointStr;
  PbrShaders[0].pNext = nullptr;
  PbrShaders[0].pSpecializationInfo = nullptr;
  PbrShaders[0].flags = 0;

  PbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[1].module = FragGBuffer->Handle();
  PbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  PbrShaders[1].pName = kDefaultShaderEntryPointStr;
  PbrShaders[1].pNext = nullptr;
  PbrShaders[1].flags = 0;
  PbrShaders[1].pSpecializationInfo = nullptr;

  GraphicsInfo.renderPass = GBufferFrameBuffer->RenderPass();
  GraphicsInfo.stageCount = 2;
  GraphicsInfo.pStages = PbrShaders;

  std::array<VkDescriptorSetLayout, 4> DLayouts;
  DLayouts[0] = gResources().GetDescriptorSetLayout(GlobalSetLayoutStr)->Layout();
  DLayouts[1] = gResources().GetDescriptorSetLayout(MeshSetLayoutStr)->Layout();
  DLayouts[2] = gResources().GetDescriptorSetLayout(MaterialSetLayoutStr)->Layout();
  DLayouts[3] = gResources().GetDescriptorSetLayout(BonesSetLayoutStr)->Layout();

  VkPipelineLayoutCreateInfo PipelineLayout = {};
  PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipelineLayout.setLayoutCount = static_cast<u32>(DLayouts.size());
  PipelineLayout.pSetLayouts = DLayouts.data();
  PipelineLayout.pPushConstantRanges = 0;
  PipelineLayout.pushConstantRangeCount = 0;

  // Initialize pbr forward pipeline.
  GBufferPipeline->Initialize(GraphicsInfo, PipelineLayout);

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

  Rhi->FreeShader(VertGBuffer);
  VertGBuffer = Rhi->CreateShader();
  LoadShader(gbuffer_StaticVertFileStr, VertGBuffer);
  
  PbrShaders[0].module = VertGBuffer->Handle();
  PipelineLayout.setLayoutCount = static_cast<u32>(DLayouts.size() - 1); // We don't need bone buffer.
  GBufferStaticPipeline->Initialize(GraphicsInfo, PipelineLayout);
  
  Rhi->FreeShader(VertGBuffer);
  Rhi->FreeShader(FragGBuffer);
}


void SetUpHDRGammaPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  GraphicsPipeline* hdrPipeline = Rhi->CreateGraphicsPipeline();
  VkPipelineLayoutCreateInfo hdrLayout = {};
  VkDescriptorSetLayout hdrSetLayout[1]; 
  hdrSetLayout[0] = gResources().GetDescriptorSetLayout(HDRGammaDescSetLayoutStr)->Layout();

  Shader* HdrFrag = Rhi->CreateShader();
  Shader* HdrVert = Rhi->CreateShader();

  LoadShader(HDRGammaVertFileStr, HdrVert);
  LoadShader(HDRGammaFragFileStr, HdrFrag);

  FrameBuffer* hdrBuffer = gResources().GetFrameBuffer(HDRGammaFrameBufferStr);
  GraphicsInfo.renderPass = hdrBuffer->RenderPass();

  VkPipelineShaderStageCreateInfo ShaderModules[2];
  ShaderModules[0].flags = 0;
  ShaderModules[0].module = HdrVert->Handle();
  ShaderModules[0].pName = kDefaultShaderEntryPointStr;
  ShaderModules[0].pNext = nullptr;
  ShaderModules[0].pSpecializationInfo = nullptr;
  ShaderModules[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  ShaderModules[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  ShaderModules[1].flags = 0;
  ShaderModules[1].module = HdrFrag->Handle();
  ShaderModules[1].pName = kDefaultShaderEntryPointStr;
  ShaderModules[1].pNext = nullptr;
  ShaderModules[1].pSpecializationInfo = nullptr;
  ShaderModules[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  ShaderModules[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  GraphicsInfo.pStages = ShaderModules;
  GraphicsInfo.stageCount = 2;

  hdrLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  hdrLayout.setLayoutCount = 1;
  hdrLayout.pSetLayouts = hdrSetLayout;

  hdrPipeline->Initialize(GraphicsInfo, hdrLayout);

  Rhi->FreeShader(HdrFrag);
  Rhi->FreeShader(HdrVert);
  gResources().RegisterGraphicsPipeline(HDRGammaPipelineStr, hdrPipeline);

}


void SetUpPhysicallyBasedPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  GraphicsPipeline* pbr_Pipeline = Rhi->CreateGraphicsPipeline();
  gResources().RegisterGraphicsPipeline(pbr_PipelineStr, pbr_Pipeline);

  FrameBuffer* pbr_FrameBuffer = gResources().GetFrameBuffer(pbr_FrameBufferStr);  

  Shader* VertPBR = Rhi->CreateShader();
  Shader* FragPBR = Rhi->CreateShader();

  LoadShader(pbr_VertStr, VertPBR);
  LoadShader(pbr_FragStr, FragPBR);

  VkPipelineShaderStageCreateInfo PbrShaders[2];
  PbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[0].module = VertPBR->Handle();
  PbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  PbrShaders[0].pName = kDefaultShaderEntryPointStr;
  PbrShaders[0].pNext = nullptr;
  PbrShaders[0].pSpecializationInfo = nullptr;
  PbrShaders[0].flags = 0;

  PbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[1].module = FragPBR->Handle();
  PbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  PbrShaders[1].pName = kDefaultShaderEntryPointStr;
  PbrShaders[1].pNext = nullptr;
  PbrShaders[1].flags = 0;
  PbrShaders[1].pSpecializationInfo = nullptr;

  GraphicsInfo.renderPass = pbr_FrameBuffer->RenderPass();
  GraphicsInfo.stageCount = 2;
  GraphicsInfo.pStages = PbrShaders;

  std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments;
  colorBlendAttachments[0] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[1] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  VkPipelineColorBlendStateCreateInfo colorBlendCI = CreateBlendStateInfo(
    static_cast<u32>(colorBlendAttachments.size()),
    colorBlendAttachments.data(),
    VK_FALSE,
    VK_LOGIC_OP_COPY
  );

  GraphicsInfo.pColorBlendState = &colorBlendCI;

  std::array<VkDescriptorSetLayout, 4> layouts;
  layouts[0] = gResources().GetDescriptorSetLayout(GlobalSetLayoutStr)->Layout();
  layouts[1] = gResources().GetDescriptorSetLayout(pbr_DescLayoutStr)->Layout();
  layouts[2] = gResources().GetDescriptorSetLayout(LightSetLayoutStr)->Layout();
  layouts[3] = gResources().GetDescriptorSetLayout(LightViewDescriptorSetLayoutStr)->Layout();

  VkPipelineLayoutCreateInfo PipelineLayout = {};
  PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipelineLayout.setLayoutCount = static_cast<u32>(layouts.size());
  PipelineLayout.pSetLayouts = layouts.data();
  PipelineLayout.pPushConstantRanges = 0;
  PipelineLayout.pushConstantRangeCount = 0;

  pbr_Pipeline->Initialize(GraphicsInfo, PipelineLayout);

  Rhi->FreeShader(VertPBR);
  Rhi->FreeShader(FragPBR);
}


void SetUpDownScalePass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;

  // TODO(): Glow and Downsampling graphics pipeline, which will be done right after pbr 
  // pass. 
  VkPipelineInputAssemblyStateCreateInfo n = { };
  n.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  n.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  n.primitiveRestartEnable = VK_FALSE;
  GraphicsInfo.pInputAssemblyState = &n;

  GraphicsPipeline* Downscale2x = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* Downscale4x = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* Downscale8x = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* Downscale16x = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* GlowPipeline = Rhi->CreateGraphicsPipeline();
  // Scaled and Final framebuffers have the same renderpass, so we can just use 
  // one of their renderpasses.
  FrameBuffer*      FrameBuffer2x = gResources().GetFrameBuffer(FrameBuffer2xHorizStr);
  FrameBuffer*      FrameBuffer4x = gResources().GetFrameBuffer(FrameBuffer4xStr);
  FrameBuffer*      FrameBuffer8x = gResources().GetFrameBuffer(FrameBuffer8xStr);
  FrameBuffer*      FrameBuffer16x = gResources().GetFrameBuffer(FrameBuffer16xStr);
  FrameBuffer*      GlowFrameBuffer = gResources().GetFrameBuffer(FrameBufferGlowStr);
  gResources().RegisterGraphicsPipeline(DownscaleBlurPipeline2xStr, Downscale2x);
  gResources().RegisterGraphicsPipeline(DownscaleBlurPipeline4xStr, Downscale4x);
  gResources().RegisterGraphicsPipeline(DownscaleBlurPipeline8xStr, Downscale8x);
  gResources().RegisterGraphicsPipeline(DownscaleBlurPipeline16xStr, Downscale16x);
  gResources().RegisterGraphicsPipeline(GlowPipelineStr, GlowPipeline);
  DescriptorSetLayout* DownscaleDescLayout = gResources().GetDescriptorSetLayout(DownscaleBlurLayoutStr);
  DescriptorSetLayout* GlowDescLayout = gResources().GetDescriptorSetLayout(GlowDescriptorSetLayoutStr);

  Shader* DbVert = Rhi->CreateShader();
  Shader* DbFrag = Rhi->CreateShader();

  LoadShader(DownscaleBlurVertFileStr, DbVert);
  LoadShader(DownscaleBlurFragFileStr, DbFrag);

  VkPushConstantRange PushConst = {};
  PushConst.offset = 0;
  PushConst.size = 12;
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
  ShaderModules[0].pName = kDefaultShaderEntryPointStr;
  ShaderModules[0].pNext = nullptr;
  ShaderModules[0].pSpecializationInfo = nullptr;
  ShaderModules[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  ShaderModules[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  ShaderModules[1].flags = 0;
  ShaderModules[1].module = DbFrag->Handle();
  ShaderModules[1].pName = kDefaultShaderEntryPointStr;
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
  GraphicsInfo.renderPass = FrameBuffer16x->RenderPass();
  Downscale16x->Initialize(GraphicsInfo, DownscaleLayout);

  Rhi->FreeShader(DbFrag);
  DbFrag = Rhi->CreateShader();

  LoadShader(GlowFragFileStr, DbFrag);

  ShaderModules[1].module = DbFrag->Handle();
  VkPipelineLayoutCreateInfo GlowPipelineLayout = { };
  VkDescriptorSetLayout GlowDescSetLayout = GlowDescLayout->Layout();
  GlowPipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  GlowPipelineLayout.pSetLayouts = &GlowDescSetLayout;
  GlowPipelineLayout.setLayoutCount = 1;
  GlowPipelineLayout.pPushConstantRanges = nullptr;
  GlowPipelineLayout.pushConstantRangeCount = 0;

  GraphicsInfo.renderPass = GlowFrameBuffer->RenderPass();
  GlowPipeline->Initialize(GraphicsInfo, GlowPipelineLayout);

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
  VkPipelineInputAssemblyStateCreateInfo n = { };

  GraphicsPipeline* quadPipeline = Rhi->CreateGraphicsPipeline();
  gResources().RegisterGraphicsPipeline(FinalPipelineStr, quadPipeline);

  LoadShader(FinalVertFileStr, quadVert);
  LoadShader(FinalFragFileStr, quadFrag);

  GraphicsInfo.renderPass = Rhi->SwapchainRenderPass();

  VkPipelineShaderStageCreateInfo FinalShaders[2];
  FinalShaders[0].flags = 0;
  FinalShaders[0].module = quadVert->Handle();
  FinalShaders[0].pName = kDefaultShaderEntryPointStr;
  FinalShaders[0].pNext = nullptr;
  FinalShaders[0].pSpecializationInfo = nullptr;
  FinalShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  FinalShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  FinalShaders[1].flags = 0;
  FinalShaders[1].module = quadFrag->Handle();
  FinalShaders[1].pName = kDefaultShaderEntryPointStr;
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


void SetUpDirectionalShadowPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = DefaultInfo;
  GraphicsPipeline* ShadowMapPipeline = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* DynamicShadowMapPipeline = Rhi->CreateGraphicsPipeline();
  gResources().RegisterGraphicsPipeline(ShadowMapPipelineStr, ShadowMapPipeline);
  gResources().RegisterGraphicsPipeline(DynamicShadowMapPipelineStr, DynamicShadowMapPipeline);

  // TODO(): Initialize shadow map pipeline.
  VkPipelineLayoutCreateInfo PipeLayout = {};
  std::array<VkDescriptorSetLayout, 3> DescLayouts;
  DescLayouts[0] = gResources().GetDescriptorSetLayout(MeshSetLayoutStr)->Layout();
  DescLayouts[1] = gResources().GetDescriptorSetLayout(LightViewDescriptorSetLayoutStr)->Layout();
  DescLayouts[2] = gResources().GetDescriptorSetLayout(BonesSetLayoutStr)->Layout();

  auto Bindings = StaticVertexDescription::GetBindingDescription();
  auto Attribs = StaticVertexDescription::GetVertexAttributes();
  VkPipelineVertexInputStateCreateInfo Info = {};
  Info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  Info.pVertexAttributeDescriptions = Attribs.data();
  Info.pVertexBindingDescriptions = &Bindings;
  Info.vertexAttributeDescriptionCount = static_cast<u32>(Attribs.size());
  Info.vertexBindingDescriptionCount = 1;
  Info.pNext = nullptr;

  PipeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipeLayout.pushConstantRangeCount = 0;
  PipeLayout.pPushConstantRanges = nullptr;
  PipeLayout.setLayoutCount = static_cast<u32>(DescLayouts.size() - 1);
  PipeLayout.pSetLayouts = DescLayouts.data();
  // ShadowMapping shader.
  // TODO(): Shadow mapping MUST be done before downsampling and glow buffers have finished!
  // This will prevent blurry shadows. It must be combined in the forward render pass (maybe?)
  Shader* SmVert = Rhi->CreateShader();
  Shader* SmFrag = Rhi->CreateShader();

  RendererPass::LoadShader(ShadowMapVertFileStr, SmVert);
  RendererPass::LoadShader(ShadowMapFragFileStr, SmFrag);

  std::array<VkPipelineShaderStageCreateInfo, 2> Shaders;
  Shaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shaders[0].flags = 0;
  Shaders[0].pName = kDefaultShaderEntryPointStr;
  Shaders[0].pNext = nullptr;
  Shaders[0].pSpecializationInfo = nullptr;
  Shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  Shaders[0].module = SmVert->Handle();

  Shaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shaders[1].flags = 0;
  Shaders[1].pName = kDefaultShaderEntryPointStr;
  Shaders[1].pNext = nullptr;
  Shaders[1].pSpecializationInfo = nullptr;
  Shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  Shaders[1].module = SmFrag->Handle();

  GraphicsPipelineInfo.pStages = Shaders.data();
  GraphicsPipelineInfo.stageCount = static_cast<u32>(Shaders.size());
  GraphicsPipelineInfo.pVertexInputState = &Info;

  VkPipelineRasterizationStateCreateInfo rasterizerCI = CreateRasterInfo(
    VK_POLYGON_MODE_FILL,
    VK_FALSE,
    VK_CULL_MODE_FRONT_BIT,
    VK_FRONT_FACE_CLOCKWISE,
    1.0f,
    VK_FALSE,
    VK_FALSE
  );

  GraphicsPipelineInfo.pRasterizationState = &rasterizerCI;
  ShadowMapPipeline->Initialize(GraphicsPipelineInfo, PipeLayout);

  Bindings = SkinnedVertexDescription::GetBindingDescription();
  Attribs = SkinnedVertexDescription::GetVertexAttributes();
  Info.pVertexAttributeDescriptions = Attribs.data();
  Info.pVertexBindingDescriptions = &Bindings;
  Info.vertexAttributeDescriptionCount = static_cast<u32>(Attribs.size());
  Info.vertexBindingDescriptionCount = 1;
  Info.pNext = nullptr;
  
  PipeLayout.setLayoutCount += 1;
  // Create the dynamic shadow map pipeline.

  
  Rhi->FreeShader(SmVert);
  SmVert = Rhi->CreateShader();
  LoadShader(DynamicShadowMapVertFileStr, SmVert);
 
  Shaders[0].module = SmVert->Handle();

  DynamicShadowMapPipeline->Initialize(GraphicsPipelineInfo, PipeLayout);

  Rhi->FreeShader(SmVert);
  Rhi->FreeShader(SmFrag);

  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
}


void SetUpSkyboxPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  Shader* vert = Rhi->CreateShader();
  Shader* frag = Rhi->CreateShader();
  VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = DefaultInfo;

  VkVertexInputBindingDescription vertInput = { };
  vertInput.binding = 0;
  vertInput.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertInput.stride = sizeof(Vector4);

  VkVertexInputAttributeDescription attrib = { };
  attrib.binding = 0;
  attrib.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attrib.location = 0;
  attrib.offset = 0;

  VkPipelineVertexInputStateCreateInfo inputState = { };
  inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  inputState.vertexAttributeDescriptionCount = 1;
  inputState.vertexBindingDescriptionCount = 1;
  inputState.pVertexBindingDescriptions = &vertInput;
  inputState.pVertexAttributeDescriptions = &attrib;
  GraphicsPipelineInfo.pVertexInputState = &inputState;

  GraphicsPipeline* sky = Rhi->CreateGraphicsPipeline();
  gResources().RegisterGraphicsPipeline(SkyboxPipelineStr, sky);
  
  DescriptorSetLayout* global = gResources().GetDescriptorSetLayout(GlobalSetLayoutStr);
  DescriptorSetLayout* skybox = gResources().GetDescriptorSetLayout(SkyboxSetLayoutStr);

  VkDescriptorSetLayout layouts[] = { 
    global->Layout(),
    skybox->Layout()
  };

  VkPipelineRasterizationStateCreateInfo raster = CreateRasterInfo(
    VK_POLYGON_MODE_FILL,
    VK_FALSE,
    VK_CULL_MODE_NONE,
    VK_FRONT_FACE_CLOCKWISE,
    1.0f,
    VK_FALSE,
    VK_FALSE
  );

  VkPipelineDepthStencilStateCreateInfo depthStencilCI = {};
  depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilCI.depthTestEnable = VK_TRUE;
  depthStencilCI.depthWriteEnable = VK_TRUE;
  depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilCI.depthBoundsTestEnable = VK_FALSE;
  depthStencilCI.minDepthBounds = 0.0f;
  depthStencilCI.maxDepthBounds = 1.0f;
  depthStencilCI.stencilTestEnable = VK_FALSE;
  depthStencilCI.back = {};
  depthStencilCI.front = {};
  
  GraphicsPipelineInfo.pRasterizationState = &raster;
  GraphicsPipelineInfo.pDepthStencilState = &depthStencilCI;

  VkPipelineLayoutCreateInfo pipelineLayout = { };
  pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; 
  pipelineLayout.setLayoutCount = 2;
  pipelineLayout.pSetLayouts = layouts;
  
  LoadShader(Sky::kSkyVertStr, vert);
  LoadShader(Sky::kSkyFragStr, frag);

  std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments;
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

  VkPipelineColorBlendStateCreateInfo colorBlendCI = CreateBlendStateInfo(
    static_cast<u32>(colorBlendAttachments.size()),
    colorBlendAttachments.data(),
    VK_FALSE,
    VK_LOGIC_OP_COPY
  );
  
  GraphicsPipelineInfo.pColorBlendState = &colorBlendCI;

  std::array<VkPipelineShaderStageCreateInfo, 2> shaders;
  shaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaders[0].flags = 0;
  shaders[0].pName = kDefaultShaderEntryPointStr;
  shaders[0].pNext = nullptr;
  shaders[0].pSpecializationInfo = nullptr;
  shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaders[0].module = vert->Handle();

  shaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaders[1].flags = 0;
  shaders[1].pName = kDefaultShaderEntryPointStr;
  shaders[1].pNext = nullptr;
  shaders[1].pSpecializationInfo = nullptr;
  shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaders[1].module = frag->Handle();

  GraphicsPipelineInfo.stageCount = 2;
  GraphicsPipelineInfo.pStages = shaders.data();
  
  GraphicsPipelineInfo.renderPass = gRenderer().SkyNative()->GetSkyboxRenderPass();
  sky->Initialize(GraphicsPipelineInfo, pipelineLayout);

  Rhi->FreeShader(vert);
  Rhi->FreeShader(frag);
}
} // RendererPass
} // Recluse