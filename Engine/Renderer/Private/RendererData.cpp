// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererData.hpp"
#include "VertexDescription.hpp"
#include "Resources.hpp"
#include "Core/Logging/Log.hpp"
#include "Renderer.hpp"
#include "HDR.hpp"
#include "GlobalDescriptor.hpp"
#include "SkyAtmosphere.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Shader.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/ComputePipeline.hpp"
#include "RHI/Framebuffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Commandbuffer.hpp"
#include "RHI/Buffer.hpp"

#include <array>


#include "Filesystem/Filesystem.hpp"

namespace Recluse {

std::string ShadersPath             = "Shaders";


std::array<Matrix4, 6> kViewMatrices;

Texture* DefaultTextureKey          = nullptr;
Sampler* DefaultSampler2DKey        = nullptr;
VkImageView DefaultTexture2DArrayView = VK_NULL_HANDLE;

GraphicsPipeline* ShadowMapPipelineKey              = nullptr;
GraphicsPipeline* DynamicShadowMapPipelineKey       = nullptr;
GraphicsPipeline* shadowMap_dynamicMorphTargetPipeline = nullptr;
GraphicsPipeline* shadowMap_staticMorphTargetPipeline = nullptr;
std::string DynamicShadowMapVertFileStr       = "DynamicShadowMapping.vert.spv";
std::string ShadowMapVertFileStr              = "ShadowMapping.vert.spv";
std::string ShadowMapFragFileStr              = "ShadowMapping.frag.spv";
std::string ShadowMapFragOpaqueFileStr        = "ShadowMapping_Opaque.frag.spv";
DescriptorSetLayout* LightViewDescriptorSetLayoutKey   = nullptr;
DescriptorSetLayout* globalIllumination_DescLR = nullptr;
DescriptorSetLayout* globalIllumination_DescNoLR = nullptr;

GraphicsPipeline* transparent_staticShadowPipe = nullptr;
GraphicsPipeline* transparent_dynamicShadowPipe = nullptr;
GraphicsPipeline* transparent_colorFilterPipe = nullptr;

GraphicsPipeline* gbuffer_PipelineKey               = nullptr;
GraphicsPipeline* gbuffer_morphTargetPipeline     = nullptr;
GraphicsPipeline* gbuffer_StaticPipelineKey         = nullptr;
GraphicsPipeline* gbuffer_staticMorphTargetPipeline = nullptr;
DescriptorSetLayout* gbuffer_LayoutKey                 = nullptr;
Texture* gbuffer_AlbedoAttachKey           = nullptr;
Texture* gbuffer_NormalAttachKey           = nullptr;
Texture* gbuffer_PositionAttachKey         = nullptr;
Texture* gbuffer_EmissionAttachKey         = nullptr;
Texture* gbuffer_DepthAttachKey            = nullptr;
Sampler* gbuffer_SamplerKey                = nullptr;
FrameBuffer* gbuffer_FrameBufferKey            = nullptr;
RenderPass* gbuffer_renderPass                = nullptr;
std::string gbuffer_VertFileStr               = "GBuffer.vert.spv";
std::string gbuffer_StaticVertFileStr         = "StaticGBuffer.vert.spv";
std::string gbuffer_FragFileStr               = "GBuffer.frag.spv";

GraphicsPipeline* pbr_Pipeline_LR                   = nullptr;
GraphicsPipeline* pbr_Pipeline_NoLR           = nullptr;
GraphicsPipeline* pbr_forwardPipeline_LR      = nullptr;
GraphicsPipeline* pbr_forwardPipeline_NoLR = nullptr;
GraphicsPipeline* pbr_staticForwardPipeline_LR    = nullptr;
GraphicsPipeline* pbr_staticForwardPipeline_NoLR  = nullptr;
GraphicsPipeline* pbr_forwardPipelineMorphTargets_LR = nullptr;
GraphicsPipeline* pbr_forwardPipelineMorphTargets_NoLR = nullptr;
GraphicsPipeline* pbr_staticForwardPipelineMorphTargets_LR = nullptr;
GraphicsPipeline* pbr_staticForwardPipelineMorphTargets_NoLR = nullptr;
ComputePipeline* pbr_computePipeline_NoLR         = nullptr;
ComputePipeline* pbr_computePipeline_LR           = nullptr;
GraphicsPipeline* pbr_static_LR_Debug = nullptr;
GraphicsPipeline* pbr_static_NoLR_Debug = nullptr;
GraphicsPipeline* pbr_dynamic_LR_Debug = nullptr;
GraphicsPipeline* pbr_dynamic_NoLR_Debug = nullptr;
GraphicsPipeline* pbr_static_mt_LR_Debug = nullptr;
GraphicsPipeline* pbr_static_mt_NoLR_Debug = nullptr;
GraphicsPipeline* pbr_dynamic_LR_mt_Debug = nullptr;
GraphicsPipeline* pbr_dynamic_NoLR_mt_Debug = nullptr;
RenderPass*      pbr_forwardRenderPass            = nullptr;
FrameBuffer* pbr_FrameBufferKey                   = nullptr;
FrameBuffer* pbr_forwardFrameBuffer               = nullptr;
RenderPass* pbr_renderPass                        = nullptr;
DescriptorSetLayout* pbr_DescLayoutKey            = nullptr;
DescriptorSetLayout* pbr_compDescLayout           = nullptr;
DescriptorSet* pbr_DescSetKey                     = nullptr;
DescriptorSet* pbr_compSet                        = nullptr;
Texture* pbr_FinalTextureKey                      = nullptr;
Texture* pbr_BrightTextureKey                     = nullptr;
std::string pbr_VertStr                           = "PBR.vert.spv";
std::string pbr_FragStrLR                         = "PBR_LR.frag.spv";
std::string pbr_FragStrNoLR                       = "PBR_NoLR.frag.spv";
std::string pbr_forwardFragStrLR                  = "ForwardPBR_LR.frag.spv";
std::string pbr_forwardFragStrNoLR                = "ForwardPBR_NoLR.frag.spv";

Texture* RenderTargetBlurHoriz4xKey  = nullptr;
FrameBuffer* FrameBuffer4xHorizKey       = nullptr;
DescriptorSetLayout* MeshSetLayoutKey            = nullptr;
DescriptorSetLayout* MaterialSetLayoutKey        = nullptr;
DescriptorSetLayout* BonesSetLayoutKey           = nullptr;
DescriptorSetLayout* GlobalSetLayoutKey          = nullptr;
DescriptorSetLayout* LightSetLayoutKey           = nullptr;

GraphicsPipeline* skybox_pipelineKey           = nullptr;
DescriptorSet* skybox_descriptorSetKey      = nullptr;
DescriptorSetLayout* skybox_setLayoutKey          = nullptr;

ComputePipeline* aa_PipelineKey                = nullptr;
DescriptorSetLayout* aa_DescLayoutKey              = nullptr;
DescriptorSet* aa_descSet                     = nullptr;
Texture* aa_outTexture                        = nullptr;

std::string renderquad_vertStr            = "RenderQuad.vert.spv";
std::string pbr_forwardVertStrLR            = "ForwardPBR.vert.spv";

// Must check with the renderer specs.
std::string aa_fragStr                    = "";

std::string fxaa_fragStr                  = "FXAA.comp.spv";
std::string fxaaNV_fragStr                = "FXAA_Nvidia.comp.spv";
std::string smaa_fragStr                  = "SMAA.frag.spv";

Sampler* ScaledSamplerKey            = nullptr;
Texture* RenderTarget2xHorizKey      = nullptr;
Texture* RenderTarget2xFinalKey      = nullptr;
Texture* RenderTarget4xScaledKey     = nullptr;
Texture* RenderTarget4xFinalKey      = nullptr;
Texture* RenderTarget8xScaledKey     = nullptr;
Texture* RenderTarget8xFinalKey      = nullptr;
Texture* RenderTarget16xScaledKey    = nullptr;
Texture* RenderTarget16xFinalKey     = nullptr;
FrameBuffer* FrameBuffer2xHorizKey       = nullptr;
FrameBuffer* FrameBuffer2xFinalKey       = nullptr;
FrameBuffer* FrameBuffer4xFinalKey       = nullptr;
FrameBuffer* FrameBuffer4xKey            = nullptr;
FrameBuffer* FrameBuffer8xKey            = nullptr;
FrameBuffer* FrameBuffer8xFinalKey       = nullptr;
FrameBuffer* FrameBuffer16xKey           = nullptr;
FrameBuffer* FrameBuffer16xFinalKey      = nullptr;
GraphicsPipeline* GlowPipelineKey             = nullptr;
std::string GlowFragFileStr             = "GlowPass.frag.spv";
Texture* RenderTargetGlowKey         = nullptr;
FrameBuffer* FrameBufferGlowKey          = nullptr;
DescriptorSetLayout* GlowDescriptorSetLayoutKey  = nullptr;
DescriptorSet* GlowDescriptorSetKey        = nullptr;
GraphicsPipeline* DownscaleBlurPipeline2xKey  = nullptr;
GraphicsPipeline* DownscaleBlurPipeline4xKey  = nullptr;
GraphicsPipeline* DownscaleBlurPipeline8xKey  = nullptr;
GraphicsPipeline* DownscaleBlurPipeline16xKey = nullptr;
DescriptorSetLayout* DownscaleBlurLayoutKey      = nullptr;
DescriptorSet* DownscaleBlurDescriptorSet2x          = nullptr;
DescriptorSet* DownscaleBlurDescriptorSet2xFinalKey  = nullptr;
DescriptorSet* DownscaleBlurDescriptorSet4x          = nullptr;
DescriptorSet* DownscaleBlurDescriptorSet4xFinalKey  = nullptr;
DescriptorSet* DownscaleBlurDescriptorSet8x          = nullptr;
DescriptorSet* DownscaleBlurDescriptorSet8xFinalKey  = nullptr;
DescriptorSet* DownscaleBlurDescriptorSet16x         = nullptr;
DescriptorSet* DownscaleBlurDescriptorSet16xFinalKey = nullptr;
std::string DownscaleBlurVertFileStr    = "DownscaleBlurPass.vert.spv";
std::string DownscaleBlurFragFileStr    = "DownscaleBlurPass.frag.spv";

Texture* RenderTargetVelocityKey     = nullptr;

GraphicsPipeline* hdr_gamma_pipelineKey         = nullptr;
Texture* hdr_gamma_colorAttachKey      = nullptr;
FrameBuffer* hdr_gamma_frameBufferKey      = nullptr;
RenderPass* hdr_renderPass                = nullptr;
Sampler* hdr_gamma_samplerKey          = nullptr;
DescriptorSet* hdr_gamma_descSetKey          = nullptr;
DescriptorSetLayout* hdr_gamma_descSetLayoutKey    = nullptr;
std::string hdr_gamma_vertFileStr         = "HDR.vert.spv";
std::string hdr_gamma_fragFileStr         = "HDR.frag.spv";

GraphicsPipeline* final_PipelineKey            = nullptr;
DescriptorSet* final_DescSetKey             = nullptr;
DescriptorSetLayout* final_DescSetLayoutKey       = nullptr;
std::string final_VertFileStr            = "FinalPass.vert.spv";
std::string final_FragFileStr            = "FinalPass.frag.spv";
Texture* final_renderTargetKey       = nullptr;
FrameBuffer* final_frameBufferKey = nullptr;
RenderPass* final_renderPass = nullptr;
DescriptorSet*  output_descSetKey = nullptr;
GraphicsPipeline* output_pipelineKey = nullptr;

GraphicsPipeline* envMap_pbrPipeline = nullptr;
FrameBuffer*       envMap_frameBuffer = nullptr;
RenderPass*        envMap_renderPass = nullptr;
Texture*           envMap_texture = nullptr;

// Default entry point on shaders.
char const* kDefaultShaderEntryPointStr = "main";


void SetUpRenderData()
{
  kViewMatrices = {
    Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(90.0f), Vector3::UP), Radians(180.0f), Vector3::RIGHT),
    Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(-90.0f), Vector3::UP), Radians(180.0f), Vector3::RIGHT),
    Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(-90.0f), Vector3::RIGHT), Radians(180.0f), Vector3::UP),
    Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(90.0f), Vector3::RIGHT), Radians(180.0f), Vector3::UP),
    Matrix4::Rotate(Matrix4::Identity(), Radians(180.0f), Vector3::BACK),
    Matrix4::Rotate(Matrix4::Rotate(Matrix4::Identity(), Radians(180.0f), Vector3::UP), Radians(180.0f), Vector3::FRONT)
  };
}


