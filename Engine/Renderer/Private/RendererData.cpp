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

std::unordered_map<PipelineGraphicsT, GraphicsPipeline*> g_graphicsPipelines;
std::unordered_map<PipelineComputeT, ComputePipeline*> g_computePipelines;
std::unordered_map<RenderTextureT, std::vector<Texture*>> g_renderTextures;
std::unordered_map<DescriptorSetT, std::vector<DescriptorSet*>> g_descriptorSets;
std::unordered_map<DescriptorSetLayoutT, DescriptorSetLayout*> g_descriptorSetLayouts;

Texture* DefaultTextureKey          = nullptr;
Sampler* DefaultSampler2DKey        = nullptr;
VkImageView DefaultTexture2DArrayView = VK_NULL_HANDLE;

GraphicsPipeline* ShadowMapPipelineKey              = nullptr;
GraphicsPipeline* DynamicShadowMapPipelineKey       = nullptr;
GraphicsPipeline* shadowMap_dynamicMorphTargetPipeline = nullptr;
GraphicsPipeline* shadowMap_staticMorphTargetPipeline = nullptr;
std::string DynamicShadowMapVertFileStr       = "DynamicDepth.vert.spv";
std::string ShadowMapVertFileStr              = "Depth.vert.spv";
std::string ShadowMapFragFileStr              = "Depth.frag.spv";
std::string ShadowMapFragOpaqueFileStr        = "Depth_Opaque.frag.spv";
DescriptorSetLayout* LightViewDescriptorSetLayoutKey   = nullptr;
DescriptorSetLayout* globalIllumination_DescLR = nullptr;
DescriptorSetLayout* globalIllumination_DescNoLR = nullptr;

GraphicsPipeline* transparent_staticShadowPipe = nullptr;
GraphicsPipeline* transparent_dynamicShadowPipe = nullptr;
GraphicsPipeline* transparent_colorFilterPipe = nullptr;

DescriptorSetLayout* gbuffer_LayoutKey                 = nullptr;
Texture* gbuffer_AlbedoAttachKey           = nullptr;
Texture* gbuffer_NormalAttachKey           = nullptr;
Texture* gbuffer_PositionAttachKey         = nullptr;
Texture* gbuffer_EmissionAttachKey         = nullptr;
Sampler* gbuffer_SamplerKey                = nullptr;
FrameBuffer* gbuffer_FrameBufferKey            = nullptr;
RenderPass* gbuffer_renderPass                = nullptr;
std::string gbuffer_VertFileStr               = "GBuffer.vert.spv";
std::string gbuffer_StaticVertFileStr         = "StaticGBuffer.vert.spv";
std::string gbuffer_FragFileStr               = "GBuffer.frag.spv";

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

std::string GlowFragFileStr             = "GlowPass.frag.spv";
Texture* RenderTargetGlowKey         = nullptr;
FrameBuffer* FrameBufferGlowKey          = nullptr;
DescriptorSetLayout* GlowDescriptorSetLayoutKey  = nullptr;
DescriptorSet* GlowDescriptorSetKey        = nullptr;
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

Texture* hdr_gamma_colorAttachKey      = nullptr;
FrameBuffer* hdr_gamma_frameBufferKey      = nullptr;
RenderPass* hdr_renderPass                = nullptr;
Sampler* hdr_gamma_samplerKey          = nullptr;
DescriptorSet* hdr_gamma_descSetKey          = nullptr;
DescriptorSetLayout* hdr_gamma_descSetLayoutKey    = nullptr;
std::string hdr_gamma_vertFileStr         = "HDR.vert.spv";
std::string hdr_gamma_fragFileStr         = "HDR.frag.spv";

DescriptorSet* final_DescSetKey             = nullptr;
DescriptorSetLayout* final_DescSetLayoutKey       = nullptr;
std::string final_VertFileStr            = "FinalPass.vert.spv";
std::string final_FragFileStr            = "FinalPass.frag.spv";
Texture* final_renderTargetKey       = nullptr;
FrameBuffer* final_frameBufferKey = nullptr;
RenderPass* final_renderPass = nullptr;
DescriptorSet*  output_descSetKey = nullptr;

GraphicsPipeline* envMap_pbrPipeline = nullptr;
FrameBuffer*       envMap_frameBuffer = nullptr;
RenderPass*        envMap_renderPass = nullptr;
Texture*           envMap_texture = nullptr;

GraphicsPipeline* debug_wireframePipelineStatic = nullptr;
GraphicsPipeline* debug_wireframePipelineAnim = nullptr;

// Default entry point on shaders.
char const* kDefaultShaderEntryPointStr = "main";


void SetUpRenderData()
{
  kViewMatrices = {
    Matrix4::rotate(Matrix4::rotate(Matrix4::identity(), 
                                    Radians(90.0f), 
                                    Vector3::UP), 
                    Radians(180.0f), 
                    Vector3::RIGHT),
    Matrix4::rotate(Matrix4::rotate(Matrix4::identity(), 
                                    Radians(-90.0f), 
                                    Vector3::UP), 
                    Radians(180.0f), 
                    Vector3::RIGHT),
    Matrix4::rotate(Matrix4::rotate(Matrix4::identity(), 
                                    Radians(-90.0f), 
                                    Vector3::RIGHT), 
                                    Radians(180.0f), 
                                    Vector3::UP),
    Matrix4::rotate(Matrix4::rotate(Matrix4::identity(), 
                                    Radians(90.0f), 
                                    Vector3::RIGHT), 
                    Radians(180.0f), 
                    Vector3::UP),
    Matrix4::rotate(Matrix4::identity(), 
                    Radians(180.0f), 
                    Vector3::BACK),
    Matrix4::rotate(Matrix4::rotate(Matrix4::identity(), 
                                    Radians(180.0f), 
                                    Vector3::UP), 
                    Radians(180.0f), 
                    Vector3::FRONT)
  };
}


void CleanUpRenderData()
{

}


