// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Math/Matrix4.hpp"
#include "RHI/VulkanRHI.hpp"
#include "UserParams.hpp"

#include <array>

#define GBUFFER_ALBEDO_FORMAT             VK_FORMAT_R8G8B8A8_UNORM
#define GBUFFER_NORMAL_FORMAT             VK_FORMAT_R8G8B8A8_UNORM
#define GBUFFER_ROUGH_METAL_FORMAT        VK_FORMAT_R8G8B8A8_UNORM 
#define GBUFFER_ADDITIONAL_INFO_FORMAT    VK_FORMAT_R8G8B8A8_UNORM

#define SHADOW_CULL_MODE                  VK_CULL_MODE_BACK_BIT
#define SHADOW_WINDING_ORDER              VK_FRONT_FACE_CLOCKWISE

#define GBUFFER_WINDING_ORDER             VK_FRONT_FACE_CLOCKWISE
#define GBUFFER_CULL_MODE                 VK_CULL_MODE_FRONT_BIT

#define COMPUTE_PBR                       0


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
class GlobalDescriptor;
class Buffer;
class HDR;

struct SimpleRenderCmd;

// Get our view matrices.
// Index                Face
// 0                    POSITIVE_X
// 1                    NEGATIVE_X
// 2                    POSITIVE_Y
// 3                    NEGATIVE_Y
// 4                    POSITIVE_Z
// 5                    NEGATIVE_Z
extern std::array<Matrix4, 6> kViewMatrices;

extern std::string ShadersPath;

extern Texture* DefaultTextureKey;
extern VkImageView DefaultTexture2DArrayView;

// enableDebug pipeline, for debugging purposes.
// This allows drawing lines, shapes, or anything that 
// may be needed to debug the renderer.
extern GraphicsPipeline*  debug_linePipeline;
extern GraphicsPipeline* debug_wireframePipelineStatic;
extern GraphicsPipeline* debug_wireframePipelineAnim;
extern RenderPass*        debug_renderPass;

extern GraphicsPipeline* ShadowMapPipelineKey;
extern GraphicsPipeline* shadowMap_staticMorphTargetsPipeline;
extern GraphicsPipeline* DynamicShadowMapPipelineKey;
extern GraphicsPipeline*  shadowMap_dynamicMorphTargetPipeline;

extern std::string ShadowMapVertFileStr;
extern std::string ShadowMapFragFileStr;
extern std::string ShadowMapFragOpaqueFileStr;
extern std::string DynamicShadowMapVertFileStr;

extern DescriptorSetLayout* LightViewDescriptorSetLayoutKey;
extern DescriptorSetLayout* globalIllumination_DescLR;
extern DescriptorSetLayout* globalIllumination_DescNoLR;

// Shadow mapping pipes for transparent/translucent objects.
extern GraphicsPipeline*    transparent_staticShadowPipe;
extern GraphicsPipeline*    transparent_dynamicShadowPipe;
extern GraphicsPipeline*    transparent_colorFilterPipe;

extern DescriptorSetLayout* gbuffer_LayoutKey;

extern Texture* gbuffer_AlbedoAttachKey;
extern Texture* gbuffer_NormalAttachKey;
extern Texture* gbuffer_PositionAttachKey;
extern Texture* gbuffer_EmissionAttachKey;
extern Sampler* gbuffer_SamplerKey;
extern FrameBuffer* gbuffer_FrameBufferKey;
extern RenderPass* gbuffer_renderPass;

extern GraphicsPipeline* pbr_static_LR_Debug;
extern GraphicsPipeline* pbr_static_NoLR_Debug;
extern GraphicsPipeline* pbr_dynamic_LR_Debug;
extern GraphicsPipeline* pbr_dynamic_NoLR_Debug;
extern GraphicsPipeline* pbr_static_mt_LR_Debug;
extern GraphicsPipeline* pbr_static_mt_NoLR_Debug;
extern GraphicsPipeline* pbr_dynamic_LR_mt_Debug;
extern GraphicsPipeline* pbr_dynamic_NoLR_mt_Debug;
extern FrameBuffer* pbr_FrameBufferKey;
extern RenderPass* pbr_renderPass;
extern DescriptorSetLayout* pbr_DescLayoutKey;
extern DescriptorSetLayout* pbr_compDescLayout;
extern DescriptorSet* pbr_DescSetKey;
extern DescriptorSet* pbr_compSet;

extern RenderPass*        pbr_forwardRenderPass;
extern FrameBuffer*       pbr_forwardFrameBuffer;

extern ComputePipeline* aa_PipelineKey;
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

extern FrameBuffer* FrameBufferGlowKey;
extern DescriptorSetLayout* GlowDescriptorSetLayoutKey;
extern DescriptorSet* GlowDescriptorSetKey;