void CleanUpRenderData()
{

}


namespace RendererPass {


void LoadShader(const std::string& Filename, Shader* S)
{
  if (!S) { Log(rError) << "Shader module is null! Can not load a shader!\n"; }
  std::string Filepath = gFilesystem().CurrentAppDirectory();
  if (!S->Initialize(Filepath
      + "/" + ShadersPath + "/" + Filename)) {
    Log(rError) << "Could not find " + Filename + "!";
  }
}


void SetUpGBufferPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  // PbrForward Pipeline Creation.
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  GraphicsPipeline* GBufferPipeline = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* GBufferStaticPipeline = Rhi->CreateGraphicsPipeline();
  FrameBuffer* GBufferFrameBuffer = gbuffer_FrameBufferKey;
  Shader* VertGBuffer = Rhi->CreateShader();
  Shader* FragGBuffer = Rhi->CreateShader();

  gbuffer_PipelineKey = GBufferPipeline;
  gbuffer_StaticPipelineKey = GBufferStaticPipeline;
  gbuffer_morphTargetPipeline = Rhi->CreateGraphicsPipeline();
  gbuffer_staticMorphTargetPipeline = Rhi->CreateGraphicsPipeline();

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

  GraphicsInfo.renderPass = GBufferFrameBuffer->RenderPassRef()->Handle();
  GraphicsInfo.stageCount = 2;
  GraphicsInfo.pStages = PbrShaders;