namespace RendererPass {


void initializePipelines(VulkanRHI* pRhi)
{ 
  for (I32 i = PIPELINE_GRAPHICS_START; i < PIPELINE_GRAPHICS_END; ++i)
  {
    g_graphicsPipelines[ (PipelineGraphicsT)i ] = pRhi->createGraphicsPipeline();
  }

  for (I32 i = PIPELINE_COMPUTE_START; i < PIPELINE_GRAPHICS_END; ++i)
  {
    g_computePipelines[ (PipelineComputeT) i ] = pRhi->createComputePipeline();
  }
}


void cleanUpPipelines(VulkanRHI* pRhi)
{
  for ( I32 i = PIPELINE_GRAPHICS_START; i < PIPELINE_GRAPHICS_END; ++i) 
  {
    pRhi->freeGraphicsPipeline(g_graphicsPipelines[ (PipelineGraphicsT) i ]);
    g_graphicsPipelines[ (PipelineGraphicsT) i ] = nullptr;
  }

  for ( I32 i = PIPELINE_COMPUTE_START; i < PIPELINE_COMPUTE_END; ++i ) {
    pRhi->freeComputePipeline(g_computePipelines[ (PipelineComputeT) i ]);
    g_computePipelines[ (PipelineComputeT) i ] = nullptr;
  }

  g_graphicsPipelines.clear();
  g_computePipelines.clear();
}


void initializeRenderTextures(Renderer* pRenderer)
{
  VulkanRHI* pRhi = pRenderer->getRHI( );
  for ( I32 i = RENDER_TEXTURE_START; i < RENDER_TEXTURE_END; ++i ) 
  {
    g_renderTextures[ (RenderTextureT) i ].resize(pRenderer->getResourceBufferCount());
    for (U32 j = 0; j < pRenderer->getResourceBufferCount(); ++j) 
    {
      g_renderTextures[ (RenderTextureT) i ][ j ] = pRhi->createTexture( );
    }
  }
}


void cleanUpRenderTextures(VulkanRHI* pRhi)
{
  for (I32 i = RENDER_TEXTURE_START; i < RENDER_TEXTURE_END; ++i) 
  {
    for (U32 j = 0; j < g_renderTextures[(RenderTextureT)i].size(); ++j) 
    {
      pRhi->freeTexture(g_renderTextures[ (RenderTextureT) i ][ j ]);
      g_renderTextures[ (RenderTextureT) i ][ j ] = nullptr; 
    } 
  }
}


Texture* getRenderTexture(RenderTextureT rt, U32 resourceIndex)
{
  return g_renderTextures[ rt ][ resourceIndex ];
}


void initializeDescriptorSetLayouts(VulkanRHI* pRhi)
{
  for (I32 i = DESCRIPTOR_SET_LAYOUT_START; i < DESCRIPTOR_SET_LAYOUT_END; ++i) 
  {
    g_descriptorSetLayouts[ (DescriptorSetLayoutT) i ] = pRhi->createDescriptorSetLayout( );
  }
}


void cleanUpDescriptorSetLayouts(VulkanRHI* pRhi)
{
  for (I32 i = DESCRIPTOR_SET_LAYOUT_START; i < DESCRIPTOR_SET_LAYOUT_END; ++i) 
  {
    pRhi->freeDescriptorSetLayout( g_descriptorSetLayouts[ (DescriptorSetLayoutT) i ]);
    g_descriptorSetLayouts[ (DescriptorSetLayoutT) i ] = nullptr;
  }
}


void initializeDescriptorSets(Renderer* pRenderer)
{
  VulkanRHI* pRhi = pRenderer->getRHI();
  for (I32 i = DESCRIPTOR_SET_START; i < DESCRIPTOR_SET_END; ++i) 
  {
    g_descriptorSets[ (DescriptorSetT) i ] = std::vector<DescriptorSet*>(pRenderer->getResourceBufferCount());
    
    for (U32 j = 0; j < g_descriptorSets[ (DescriptorSetT) i ].size(); ++j) 
    {
      g_descriptorSets[ (DescriptorSetT) i ][ j ] = pRhi->createDescriptorSet( );
    }
  }
}


void cleanUpDescriptorSets(VulkanRHI* pRhi)
{
  for (I32 i = DESCRIPTOR_SET_START; i < DESCRIPTOR_SET_END; ++i) 
  {
    for (U32 j = 0; j < g_descriptorSets[(DescriptorSetT)i].size(); ++j) 
    {
      pRhi->freeDescriptorSet(g_descriptorSets[(DescriptorSetT)i][j]);
    
      g_descriptorSets[ (DescriptorSetT) i ][ j ] = nullptr;
    }
  }
}


DescriptorSet* getDescriptorSet(DescriptorSetT set, U32 resourceIndex)
{
  return g_descriptorSets[ set ][ resourceIndex ];
}


DescriptorSetLayout* getDescriptorSetLayout(DescriptorSetLayoutT layout)
{
  return g_descriptorSetLayouts[ layout ];
}


void loadShader(const std::string& Filename, Shader* S)
{
  if (!S) { Log(rError) << "Shader module is null! Can not load a shader!\n"; }
  std::string Filepath = gFilesystem().CurrentAppDirectory();
  if (!S->initialize(Filepath
      + "/" + ShadersPath + "/" + Filename)) 
  {
    Log(rError) << "Could not find " + Filename + "!";
  }
}


void SetUpGBufferPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  // PbrForward Pipeline Creation.
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;

  VkPipelineDepthStencilStateCreateInfo depthStencilCI = {};
  depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilCI.depthTestEnable = VK_TRUE;
  depthStencilCI.depthWriteEnable = VK_TRUE;
  depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilCI.depthBoundsTestEnable = Rhi->depthBoundsAllowed();
  depthStencilCI.minDepthBounds = 0.0f;
  depthStencilCI.maxDepthBounds = 1.0f;
  depthStencilCI.stencilTestEnable = VK_FALSE;
  depthStencilCI.back = {};
  depthStencilCI.front = {};

  if (Rhi->stencilTestAllowed()) 
  {
    depthStencilCI.stencilTestEnable = VK_TRUE;
    depthStencilCI.back.compareMask = 0xff;
    depthStencilCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilCI.back.depthFailOp = VK_STENCIL_OP_REPLACE;
    depthStencilCI.back.passOp = VK_STENCIL_OP_REPLACE;
    depthStencilCI.back.failOp = VK_STENCIL_OP_ZERO;
    depthStencilCI.back.writeMask = 0xff;
    depthStencilCI.back.reference = 1;
    depthStencilCI.front = depthStencilCI.back;
  }

  GraphicsInfo.pDepthStencilState = &depthStencilCI;

  FrameBuffer* GBufferFrameBuffer = gbuffer_FrameBufferKey;
  Shader* VertGBuffer = Rhi->createShader();
  Shader* FragGBuffer = Rhi->createShader();

  loadShader(gbuffer_VertFileStr, VertGBuffer);
  loadShader(gbuffer_FragFileStr, FragGBuffer);

  VkPipelineShaderStageCreateInfo PbrShaders[2];
  PbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[0].module = VertGBuffer->getHandle();
  PbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  PbrShaders[0].pName = kDefaultShaderEntryPointStr;
  PbrShaders[0].pNext = nullptr;
  PbrShaders[0].pSpecializationInfo = nullptr;
  PbrShaders[0].flags = 0;

  PbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[1].module = FragGBuffer->getHandle();
  PbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  PbrShaders[1].pName = kDefaultShaderEntryPointStr;
  PbrShaders[1].pNext = nullptr;
  PbrShaders[1].flags = 0;
  PbrShaders[1].pSpecializationInfo = nullptr;

  GraphicsInfo.renderPass = GBufferFrameBuffer->RenderPassRef()->getHandle();
  GraphicsInfo.stageCount = 2;
  GraphicsInfo.pStages = PbrShaders;

  std::array<VkDescriptorSetLayout, 4> DLayouts;
  DLayouts[0] = GlobalSetLayoutKey->getLayout();
  DLayouts[1] = MeshSetLayoutKey->getLayout();
  DLayouts[2] = MaterialSetLayoutKey->getLayout();
  DLayouts[3] = BonesSetLayoutKey->getLayout();

  VkPipelineLayoutCreateInfo PipelineLayout = {};
  PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipelineLayout.setLayoutCount = static_cast<U32>(DLayouts.size());
  PipelineLayout.pSetLayouts = DLayouts.data();
  PipelineLayout.pPushConstantRanges = 0;
  PipelineLayout.pushConstantRangeCount = 0;

  // Initialize pbr forward pipeline.
  g_graphicsPipelines[ PIPELINE_GRAPHICS_GBUFFER_DYNAMIC ]->initialize(GraphicsInfo, PipelineLayout);

  {
    VkGraphicsPipelineCreateInfo ginfo = GraphicsInfo;
    VkPipelineVertexInputStateCreateInfo input = { };
    ginfo.pVertexInputState = &input;
    auto bindings = MorphTargetVertexDescription::GetBindingDescriptions(SkinnedVertexDescription::GetBindingDescription());
    auto attribs = MorphTargetVertexDescription::GetVertexAttributes(SkinnedVertexDescription::GetVertexAttributes());
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.pVertexAttributeDescriptions = attribs.data();
    input.pVertexBindingDescriptions = bindings.data();
    input.vertexAttributeDescriptionCount = static_cast<U32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<U32>(bindings.size());

    // Initialize pbr forward morph target pipeline.
    Rhi->freeShader(VertGBuffer);
    VertGBuffer = Rhi->createShader();
    loadShader("GBuffer_MorphTargets.vert.spv", VertGBuffer);
    PbrShaders[0].module = VertGBuffer->getHandle();
    g_graphicsPipelines[ PIPELINE_GRAPHICS_GBUFFER_DYNAMIC_MORPH_TARGETS ]->initialize(ginfo, PipelineLayout);
    Rhi->freeShader(VertGBuffer);
    VertGBuffer = nullptr;
  }
  // isStatic pipeline creation.
  auto Bindings = StaticVertexDescription::GetBindingDescription();
  auto VertexAttribs = StaticVertexDescription::GetVertexAttributes();
  VkPipelineVertexInputStateCreateInfo Input = { };

  GraphicsInfo.pVertexInputState = &Input;
  Input.vertexAttributeDescriptionCount = static_cast<U32>(VertexAttribs.size());
  Input.vertexBindingDescriptionCount = 1;
  Input.pVertexBindingDescriptions = &Bindings;
  Input.pVertexAttributeDescriptions = VertexAttribs.data();
  Input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  Input.pNext = nullptr;

  Rhi->freeShader(VertGBuffer);
  VertGBuffer = Rhi->createShader();
  loadShader(gbuffer_StaticVertFileStr, VertGBuffer);
  
  PbrShaders[0].module = VertGBuffer->getHandle();
  PipelineLayout.setLayoutCount = static_cast<U32>(DLayouts.size() - 1); // We don't need bone buffer.
  g_graphicsPipelines[ PIPELINE_GRAPHICS_GBUFFER_STATIC ]->initialize(GraphicsInfo, PipelineLayout);
  
