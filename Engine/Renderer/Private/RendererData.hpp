// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "RHI/VulkanRHI.hpp"
#include "UserParams.hpp"


#define GBUFFER_ALBEDO_FORMAT             VK_FORMAT_R8G8B8A8_UNORM
#define GBUFFER_NORMAL_FORMAT             VK_FORMAT_R8G8B8A8_UNORM
#define GBUFFER_ROUGH_METAL_FORMAT        VK_FORMAT_R8G8B8A8_UNORM 
#define GBUFFER_ADDITIONAL_INFO_FORMAT    VK_FORMAT_R8G8B8A8_UNORM

#define SHADOW_CULL_MODE                  VK_CULL_MODE_BACK_BIT
#define SHADOW_WINDING_ORDER              VK_FRONT_FACE_CLOCKWISE

#define GBUFFER_WINDING_ORDER             VK_FRONT_FACE_CLOCKWISE
#define GBUFFER_CULL_MODE                 VK_CULL_MODE_FRONT_BIT


namespace Recluse {

class Renderer;
class Sampler;
class Texture;
class FrameBuffer;
class RenderPass;
class GraphicsPipeline;
class ComputePipeline;
class DescriptorSetLayout;
class DescriptorSet;
class Buffer;

extern std::string ShadersPath;

extern Texture* DefaultTextureKey;
extern Sampler* DefaultSampler2DKey;

// Debug pipeline, for debugging purposes.
// This allows drawing lines, shapes, or anything that 
// may be needed to debug the renderer.
extern GraphicsPipeline*  debug_linePipeline;
extern GraphicsPipeline*  debug_wireframePipeline;
extern RenderPass*        debug_renderPass;

extern GraphicsPipeline* ShadowMapPipelineKey;
extern GraphicsPipeline* DynamicShadowMapPipelineKey;
extern std::string ShadowMapVertFileStr;
extern std::string ShadowMapFragFileStr;
extern DescriptorSetLayout* LightViewDescriptorSetLayoutKey;
extern DescriptorSetLayout* globalIllumination_DescLR;
extern DescriptorSetLayout* globalIllumination_DescNoLR;

// Shadow mapping pipes for transparent/translucent objects.
extern GraphicsPipeline*    transparent_staticShadowPipe;
extern GraphicsPipeline*    transparent_dynamicShadowPipe;
extern GraphicsPipeline*    transparent_colorFilterPipe;

extern GraphicsPipeline* gbuffer_PipelineKey;
extern GraphicsPipeline* gbuffer_StaticPipelineKey;
extern DescriptorSetLayout* gbuffer_LayoutKey;
extern Texture* gbuffer_AlbedoAttachKey;
extern Texture* gbuffer_NormalAttachKey;
extern Texture* gbuffer_PositionAttachKey;
extern Texture* gbuffer_EmissionAttachKey;
extern Sampler* gbuffer_SamplerKey;
extern Texture* gbuffer_DepthAttachKey;
extern FrameBuffer* gbuffer_FrameBufferKey;
extern RenderPass* gbuffer_renderPass;

extern GraphicsPipeline* pbr_Pipeline_LR;
extern GraphicsPipeline* pbr_Pipeline_NoLR;
extern FrameBuffer* pbr_FrameBufferKey;
extern RenderPass* pbr_renderPass;
extern DescriptorSetLayout* pbr_DescLayoutKey;
extern DescriptorSet* pbr_DescSetKey;
extern Texture* pbr_FinalTextureKey;
extern Texture* pbr_BrightTextureKey;

extern GraphicsPipeline* pbr_forwardPipeline_LR;
extern GraphicsPipeline* pbr_forwardPipeline_NoLR;
extern GraphicsPipeline* pbr_staticForwardPipeline_LR;
extern GraphicsPipeline* pbr_staticForwardPipeline_NoLR;
extern RenderPass*      pbr_forwardRenderPass;

extern GraphicsPipeline* aa_PipelineKey;
extern FrameBuffer* aa_FrameBufferKey;
extern DescriptorSetLayout* aa_DescLayoutKey;   // Depends on the aliasing technique.
extern Texture* aa_outputTextureKey;

// Work in progress.
extern ComputePipeline* cluster_FrustumPipelineKey;
extern ComputePipeline* cluster_LightCullPipelineKey;
extern Texture* cluster_LightGridKey;
extern Texture* cluster_FrustumTextureKey;

extern Texture* hiz_FullTexture;
extern Texture* hiz_2xTex;
extern Texture* hiz_4xTex;

extern GraphicsPipeline* skybox_pipelineKey;
extern DescriptorSet* skybox_descriptorSetKey;
extern DescriptorSetLayout* skybox_setLayoutKey;

extern DescriptorSetLayout*  MeshSetLayoutKey;
extern DescriptorSetLayout* MaterialSetLayoutKey;
extern DescriptorSetLayout* BonesSetLayoutKey;
extern DescriptorSetLayout* GlobalSetLayoutKey;
extern DescriptorSetLayout* LightSetLayoutKey;

extern Sampler* ScaledSamplerKey;
extern Texture* RenderTarget2xHorizKey;
extern Texture* RenderTarget2xFinalKey;
extern Texture* RenderTarget4xScaledKey;
extern Texture* RenderTarget4xFinalKey;
extern Texture* RenderTarget8xScaledKey;
extern Texture* RenderTarget8xFinalKey;
extern Texture* RenderTarget16xScaledKey;
extern Texture* RenderTarget16xFinalKey;
extern FrameBuffer* FrameBuffer2xHorizKey;
extern FrameBuffer* FrameBuffer2xFinalKey;
extern FrameBuffer* FrameBuffer4xKey;
extern FrameBuffer* FrameBuffer4xFinalKey;
extern FrameBuffer* FrameBuffer8xKey;
extern FrameBuffer* FrameBuffer8xFinalKey;
extern FrameBuffer* FrameBuffer16xKey;
extern FrameBuffer* FrameBuffer16xFinalKey;
extern DescriptorSetLayout* DownscaleBlurLayoutKey;
extern DescriptorSet* DownscaleBlurDescriptorSet2x;
extern DescriptorSet* DownscaleBlurDescriptorSet2xFinalKey;
extern DescriptorSet* DownscaleBlurDescriptorSet4x;
extern DescriptorSet* DownscaleBlurDescriptorSet4xFinalKey;
extern DescriptorSet* DownscaleBlurDescriptorSet8x;
extern DescriptorSet* DownscaleBlurDescriptorSet8xFinalKey;
extern DescriptorSet* DownscaleBlurDescriptorSet16x;
extern DescriptorSet* DownscaleBlurDescriptorSet16xFinalKey;
extern GraphicsPipeline* GlowPipelineKey;
extern Texture* RenderTargetGlowKey;
extern FrameBuffer* FrameBufferGlowKey;
extern DescriptorSetLayout* GlowDescriptorSetLayoutKey;
extern DescriptorSet* GlowDescriptorSetKey;
extern GraphicsPipeline* DownscaleBlurPipeline2xKey;
extern GraphicsPipeline* DownscaleBlurPipeline4xKey;
extern GraphicsPipeline* DownscaleBlurPipeline8xKey;
extern GraphicsPipeline* DownscaleBlurPipeline16xKey;

extern Texture* RenderTargetVelocityKey;

extern GraphicsPipeline* hdr_gamma_pipelineKey;
extern Texture* hdr_gamma_colorAttachKey;
extern FrameBuffer* hdr_gamma_frameBufferKey;
extern RenderPass* hdr_renderPass;
extern Sampler* hdr_gamma_samplerKey;
extern DescriptorSet* hdr_gamma_descSetKey;
extern DescriptorSetLayout* hdr_gamma_descSetLayoutKey;

extern GraphicsPipeline* final_PipelineKey;
extern DescriptorSet* final_DescSetKey;
extern DescriptorSetLayout* final_DescSetLayoutKey;
extern FrameBuffer* final_frameBufferKey;
extern RenderPass* final_renderPass;
extern Texture* final_renderTargetKey;

extern DescriptorSet* output_descSetKey;
extern GraphicsPipeline* output_pipelineKey;

extern char const* kDefaultShaderEntryPointStr;

namespace RendererPass {


void LoadShader(std::string Filename, Shader* S);

// Set up the downscale pass.
void SetUpDownScalePass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

// Set up the HDR Gamma pass.
void SetUpHDRGammaPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpGBufferPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpDeferredPhysicallyBasedPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpForwardPhysicallyBasedPass(VulkanRHI* rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpFinalPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpDirectionalShadowPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpSkyboxPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);

void SetUpAAPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo, AntiAliasing aa);


class AntiAliasingPipe {
public:
  enum AliasingType {
    FXAA,
    SMAA
  };

  static DescriptorSetLayout* CreateDescriptorSetLayout(AliasingType type);
  static GraphicsPipeline*    CreateGraphicsPipeline(AliasingType type); 
};
} // RendererPass
} // Recluse