  std::array<VkDescriptorSetLayout, 4> DLayouts;
  DLayouts[0] = GlobalSetLayoutKey->Layout();
  DLayouts[1] = MeshSetLayoutKey->Layout();
  DLayouts[2] = MaterialSetLayoutKey->Layout();
  DLayouts[3] = BonesSetLayoutKey->Layout();

  VkPipelineLayoutCreateInfo PipelineLayout = {};
  PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipelineLayout.setLayoutCount = static_cast<u32>(DLayouts.size());
  PipelineLayout.pSetLayouts = DLayouts.data();
  PipelineLayout.pPushConstantRanges = 0;
  PipelineLayout.pushConstantRangeCount = 0;

  // Initialize pbr forward pipeline.
  GBufferPipeline->Initialize(GraphicsInfo, PipelineLayout);

  {
    VkGraphicsPipelineCreateInfo ginfo = GraphicsInfo;
    VkPipelineVertexInputStateCreateInfo input = { };
    ginfo.pVertexInputState = &input;
    auto bindings = MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription());
    auto attribs = MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes());
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.pVertexAttributeDescriptions = attribs.data();
    input.pVertexBindingDescriptions = bindings.data();
    input.vertexAttributeDescriptionCount = static_cast<u32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<u32>(bindings.size());

    // Initialize pbr forward morph target pipeline.
    Rhi->FreeShader(VertGBuffer);
    VertGBuffer = Rhi->CreateShader();
    LoadShader("GBuffer_MorphTargets.vert.spv", VertGBuffer);
    PbrShaders[0].module = VertGBuffer->Handle();
    gbuffer_morphTargetPipeline->Initialize(ginfo, PipelineLayout);
    Rhi->FreeShader(VertGBuffer);
    VertGBuffer = nullptr;
  }
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
  VertGBuffer = nullptr;
  // Static Morph Target gbuffer pipeline.
  {
    VkGraphicsPipelineCreateInfo ginfo = GraphicsInfo;
    VkPipelineVertexInputStateCreateInfo input = {};
    ginfo.pVertexInputState = &input;
    auto bindings = MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription());
    auto attribs = MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes());
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.pVertexAttributeDescriptions = attribs.data();
    input.pVertexBindingDescriptions = bindings.data();
    input.vertexAttributeDescriptionCount = static_cast<u32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<u32>(bindings.size());

    // Initialize pbr forward morph target pipeline.
    Rhi->FreeShader(VertGBuffer);
    VertGBuffer = Rhi->CreateShader();
    LoadShader("StaticGBuffer_MorphTargets.vert.spv", VertGBuffer);
    PbrShaders[0].module = VertGBuffer->Handle();
    gbuffer_staticMorphTargetPipeline->Initialize(ginfo, PipelineLayout);
    Rhi->FreeShader(VertGBuffer);
    VertGBuffer = nullptr;
  }
  Rhi->FreeShader(FragGBuffer);
}


void SetUpHDRGammaPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo, HDR* pHDR)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  GraphicsPipeline* hdrPipeline = Rhi->CreateGraphicsPipeline();
  VkPipelineLayoutCreateInfo hdrLayout = {};
  std::array<VkDescriptorSetLayout, 3> hdrSetLayout; 
  hdrSetLayout[0] = GlobalSetLayoutKey->Layout();
  hdrSetLayout[1] = hdr_gamma_descSetLayoutKey->Layout();
  hdrSetLayout[2] = pHDR->GetSetLayout()->Layout();

  Shader* HdrFrag = Rhi->CreateShader();
  Shader* HdrVert = Rhi->CreateShader();

  LoadShader(hdr_gamma_vertFileStr, HdrVert);
  LoadShader(hdr_gamma_fragFileStr, HdrFrag);

  FrameBuffer* hdrBuffer = hdr_gamma_frameBufferKey;
  GraphicsInfo.renderPass = hdrBuffer->RenderPassRef()->Handle();

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

  // For parameter push constant in HDR.
  VkPushConstantRange pushConstantRange = { };
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(ParamsHDR);
  pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  hdrLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  hdrLayout.setLayoutCount = static_cast<u32>(hdrSetLayout.size());
  hdrLayout.pSetLayouts = hdrSetLayout.data();
  hdrLayout.pPushConstantRanges = &pushConstantRange;
  hdrLayout.pushConstantRangeCount = 1;

  hdrPipeline->Initialize(GraphicsInfo, hdrLayout);

  Rhi->FreeShader(HdrFrag);
  Rhi->FreeShader(HdrVert);
  hdr_gamma_pipelineKey = hdrPipeline;

}


void SetUpDeferredPhysicallyBasedPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  {
    VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
    GraphicsPipeline* pbrPipeline_LR = Rhi->CreateGraphicsPipeline();
    pbr_Pipeline_LR = pbrPipeline_LR;
    pbr_Pipeline_NoLR = Rhi->CreateGraphicsPipeline();

    FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;  

    Shader* VertPBR = Rhi->CreateShader();
    Shader* FragPBR = Rhi->CreateShader();

    LoadShader(pbr_VertStr, VertPBR);
    LoadShader(pbr_FragStrLR, FragPBR);

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

    GraphicsInfo.renderPass = pbr_FrameBuffer->RenderPassRef()->Handle();
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

    std::array<VkDescriptorSetLayout, 5> layouts;
    layouts[0] = GlobalSetLayoutKey->Layout();
    layouts[1] = pbr_DescLayoutKey->Layout();
    layouts[2] = LightSetLayoutKey->Layout();
    layouts[3] = LightViewDescriptorSetLayoutKey->Layout();
    //layouts[4] = LightViewDescriptorSetLayoutKey->Layout();
    layouts[4] = globalIllumination_DescLR->Layout();

    VkPipelineLayoutCreateInfo PipelineLayout = {};
    PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayout.setLayoutCount = static_cast<u32>(layouts.size());
    PipelineLayout.pSetLayouts = layouts.data();
    PipelineLayout.pPushConstantRanges = 0;
    PipelineLayout.pushConstantRangeCount = 0;

    pbrPipeline_LR->Initialize(GraphicsInfo, PipelineLayout);

    Rhi->FreeShader(FragPBR);

    FragPBR = Rhi->CreateShader();
    LoadShader(pbr_FragStrNoLR, FragPBR);

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

    layouts[4] = globalIllumination_DescNoLR->Layout();
    pbr_Pipeline_NoLR->Initialize(GraphicsInfo, PipelineLayout);

    Rhi->FreeShader(VertPBR);
    Rhi->FreeShader(FragPBR);
  }

  u32 vendorId = Rhi->VendorID();

  // PBR compute bound.
  {
    std::string noLr = "";
    std::string lr = "";
    switch (vendorId) {
      case AMD_VENDOR_ID:
      {
        noLr = "PBR_NoLR_Amd.comp.spv";
        lr = "PBR_LR_Amd.comp.spv";
      }
      case NVIDIA_VENDOR_ID:
      {
        lr = "PBR_LR_Nvidia.comp.spv";
        noLr = "PBR_NoLR_Nvidia.comp.spv";
      } break;
      case INTEL_VENDOR_ID:
      default:
      {
        noLr = "PBR_NoLR_Intel.comp.spv";
        lr = "PBR_LR_Intel.comp.spv";
      } break;
    }
    Shader* compShader = Rhi->CreateShader();
    LoadShader(noLr, compShader);

    VkDescriptorSetLayout layouts[6] = { 
      GlobalSetLayoutKey->Layout(),
      pbr_DescLayoutKey->Layout(),
      LightSetLayoutKey->Layout(),
      LightViewDescriptorSetLayoutKey->Layout(),
      //LightViewDescriptorSetLayoutKey->Layout(),
      globalIllumination_DescNoLR->Layout(),
      pbr_compDescLayout->Layout()
    };    

    pbr_computePipeline_NoLR = Rhi->CreateComputePipeline();
    VkComputePipelineCreateInfo computeCi = { };
    VkPipelineLayoutCreateInfo compLayoutCi = { };
    compLayoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    compLayoutCi.setLayoutCount = 6;
    compLayoutCi.pSetLayouts = layouts;

    VkPipelineShaderStageCreateInfo shaderCi = { };
    shaderCi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderCi.module = compShader->Handle();
    shaderCi.pName = kDefaultShaderEntryPointStr;
    shaderCi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeCi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computeCi.basePipelineHandle = VK_NULL_HANDLE;
    computeCi.basePipelineIndex = -1;
    computeCi.stage = shaderCi;
    
    pbr_computePipeline_NoLR->Initialize(computeCi, compLayoutCi);
    Rhi->FreeShader(compShader);
    pbr_computePipeline_LR = Rhi->CreateComputePipeline();
    compShader = Rhi->CreateShader();
    LoadShader(lr, compShader);

    layouts[4] = globalIllumination_DescLR->Layout();
    shaderCi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderCi.module = compShader->Handle();
    shaderCi.pName = kDefaultShaderEntryPointStr;
    shaderCi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeCi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computeCi.stage = shaderCi;
    computeCi.basePipelineHandle = VK_NULL_HANDLE;
    computeCi.basePipelineIndex = -1;
    computeCi.stage = shaderCi;

    pbr_computePipeline_LR->Initialize(computeCi, compLayoutCi);
    Rhi->FreeShader(compShader);
  }
}


void SetUpForwardPhysicallyBasedPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  GraphicsPipeline* pbr_Pipeline = Rhi->CreateGraphicsPipeline();
  pbr_staticForwardPipeline_LR = Rhi->CreateGraphicsPipeline();
  pbr_staticForwardPipeline_NoLR = Rhi->CreateGraphicsPipeline();
  pbr_forwardPipeline_LR = pbr_Pipeline;
  pbr_forwardPipeline_NoLR = Rhi->CreateGraphicsPipeline();
  pbr_staticForwardPipelineMorphTargets_LR = Rhi->CreateGraphicsPipeline();
  pbr_staticForwardPipelineMorphTargets_NoLR = Rhi->CreateGraphicsPipeline();
  pbr_forwardPipelineMorphTargets_LR =  Rhi->CreateGraphicsPipeline();
  pbr_forwardPipelineMorphTargets_NoLR = Rhi->CreateGraphicsPipeline();
  pbr_static_LR_Debug = Rhi->CreateGraphicsPipeline();
  pbr_static_NoLR_Debug = Rhi->CreateGraphicsPipeline();
  pbr_dynamic_LR_Debug = Rhi->CreateGraphicsPipeline();
  pbr_dynamic_NoLR_Debug = Rhi->CreateGraphicsPipeline();
  pbr_static_mt_LR_Debug = Rhi->CreateGraphicsPipeline();
  pbr_static_mt_NoLR_Debug = Rhi->CreateGraphicsPipeline();
  pbr_dynamic_LR_mt_Debug = Rhi->CreateGraphicsPipeline();
  pbr_dynamic_NoLR_mt_Debug = Rhi->CreateGraphicsPipeline();

  FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;

  Shader* VertPBRStatic = Rhi->CreateShader();
  Shader* VertPBRLR = Rhi->CreateShader();
  Shader* FragPBRLR = Rhi->CreateShader();
  Shader* FragPBRNoLR = Rhi->CreateShader();
  Shader* FragPBRLRDebug = Rhi->CreateShader();
  Shader* FragPBRNoLRDebug = Rhi->CreateShader();
  Shader* VertMorphSkin = Rhi->CreateShader();
  Shader* VertMorphStatic = Rhi->CreateShader();

  LoadShader(pbr_forwardVertStrLR, VertPBRLR);
  LoadShader(gbuffer_StaticVertFileStr, VertPBRStatic);
  LoadShader(pbr_forwardFragStrLR, FragPBRLR);
  LoadShader(pbr_forwardFragStrNoLR, FragPBRNoLR);
  LoadShader("ForwardPBR_LR_Debug.frag.spv", FragPBRLRDebug);
  LoadShader("ForwardPBR_NoLR_Debug.frag.spv", FragPBRNoLRDebug);
  LoadShader("ForwardPBR_MorphTargets.vert.spv", VertMorphSkin);
  LoadShader("StaticGBuffer_MorphTargets.vert.spv", VertMorphStatic);
  
  VkPipelineShaderStageCreateInfo PbrShaders[2];
  PbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[0].module = VertPBRLR->Handle();
  PbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  PbrShaders[0].pName = kDefaultShaderEntryPointStr;
  PbrShaders[0].pNext = nullptr;
  PbrShaders[0].pSpecializationInfo = nullptr;
  PbrShaders[0].flags = 0;

  PbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[1].module = FragPBRLR->Handle();
  PbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  PbrShaders[1].pName = kDefaultShaderEntryPointStr;
  PbrShaders[1].pNext = nullptr;
  PbrShaders[1].flags = 0;
  PbrShaders[1].pSpecializationInfo = nullptr;

  GraphicsInfo.renderPass = pbr_forwardRenderPass->Handle();
  GraphicsInfo.stageCount = 2;
  GraphicsInfo.pStages = PbrShaders;

  std::array<VkPipelineColorBlendAttachmentState, 6> colorBlendAttachments;
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

  colorBlendAttachments[2] = CreateColorBlendAttachmentState(
    VK_TRUE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[3] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[4] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[5] = CreateColorBlendAttachmentState(
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

  std::array<VkDescriptorSetLayout, 7> layouts;
  layouts[0] = GlobalSetLayoutKey->Layout();
  layouts[1] = MeshSetLayoutKey->Layout();
  layouts[2] = MaterialSetLayoutKey->Layout();
  layouts[3] = LightSetLayoutKey->Layout();
  layouts[4] = LightViewDescriptorSetLayoutKey->Layout();
  //layouts[5] = LightViewDescriptorSetLayoutKey->Layout();
  layouts[5] = globalIllumination_DescLR->Layout();
  layouts[6] = BonesSetLayoutKey->Layout();

  struct ivec4 {
    i32 v[4];
  };

  VkPushConstantRange range = { };
  range.offset = 0;
  range.size = sizeof(ivec4);
  range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineLayoutCreateInfo PipelineLayout = {};
  PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipelineLayout.setLayoutCount = static_cast<u32>(layouts.size());
  PipelineLayout.pSetLayouts = layouts.data();
  PipelineLayout.pPushConstantRanges = &range;
  PipelineLayout.pushConstantRangeCount = 0;

  pbr_forwardPipeline_LR->Initialize(GraphicsInfo, PipelineLayout);

  PbrShaders[1].module = FragPBRLRDebug->Handle();
  PipelineLayout.pushConstantRangeCount = 1;
  pbr_dynamic_LR_Debug->Initialize(GraphicsInfo, PipelineLayout);
  PipelineLayout.pushConstantRangeCount = 0;

  {
    VkGraphicsPipelineCreateInfo ginfo = GraphicsInfo;
    VkPipelineVertexInputStateCreateInfo input = {};
    ginfo.pVertexInputState = &input;
    auto bindings = MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription());
    auto attribs = MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes());
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.pVertexAttributeDescriptions = attribs.data();
    input.pVertexBindingDescriptions = bindings.data();
    input.vertexAttributeDescriptionCount = static_cast<u32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<u32>(bindings.size());

    PbrShaders[0].module = VertMorphSkin->Handle();
    PbrShaders[1].module = FragPBRLR->Handle();
    pbr_forwardPipelineMorphTargets_LR->Initialize(ginfo, PipelineLayout);

    PbrShaders[1].module = FragPBRLRDebug->Handle();  
    PipelineLayout.pushConstantRangeCount = 1;
    pbr_dynamic_LR_mt_Debug->Initialize(ginfo, PipelineLayout);
    PipelineLayout.pushConstantRangeCount = 0;
  }

  PbrShaders[0].module = VertPBRLR->Handle();
  PbrShaders[1].module = FragPBRNoLR->Handle();
  layouts[5] = globalIllumination_DescNoLR->Layout();
  PipelineLayout.setLayoutCount = static_cast<u32>(layouts.size());
  pbr_forwardPipeline_NoLR->Initialize(GraphicsInfo, PipelineLayout);

  PbrShaders[1].module = FragPBRNoLRDebug->Handle();
  PipelineLayout.pushConstantRangeCount = 1;
  pbr_dynamic_NoLR_Debug->Initialize(GraphicsInfo, PipelineLayout);
  PipelineLayout.pushConstantRangeCount = 0;

  {
    VkGraphicsPipelineCreateInfo ginfo = GraphicsInfo;
    VkPipelineVertexInputStateCreateInfo input = {};
    ginfo.pVertexInputState = &input;
    auto bindings = MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription());
    auto attribs = MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes());
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.pVertexAttributeDescriptions = attribs.data();
    input.pVertexBindingDescriptions = bindings.data();
    input.vertexAttributeDescriptionCount = static_cast<u32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<u32>(bindings.size());

    PbrShaders[0].module = VertMorphSkin->Handle();
    PbrShaders[1].module = FragPBRNoLR->Handle();
    pbr_forwardPipelineMorphTargets_NoLR->Initialize(ginfo, PipelineLayout);

    PbrShaders[1].module = FragPBRNoLRDebug->Handle();
    PipelineLayout.pushConstantRangeCount = 1;
    pbr_dynamic_NoLR_mt_Debug->Initialize(ginfo, PipelineLayout);
    PipelineLayout.pushConstantRangeCount = 0;
  }

  PbrShaders[0].module = VertPBRStatic->Handle();
  PbrShaders[1].module = FragPBRLR->Handle();
  PipelineLayout.setLayoutCount = static_cast<u32>(layouts.size() - 1);

  // Static pipeline creation.
  auto StaticBindings = StaticVertexDescription::GetBindingDescription();
  auto StaticVertexAttribs = StaticVertexDescription::GetVertexAttributes();
  VkPipelineVertexInputStateCreateInfo Input = {};

  GraphicsInfo.pVertexInputState = &Input;
  Input.vertexAttributeDescriptionCount = static_cast<u32>(StaticVertexAttribs.size());
  Input.vertexBindingDescriptionCount = 1;
  Input.pVertexBindingDescriptions = &StaticBindings;
  Input.pVertexAttributeDescriptions = StaticVertexAttribs.data();
  Input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  Input.pNext = nullptr;
  layouts[5] = globalIllumination_DescLR->Layout();
  pbr_staticForwardPipeline_LR->Initialize(GraphicsInfo, PipelineLayout);

  PbrShaders[1].module = FragPBRLRDebug->Handle();
  PipelineLayout.pushConstantRangeCount = 1;
  pbr_static_LR_Debug->Initialize(GraphicsInfo, PipelineLayout);
  PipelineLayout.pushConstantRangeCount = 0;

  {
    VkGraphicsPipelineCreateInfo ginfo = GraphicsInfo;
    VkPipelineVertexInputStateCreateInfo input = {};
    ginfo.pVertexInputState = &input;
    auto bindings = MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription());
    auto attribs = MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes());
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.pVertexAttributeDescriptions = attribs.data();
    input.pVertexBindingDescriptions = bindings.data();
    input.vertexAttributeDescriptionCount = static_cast<u32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<u32>(bindings.size());

    PbrShaders[0].module = VertMorphStatic->Handle();
    PbrShaders[1].module = FragPBRLR->Handle();
    pbr_staticForwardPipelineMorphTargets_LR->Initialize(ginfo, PipelineLayout);

    PbrShaders[1].module = FragPBRLRDebug->Handle();
    PipelineLayout.pushConstantRangeCount = 1;
    pbr_static_mt_LR_Debug->Initialize(ginfo, PipelineLayout);
    PipelineLayout.pushConstantRangeCount = 0;
  }


  // No Local Reflections pipelines.
  PbrShaders[0].module = VertPBRStatic->Handle();
  PbrShaders[1].module = FragPBRNoLR->Handle();
  layouts[5] = globalIllumination_DescNoLR->Layout();
  PipelineLayout.setLayoutCount = static_cast<u32>(layouts.size() - 1);
  pbr_staticForwardPipeline_NoLR->Initialize(GraphicsInfo, PipelineLayout);

  PbrShaders[1].module = FragPBRNoLRDebug->Handle();
  PipelineLayout.pushConstantRangeCount = 1;
  pbr_static_NoLR_Debug->Initialize(GraphicsInfo, PipelineLayout);
  PipelineLayout.pushConstantRangeCount = 0;

  {
    VkGraphicsPipelineCreateInfo ginfo = GraphicsInfo;
    VkPipelineVertexInputStateCreateInfo input = {};
    ginfo.pVertexInputState = &input;
    auto bindings = MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription());
    auto attribs = MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes());
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.pVertexAttributeDescriptions = attribs.data();
    input.pVertexBindingDescriptions = bindings.data();
    input.vertexAttributeDescriptionCount = static_cast<u32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<u32>(bindings.size());

    PbrShaders[0].module = VertMorphStatic->Handle();
    PbrShaders[1].module = FragPBRNoLR->Handle();
    pbr_staticForwardPipelineMorphTargets_NoLR->Initialize(ginfo, PipelineLayout);

    PbrShaders[1].module = FragPBRNoLRDebug->Handle();
    PipelineLayout.pushConstantRangeCount = 1;
    pbr_static_mt_NoLR_Debug->Initialize(ginfo, PipelineLayout);
    PipelineLayout.pushConstantRangeCount = 0;
  }

  Rhi->FreeShader(VertPBRStatic);
  Rhi->FreeShader(VertPBRLR);
  Rhi->FreeShader(FragPBRLR);
  Rhi->FreeShader(FragPBRNoLR);
  Rhi->FreeShader(VertMorphSkin);
  Rhi->FreeShader(VertMorphStatic);
  Rhi->FreeShader(FragPBRLRDebug);
  Rhi->FreeShader(FragPBRNoLRDebug);
}