  Rhi->freeShader(VertGBuffer);
  VertGBuffer = nullptr;
  // isStatic Morph Target gbuffer pipeline.
  {
    VkGraphicsPipelineCreateInfo ginfo = GraphicsInfo;
    VkPipelineVertexInputStateCreateInfo input = {};
    ginfo.pVertexInputState = &input;
    auto bindings = MorphTargetVertexDescription::GetBindingDescriptions(StaticVertexDescription::GetBindingDescription());
    auto attribs = MorphTargetVertexDescription::GetVertexAttributes(StaticVertexDescription::GetVertexAttributes());
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.pVertexAttributeDescriptions = attribs.data();
    input.pVertexBindingDescriptions = bindings.data();
    input.vertexAttributeDescriptionCount = static_cast<U32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<U32>(bindings.size());

    // Initialize pbr forward morph target pipeline.
    Rhi->freeShader(VertGBuffer);
    VertGBuffer = Rhi->createShader();
    loadShader("StaticGBuffer_MorphTargets.vert.spv", VertGBuffer);
    PbrShaders[0].module = VertGBuffer->getHandle();
    g_graphicsPipelines[ PIPELINE_GRAPHICS_GBUFFER_STATIC_MORPH_TARGETS ]->initialize(ginfo, PipelineLayout);
    Rhi->freeShader(VertGBuffer);
    VertGBuffer = nullptr;
  }


  {
    
  }
  Rhi->freeShader(FragGBuffer);
}


GraphicsPipeline* getGraphicsPipeline(PipelineGraphicsT pipeline)
{
  return g_graphicsPipelines[ pipeline ];
}


ComputePipeline* getComputePipeline(PipelineComputeT pipeline)
{
  return g_computePipelines[ pipeline ];
}


void SetUpHDRGammaPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo, HDR* pHDR)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  VkPipelineLayoutCreateInfo hdrLayout = {};
  std::array<VkDescriptorSetLayout, 3> hdrSetLayout; 
  hdrSetLayout[0] = GlobalSetLayoutKey->getLayout();
  hdrSetLayout[1] = hdr_gamma_descSetLayoutKey->getLayout();
  hdrSetLayout[2] = pHDR->getSetLayout()->getLayout();

  Shader* HdrFrag = Rhi->createShader();
  Shader* HdrVert = Rhi->createShader();

  loadShader(hdr_gamma_vertFileStr, HdrVert);
  loadShader(hdr_gamma_fragFileStr, HdrFrag);

  FrameBuffer* hdrBuffer = hdr_gamma_frameBufferKey;
  GraphicsInfo.renderPass = hdrBuffer->RenderPassRef()->getHandle();

  VkPipelineShaderStageCreateInfo ShaderModules[2];
  ShaderModules[0].flags = 0;
  ShaderModules[0].module = HdrVert->getHandle();
  ShaderModules[0].pName = kDefaultShaderEntryPointStr;
  ShaderModules[0].pNext = nullptr;
  ShaderModules[0].pSpecializationInfo = nullptr;
  ShaderModules[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  ShaderModules[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  ShaderModules[1].flags = 0;
  ShaderModules[1].module = HdrFrag->getHandle();
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
  hdrLayout.setLayoutCount = static_cast<U32>(hdrSetLayout.size());
  hdrLayout.pSetLayouts = hdrSetLayout.data();
  hdrLayout.pPushConstantRanges = &pushConstantRange;
  hdrLayout.pushConstantRangeCount = 1;

  g_graphicsPipelines[ PIPELINE_GRAPHICS_HDR_GAMMA ]->initialize(GraphicsInfo, hdrLayout);

  Rhi->freeShader(HdrFrag);
  Rhi->freeShader(HdrVert);

}


void SetUpDeferredPhysicallyBasedPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  {
    VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilCI = {};
    depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCI.depthTestEnable = VK_FALSE;
    depthStencilCI.depthWriteEnable = VK_FALSE;
    depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCI.depthBoundsTestEnable = Rhi->depthBoundsAllowed();
    depthStencilCI.minDepthBounds = 0.0f;
    depthStencilCI.maxDepthBounds = 1.0f;
    depthStencilCI.stencilTestEnable = VK_FALSE;
    depthStencilCI.back = {};
    depthStencilCI.front = {};
    if (Rhi->stencilTestAllowed()) {
      depthStencilCI.stencilTestEnable = VK_TRUE;
      depthStencilCI.back.compareMask = 0xff;
      depthStencilCI.back.compareOp = VK_COMPARE_OP_EQUAL;
      depthStencilCI.back.depthFailOp = VK_STENCIL_OP_KEEP;
      depthStencilCI.back.passOp = VK_STENCIL_OP_KEEP;
      depthStencilCI.back.failOp = VK_STENCIL_OP_KEEP;
      depthStencilCI.back.writeMask = 0xff;
      depthStencilCI.back.reference = 1;
      depthStencilCI.front = depthStencilCI.back;
    }

    GraphicsInfo.pDepthStencilState = &depthStencilCI;

    FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;  

    Shader* VertPBR = Rhi->createShader();
    Shader* FragPBR = Rhi->createShader();

    loadShader(pbr_VertStr, VertPBR);
    loadShader(pbr_FragStrLR, FragPBR);

    VkPipelineShaderStageCreateInfo PbrShaders[2];
    PbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    PbrShaders[0].module = VertPBR->getHandle();
    PbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    PbrShaders[0].pName = kDefaultShaderEntryPointStr;
    PbrShaders[0].pNext = nullptr;
    PbrShaders[0].pSpecializationInfo = nullptr;
    PbrShaders[0].flags = 0;

    PbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    PbrShaders[1].module = FragPBR->getHandle();
    PbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    PbrShaders[1].pName = kDefaultShaderEntryPointStr;
    PbrShaders[1].pNext = nullptr;
    PbrShaders[1].flags = 0;
    PbrShaders[1].pSpecializationInfo = nullptr;

    GraphicsInfo.renderPass = pbr_FrameBuffer->RenderPassRef()->getHandle();
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
      static_cast<U32>(colorBlendAttachments.size()),
      colorBlendAttachments.data(),
      VK_FALSE,
      VK_LOGIC_OP_COPY
    );

    GraphicsInfo.pColorBlendState = &colorBlendCI;

    std::array<VkDescriptorSetLayout, 5> layouts;
    layouts[0] = GlobalSetLayoutKey->getLayout();
    layouts[1] = pbr_DescLayoutKey->getLayout();
    layouts[2] = LightSetLayoutKey->getLayout();
    layouts[3] = getDescriptorSetLayout( DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE_OUT )->getLayout();
    //layouts[4] = LightViewDescriptorSetLayoutKey->getLayout();
    layouts[4] = globalIllumination_DescLR->getLayout();

    VkPipelineLayoutCreateInfo PipelineLayout = {};
    PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayout.setLayoutCount = static_cast<U32>(layouts.size());
    PipelineLayout.pSetLayouts = layouts.data();
    PipelineLayout.pPushConstantRanges = 0;
    PipelineLayout.pushConstantRangeCount = 0;

    g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_DEFERRED_LR ]->initialize(GraphicsInfo, PipelineLayout);

    Rhi->freeShader(FragPBR);

    FragPBR = Rhi->createShader();
    loadShader(pbr_FragStrNoLR, FragPBR);

    PbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    PbrShaders[0].module = VertPBR->getHandle();
    PbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    PbrShaders[0].pName = kDefaultShaderEntryPointStr;
    PbrShaders[0].pNext = nullptr;
    PbrShaders[0].pSpecializationInfo = nullptr;
    PbrShaders[0].flags = 0;

    PbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    PbrShaders[1].module = FragPBR->getHandle();
    PbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    PbrShaders[1].pName = kDefaultShaderEntryPointStr;
    PbrShaders[1].pNext = nullptr;
    PbrShaders[1].flags = 0;
    PbrShaders[1].pSpecializationInfo = nullptr;

    layouts[4] = globalIllumination_DescNoLR->getLayout();
    g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_DEFERRED_NOLR ]->initialize(GraphicsInfo, PipelineLayout);

    Rhi->freeShader(VertPBR);
    Rhi->freeShader(FragPBR);
  }

  U32 vendorId = Rhi->vendorID();

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
    Shader* compShader = Rhi->createShader();
    loadShader(noLr, compShader);

    VkDescriptorSetLayout layouts[6] = { 
      GlobalSetLayoutKey->getLayout(),
      pbr_DescLayoutKey->getLayout(),
      LightSetLayoutKey->getLayout(),
      getDescriptorSetLayout( DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE_OUT )->getLayout(),
      //LightViewDescriptorSetLayoutKey->getLayout(),
      globalIllumination_DescNoLR->getLayout(),
      pbr_compDescLayout->getLayout()
    };    

    g_computePipelines[ PIPELINE_COMPUTE_PBR_DEFERRED_NOLR ] = Rhi->createComputePipeline();
    VkComputePipelineCreateInfo computeCi = { };
    VkPipelineLayoutCreateInfo compLayoutCi = { };
    compLayoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    compLayoutCi.setLayoutCount = 6;
    compLayoutCi.pSetLayouts = layouts;

    VkPipelineShaderStageCreateInfo shaderCi = { };
    shaderCi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderCi.module = compShader->getHandle();
    shaderCi.pName = kDefaultShaderEntryPointStr;
    shaderCi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeCi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computeCi.basePipelineHandle = VK_NULL_HANDLE;
    computeCi.basePipelineIndex = -1;
    computeCi.stage = shaderCi;
    
    g_computePipelines[ PIPELINE_COMPUTE_PBR_DEFERRED_NOLR ]->initialize(computeCi, compLayoutCi);
    Rhi->freeShader(compShader);
    g_computePipelines[ PIPELINE_COMPUTE_PBR_DEFERRED_LR ] = Rhi->createComputePipeline();
    compShader = Rhi->createShader();
    loadShader(lr, compShader);

    layouts[4] = globalIllumination_DescLR->getLayout();
    shaderCi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderCi.module = compShader->getHandle();
    shaderCi.pName = kDefaultShaderEntryPointStr;
    shaderCi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeCi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computeCi.stage = shaderCi;
    computeCi.basePipelineHandle = VK_NULL_HANDLE;
    computeCi.basePipelineIndex = -1;
    computeCi.stage = shaderCi;

    g_computePipelines[ PIPELINE_COMPUTE_PBR_DEFERRED_LR ]->initialize(computeCi, compLayoutCi);
    Rhi->freeShader(compShader);
  }
}


void SetUpForwardPhysicallyBasedPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;

  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_LR ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_NOLR ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_LR ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_NOLR ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_MORPH_LR ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_MORPH_NOLR ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_MORPH_LR ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_MORPH_NOLR ] = Rhi->createGraphicsPipeline();

  pbr_static_LR_Debug = Rhi->createGraphicsPipeline();
  pbr_static_NoLR_Debug = Rhi->createGraphicsPipeline();
  pbr_dynamic_LR_Debug = Rhi->createGraphicsPipeline();
  pbr_dynamic_NoLR_Debug = Rhi->createGraphicsPipeline();
  pbr_static_mt_LR_Debug = Rhi->createGraphicsPipeline();
  pbr_static_mt_NoLR_Debug = Rhi->createGraphicsPipeline();
  pbr_dynamic_LR_mt_Debug = Rhi->createGraphicsPipeline();
  pbr_dynamic_NoLR_mt_Debug = Rhi->createGraphicsPipeline();

  FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;

  Shader* VertPBRStatic = Rhi->createShader();
  Shader* VertPBRLR = Rhi->createShader();
  Shader* FragPBRLR = Rhi->createShader();
  Shader* FragPBRNoLR = Rhi->createShader();
  Shader* FragPBRLRDebug = Rhi->createShader();
  Shader* FragPBRNoLRDebug = Rhi->createShader();
  Shader* VertMorphSkin = Rhi->createShader();
  Shader* VertMorphStatic = Rhi->createShader();

  loadShader(pbr_forwardVertStrLR, VertPBRLR);
  loadShader(gbuffer_StaticVertFileStr, VertPBRStatic);
  loadShader(pbr_forwardFragStrLR, FragPBRLR);
  loadShader(pbr_forwardFragStrNoLR, FragPBRNoLR);
  loadShader("ForwardPBR_LR_Debug.frag.spv", FragPBRLRDebug);
  loadShader("ForwardPBR_NoLR_Debug.frag.spv", FragPBRNoLRDebug);
  loadShader("ForwardPBR_MorphTargets.vert.spv", VertMorphSkin);
  loadShader("StaticGBuffer_MorphTargets.vert.spv", VertMorphStatic);
  
  VkPipelineShaderStageCreateInfo PbrShaders[2];
  PbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[0].module = VertPBRLR->getHandle();
  PbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  PbrShaders[0].pName = kDefaultShaderEntryPointStr;
  PbrShaders[0].pNext = nullptr;
  PbrShaders[0].pSpecializationInfo = nullptr;
  PbrShaders[0].flags = 0;

  PbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  PbrShaders[1].module = FragPBRLR->getHandle();
  PbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  PbrShaders[1].pName = kDefaultShaderEntryPointStr;
  PbrShaders[1].pNext = nullptr;
  PbrShaders[1].flags = 0;
  PbrShaders[1].pSpecializationInfo = nullptr;

  GraphicsInfo.renderPass = pbr_forwardRenderPass->getHandle();
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
    static_cast<U32>(colorBlendAttachments.size()),
    colorBlendAttachments.data(),
    VK_FALSE,
    VK_LOGIC_OP_COPY
  );

  GraphicsInfo.pColorBlendState = &colorBlendCI;

  std::array<VkDescriptorSetLayout, 7> layouts;
  layouts[0] = GlobalSetLayoutKey->getLayout();
  layouts[1] = MeshSetLayoutKey->getLayout();
  layouts[2] = MaterialSetLayoutKey->getLayout();
  layouts[3] = LightSetLayoutKey->getLayout();
  layouts[4] = getDescriptorSetLayout( DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE_OUT )->getLayout();
  //layouts[5] = LightViewDescriptorSetLayoutKey->getLayout();
  layouts[5] = globalIllumination_DescLR->getLayout();
  layouts[6] = BonesSetLayoutKey->getLayout();

  struct ivec4 {
    I32 v[4];
  };

  VkPushConstantRange range = { };
  range.offset = 0;
  range.size = sizeof(ivec4);
  range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineLayoutCreateInfo PipelineLayout = {};
  PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipelineLayout.setLayoutCount = static_cast<U32>(layouts.size());
  PipelineLayout.pSetLayouts = layouts.data();
  PipelineLayout.pPushConstantRanges = &range;
  PipelineLayout.pushConstantRangeCount = 0;

  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_LR ]->initialize(GraphicsInfo, PipelineLayout);

  PbrShaders[1].module = FragPBRLRDebug->getHandle();
  PipelineLayout.pushConstantRangeCount = 1;
  pbr_dynamic_LR_Debug->initialize(GraphicsInfo, PipelineLayout);
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
    input.vertexAttributeDescriptionCount = static_cast<U32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<U32>(bindings.size());

    PbrShaders[0].module = VertMorphSkin->getHandle();
    PbrShaders[1].module = FragPBRLR->getHandle();
    g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_MORPH_LR ]->initialize(ginfo, PipelineLayout);

    PbrShaders[1].module = FragPBRLRDebug->getHandle();  
    PipelineLayout.pushConstantRangeCount = 1;
    pbr_dynamic_LR_mt_Debug->initialize(ginfo, PipelineLayout);
    PipelineLayout.pushConstantRangeCount = 0;
  }

  PbrShaders[0].module = VertPBRLR->getHandle();
  PbrShaders[1].module = FragPBRNoLR->getHandle();
  layouts[5] = globalIllumination_DescNoLR->getLayout();
  PipelineLayout.setLayoutCount = static_cast<U32>(layouts.size());
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_NOLR ]->initialize(GraphicsInfo, PipelineLayout);

  PbrShaders[1].module = FragPBRNoLRDebug->getHandle();
  PipelineLayout.pushConstantRangeCount = 1;
  pbr_dynamic_NoLR_Debug->initialize(GraphicsInfo, PipelineLayout);
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
    input.vertexAttributeDescriptionCount = static_cast<U32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<U32>(bindings.size());

    PbrShaders[0].module = VertMorphSkin->getHandle();
    PbrShaders[1].module = FragPBRNoLR->getHandle();
    g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_MORPH_NOLR ]->initialize(ginfo, PipelineLayout);

    PbrShaders[1].module = FragPBRNoLRDebug->getHandle();
    PipelineLayout.pushConstantRangeCount = 1;
    pbr_dynamic_NoLR_mt_Debug->initialize(ginfo, PipelineLayout);
    PipelineLayout.pushConstantRangeCount = 0;
  }

  PbrShaders[0].module = VertPBRStatic->getHandle();
  PbrShaders[1].module = FragPBRLR->getHandle();
  PipelineLayout.setLayoutCount = static_cast<U32>(layouts.size() - 1);

  // isStatic pipeline creation.
  auto StaticBindings = StaticVertexDescription::GetBindingDescription();
  auto StaticVertexAttribs = StaticVertexDescription::GetVertexAttributes();
  VkPipelineVertexInputStateCreateInfo Input = {};

  GraphicsInfo.pVertexInputState = &Input;
  Input.vertexAttributeDescriptionCount = static_cast<U32>(StaticVertexAttribs.size());
  Input.vertexBindingDescriptionCount = 1;
  Input.pVertexBindingDescriptions = &StaticBindings;
  Input.pVertexAttributeDescriptions = StaticVertexAttribs.data();
  Input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  Input.pNext = nullptr;
  layouts[5] = globalIllumination_DescLR->getLayout();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_LR ]->initialize(GraphicsInfo, PipelineLayout);

  PbrShaders[1].module = FragPBRLRDebug->getHandle();
  PipelineLayout.pushConstantRangeCount = 1;
  pbr_static_LR_Debug->initialize(GraphicsInfo, PipelineLayout);
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
    input.vertexAttributeDescriptionCount = static_cast<U32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<U32>(bindings.size());

    PbrShaders[0].module = VertMorphStatic->getHandle();
    PbrShaders[1].module = FragPBRLR->getHandle();
    g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_MORPH_LR ]->initialize(ginfo, PipelineLayout);

    PbrShaders[1].module = FragPBRLRDebug->getHandle();
    PipelineLayout.pushConstantRangeCount = 1;
    pbr_static_mt_LR_Debug->initialize(ginfo, PipelineLayout);
    PipelineLayout.pushConstantRangeCount = 0;
  }


  // No Local Reflections pipelines.
  PbrShaders[0].module = VertPBRStatic->getHandle();
  PbrShaders[1].module = FragPBRNoLR->getHandle();
  layouts[5] = globalIllumination_DescNoLR->getLayout();
  PipelineLayout.setLayoutCount = static_cast<U32>(layouts.size() - 1);
  g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_NOLR ]->initialize(GraphicsInfo, PipelineLayout);

  PbrShaders[1].module = FragPBRNoLRDebug->getHandle();
  PipelineLayout.pushConstantRangeCount = 1;
  pbr_static_NoLR_Debug->initialize(GraphicsInfo, PipelineLayout);
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
    input.vertexAttributeDescriptionCount = static_cast<U32>(attribs.size());
    input.vertexBindingDescriptionCount = static_cast<U32>(bindings.size());

    PbrShaders[0].module = VertMorphStatic->getHandle();
    PbrShaders[1].module = FragPBRNoLR->getHandle();
    g_graphicsPipelines[ PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_MORPH_NOLR ]->initialize(ginfo, PipelineLayout);

    PbrShaders[1].module = FragPBRNoLRDebug->getHandle();
    PipelineLayout.pushConstantRangeCount = 1;
    pbr_static_mt_NoLR_Debug->initialize(ginfo, PipelineLayout);
    PipelineLayout.pushConstantRangeCount = 0;
  }

  Rhi->freeShader(VertPBRStatic);
  Rhi->freeShader(VertPBRLR);
  Rhi->freeShader(FragPBRLR);
  Rhi->freeShader(FragPBRNoLR);
  Rhi->freeShader(VertMorphSkin);
  Rhi->freeShader(VertMorphStatic);
  Rhi->freeShader(FragPBRLRDebug);
  Rhi->freeShader(FragPBRNoLRDebug);
}