extern Texture* RenderTargetVelocityKey;

extern Texture* hdr_gamma_colorAttachKey;
extern FrameBuffer* hdr_gamma_frameBufferKey;
extern RenderPass* hdr_renderPass;
extern Sampler* hdr_gamma_samplerKey;
extern DescriptorSet* hdr_gamma_descSetKey;
extern DescriptorSetLayout* hdr_gamma_descSetLayoutKey;

extern DescriptorSet* final_DescSetKey;
extern DescriptorSetLayout* final_DescSetLayoutKey;
extern FrameBuffer* final_frameBufferKey;
extern RenderPass* final_renderPass;

extern DescriptorSet* output_descSetKey;

enum PipelineGraphicsT {
  PIPELINE_GRAPHICS_START = 0,
  PIPELINE_GRAPHICS_PREZ_DYNAMIC = PIPELINE_GRAPHICS_START,
  PIPELINE_GRAPHICS_PREZ_DYNAMIC_MORPH_TARGETS,
  PIPELINE_GRAPHICS_PREZ_STATIC,
  PIPELINE_GRAPHICS_PREZ_STATIC_MORPH_TARGETS,
  PIPELINE_GRAPHICS_FINAL,
  PIPELINE_GRAPHICS_CLUSTER,
  PIPELINE_GRAPHICS_GBUFFER_STATIC,
  PIPELINE_GRAPHICS_GBUFFER_STATIC_MORPH_TARGETS,
  PIPELINE_GRAPHICS_GBUFFER_DYNAMIC,
  PIPELINE_GRAPHICS_GBUFFER_DYNAMIC_MORPH_TARGETS,
  PIPELINE_GRAPHICS_PBR_FORWARD_LR,
  PIPELINE_GRAPHICS_PBR_FORWARD_NOLR,
  PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_LR,
  PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_NOLR,
  PIPELINE_GRAPHICS_PBR_FORWARD_MORPH_LR,
  PIPELINE_GRAPHICS_PBR_FORWARD_MORPH_NOLR,
  PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_MORPH_LR,
  PIPELINE_GRAPHICS_PBR_FORWARD_STATIC_MORPH_NOLR,
  PIPELINE_GRAPHICS_PBR_DEFERRED_LR,
  PIPELINE_GRAPHICS_PBR_DEFERRED_NOLR,
  PIPELINE_GRAPHICS_PBR_FORWARD,
  PIPELINE_GRAPHICS_HDR_GAMMA,
  PIPELINE_GRAPHICS_GLOW,
  PIPELINE_GRAPHICS_DOWNSCALE_BLUR_2X,
  PIPELINE_GRAPHICS_DOWNSCALE_BLUR_4X,
  PIPELINE_GRAPHICS_DOWNSCALE_BLUR_8X,
  PIPELINE_GRAPHICS_DOWNSCALE_BLUR_16X,
  PIPELINE_GRAPHICS_OUTPUT,
  PIPELINE_GRAPHICS_END,
};


enum PipelineComputeT {
  PIPELINE_COMPUTE_START = 0,
  PIPELINE_COMPUTE_PBR_DEFERRED_NOLR = PIPELINE_COMPUTE_START,
  PIPELINE_COMPUTE_PBR_DEFERRED_LR,
  PIPELINE_COMPUTE_SHADOW_RESOLVE,
  PIPELINE_COMPUTE_BLOOM_ACCUMULATION,
  PIPELINE_COMPUTE_DOWNSCALE_BRIGHT_FILTER,
  PIPELINE_COMPUTE_CLUSTER_ASSIGNMENT,
  PIPELINE_COMPUTE_LIGHT_ASSIGNMENT,
  PIPELINE_COMPUTE_END
};


enum FrameBufferT {
  FRAME_BUFFER_START = 0,
  FRAME_BUFFER_PREZ = FRAME_BUFFER_START,
  FRAME_BUFFER_END
};


enum RenderPassT {
  RENDER_PASS_START = 0,
  RENDER_PASS_PREZ = RENDER_PASS_START,
  RENDER_PASS_END
};


enum SamplerT {
  SAMPLER_START = 0,
  SAMPLER_DEFAULT = SAMPLER_START,
  SAMPLER_END
};


enum DescriptorSetLayoutT {
  DESCRIPTOR_SET_LAYOUT_START = 0,
  DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE = DESCRIPTOR_SET_LAYOUT_START,
  DESCRIPTOR_SET_LAYOUT_SHADOW_RESOLVE_OUT,
  DESCRIPTOR_SET_LAYOUT_MESH_DESCRIPTOR,
  DESCRIPTOR_SET_LAYOUT_MATERIAL_DESCRIPTOR,
  DESCRIPTOR_SET_LAYOUT_JOINT_DESCRIPTOR,
  DESCRIPTOR_SET_LAYOUT_GLOBAL_DESCRIPTOR,  
  DESCRIPTOR_SET_LAYOUT_BLOOM_ACCUMULATION,
  DESCRIPTOR_SET_LAYOUT_DOWNSCALE_BRIGHT_FILTER,
  DESCRIPTOR_SET_LAYOUT_END
};


