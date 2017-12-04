// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererData.hpp"


namespace Recluse {

std::string ShadersPath               = "Shaders";

std::string DefaultTextureStr         = "DefaultTexture";
std::string DefaultSamplerStr         = "DefaultSampler";

std::string ShadowMapPipelineStr      = "ShadowMapPipeline";
std::string ShadowMapVertFileStr      = "ShadowMapping.vert.spv";
std::string ShadowMapFragFileStr      = "ShadowMapping.frag.spv";

std::string PBRPipelineStr            = "PBRPipeline";
std::string PBRStaticPipelineStr      = "PBRStaticPipeline";
std::string PBRLayoutStr              = "PBRLayout";
std::string PBRColorAttachStr         = "PBRColor";
std::string PBRNormalAttachStr        = "PBRNormal";
std::string PBRDepthAttachStr         = "PBRDepth";
std::string PBRSamplerStr             = "PBRSampler";
std::string PBRFrameBufferStr         = "PBRFrameBuffer";
std::string PBRGlobalMatLayoutStr     = "PBRGlobalMaterialLayout";
std::string PBRLightMatLayoutStr      = "PBRLightMaterialLayout";
std::string PBRObjMatLayoutStr        = "PBRObjectMaterialLayout";
std::string PBRVertFileStr            = "PBRPass.vert.spv";
std::string PBRFragFileStr            = "PBRPass.frag.spv";

std::string ScaledSamplerStr          = "ScaledSampler";
std::string RenderTarget2xScaledStr   = "RenderTarget2x";
std::string RenderTarget4xScaledStr   = "RenderTarget4x";
std::string RenderTarget8xScaledStr   = "RenderTarget8x";
std::string FrameBuffer2xStr          = "FrameBuffer2x";
std::string FrameBuffer4xStr          = "FrameBuffer4x";
std::string FrameBuffer8xStr          = "FrameBuffer8x";
std::string DownscaleBlurPipelineStr  = "DownscaleBlurPipeline";
std::string DownscaleBlurLayoutStr    = "DownscaleBlurLayout";
std::string DownscaleBlurVertFileStr  = "DownscaleBlurPass.vert.spv";
std::string DownscaleBlurFragFileStr  = "DownscaleBlurPass.frag.spv";

std::string RenderTargetVelocityStr   = "VelocityMap";

std::string HDRGammaPipelineStr       = "HDRGammaPipeline";
std::string HDRGammaColorAttachStr    = "HDRGammaColor";
std::string HDRGammaFrameBufferStr    = "HDRGammaFrameBuffer";
std::string HDRGammaSamplerStr        = "HDRGammaSampler";
std::string HDRGammaDescSetStr        = "HDRGammaSet";
std::string HDRGammaDescSetLayoutStr  = "HDRGammaSetLayout";
std::string HDRGammaVertFileStr       = "HDRGammaPass.vert.spv";
std::string HDRGammaFragFileStr       = "HDRGammaPass.frag.spv";

std::string FinalPipelineStr          = "FinalPipeline";
std::string FinalDescSetStr           = "FinalSet";
std::string FinalDescSetLayoutStr     = "FinalSetLayout";
std::string FinalVertFileStr          = "FinalPass.vert.spv";
std::string FinalFragFileStr          = "FinalPass.frag.spv";
} // Recluse