// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "RHI/VulkanRHI.hpp"


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

extern std::string gbuffer_PipelineStr;
extern std::string gbuffer_StaticPipelineStr;
extern std::string gbuffer_LayoutStr;
extern std::string gbuffer_AlbedoAttachStr;
extern std::string gbuffer_NormalAttachStr;
extern std::string gbuffer_PositionAttachStr;
extern std::string gbuffer_RoughMetalAttachStr;
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

extern std::string SkyboxPipelineStr;
extern std::string SkyboxDescriptorSetStr;
extern std::string SkyboxSetLayoutStr;

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

extern std::string HDRGammaPipelineStr;
extern std::string HDRGammaColorAttachStr;
extern std::string HDRGammaFrameBufferStr;
extern std::string HDRGammaSamplerStr;
extern std::string HDRGammaDescSetStr;
extern std::string HDRGammaDescSetLayoutStr;
extern std::string HDRGammaVertFileStr;
extern std::string HDRGammaFragFileStr;

extern std::string FinalPipelineStr;
extern std::string FinalDescSetStr;
extern std::string FinalDescSetLayoutStr;
extern std::string FinalVertFileStr;
extern std::string FinalFragFileStr;

extern char const* kDefaultShaderEntryPointStr;

namespace RendererPass {


void LoadShader(std::string Filename, Shader* S);

// Set up the downscale pass.
void SetUpDownScalePass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

// Set up the HDR Gamma pass.
void SetUpHDRGammaPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpGBufferPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpPhysicallyBasedPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpFinalPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpDirectionalShadowPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpSkyboxPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);
}
} // Recluse