// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "RHI/VulkanRHI.hpp"
#include "UserParams.hpp"


#define GBUFFER_ALBEDO_FORMAT             VK_FORMAT_R8G8B8A8_UNORM
#define GBUFFER_NORMAL_FORMAT             VK_FORMAT_R16G16_UNORM
#define GBUFFER_ROUGH_METAL_FORMAT        VK_FORMAT_R8G8B8A8_UNORM 
#define GBUFFER_ADDITIONAL_INFO_FORMAT    VK_FORMAT_R8G8B8A8_UNORM


namespace Recluse {

class Renderer;

extern std::string ShadersPath;

extern std::string DefaultTextureStr;
extern std::string DefaultSamplerStr;

extern std::string ShadowMapPipelineStr;
extern std::string DynamicShadowMapPipelineStr;
extern std::string ShadowMapVertFileStr;
extern std::string ShadowMapFragFileStr;
extern std::string LightViewDescriptorSetLayoutStr;
extern std::string illumination_reflectProbeDescLayoutStr;

extern std::string gbuffer_PipelineStr;
extern std::string gbuffer_StaticPipelineStr;
extern std::string gbuffer_LayoutStr;
extern std::string gbuffer_AlbedoAttachStr;
extern std::string gbuffer_NormalAttachStr;
extern std::string gbuffer_PositionAttachStr;
extern std::string gbuffer_EmissionAttachStr;
extern std::string gbuffer_SamplerStr;
extern std::string gbuffer_DepthAttachStr;
extern std::string gbuffer_FrameBufferStr;
extern std::string gbuffer_VertFileStr;
extern std::string gbuffer_StaticVertFileStr;
extern std::string gbuffer_FragFileStr;

extern std::string pbr_PipelineStr;
extern std::string pbr_FrameBufferStr;
extern std::string pbr_DescLayoutStr;
extern std::string pbr_DescSetStr;
extern std::string pbr_FinalTextureStr;
extern std::string pbr_BrightTextureStr;
extern std::string pbr_VertStr;
extern std::string pbr_FragStr;

extern std::string pbr_forwardPipelineStr;
extern const char* pbr_forwardVertStr;
extern const char* pbr_forwardFragStr;

extern std::string aa_PipelineStr;
extern std::string aa_FrameBufferStr;
extern std::string aa_DescLayoutStr;
extern std::string aa_outputTextureStr;
extern std::string aa_fragStr;

// Work in progress.
extern std::string cluster_FrustumPipelineStr;
extern std::string cluster_LightCullPipelineStr;
extern std::string cluster_LightGridStr;
extern std::string cluster_FrustumTextureStr;

extern std::string renderquad_vertStr;

extern std::string hiz_FullTexture;
extern std::string hiz_2xTex;
extern std::string hiz_4xTex;

extern std::string skybox_pipelineStr;
extern std::string skybox_descriptorSetStr;
extern std::string skybox_setLayoutStr;

extern std::string MeshSetLayoutStr;
extern std::string MaterialSetLayoutStr;
extern std::string BonesSetLayoutStr;
extern std::string GlobalSetLayoutStr;
extern std::string LightSetLayoutStr;

extern std::string ScaledSamplerStr;
extern std::string RenderTarget2xHorizStr;
extern std::string RenderTarget2xFinalStr;
extern std::string RenderTarget4xScaledStr;
extern std::string RenderTarget4xFinalStr;
extern std::string RenderTarget8xScaledStr;
extern std::string RenderTarget8xFinalStr;
extern std::string RenderTarget16xScaledStr;
extern std::string RenderTarget16xFinalStr;
extern std::string FrameBuffer2xHorizStr;
extern std::string FrameBuffer2xFinalStr;
extern std::string FrameBuffer4xStr;
extern std::string FrameBuffer4xFinalStr;
extern std::string FrameBuffer8xStr;
extern std::string FrameBuffer8xFinalStr;
extern std::string FrameBuffer16xStr;
extern std::string FrameBuffer16xFinalStr;
extern std::string DownscaleBlurLayoutStr;
extern std::string DownscaleBlurDescriptorSet2x;
extern std::string DownscaleBlurDescriptorSet2xFinalStr;
extern std::string DownscaleBlurDescriptorSet4x;
extern std::string DownscaleBlurDescriptorSet4xFinalStr;
extern std::string DownscaleBlurDescriptorSet8x;
extern std::string DownscaleBlurDescriptorSet8xFinalStr;
extern std::string DownscaleBlurDescriptorSet16x;
extern std::string DownscaleBlurDescriptorSet16xFinalStr;
extern std::string GlowPipelineStr;
extern std::string GlowFragFileStr;
extern std::string RenderTargetGlowStr;
extern std::string FrameBufferGlowStr;
extern std::string GlowDescriptorSetLayoutStr;
extern std::string GlowDescriptorSetStr;
extern std::string DownscaleBlurPipeline2xStr;
extern std::string DownscaleBlurPipeline4xStr;
extern std::string DownscaleBlurPipeline8xStr;
extern std::string DownscaleBlurPipeline16xStr;
extern std::string DownscaleBlurVertFileStr;
extern std::string DownscaleBlurFragFileStr;

extern std::string RenderTargetVelocityStr;

extern std::string hdr_gamma_pipelineStr;
extern std::string hdr_gamma_colorAttachStr;
extern std::string hdr_gamma_frameBufferStr;
extern std::string hdr_gamma_samplerStr;
extern std::string hdr_gamma_descSetStr;
extern std::string hdr_gamma_descSetLayoutStr;
extern std::string hdr_gamma_vertFileStr;
extern std::string hdr_gamma_fragFileStr;

extern std::string final_PipelineStr;
extern std::string final_DescSetStr;
extern std::string final_DescSetLayoutStr;
extern std::string final_VertFileStr;
extern std::string final_FragFileStr;

extern char const* kDefaultShaderEntryPointStr;

namespace RendererPass {


void LoadShader(std::string Filename, Shader* S);

// Set up the downscale pass.
void SetUpDownScalePass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

// Set up the HDR Gamma pass.
void SetUpHDRGammaPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpGBufferPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpPhysicallyBasedPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpFinalPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpDirectionalShadowPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpSkyboxPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpAAPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo, AntiAliasing aa);

}
} // Recluse