void SetUpDownScalePass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
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
  FrameBuffer*      FrameBuffer2x = FrameBuffer2xHorizKey;
  FrameBuffer*      FrameBuffer4x = FrameBuffer4xKey;
  FrameBuffer*      FrameBuffer8x = FrameBuffer8xKey;
  FrameBuffer*      FrameBuffer16x = FrameBuffer16xKey;
  FrameBuffer*      GlowFrameBuffer = FrameBufferGlowKey;
  DownscaleBlurPipeline2xKey = Downscale2x;
  DownscaleBlurPipeline4xKey = Downscale4x;
  DownscaleBlurPipeline8xKey = Downscale8x;
  DownscaleBlurPipeline16xKey = Downscale16x;
  GlowPipelineKey = GlowPipeline;
  DescriptorSetLayout* DownscaleDescLayout = DownscaleBlurLayoutKey;
  DescriptorSetLayout* GlowDescLayout = GlowDescriptorSetLayoutKey;

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

  GraphicsInfo.renderPass = FrameBuffer2x->RenderPassRef()->Handle();
  Downscale2x->Initialize(GraphicsInfo, DownscaleLayout);
  GraphicsInfo.renderPass = FrameBuffer4x->RenderPassRef()->Handle();
  Downscale4x->Initialize(GraphicsInfo, DownscaleLayout);
  GraphicsInfo.renderPass = FrameBuffer8x->RenderPassRef()->Handle();
  Downscale8x->Initialize(GraphicsInfo, DownscaleLayout);
  GraphicsInfo.renderPass = FrameBuffer16x->RenderPassRef()->Handle();
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

  GraphicsInfo.renderPass = GlowFrameBuffer->RenderPassRef()->Handle();
  GlowPipeline->Initialize(GraphicsInfo, GlowPipelineLayout);

  Rhi->FreeShader(DbVert);
  Rhi->FreeShader(DbFrag);
}


void SetUpFinalPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  Shader* quadVert = Rhi->CreateShader();
  Shader* quadFrag = Rhi->CreateShader();
  // TODO(): Need to structure all of this into more manageable modules.
  //
  VkPipelineInputAssemblyStateCreateInfo n = { };

  GraphicsPipeline* quadPipeline = Rhi->CreateGraphicsPipeline();
  final_PipelineKey = quadPipeline;

  LoadShader(final_VertFileStr, quadVert);
  LoadShader(final_FragFileStr, quadFrag);

  GraphicsInfo.renderPass = final_frameBufferKey->RenderPassRef()->Handle();

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
  VkDescriptorSetLayout finalL = final_DescSetLayoutKey->Layout();
  finalLayout.pSetLayouts = &finalL;
  finalLayout.pushConstantRangeCount = 0;
  finalLayout.pPushConstantRanges = nullptr;

  quadPipeline->Initialize(GraphicsInfo, finalLayout);

  // Now create the output pipeline.
  output_pipelineKey = Rhi->CreateGraphicsPipeline();
  GraphicsInfo.renderPass = Rhi->SwapchainRenderPass();
  output_pipelineKey->Initialize(GraphicsInfo, finalLayout);

  Rhi->FreeShader(quadVert);
  Rhi->FreeShader(quadFrag);
}


void SetUpDirectionalShadowPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = DefaultInfo;
  GraphicsPipeline* ShadowMapPipeline = Rhi->CreateGraphicsPipeline();
  GraphicsPipeline* DynamicShadowMapPipeline = Rhi->CreateGraphicsPipeline();
  ShadowMapPipelineKey = ShadowMapPipeline;
  DynamicShadowMapPipelineKey = DynamicShadowMapPipeline;

  // TODO(): Initialize shadow map pipeline.
  VkPipelineLayoutCreateInfo PipeLayout = {};
  std::array<VkDescriptorSetLayout, 4> DescLayouts;
  DescLayouts[0] = MeshSetLayoutKey->Layout();
  DescLayouts[1] = LightViewDescriptorSetLayoutKey->Layout();
  DescLayouts[2] = MaterialSetLayoutKey->Layout();
  DescLayouts[3] = BonesSetLayoutKey->Layout();

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
    SHADOW_CULL_MODE,
    SHADOW_WINDING_ORDER,
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


void SetUpSkyboxPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
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
  skybox_pipelineKey = sky;
  
  DescriptorSetLayout* global = GlobalSetLayoutKey;
  DescriptorSetLayout* skybox = skybox_setLayoutKey;

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
  depthStencilCI.depthWriteEnable = VK_FALSE;
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
  
  LoadShader(SkyRenderer::kSkyVertStr, vert);
  LoadShader(SkyRenderer::kSkyFragStr, frag);

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
  
  GraphicsPipelineInfo.renderPass = gRenderer().SkyRendererNative()->GetSkyboxRenderPass()->Handle();
  sky->Initialize(GraphicsPipelineInfo, pipelineLayout);

  Rhi->FreeShader(vert);
  Rhi->FreeShader(frag);
}


void SetUpAAPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo, AntiAliasing aa)
{
}
} // RendererPass


void AntiAliasingFXAA::Initialize(VulkanRHI* pRhi, GlobalDescriptor* pWorld)
{
  
  CreateTexture(pRhi);
  CreateSampler(pRhi);
  CreateDescriptorSetLayout(pRhi);
  CreateDescriptorSet(pRhi, pWorld);

  VkPipelineLayoutCreateInfo layoutCi{};
  layoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutCi.pPushConstantRanges = nullptr;

  VkDescriptorSetLayout setlayouts[] = {
    GlobalSetLayoutKey->Layout(),
    m_layout->Layout()
  };

  layoutCi.setLayoutCount = 2;
  layoutCi.pSetLayouts = setlayouts;

  Shader* shader = pRhi->CreateShader();
  switch (pRhi->VendorID()) {
    case NVIDIA_VENDOR_ID:
    {
      RendererPass::LoadShader("FXAA.comp.spv", shader);
      //m_groupSz = NVIDIA_WARP_SIZE;
    } break;
    default:
    {
      RendererPass::LoadShader("FXAA.comp.spv", shader);
    } break;
  }  

  VkPipelineShaderStageCreateInfo shaderStageCi{};
  shaderStageCi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCi.module = shader->Handle();
  shaderStageCi.pName = kDefaultShaderEntryPointStr;
  shaderStageCi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCi.flags = 0;

  VkComputePipelineCreateInfo compCi{};
  compCi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  compCi.stage = shaderStageCi;
  compCi.basePipelineHandle = VK_NULL_HANDLE;

  m_pipeline = pRhi->CreateComputePipeline();
  m_pipeline->Initialize(compCi, layoutCi);
  pRhi->FreeShader(shader);
}


void AntiAliasingFXAA::CleanUp(VulkanRHI* pRhi)
{
  if ( m_output ) {
    pRhi->FreeTexture(m_output);
    m_output = nullptr;
  }

  if ( m_outputSampler ) {
    pRhi->FreeSampler(m_outputSampler);
    m_outputSampler = nullptr;
  }

  if ( m_layout ) {
    pRhi->FreeDescriptorSetLayout(m_layout);
    m_layout = nullptr;
  }

  if ( m_descSet ) {
    pRhi->FreeDescriptorSet(m_descSet);
    m_descSet = nullptr;
  }

  if ( m_pipeline ) {
    pRhi->FreeComputePipeline(m_pipeline);
    m_pipeline = nullptr;
  }
}


void AntiAliasingFXAA::CreateTexture(VulkanRHI* pRhi)
{
  m_output = pRhi->CreateTexture();
  VkExtent2D extent = pRhi->SwapchainObject()->SwapchainExtent();
  VkImageCreateInfo imgCi = { };
  VkImageViewCreateInfo imgViewCi = { };
  imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCi.arrayLayers = 1;
  imgCi.extent.width = extent.width;
  imgCi.extent.height = extent.height;
  imgCi.extent.depth = 1;
  imgCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCi.imageType = VK_IMAGE_TYPE_2D;
  imgCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCi.mipLevels = 1;
  imgCi.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  
  imgViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imgViewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgViewCi.subresourceRange.baseArrayLayer = 0;
  imgViewCi.subresourceRange.baseMipLevel = 0;
  imgViewCi.subresourceRange.layerCount = 1;
  imgViewCi.subresourceRange.levelCount = 1;
  imgViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;

  RDEBUG_SET_VULKAN_NAME(m_output, "FXAA");
  m_output->Initialize(imgCi, imgViewCi);
}