enum DescriptorSetT {
  DESCRIPTOR_SET_START = 0,
  DESCRIPTOR_SET_SHADOW_RESOLVE = DESCRIPTOR_SET_START,
  DESCRIPTOR_SET_SHADOW_RESOLVE_OUT,
  DESCRIPTOR_SET_BLOOM_ACCUMULATION_16X_8X,
  DESCRIPTOR_SET_BLOOM_ACCUMULATION_8X_4X,
  DESCRIPTOR_SET_BLOOM_ACCUMULATION_4X_2X,
  DESCRIPTOR_SET_BLOOM_ACCUMULATION_2X_FULL,
  DESCRIPTOR_SET_DOWNSCALE_BRIGHT_FILTER_FULL_2X,
  DESCRIPTOR_SET_DOWNSCALE_BRIGHT_FILTER_2X_4X,
  DESCRIPTOR_SET_DOWNSCALE_BRIGHT_FILTER_4X_8X,
  DESCRIPTOR_SET_DOWNSCALE_BRIGHT_FILTER_8X_16X,
  DESCRIPTOR_SET_END
};

enum RenderTextureT {
  RENDER_TEXTURE_START = 0,
  RENDER_TEXTURE_SHADOW_RESOLVE_OUTPUT = RENDER_TEXTURE_START,
  RENDER_TEXTURE_SCENE_DEPTH,
  //RENDER_TEXTURE_DOWNSCALE_2X_START,
  RENDER_TEXTURE_DOWNSCALE_2X_SCALED,
  RENDER_TEXTURE_DOWNSCALE_2X_FINAL,
  //RENDER_TEXTURE_DOWNSCALE_4X_START,
  RENDER_TEXTURE_DOWNSCALE_4X_SCALED,
  RENDER_TEXTURE_DOWNSCALE_4X_FINAL,
  //RENDER_TEXTURE_DOWNSCALE_8X_START,
  RENDER_TEXTURE_DOWNSCALE_8X_SCALED,
  RENDER_TEXTURE_DOWNSCALE_8X_FINAL,
  RENDER_TEXTURE_DOWNSCALE_16X_START,
  RENDER_TEXTURE_DOWNSCALE_16X_SCALED,
  RENDER_TEXTURE_DOWNSCALE_16X_FINAL,
  RENDER_TEXTURE_GLOW,
  RENDER_TEXTURE_FINAL_COMPOSITE,
  RENDER_TEXTURE_BRIGHTNESS,
  RENDER_TEXTURE_LIGHTING,
  RENDER_TEXTURE_END
};

extern char const* kDefaultShaderEntryPointStr;

void SetUpRenderData();
void CleanUpRenderData();


