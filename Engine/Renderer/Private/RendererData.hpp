// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "RHI/VulkanRHI.hpp"


namespace Recluse {

extern std::string ShadersPath;

extern std::string DefaultTextureStr;
extern std::string DefaultSamplerStr;

extern std::string ShadowMapPipelineStr;
extern std::string ShadowMapFrameBufferStr;

extern std::string PBRPipelineStr;
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
extern std::string PBRFragFileStr;

extern std::string ScaledSamplerStr;
extern std::string RenderTarget2xScaledStr;
extern std::string RenderTarget4xScaledStr;
extern std::string RenderTarget8xScaledStr;

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
} // Recluse