void AntiAliasingFXAA::CreateDescriptorSet(VulkanRHI* pRhi, GlobalDescriptor* pDescriptor)
{
  m_descSet = pRhi->CreateDescriptorSet();
  m_descSet->Allocate(pRhi->DescriptorPool(), m_layout);

  UpdateSets(pRhi, pDescriptor);
}


void AntiAliasingFXAA::UpdateSets(VulkanRHI* pRhi, GlobalDescriptor* pDescriptor)
{
  pRhi->FreeTexture(m_output);

  CreateTexture(pRhi);

  VkDescriptorBufferInfo worldInfo = {};
  worldInfo.buffer = pDescriptor->Handle(pRhi->CurrentFrame())->NativeBuffer();
  worldInfo.offset = 0;
  worldInfo.range = VkDeviceSize(sizeof(GlobalDescriptor));

  VkDescriptorImageInfo inputInfo = {};
  inputInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  inputInfo.imageView = pbr_FinalTextureKey->View();
  inputInfo.sampler = gbuffer_SamplerKey->Handle();

  VkDescriptorImageInfo outputInfo = {};
  outputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  outputInfo.imageView = m_output->View();
  outputInfo.sampler = nullptr;

  std::array<VkWriteDescriptorSet, 2> writes;
  writes[0] = {};
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writes[0].dstArrayElement = 0;
  writes[0].dstBinding = 0;
  writes[0].pImageInfo = &inputInfo;

  writes[1] = {};
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].descriptorCount = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[1].dstArrayElement = 0;
  writes[1].dstBinding = 1;
  writes[1].pImageInfo = &outputInfo;

  m_descSet->Update(static_cast<u32>(writes.size()), writes.data());
}


void AntiAliasingFXAA::CreateDescriptorSetLayout(VulkanRHI* pRhi)
{
  VkDescriptorSetLayoutCreateInfo layoutCi = { };
  layoutCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

  std::array<VkDescriptorSetLayoutBinding, 2> binds;
  binds[0] = { };
  binds[0].binding = 0;
  binds[0].descriptorCount = 1;
  binds[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binds[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  binds[1] = { };
  binds[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  binds[1].descriptorCount = 1;
  binds[1].binding = 1;
  binds[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

  layoutCi.bindingCount = static_cast<u32>(binds.size());
  layoutCi.pBindings = binds.data();

  m_layout = pRhi->CreateDescriptorSetLayout();
  m_layout->Initialize(layoutCi);
}


void AntiAliasingFXAA::GenerateCommands(VulkanRHI* pRhi, CommandBuffer* pOutput, GlobalDescriptor* pGlobal, u32 frameIndex)
{
  VkImageSubresourceRange subrange = {};
  subrange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subrange.baseArrayLayer = 0;
  subrange.baseMipLevel = 0;
  subrange.layerCount = 1;
  subrange.levelCount = 1;

  VkImageMemoryBarrier imageMemBarrier = { };
  imageMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemBarrier.subresourceRange = subrange;
  imageMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageMemBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  imageMemBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  imageMemBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  imageMemBarrier.image = m_output->Image();

  pOutput->PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0, 0, nullptr, 0, nullptr, 1u, &imageMemBarrier);

  VkDescriptorSet sets[] = {
    pGlobal->Set(frameIndex)->Handle(),
    m_descSet->Handle()
  };

  VkExtent2D extent = pRhi->SwapchainObject()->SwapchainExtent();
  pOutput->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->Pipeline());
  pOutput->BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->Layout(), 0, 2, sets, 0, nullptr);
  pOutput->Dispatch((extent.width / m_groupSz) + 1, (extent.height / m_groupSz) + 1, 1);

  imageMemBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  imageMemBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  imageMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  imageMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  pOutput->PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0, 0, nullptr, 0, nullptr, 1u, &imageMemBarrier);
}


void AntiAliasingFXAA::CreateSampler(VulkanRHI* pRhi)
{
  m_outputSampler = pRhi->CreateSampler();
  VkSamplerCreateInfo  samplerCi = { };
  samplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.anisotropyEnable = VK_FALSE;
  samplerCi.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerCi.compareEnable = VK_FALSE;
  samplerCi.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerCi.magFilter = VK_FILTER_NEAREST;
  samplerCi.minLod = 0.0f;
  samplerCi.maxAnisotropy = 1.0f;
  samplerCi.minFilter = VK_FILTER_NEAREST;
  samplerCi.maxLod = 1.0f;
  samplerCi.mipLodBias = 0.0f;
  samplerCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerCi.unnormalizedCoordinates = VK_FALSE;
  m_outputSampler->Initialize(samplerCi);
}


void DebugManager::InitializeRenderPass(VulkanRHI* pRhi)
{
  Texture* pbrFinal = pbr_FinalTextureKey;
  std::array<VkAttachmentDescription, 1> attachments;
  attachments[0].format = pbrFinal->Format();
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachments[0].samples = pbrFinal->Samples();
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  std::array<VkAttachmentReference, 1> refs;
  refs[0].attachment = 0;
  refs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthRef;
  depthRef.attachment = 1;
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription debugSubpass = { };
  debugSubpass.colorAttachmentCount = 1;
  debugSubpass.inputAttachmentCount = 0;
  debugSubpass.pColorAttachments = refs.data();
  debugSubpass.pDepthStencilAttachment = &depthRef;
  debugSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  VkRenderPassCreateInfo rpCi = { };
  rpCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpCi.attachmentCount = attachments.size();
  rpCi.pAttachments = attachments.data();
  rpCi.pSubpasses = &debugSubpass;
  rpCi.subpassCount = 1;

  m_renderPass = pRhi->CreateRenderPass();
  m_renderPass->Initialize(rpCi);
}


void DebugManager::Initialize(VulkanRHI* pRhi)
{
  InitializeRenderPass(pRhi);

}


void DebugManager::CleanUp(VulkanRHI* pRhi)
{
  if (m_renderPass) {
    pRhi->FreeRenderPass(m_renderPass);
    m_renderPass = nullptr;
  }
}
} // Recluse