namespace RendererPass {


void initializePipelines(VulkanRHI* pRhi);
void cleanUpPipelines(VulkanRHI* pRhi);

void initializeDescriptorSetLayouts(VulkanRHI* pRhi);
void cleanUpDescriptorSetLayouts(VulkanRHI* pRhi);

void initializeRenderTextures(Renderer* pRenderer);
void initializeSamplers(VulkanRHI* pRhi);
void cleanUpRenderTextures(VulkanRHI* pRhi);
void cleanUpSamplers(VulkanRHI* pRhi);

void initializeDescriptorSets(Renderer* pRenderer);
void cleanUpDescriptorSets(VulkanRHI* pRhi);

void initializeFrameBuffers(Renderer* pRenderer);
void cleanUpFrameBuffers(VulkanRHI* pRhi);

void initializeRenderPasses(Renderer* pRenderer);
void cleanUpRenderPasses(VulkanRHI* pRhi);

GraphicsPipeline* getGraphicsPipeline(PipelineGraphicsT pipeline);
ComputePipeline* getComputePipeline(PipelineComputeT pipeline);

Texture* getRenderTexture(RenderTextureT rt, U32 resourceIndex);
Sampler* getSampler(SamplerT samp);
FrameBuffer* getFrameBuffer(FrameBufferT fb);
RenderPass* getRenderPass(RenderPassT rp);
DescriptorSetLayout* getDescriptorSetLayout(DescriptorSetLayoutT layout);
DescriptorSet* getDescriptorSet(DescriptorSetT set, U32 resourceIndex = 0);

void loadShader(const std::string& Filename, Shader* S);

// Set up the downscale pass.
void setUpDownScalePass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);
// Set up the HDR getGamma pass.
void SetUpHDRGammaPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo, HDR* pHDR);
void SetUpGBufferPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);
void SetUpDeferredPhysicallyBasedPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);
void SetUpForwardPhysicallyBasedPass(VulkanRHI* rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);
void SetUpFinalPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);
void SetUpDirectionalShadowPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);
void SetUpSkyboxPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo);
void setUpDebugPass(VulkanRHI* rhi, const VkGraphicsPipelineCreateInfo& defaultInfo);
void setUpAAPass(VulkanRHI* Rhi, const VkGraphicsPipelineCreateInfo& DefaultInfo, AntiAliasing aa);
void initShadowMaskTexture(Renderer* pRenderer, const VkExtent2D& renderRes);
void initShadowResolvePipeline(VulkanRHI* pRhi);
void initShadowResolveDescriptorSetLayout(VulkanRHI* pRhi);
void initShadowReolveDescriptorSet(Renderer* pRenderer, GlobalDescriptor* pGlobal, Texture* pSceneDepth);
void initPreZRenderPass(VulkanRHI* pRhi);
void initPreZPipelines(VulkanRHI* pRhi, const VkGraphicsPipelineCreateInfo& info, VkExtent2D targetExtent);
void initBloomAccumulationDescriptorSetLayouts(VulkanRHI* pRhi);
void initBloomAccumulationDescriptorSets(VulkanRHI* pRhi);
void initBloomAccumulationPipeline(VulkanRHI* pRhi);
void initDownscaleBrightnessPipeline(VulkanRHI* pRhi);
void initDownscaleBrightnessDescriptorSets(VulkanRHI* pRhi);
void initDownscaleBrightnessDescriptorSetLayouts(VulkanRHI* pRhi);
void initClusterComputePipeline(VulkanRHI* pRhi, U32 cellSzX, U32 cellSzY, U32 cellSzZ, U32 cellSz);

enum AntiAliasingType {
  FXAA,
  SMAA
};
} // RendererPass


class AntiAliasingSMAA {
public:

  GraphicsPipeline* m;
  Texture* _edgesTex;
  Texture* _blendTex;
  Sampler* _sampler;

};


class AntiAliasingFXAA {
public:
  AntiAliasingFXAA()
    : m_output(nullptr)
    , m_outputSampler(nullptr)
    , m_pipeline(nullptr)
    , m_descSets(0)
    , m_layout(nullptr)
    , m_groupSz(16) { }


  void    initialize(Renderer* pRenderer, GlobalDescriptor* pWorld);
  void    cleanUp(VulkanRHI* pRhi);

  void    generateCommands(VulkanRHI* pRhi, 
                           CommandBuffer* pOut, 
                           GlobalDescriptor* pDescriptor, 
                           U32 resourceIndex);
  void    updateSets(Renderer* pRenderer, 
                     GlobalDescriptor* pDescriptor);
  Texture*  GetOutput() { return m_output; }
  Sampler*  GetOutputSampler() { return m_outputSampler; }

private:

  void                    createTexture(VulkanRHI* pRhi, GlobalDescriptor* pGlobal);
  void                    createDescriptorSetLayout(VulkanRHI* pRhi);
  void                    createDescriptorSet(VulkanRHI* pRhi, GlobalDescriptor* pDescriptor, U32 resourceIndex);
  void                    createSampler(VulkanRHI* pRhi);


  ComputePipeline*        m_pipeline;
  Texture*                m_output;
  Sampler*                m_outputSampler;
  DescriptorSetLayout*    m_layout;
  std::vector<DescriptorSet*> m_descSets;
  U32                     m_groupSz;           
};


class DebugManager {
public:
  DebugManager()
    : m_renderPass(nullptr)
    , m_staticWireframePipeline(nullptr)
    , m_dynamicWireframePipeline(nullptr) { }

  void                initialize(VulkanRHI* pRhi);
  void                cleanUp(VulkanRHI* pRhi);


  void RecordDebugCommands(VulkanRHI* pRhi, 
                           CommandBuffer* pBuf, 
                           SimpleRenderCmd* renderCmds, 
                           U32 count);

  RenderPass*         getRenderPass() { return m_renderPass; }
  GraphicsPipeline*   GetWireFrameStaticPipeline() { }
  GraphicsPipeline*   GetWireFrameDynamicPipeline() { }

private:
  void                initializeRenderPass(VulkanRHI* pRhi);
  void                createPipelines(VulkanRHI* pRhi);

  // Simple renderer pipeline requires no material information, 
  // is not shaded with shadow, and has no ibl. For debugging purposes.
  GraphicsPipeline*  m_staticWireframePipeline;
  GraphicsPipeline*  m_dynamicWireframePipeline;
  RenderPass*        m_renderPass;
};
} // Recluse