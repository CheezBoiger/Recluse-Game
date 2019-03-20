// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Matrix3.hpp"
#include "Core/Math/Vector4.hpp"

#include "Renderer/UserParams.hpp"

#include "CmdList.hpp"
#include "RenderCmd.hpp"

namespace Recluse {

class VulkanRHI;
class DescriptorSet;
class Buffer;
class Texture;
class Sampler;
class Texture2D;
class CommandBuffer;
class TextureSampler;
class Texture3D;
class GraphicsPipeline;
class Texture1D;
class Texture2DArray;
class TextureCube;
class ImageView;
class FrameBuffer;
class RenderPass;

struct GlobalBuffer;


struct DirectionalLight {
  Vector4           _Direction;
  Vector4           _Ambient;        // Ambient override. Not needed if Object is influenced by a light probe.
  Vector4           _Color;
  r32               _Intensity;
  i32               _Enable;
  i32               _Pad[2];

  DirectionalLight()
    : _Enable(false) { }
};


struct PointLight {
  Vector4           _Position;
  Vector4           _Color;
  r32               _Range;
  r32               _Intensity;
  i32               _Enable;
  i32               _shadowIndex;

  PointLight()
    : _Enable(false), _Range(1.0f) { }
};


struct SpotLight {
  Vector4           _Position;
  Vector4           _Direction;
  Vector4           _Color;
  r32               _Range;
  r32               _OuterCutOff;
  r32               _InnerCutOff;
  i32               _Enable;
  r32               _goboIndex;
  r32               _shadowIndex;
  Vector2           _pad;
};


struct LightViewSpace {
  Matrix4           _ViewProj;
  Vector4           _near;
  Vector4           _lightSz;   // worldlightsz / frustum width.
  Vector4           _shadowTechnique; // 0 for pcf, 1 for pcss.
};


struct LightViewCascadeSpace {
  Matrix4         _ViewProj[4]; // must correspond to kTotalCascades in ShadowMapSystem!
  Vector4         _split;
  Vector4         _near;
  Vector4         _lightSz;
  Vector4         _shadowTechnique;
};


struct LightGridBuffer {
  
};


#define MAX_DIRECTIONAL_LIGHTS  4
#define MAX_SPOT_LIGHTS         16
#define MAX_POINT_LIGHTS        64


struct LightBuffer {
  static u32        maxNumDirectionalLights();
  static u32        maxNumPointLights();
  static u32        maxNumSpotLights();
  // NOTE(): Do we want more directional lights? This can be done if needed.
  DirectionalLight  _PrimaryLight;
  DirectionalLight  _DirectionalLights [MAX_DIRECTIONAL_LIGHTS];
  PointLight        _PointLights       [MAX_POINT_LIGHTS];
  SpotLight         _SpotLights        [MAX_SPOT_LIGHTS];
};



class ShadowEngine {
public:
  
};


// Shadow System, that takes care of shadow rendering. Reponsible for one particular light source,
// depending on whether it is a Directional, or Point, light source.
class ShadowMapSystem {
  // Maximum shadow map pixel dimension.
  static const u32          kMaxShadowDim;
  static const u32          kTotalCascades;
public:
                            ShadowMapSystem();
                            ~ShadowMapSystem();

  void                      initialize(VulkanRHI* pRhi, GraphicsQuality dynamicShadowDetail, GraphicsQuality staticShadowDetail, 
                              b32 staticSoftShadows = true, b32 dynamicSoftShadows = true);
  
  void                      cleanUp(VulkanRHI* pRhi);

  Texture*                  getStaticMap() { return m_pStaticMap; }
  //Texture*                  DynamicMap() { return m_pDynamicMap; }

  LightViewSpace&           getViewSpace() { return m_viewSpace; }
  LightViewSpace&           getStaticViewSpace() { return m_staticViewSpace; }

  // Update based on a particular light source in the buffer. -1 defaults to primary light source.
  void                      update(VulkanRHI* pRhi, GlobalBuffer* gBuffer, LightBuffer* buffer, i32 idx = -1, u32 frameIndex = 0);

  void                      setViewportDim(r32 dim) { m_rShadowViewportDim = dim; }

  void                      generateDynamicShadowCmds(CommandBuffer* cmdBuffer, CmdList<PrimitiveRenderCmd>& dynamicCmds, u32 frameIndex);
  void                      generateStaticShadowCmds(CommandBuffer* cmdBuffer, CmdList<PrimitiveRenderCmd>& staticCmds, u32 frameIndex);

  void                      signalStaticMapUpdate() { m_staticMapNeedsUpdate = true; }