void setUpDownScalePass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;

  // TODO(): Glow and Downsampling graphics pipeline, which will be done right after pbr 
  // pass. 
  VkPipelineInputAssemblyStateCreateInfo n = { };
  n.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  n.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  n.primitiveRestartEnable = VK_FALSE;
  GraphicsInfo.pInputAssemblyState = &n;

  g_graphicsPipelines[ PIPELINE_GRAPHICS_DOWNSCALE_BLUR_2X ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_DOWNSCALE_BLUR_4X ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_DOWNSCALE_BLUR_8X ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_DOWNSCALE_BLUR_16X ] = Rhi->createGraphicsPipeline();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_GLOW ] = Rhi->createGraphicsPipeline();
  // Scaled and Final framebuffers have the same renderpass, so we can just use 
  // one of their renderpasses.
  FrameBuffer*      FrameBuffer2x = FrameBuffer2xHorizKey;
  FrameBuffer*      FrameBuffer4x = FrameBuffer4xKey;
  FrameBuffer*      FrameBuffer8x = FrameBuffer8xKey;
  FrameBuffer*      FrameBuffer16x = FrameBuffer16xKey;
  FrameBuffer*      GlowFrameBuffer = FrameBufferGlowKey;

  DescriptorSetLayout* DownscaleDescLayout = DownscaleBlurLayoutKey;
  DescriptorSetLayout* GlowDescLayout = GlowDescriptorSetLayoutKey;

  Shader* DbVert = Rhi->createShader();
  Shader* DbFrag = Rhi->createShader();

  loadShader(DownscaleBlurVertFileStr, DbVert);
  loadShader(DownscaleBlurFragFileStr, DbFrag);

  VkPushConstantRange PushConst = {};
  PushConst.offset = 0;
  PushConst.size = 12;
  PushConst.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayout DwnsclLayout[] = { DownscaleDescLayout->getLayout() };
  VkPipelineLayoutCreateInfo DownscaleLayout = {};
  DownscaleLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  DownscaleLayout.pushConstantRangeCount = 1;
  DownscaleLayout.pPushConstantRanges = &PushConst;
  DownscaleLayout.setLayoutCount = 1;
  DownscaleLayout.pSetLayouts = DwnsclLayout;

  VkPipelineShaderStageCreateInfo ShaderModules[2];
  ShaderModules[0].flags = 0;
  ShaderModules[0].module = DbVert->getHandle();
  ShaderModules[0].pName = kDefaultShaderEntryPointStr;
  ShaderModules[0].pNext = nullptr;
  ShaderModules[0].pSpecializationInfo = nullptr;
  ShaderModules[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  ShaderModules[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  ShaderModules[1].flags = 0;
  ShaderModules[1].module = DbFrag->getHandle();
  ShaderModules[1].pName = kDefaultShaderEntryPointStr;
  ShaderModules[1].pNext = nullptr;
  ShaderModules[1].pSpecializationInfo = nullptr;
  ShaderModules[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  ShaderModules[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  GraphicsInfo.pStages = ShaderModules;
  GraphicsInfo.stageCount = 2;

  VkViewport viewport;
  VkRect2D scissor;

  viewport = { 0.0f, 0.0f, 
              (R32)FrameBuffer2x->getWidth(), (R32)FrameBuffer2x->getHeight(),
              0.0f, 1.0f };

  VkPipelineViewportStateCreateInfo viewportCi = { };
  viewportCi.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportCi.scissorCount = 1;
  viewportCi.viewportCount = 1;
  viewportCi.pViewports = &viewport;
  viewportCi.pScissors = &scissor;
    

  scissor = { { 0, 0 }, { FrameBuffer2x->getWidth(), FrameBuffer2x->getHeight() } };
  GraphicsInfo.pViewportState = &viewportCi;
  GraphicsInfo.renderPass = FrameBuffer2x->RenderPassRef()->getHandle();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_DOWNSCALE_BLUR_2X ]->initialize(GraphicsInfo, DownscaleLayout);

  viewport.width = (R32)FrameBuffer4x->getWidth();
  viewport.height = (R32)FrameBuffer4x->getHeight();
  scissor.extent = { FrameBuffer4x->getWidth(), FrameBuffer4x->getHeight() };
  GraphicsInfo.renderPass = FrameBuffer4x->RenderPassRef()->getHandle();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_DOWNSCALE_BLUR_4X ]->initialize(GraphicsInfo, DownscaleLayout);

  viewport.width = (R32)FrameBuffer8x->getWidth();
  viewport.height = (R32)FrameBuffer8x->getHeight();
  scissor.extent = {FrameBuffer8x->getWidth(), FrameBuffer8x->getHeight()};
  GraphicsInfo.renderPass = FrameBuffer8x->RenderPassRef()->getHandle();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_DOWNSCALE_BLUR_8X ]->initialize(GraphicsInfo, DownscaleLayout);

  viewport.width = (R32)FrameBuffer16x->getWidth();
  viewport.height = (R32)FrameBuffer16x->getHeight();
  scissor.extent = {FrameBuffer16x->getWidth(), FrameBuffer16x->getHeight()};
  GraphicsInfo.renderPass = FrameBuffer16x->RenderPassRef()->getHandle();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_DOWNSCALE_BLUR_16X ]->initialize(GraphicsInfo, DownscaleLayout);

  Rhi->freeShader(DbFrag);
  DbFrag = Rhi->createShader();

  loadShader(GlowFragFileStr, DbFrag);

  ShaderModules[1].module = DbFrag->getHandle();
  VkPipelineLayoutCreateInfo GlowPipelineLayout = { };
  VkDescriptorSetLayout GlowDescSetLayout = GlowDescLayout->getLayout();
  GlowPipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  GlowPipelineLayout.pSetLayouts = &GlowDescSetLayout;
  GlowPipelineLayout.setLayoutCount = 1;
  GlowPipelineLayout.pPushConstantRanges = nullptr;
  GlowPipelineLayout.pushConstantRangeCount = 0;

  viewport.width = (R32)GlowFrameBuffer->getWidth();
  viewport.height = (R32)GlowFrameBuffer->getHeight();
  scissor.extent = { GlowFrameBuffer->getWidth(), GlowFrameBuffer->getHeight() };
  GraphicsInfo.renderPass = GlowFrameBuffer->RenderPassRef()->getHandle();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_GLOW ]->initialize(GraphicsInfo, GlowPipelineLayout);

  Rhi->freeShader(DbVert);
  Rhi->freeShader(DbFrag);
}


void SetUpFinalPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
  Shader* quadVert = Rhi->createShader();
  Shader* quadFrag = Rhi->createShader();
  // TODO(): Need to structure all of this into more manageable modules.
  //
  VkPipelineInputAssemblyStateCreateInfo n = { };

  g_graphicsPipelines[ PIPELINE_GRAPHICS_FINAL ] = Rhi->createGraphicsPipeline();

  loadShader(final_VertFileStr, quadVert);
  loadShader(final_FragFileStr, quadFrag);

  GraphicsInfo.renderPass = final_frameBufferKey->RenderPassRef()->getHandle();

  VkPipelineShaderStageCreateInfo FinalShaders[2];
  FinalShaders[0].flags = 0;
  FinalShaders[0].module = quadVert->getHandle();
  FinalShaders[0].pName = kDefaultShaderEntryPointStr;
  FinalShaders[0].pNext = nullptr;
  FinalShaders[0].pSpecializationInfo = nullptr;
  FinalShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  FinalShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  FinalShaders[1].flags = 0;
  FinalShaders[1].module = quadFrag->getHandle();
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
  VkDescriptorSetLayout finalL = final_DescSetLayoutKey->getLayout();
  finalLayout.pSetLayouts = &finalL;
  finalLayout.pushConstantRangeCount = 0;
  finalLayout.pPushConstantRanges = nullptr;

  VkPipelineDynamicStateCreateInfo dd = { };
  VkDynamicState ds[] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
  dd.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dd.dynamicStateCount = 2;
  dd.pDynamicStates = ds;
  GraphicsInfo.pDynamicState = &dd;

  g_graphicsPipelines[ PIPELINE_GRAPHICS_FINAL ]->initialize(GraphicsInfo, finalLayout);

  // Now create the output pipeline.
  g_graphicsPipelines[ PIPELINE_GRAPHICS_OUTPUT ] = Rhi->createGraphicsPipeline();
  GraphicsInfo.renderPass = Rhi->swapchainRenderPass();
  g_graphicsPipelines[ PIPELINE_GRAPHICS_OUTPUT ]->initialize(GraphicsInfo, finalLayout);

  Rhi->freeShader(quadVert);
  Rhi->freeShader(quadFrag);
}


void SetUpDirectionalShadowPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = DefaultInfo;
  GraphicsPipeline* ShadowMapPipeline = Rhi->createGraphicsPipeline();
  GraphicsPipeline* DynamicShadowMapPipeline = Rhi->createGraphicsPipeline();
  ShadowMapPipelineKey = ShadowMapPipeline;
  DynamicShadowMapPipelineKey = DynamicShadowMapPipeline;

  // TODO(): Initialize shadow map pipeline.
  VkPipelineLayoutCreateInfo PipeLayout = {};
  std::array<VkDescriptorSetLayout, 4> DescLayouts;
  DescLayouts[0] = MeshSetLayoutKey->getLayout();
  DescLayouts[1] = LightViewDescriptorSetLayoutKey->getLayout();
  DescLayouts[2] = MaterialSetLayoutKey->getLayout();
  DescLayouts[3] = BonesSetLayoutKey->getLayout();

  auto Bindings = StaticVertexDescription::GetBindingDescription();
  auto Attribs = StaticVertexDescription::GetVertexAttributes();
  VkPipelineVertexInputStateCreateInfo Info = {};
  Info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  Info.pVertexAttributeDescriptions = Attribs.data();
  Info.pVertexBindingDescriptions = &Bindings;
  Info.vertexAttributeDescriptionCount = static_cast<U32>(Attribs.size());
  Info.vertexBindingDescriptionCount = 1;
  Info.pNext = nullptr;

  PipeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipeLayout.pushConstantRangeCount = 0;
  PipeLayout.pPushConstantRanges = nullptr;
  PipeLayout.setLayoutCount = static_cast<U32>(DescLayouts.size() - 1);
  PipeLayout.pSetLayouts = DescLayouts.data();
  // ShadowMapping shader.
  // TODO(): Shadow mapping MUST be done before downsampling and glow buffers have finished!
  // This will prevent blurry shadows. It must be combined in the forward render pass (maybe?)
  Shader* SmVert = Rhi->createShader();
  Shader* SmFrag = Rhi->createShader();

  RendererPass::loadShader(ShadowMapVertFileStr, SmVert);
  RendererPass::loadShader(ShadowMapFragFileStr, SmFrag);

  std::array<VkPipelineShaderStageCreateInfo, 2> Shaders;
  Shaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shaders[0].flags = 0;
  Shaders[0].pName = kDefaultShaderEntryPointStr;
  Shaders[0].pNext = nullptr;
  Shaders[0].pSpecializationInfo = nullptr;
  Shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  Shaders[0].module = SmVert->getHandle();

  Shaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shaders[1].flags = 0;
  Shaders[1].pName = kDefaultShaderEntryPointStr;
  Shaders[1].pNext = nullptr;
  Shaders[1].pSpecializationInfo = nullptr;
  Shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  Shaders[1].module = SmFrag->getHandle();

  GraphicsPipelineInfo.pStages = Shaders.data();
  GraphicsPipelineInfo.stageCount = static_cast<U32>(Shaders.size());
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
  ShadowMapPipeline->initialize(GraphicsPipelineInfo, PipeLayout);

  Bindings = SkinnedVertexDescription::GetBindingDescription();
  Attribs = SkinnedVertexDescription::GetVertexAttributes();
  Info.pVertexAttributeDescriptions = Attribs.data();
  Info.pVertexBindingDescriptions = &Bindings;
  Info.vertexAttributeDescriptionCount = static_cast<U32>(Attribs.size());
  Info.vertexBindingDescriptionCount = 1;
  Info.pNext = nullptr;
  
  PipeLayout.setLayoutCount += 1;
  // Create the dynamic shadow map pipeline.

  
  Rhi->freeShader(SmVert);
  SmVert = Rhi->createShader();
  loadShader(DynamicShadowMapVertFileStr, SmVert);
 
  Shaders[0].module = SmVert->getHandle();

  DynamicShadowMapPipeline->initialize(GraphicsPipelineInfo, PipeLayout);

  Rhi->freeShader(SmVert);
  Rhi->freeShader(SmFrag);

  VkGraphicsPipelineCreateInfo GraphicsInfo = DefaultInfo;
}


void SetUpSkyboxPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo)
{
  Shader* vert = Rhi->createShader();
  Shader* frag = Rhi->createShader();
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

  GraphicsPipeline* sky = Rhi->createGraphicsPipeline();
  skybox_pipelineKey = sky;
  
  DescriptorSetLayout* global = GlobalSetLayoutKey;
  DescriptorSetLayout* skybox = skybox_setLayoutKey;

  VkDescriptorSetLayout layouts[] = { 
    global->getLayout(),
    skybox->getLayout()
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
  
  loadShader(SkyRenderer::kSkyVertStr, vert);
  loadShader(SkyRenderer::kSkyFragStr, frag);

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
    static_cast<U32>(colorBlendAttachments.size()),
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
  shaders[0].module = vert->getHandle();

  shaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaders[1].flags = 0;
  shaders[1].pName = kDefaultShaderEntryPointStr;
  shaders[1].pNext = nullptr;
  shaders[1].pSpecializationInfo = nullptr;
  shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaders[1].module = frag->getHandle();

  GraphicsPipelineInfo.stageCount = 2;
  GraphicsPipelineInfo.pStages = shaders.data();
  
  GraphicsPipelineInfo.renderPass = gRenderer().getSkyRendererNative()->getSkyboxRenderPass()->getHandle();
  sky->initialize(GraphicsPipelineInfo, pipelineLayout);

  Rhi->freeShader(vert);
  Rhi->freeShader(frag);
}


void setUpAAPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo, AntiAliasing aa)
{
}


void initShadowMaskTexture(Renderer* pRenderer, const VkExtent2D& renderRes) 
{
  VkImageCreateInfo imgInfo = { };
  VkImageViewCreateInfo viewInfo = { };

  imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgInfo.arrayLayers = 1;
  imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgInfo.imageType = VK_IMAGE_TYPE_2D;
  imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgInfo.mipLevels = 1;
  imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgInfo.extent = { renderRes.width, renderRes.height, 1 };

  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.layerCount = 1;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

  for (U32 i = 0; i < pRenderer->getResourceBufferCount(); ++i) {
    Texture* shadowMask = getRenderTexture(RENDER_TEXTURE_SHADOW_RESOLVE_OUTPUT, i);
    shadowMask->initialize(imgInfo, viewInfo);
  }
}

void initShadowResolvePipeline(VulkanRHI* pRhi)
{
  ComputePipeline* pipeline = getComputePipeline( PIPELINE_COMPUTE_SHADOW_RESOLVE );
  Shader* pShader = pRhi->createShader();

  VkDescriptorSetLayout dLayouts[] = {
    GlobalSetLayoutKey->getLayout(),
    LightViewDescriptorSetLayoutKey->getLayout(),
    getDescriptorSetLayout( DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE )->getLayout()
  };

  loadShader("ShadowResolve.comp.spv", pShader);
  VkComputePipelineCreateInfo info = { };
  VkPipelineShaderStageCreateInfo stage = {};

  stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stage.module = pShader->getHandle();
  stage.pName = kDefaultShaderEntryPointStr;
  stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;

  VkPipelineLayoutCreateInfo pipeCi = { };
  pipeCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeCi.setLayoutCount = 3;
  pipeCi.pSetLayouts = dLayouts;

  info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  info.stage = stage;

  pipeline->initialize(info, pipeCi);
  pRhi->freeShader(pShader);
}


void initShadowResolveDescriptorSetLayout(VulkanRHI* pRhi)
{
  DescriptorSetLayout* layout = getDescriptorSetLayout( DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE );
  VkDescriptorSetLayoutCreateInfo info = { };
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

  std::array<VkDescriptorSetLayoutBinding, 2> binds;
  
  binds[0] = { };
  binds[0].binding = 1;
  binds[0].descriptorCount = 1;
  binds[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binds[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  
  binds[1].binding = 0;
  binds[1].descriptorCount = 1;
  binds[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  binds[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  info.bindingCount = binds.size();
  info.pBindings = binds.data();

  layout->initialize(info);

  // For output to pbr lighting pass.
  layout = getDescriptorSetLayout( DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE_OUT );
  
  binds[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binds[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
  binds[0].binding = 0;

  info.bindingCount = 1;
  info.pBindings = binds.data();
  layout->initialize(info);
}


void initShadowReolveDescriptorSet(Renderer* pRenderer, 
                                   GlobalDescriptor* pGlobal,
                                   Texture* pSceneDepth)
{
  DescriptorSetLayout* layout = getDescriptorSetLayout( DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE );
  DescriptorSetLayout* outLayout = getDescriptorSetLayout( DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE_OUT );
  std::array<VkWriteDescriptorSet, 2> writeSets;

  VkDescriptorImageInfo depthInfo = { };
  depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  depthInfo.imageView = pSceneDepth->getView();
  depthInfo.sampler = DefaultSampler2DKey->getHandle();

  VkDescriptorImageInfo mask = { };
  mask.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  mask.sampler = DefaultSampler2DKey->getHandle();

  writeSets[0] = { };
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].dstBinding = 1;
  writeSets[0].pImageInfo = &depthInfo;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

  writeSets[1] = { };
  writeSets[1].descriptorCount = 1;
  writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writeSets[1].dstArrayElement = 0;
  writeSets[1].dstBinding = 0;
  writeSets[1].pImageInfo = &mask;
  writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  
  for (U32 i = 0; i < pRenderer->getResourceBufferCount(); ++ i ) {
    Texture* shadowMaskTex = getRenderTexture( RENDER_TEXTURE_SHADOW_RESOLVE_OUTPUT, i );
    DescriptorSet* set = getDescriptorSet(DESCRIPTOR_SET_SHADOW_RESOLVE, i);
    mask.imageView = shadowMaskTex->getView();
    set->allocate(pRenderer->getRHI()->descriptorPool(), layout);
    set->update(writeSets.size(), writeSets.data());
  }

  // Output of shadow mask to be readable and sampled.
  mask.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  writeSets[0].pImageInfo = &mask;
  writeSets[0].dstBinding = 0;
  
  for (U32 i = 0; i < pRenderer->getResourceBufferCount(); ++i) {
    Texture* shadowMaskTex = getRenderTexture( RENDER_TEXTURE_SHADOW_RESOLVE_OUTPUT, i );
    DescriptorSet* outSet = getDescriptorSet(DESCRIPTOR_SET_SHADOW_RESOLVE_OUT, i);
    mask.imageView = shadowMaskTex->getView();
    outSet->allocate(pRenderer->getRHI()->descriptorPool(), outLayout);
    outSet->update(1, writeSets.data());
  }
}


void initPreZPipelines(VulkanRHI* pRhi, const VkGraphicsPipelineCreateInfo& info)
{
  Shader* pFragDepth = pRhi->createShader( );
  loadShader("Depth.frag.spv", pFragDepth);

  VkGraphicsPipelineCreateInfo graphics = info;

  // ViewSpace push constant.
  VkPushConstantRange pushconstant = { };
  pushconstant.offset = 0;
  pushconstant.size = sizeof(Matrix4);
  pushconstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; 

  VkDescriptorSetLayout setLayouts[] = {
    MeshSetLayoutKey->getLayout(),
    MaterialSetLayoutKey->getLayout(),
    BonesSetLayoutKey->getLayout()
  };

  VkPipelineLayoutCreateInfo layoutCi = { };
  layoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutCi.pushConstantRangeCount = 1;
  layoutCi.pPushConstantRanges = &pushconstant;
  layoutCi.setLayoutCount = 2; // static, we will include joint buffer layout after.
  layoutCi.pSetLayouts = setLayouts;

  pRhi->freeShader(pFragDepth);
}
} // RendererPass


void AntiAliasingFXAA::initialize(Renderer* pRenderer, GlobalDescriptor* pWorld)
{
  VulkanRHI* pRhi = pRenderer->getRHI();
  createTexture(pRhi, pWorld);
  createSampler(pRhi);
  createDescriptorSetLayout(pRhi);

  m_descSets.resize(pRenderer->getResourceBufferCount());
  for (U32 i = 0; i < m_descSets.size(); ++i) {
    createDescriptorSet(pRhi, pWorld, i);
  }

  updateSets(pRenderer, pWorld);

  VkPipelineLayoutCreateInfo layoutCi{};
  layoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutCi.pPushConstantRanges = nullptr;

  VkDescriptorSetLayout setlayouts[] = {
    GlobalSetLayoutKey->getLayout(),
    m_layout->getLayout()
  };

  layoutCi.setLayoutCount = 2;
  layoutCi.pSetLayouts = setlayouts;

  Shader* shader = pRhi->createShader();
  switch (pRhi->vendorID()) {
    case NVIDIA_VENDOR_ID:
    {
      RendererPass::loadShader("FXAA.comp.spv", shader);
      //m_groupSz = NVIDIA_WARP_SIZE;
    } break;
    default:
    {
      RendererPass::loadShader("FXAA.comp.spv", shader);
    } break;
  }  

  VkPipelineShaderStageCreateInfo shaderStageCi{};
  shaderStageCi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCi.module = shader->getHandle();
  shaderStageCi.pName = kDefaultShaderEntryPointStr;
  shaderStageCi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCi.flags = 0;

  VkComputePipelineCreateInfo compCi{};
  compCi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  compCi.stage = shaderStageCi;
  compCi.basePipelineHandle = VK_NULL_HANDLE;

  m_pipeline = pRhi->createComputePipeline();
  m_pipeline->initialize(compCi, layoutCi);
  pRhi->freeShader(shader);
}


void AntiAliasingFXAA::cleanUp(VulkanRHI* pRhi)
{
  if ( m_output ) {
    pRhi->freeTexture(m_output);
    m_output = nullptr;
  }

  if ( m_outputSampler ) {
    pRhi->freeSampler(m_outputSampler);
    m_outputSampler = nullptr;
  }

  if ( m_layout ) {
    pRhi->freeDescriptorSetLayout(m_layout);
    m_layout = nullptr;
  }

  for (U32 i = 0; i < m_descSets.size(); ++i) {
    if ( m_descSets[i] ) {
      pRhi->freeDescriptorSet(m_descSets[i]);
      m_descSets[i] = nullptr;
    }
  }

  if ( m_pipeline ) {
    pRhi->freeComputePipeline(m_pipeline);
    m_pipeline = nullptr;
  }
}


void AntiAliasingFXAA::createTexture(VulkanRHI* pRhi, GlobalDescriptor* pGlobal)
{
  m_output = pRhi->createTexture();
  VkExtent2D extent = { (U32)pGlobal->getData()->_ScreenSize[0],
                        (U32)pGlobal->getData()->_ScreenSize[1] };
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
  m_output->initialize(imgCi, imgViewCi);
}


void AntiAliasingFXAA::createDescriptorSet(VulkanRHI* pRhi, GlobalDescriptor* pDescriptor, U32 resourceIndex)
{
  m_descSets[resourceIndex] = pRhi->createDescriptorSet();
  m_descSets[resourceIndex]->allocate(pRhi->descriptorPool(), m_layout);
}


void AntiAliasingFXAA::updateSets(Renderer* pRenderer, GlobalDescriptor* pDescriptor)
{
  if (pRenderer->getResourceBufferCount() != m_descSets.size()) {
    for (U32 i = 0; i < m_descSets.size(); ++i) pRenderer->getRHI()->freeDescriptorSet(m_descSets[i]);
    m_descSets.resize(pRenderer->getResourceBufferCount());
    for(U32 i = 0; i < m_descSets.size(); ++i) createDescriptorSet(pRenderer->getRHI(), pDescriptor, i);
  }

  VulkanRHI* pRhi = pRenderer->getRHI();
  pRhi->freeTexture(m_output);

  createTexture(pRhi, pDescriptor);

  VkDescriptorBufferInfo worldInfo = {};
  worldInfo.buffer = nullptr; // to be updated at end.
  worldInfo.offset = 0;
  worldInfo.range = VkDeviceSize(sizeof(GlobalDescriptor));

  VkDescriptorImageInfo inputInfo = {};
  inputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  inputInfo.imageView = pbr_FinalTextureKey->getView();
  inputInfo.sampler = gbuffer_SamplerKey->getHandle();

  VkDescriptorImageInfo outputInfo = {};
  outputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  outputInfo.imageView = m_output->getView();
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

  for (U32 i = 0; i < m_descSets.size(); ++i) {
    worldInfo.buffer = pDescriptor->getHandle(i)->getNativeBuffer();
    m_descSets[i]->update(static_cast<U32>(writes.size()), writes.data());
  }
}


void AntiAliasingFXAA::createDescriptorSetLayout(VulkanRHI* pRhi)
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

  layoutCi.bindingCount = static_cast<U32>(binds.size());
  layoutCi.pBindings = binds.data();

  m_layout = pRhi->createDescriptorSetLayout();
  m_layout->initialize(layoutCi);
}


void AntiAliasingFXAA::generateCommands(VulkanRHI* pRhi, CommandBuffer* pOutput, GlobalDescriptor* pGlobal, U32 resourceIndex)
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
  imageMemBarrier.image = m_output->getImage();

  pOutput->pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0, 0, nullptr, 0, nullptr, 1u, &imageMemBarrier);

  VkDescriptorSet sets[] = {
    pGlobal->getDescriptorSet(resourceIndex)->getHandle(),
    m_descSets[resourceIndex]->getHandle()
  };

  VkExtent2D extent = { (U32)pGlobal->getData()->_ScreenSize[0],
                        (U32)pGlobal->getData()->_ScreenSize[1] };
  pOutput->bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getNative());
  pOutput->bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getLayout(), 0, 2, sets, 0, nullptr);
  pOutput->dispatch((extent.width / m_groupSz) + 1, (extent.height / m_groupSz) + 1, 1);

  imageMemBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  imageMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  imageMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  pOutput->pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0, 0, nullptr, 0, nullptr, 1u, &imageMemBarrier);
}


