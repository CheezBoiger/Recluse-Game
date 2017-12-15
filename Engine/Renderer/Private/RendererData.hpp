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
extern std::string ShadowMapVertFileStr;
extern std::string ShadowMapFragFileStr;
extern std::string LightViewDescriptorSetLayoutStr;

extern std::string PBRPipelineStr;
extern std::string PBRStaticPipelineStr;
extern std::string PBRLayoutStr;
extern std::string PBRColorAttachStr;
extern std::string PBRNormalAttachStr;
extern std::string PBRSamplerStr;
extern std::string PBRDepthAttachStr;
extern std::string PBRFrameBufferStr;
extern std::string PBRGlobalMatLayoutStr;
extern std::string PBRLightMatLayoutStr;
extern std::string PBRObjMatLayoutStr;
extern std::string PBRVertFileStr;
extern std::string PBRStaticVertFileStr;
extern std::string PBRFragFileStr;
extern std::string RenderTargetBrightStr;

extern std::string ScaledSamplerStr;
extern std::string RenderTarget2xHorizStr;
extern std::string RenderTarget2xFinalStr;
extern std::string RenderTarget4xScaledStr;
extern std::string RenderTarget4xFinalStr;
extern std::string RenderTarget8xScaledStr;
extern std::string RenderTarget8xFinalStr;
extern std::string FrameBuffer2xHorizStr;
extern std::string FrameBuffer2xFinalStr;
extern std::string FrameBuffer4xStr;
extern std::string FrameBuffer4xFinalStr;
extern std::string FrameBuffer8xStr;
extern std::string FrameBuffer8xFinalStr;
extern std::string DownscaleBlurLayoutStr;
extern std::string DownscaleBlurDescriptorSet2x;
extern std::string DownscaleBlurDescriptorSet2xFinalStr;
extern std::string DownscaleBlurDescriptorSet4x;
extern std::string DownscaleBlurDescriptorSet4xFinalStr;
extern std::string DownscaleBlurDescriptorSet8x;
extern std::string DownscaleBlurDescriptorSet8xFinalStr;
extern std::string GlowPipelineStr;
extern std::string GlowFragFileStr;
extern std::string RenderTargetGlowStr;
extern std::string FrameBufferGlowStr;
extern std::string GlowDescriptorSetLayoutStr;
extern std::string GlowDescriptorSetStr;
extern std::string DownscaleBlurPipeline2xStr;
extern std::string DownscaleBlurPipeline4xStr;
extern std::string DownscaleBlurPipeline8xStr;
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


namespace RendererPass {

// Set up the downscale pass.
void SetUpDownScalePass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

// Set up the HDR Gamma pass.
void SetUpHDRGammaPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpPBRForwardPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpFinalPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpDirectionalShadowPass(VulkanRHI* Rhi, const std::string& Filepath, const VkGraphicsPipelineCreateInfo& DefaultInfo);
}
} // Recluse