  DescriptorSet*            getShadowMapViewDescriptor(u32 frameIndex) { return m_pLightViewDescriptorSets[frameIndex]; }
  DescriptorSet*            getStaticShadowMapViewDescriptor(u32 frameIndex) { return m_pStaticLightViewDescriptorSets[frameIndex]; }

  Sampler*                  _pSampler;
  b32                       staticMapNeedsUpdate() const { return m_staticMapNeedsUpdate; }

  void                      setStaticViewerPosition(const Vector3& pos) { m_staticViewerPos = pos; }
  void                      setStaticShadowMapDim(r32 dim) { m_staticShadowViewportDim = dim; }

  void                      enableStaticMapSoftShadows(b32 enable);
  void                      enableDynamicMapSoftShadows(b32 enable);

  r32                       getStaticShadowMapDim() const { return m_staticShadowViewportDim; }

  Texture*                  getSpotLightShadowMapArray() { return m_pSpotLightMapArray; }
  Texture*                  getOmniLightShadowMapArray() { return m_pOmniMapArray; }

  LightViewSpace&           getSpotLightSpace(u32 idx) { return m_spotLightShadowMaps[idx]._space; }

  static void               initializeShadowPipelines(VulkanRHI* pRhi);
  static void               cleanUpShadowPipelines(VulkanRHI* pRhi);

private:

  void                      initializeShadowMapD(VulkanRHI* pRhi, GraphicsQuality dynamicShadowDetail, GraphicsQuality staticShadowDetail);
  void                      initializeShadowMapDescriptors(VulkanRHI* pRhi);
  void                      initializeSpotLightShadowMapArray(VulkanRHI* pRhi, u32 layers = 4, u32 resolution = 512u);
  void                      initializeCascadeShadowMap(VulkanRHI* pRhi, GraphicsQuality dynamicShadowDetail);

  void                      cleanUpSpotLightShadowMapArray(VulkanRHI* pRhi);
  void                      cleanUpShadowMapCascades(VulkanRHI* pRhi);

  Vector3                   m_staticViewerPos;
  r32                       m_staticShadowViewportDim;

  // Maps that contain shadow information on the directional, sunlight/moonlight object.
  // These are 2D arrays containing cascaded shadow maps.
  Texture*                  m_pStaticMap;       
  //Texture*                  m_pDynamicMap;      
  Texture*                  m_pShadowMergeMap;  // Merging map of the static and dynamic shadow maps.
  // common used pipelines.
  // TODO(): We don't need to create multiple instances of these pipelines, we must place them in 
  // a static object.
  static std::vector<GraphicsPipeline*>  k_pSkinnedPipeline;
  static std::vector<GraphicsPipeline*>  k_pSkinnedMorphPipeline;
  static std::vector<GraphicsPipeline*>  k_pStaticMorphPipeline;
  static std::vector<GraphicsPipeline*>  k_pStaticPipeline;

  static std::vector<GraphicsPipeline*>  k_pSkinnedPipelineOpaque;
  static std::vector<GraphicsPipeline*>  k_pSkinnedMorphPipelineOpaque;
  static std::vector<GraphicsPipeline*>  k_pStaticMorphPipelineOpaque;
  static std::vector<GraphicsPipeline*>  k_pStaticPipelineOpaque;

  static GraphicsPipeline*  k_pStaticSkinnedPipeline;
  static GraphicsPipeline*  k_pStaticSkinnedMorphPipeline;
  static GraphicsPipeline*  k_pStaticStaticPipeline;
  static GraphicsPipeline*  k_pStaticStaticMorphPipeline;

  static GraphicsPipeline*  k_pStaticSkinnedPipelineOpaque;
  static GraphicsPipeline*  k_pStaticSkinnedMorphPipelineOpaque;
  static GraphicsPipeline*  k_pStaticStaticPipelineOpaque;
  static GraphicsPipeline*  k_pStaticStaticMorphPipelineOpaque;
  
  
  static RenderPass*        k_pDynamicRenderPass;
  static RenderPass*        k_pStaticRenderPass;
  static RenderPass*        k_pCascadeRenderPass;

  // Items that are dependent on each instance of a Shadow Mapping system.
  FrameBuffer*              m_pStaticFrameBuffer;
  //FrameBuffer*              m_pDynamicFrameBuffer;

  struct Cascade {
    ImageView*              _view;
  };


  struct ShadowMapLayer {
    ImageView*              _view;
    FrameBuffer*            _dynamicFB;
    FrameBuffer*            _staticFB;
    LightViewSpace          _space;
    DescriptorSet*          _staticDS;
    DescriptorSet*          _dynamicDS;
  };
  