void AntiAliasingFXAA::createSampler(VulkanRHI* pRhi)
{
  m_outputSampler = pRhi->createSampler();
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
  m_outputSampler->initialize(samplerCi);
}


void DebugManager::initializeRenderPass(VulkanRHI* pRhi)
{
  Texture* pbrFinal = pbr_FinalTextureKey;
  Texture* depthFinal = RendererPass::getRenderTexture(RENDER_TEXTURE_SCENE_DEPTH, 0);
  std::array<VkAttachmentDescription, 2> attachments;
  attachments[0] = { };
  attachments[0].format = pbrFinal->getFormat();
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachments[0].samples = pbrFinal->getSamples();
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  attachments[1] = { };
  attachments[1].format = depthFinal->getFormat();
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachments[1].samples = depthFinal->getSamples();
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

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

  VkSubpassDependency dependencies[2];

  dependencies[0] = CreateSubPassDependency(
    VK_SUBPASS_EXTERNAL, 
    VK_ACCESS_MEMORY_WRITE_BIT, 
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    0, 
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 
    VK_DEPENDENCY_BY_REGION_BIT
  );

  dependencies[1] = CreateSubPassDependency(
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_WRITE_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  VkRenderPassCreateInfo rpCi = { };
  rpCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpCi.attachmentCount = static_cast<U32>(attachments.size());
  rpCi.pAttachments = attachments.data();
  rpCi.pSubpasses = &debugSubpass;
  rpCi.subpassCount = 1;
  rpCi.dependencyCount = 2;
  rpCi.pDependencies = dependencies;

  m_renderPass = pRhi->createRenderPass();
  m_renderPass->initialize(rpCi);
}


void DebugManager::initialize(VulkanRHI* pRhi)
{
  initializeRenderPass(pRhi);
  createPipelines(pRhi);
}


void DebugManager::cleanUp(VulkanRHI* pRhi)
{
  if (m_renderPass) {
    pRhi->freeRenderPass(m_renderPass);
    m_renderPass = nullptr;
  }

  if (m_staticWireframePipeline) {
    pRhi->freeGraphicsPipeline(m_staticWireframePipeline);
    m_staticWireframePipeline = nullptr;
  }
}



void DebugManager::createPipelines(VulkanRHI* pRhi)
{
  m_staticWireframePipeline = pRhi->createGraphicsPipeline();
  VkGraphicsPipelineCreateInfo ci = { };
  ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  VkPipelineInputAssemblyStateCreateInfo assem = { };
  assem.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assem.primitiveRestartEnable = VK_FALSE;
  assem.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineVertexInputStateCreateInfo input = { };
  input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO; 

  VkPipelineRasterizationStateCreateInfo rast = { };
  rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

  VkPipelineDepthStencilStateCreateInfo ds = { };
  ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  ds.depthTestEnable = VK_TRUE;
  ds.depthWriteEnable = VK_TRUE;
  ds.maxDepthBounds = 1.0f;
  ds.minDepthBounds = 0.0f;
  ds.stencilTestEnable = VK_FALSE;
  ds.depthBoundsTestEnable = VK_TRUE;
  ds.depthCompareOp = VK_COMPARE_OP_LESS;

  ci.renderPass = m_renderPass->getHandle();
  ci.pRasterizationState = &rast;
  ci.pVertexInputState = &input;
  ci.pInputAssemblyState = &assem;
  ci.pDepthStencilState = &ds;

  ci.stageCount = 2;  

}


void DebugManager::RecordDebugCommands(VulkanRHI* pRhi,
                                       CommandBuffer* pBuf,
                                       SimpleRenderCmd* renderCmds,
                                       U32 count)
{

}
} // Recluse