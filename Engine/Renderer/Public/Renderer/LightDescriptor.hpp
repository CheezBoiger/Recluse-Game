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
  R32               _Intensity;
  I32               _Enable;
  I32               _Pad[2];

  DirectionalLight()
    : _Enable(false) { }
};


struct PointLight {
  Vector4           _Position;
  Vector4           _Color;
  R32               _Range;
  R32               _Intensity;
  I32               _Enable;
  I32               _shadowIndex;

  PointLight()
    : _Enable(false), _Range(1.0f) { }
};


struct SpotLight {
  Vector4           _Position;
  Vector4           _Direction;
  Vector4           _Color;
  R32               _Range;
  R32               _OuterCutOff;
  R32               _InnerCutOff;
  I32               _Enable;
  R32               _goboIndex;
  R32               _shadowIndex;
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
#define MAX_POINT_LIGHTS        32
#define MAX_PL_SHADOW_MAPS      2


struct LightBuffer {
  static U32        maxNumDirectionalLights();
  static U32        maxNumPointLights();
  static U32        maxNumSpotLights();
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
  static const U32          kMaxShadowDim;
  static const U32          kTotalCascades;
public:
                            ShadowMapSystem();
                            ~ShadowMapSystem();

  void                      initialize(Renderer* pRenderer, const GraphicsConfigParams* pParams);
  
  void                      cleanUp(VulkanRHI* pRhi);

  Texture*                  getStaticMap() { return m_pStaticMap; }
  //Texture*                  DynamicMap() { return m_pDynamicMap; }

  LightViewSpace&           getViewSpace() { return m_viewSpace; }
  LightViewSpace&           getStaticViewSpace() { return m_staticViewSpace; }

  // Update based on a particular light source in the buffer. -1 defaults to primary light source.
  void                      update(VulkanRHI* pRhi, GlobalBuffer* gBuffer, LightBuffer* buffer, I32 idx = -1, U32 resourceIndex = 0);

  void                      setViewportDim(R32 dim) { m_rShadowViewportDim = dim; }
  void                      adjustSoftShadowLightSz(R32 sz) { m_rShadowLightSz = sz; }

  void                      generateDynamicShadowCmds(CommandBuffer* cmdBuffer, CmdList<PrimitiveRenderCmd>& dynamicCmds, U32 resourceIndex);
  void                      generateStaticShadowCmds(CommandBuffer* cmdBuffer, CmdList<PrimitiveRenderCmd>& staticCmds, U32 resourceIndex);
  void                      transitionEmptyShadowMap(CommandBuffer* cmdBuffer, U32 resourceIndex);
  void                      signalStaticMapUpdate() { m_staticMapNeedsUpdate = true; }

  DescriptorSet*            getShadowMapViewDescriptor(U32 resourceIndex) { return m_pLightViewDescriptorSets[resourceIndex]; }
  DescriptorSet*            getStaticShadowMapViewDescriptor(U32 resourceIndex) { return m_pStaticLightViewDescriptorSets[resourceIndex]; }

  Sampler*                  _pSampler;
  B32                       staticMapNeedsUpdate() const { return m_staticMapNeedsUpdate; }

  void                      setStaticViewerPosition(const Vector3& pos) { m_staticViewerPos = pos; }
  void                      setStaticShadowMapDim(R32 dim) { m_staticShadowViewportDim = dim; }

  void                      enableStaticMapSoftShadows(B32 enable);
  void                      enableDynamicMapSoftShadows(B32 enable);

  R32                       getStaticShadowMapDim() const { return m_staticShadowViewportDim; }

  Texture*                  getSpotLightShadowMapArray() { return m_pSpotLightMapArray; }
  Texture*                  getOmniLightShadowMapArray() { return m_pOmniMapArray; }

  LightViewSpace&           getSpotLightSpace(U32 idx) { return m_spotLightShadowMaps[idx]._space; }

  static void               initializeShadowPipelines(VulkanRHI* pRhi, U32 numCascades);
  static void               cleanUpShadowPipelines(VulkanRHI* pRhi);

private:

  void initializeShadowMapD(Renderer* pRenderer, U32 resolution = 1u);
  void initializeShadowMapDescriptors(Renderer* pRenderer);
  void initializeSpotLightShadowMapArray(VulkanRHI* pRhi, 
                                         U32 layers = 4,
                                         U32 resolution = 512u);
  void initializeCascadeShadowMap(Renderer* pRenderer, U32 resolution = 1u);

  void                      cleanUpSpotLightShadowMapArray(VulkanRHI* pRhi);
  void                      cleanUpShadowMapCascades(VulkanRHI* pRhi);

  Vector3                   m_staticViewerPos;
  R32                       m_staticShadowViewportDim;

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
  R32                           m_rShadowViewportDim;
  B32                           m_staticMapNeedsUpdate;
  R32 m_rShadowLightSz;
  R32 m_rSoftShadowNear;
  U32                           m_numPointLights;
  U32                           m_numCascadeShadowMaps;
  GraphicsQuality               m_shadowQuality;
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
  void update(Renderer* pRenderer, GlobalBuffer* gBuffer, U32 resourceIndex);

  // Initialize. 
  void initialize(Renderer* pRenderer, const GraphicsConfigParams* params);
  void checkBuffering(Renderer* pRenderer, const GraphicsConfigParams* params);

  // Cleanup.
  void cleanUp(VulkanRHI* pRhi);

  LightBuffer* getData() { return &m_Lights; }

  DescriptorSet* getDescriptorSet(U32 frameIdx) { return m_pLightDescriptorSets[frameIdx]; }
#if 0
  DescriptorSet*      ViewSet() { return m_pLightViewDescriptorSet; }

  Texture*            PrimaryShadowMap() { return m_pOpaqueShadowMap; }
#endif
  Sampler* getShadowSampler() { return m_pShadowSampler; }
  void setViewerPosition(Vector3 viewer) { m_vViewerPos = viewer; }
  Vector3 getViewerPos() const { return m_vViewerPos; }

  void setShadowViewport(R32 width, R32 height) { m_rShadowViewportWidth = width; m_rShadowViewportHeight = height; }
  void enablePrimaryShadow(B8 enable) { m_PrimaryShadowEnable = enable; }

  B32 isPrimaryShadowEnabled() const { return m_PrimaryShadowEnable; }

  ShadowMapSystem& getPrimaryShadowMapSystem() { return m_primaryMapSystem; }

private:
  void initializeNativeLights(Renderer* pRenderer);

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
  R32 m_rShadowViewportWidth;
  R32 m_rShadowViewportHeight;
  B32 m_PrimaryShadowEnable;

  friend class Renderer;
};
} // Recluse