  // Point map based framebuffer.
  std::vector<FrameBuffer*>     m_pCascadeFrameBuffers;
  FrameBuffer*                  m_pStaticOmniFrameBuffer;
  FrameBuffer*                  m_pDynamicOmniFrameBuffer;
  std::vector<Buffer*>          m_pLightViewBuffers;
  std::vector<Buffer*>          m_pStaticLightViewBuffers;
  std::vector<Buffer*>          m_pCascadeLightViewBuffers;
  std::vector<DescriptorSet*>   m_pLightViewDescriptorSets;
  std::vector<DescriptorSet*>   m_pStaticLightViewDescriptorSets;
  std::vector<DescriptorSet*>   m_pCascadeDescriptorSets;
  Texture*                      m_pOmniMapArray;
  Texture*                      m_pStaticOmniMapArray;
  Texture*                      m_pSpotLightMapArray;
  Texture*                      m_pStaticSpotLightMapArray;
  LightViewSpace                m_viewSpace;
  LightViewSpace                m_staticViewSpace;
  LightViewCascadeSpace         m_cascadeViewSpace;
  r32                           m_rShadowViewportDim;
  b32                           m_staticMapNeedsUpdate;
  u32                           m_numPointLights;
  std::vector<Texture*>         m_pCascadeShadowMapD;
  std::vector<Vector3>          m_pointMapPositions;
  std::vector<std::vector<Cascade>>          m_cascades;
  std::vector<ShadowMapLayer>   m_spotLightShadowMaps;
};


// Light material.
// TODO(): Shadow mapping might require us to create a separate buffer that holds
// shadow maps for each point light, however we should limit the number of shadow maps
// for pointlights to remain consistent in performance.
class LightDescriptor {
public:

  LightDescriptor();
  ~LightDescriptor();

  // Update the light information on the gpu, for use in our shaders.
  void update(VulkanRHI* pRhi, GlobalBuffer* gBuffer, u32 frameIndex);

  // Initialize. 
  void initialize(VulkanRHI* pRhi, GraphicsQuality shadowDetail, b32 enableSoftShadows = true);

  // Cleanup.
  void cleanUp(VulkanRHI* pRhi);

  LightBuffer* getData() { return &m_Lights; }

  DescriptorSet* getDescriptorSet(u32 frameIdx) { return m_pLightDescriptorSets[frameIdx]; }
#if 0
  DescriptorSet*      ViewSet() { return m_pLightViewDescriptorSet; }

  Texture*            PrimaryShadowMap() { return m_pOpaqueShadowMap; }
#endif
  Sampler* getShadowSampler() { return m_pShadowSampler; }
  void setViewerPosition(Vector3 viewer) { m_vViewerPos = viewer; }
  Vector3 getViewerPos() const { return m_vViewerPos; }

  void setShadowViewport(r32 width, r32 height) { m_rShadowViewportWidth = width; m_rShadowViewportHeight = height; }
  void enablePrimaryShadow(b8 enable) { m_PrimaryShadowEnable = enable; }

  b32 isPrimaryShadowEnabled() const { return m_PrimaryShadowEnable; }

  ShadowMapSystem& getPrimaryShadowMapSystem() { return m_primaryMapSystem; }

private:
  void initializeNativeLights(VulkanRHI* pRhi);

#if 0
  void                InitializePrimaryShadow(VulkanRHI* pRhi);
#endif
  Vector3 m_vViewerPos;
  // Descriptor Set.
  std::vector<DescriptorSet*> m_pLightDescriptorSets;
  // Light list is this!
  std::vector<Buffer*> m_pLightBuffers; 

  ShadowMapSystem m_primaryMapSystem;

#if 0
  DescriptorSet*      m_pLightViewDescriptorSet;
w
  // After computing the clusters to shade our lights, we store them in here!
  Buffer*             m_pLightGrid;

  // Light view buffer.
  Buffer*             m_pLightViewBuffer;

  // Shadow map, this is mainly for our directional light, the primary light.
  Texture*            m_pOpaqueShadowMap;

  // Shadow map for transparent objects.
  Texture*            m_pTransparentShadowMap;

  // Color filter map for transparent objects.
  Texture*            m_pTransparentColorFilter;

  // Framebuffer object used for command buffer pass through. Shadow mapping can't be
  // statically stored into the renderer pipeline. Only possibility is to create this 
  // outside.
  FrameBuffer*        m_pFrameBuffer;

  // RenderPass instance for the framebuffer.
  RenderPass*         m_pRenderPass;

  // Information of our lights, to which we use this to modify light sources.
  LightViewSpace      m_PrimaryLightSpace;
#endif
  // Shadow map sampler.
  Sampler* m_pShadowSampler;
  LightBuffer m_Lights;
  r32 m_rShadowViewportWidth;
  r32 m_rShadowViewportHeight;
  b32 m_PrimaryShadowEnable;

  friend class Renderer;
};
} // Recluse