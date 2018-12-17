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
  i32               _Pad;

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
};


struct LightViewSpace {
  Matrix4           _ViewProj;
  Vector4           _near;
  Vector4           _lightSz;   // worldlightsz / frustum width.
  Vector4           _shadowTechnique; // 0 for pcf, 1 for pcss.
};


struct LightGridBuffer {
  
};


#define MAX_DIRECTIONAL_LIGHTS  4
#define MAX_SPOT_LIGHTS         16
#define MAX_POINT_LIGHTS        64


struct LightBuffer {
  static u32        MaxNumDirectionalLights();
  static u32        MaxNumPointLights();
  static u32        MaxNumSpotLights();
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
  static const u32          kMaxShadowDim = 8192u;
public:
                            ShadowMapSystem();
                            ~ShadowMapSystem();

  void                      Initialize(VulkanRHI* pRhi, GraphicsQuality dynamicShadowDetail, GraphicsQuality staticShadowDetail, 
                              b32 staticSoftShadows = true, b32 dynamicSoftShadows = true);
  
  void                      CleanUp(VulkanRHI* pRhi);

  Texture*                  StaticMap() { return m_pStaticMap; }
  Texture*                  DynamicMap() { return m_pDynamicMap; }

  LightViewSpace&           ViewSpace() { return m_viewSpace; }
  LightViewSpace&           StaticViewSpace() { return m_staticViewSpace; }

  // Update based on a particular light source in the buffer. -1 defaults to primary light source.
  void                      Update(VulkanRHI* pRhi, GlobalBuffer* gBuffer, LightBuffer* buffer, i32 idx = -1);

  void                      SetViewportDim(r32 dim) { m_rShadowViewportDim = dim; }

  void                      GenerateDynamicShadowCmds(CommandBuffer* cmdBuffer, CmdList<PrimitiveRenderCmd>& dynamicCmds);
  void                      GenerateStaticShadowCmds(CommandBuffer* cmdBuffer, CmdList<PrimitiveRenderCmd>& staticCmds);

  void                      SignalStaticMapUpdate() { m_staticMapNeedsUpdate = true; }

  DescriptorSet*            ShadowMapViewDescriptor() { return m_pLightViewDescriptorSet; }
  DescriptorSet*            StaticShadowMapViewDescriptor() { return m_pStaticLightViewDescriptorSet; }

  Sampler*                  _pSampler;
  b32                       StaticMapNeedsUpdate() const { return m_staticMapNeedsUpdate; }

  void                      SetStaticViewerPosition(const Vector3& pos) { m_staticViewerPos = pos; }
  void                      SetStaticShadowMapDim(r32 dim) { m_staticShadowViewportDim = dim; }

  void                      EnableStaticMapSoftShadows(b32 enable);
  void                      EnableDynamicMapSoftShadows(b32 enable);

  r32                       GetStaticShadowMapDim() const { return m_staticShadowViewportDim; }

  Texture*                  GetSpotLightShadowMapArray() { return m_pSpotLightMapArray; }
  Texture*                  GetOmniLightShadowMapArray() { return m_pOmniMapArray; }

  LightViewSpace&           GetSpotLightSpace(u32 idx) { return m_spotLightShadowMaps[idx]._space; }

  static void               InitializeShadowPipelines(VulkanRHI* pRhi);
  static void               CleanUpShadowPipelines(VulkanRHI* pRhi);

private:

  void                      InitializeShadowMap(VulkanRHI* pRhi);
  void                      InitializeSpotLightShadowMapArray(VulkanRHI* pRhi, u32 resolution = 512u);


  void                      CleanUpSpotLightShadowMapArray(VulkanRHI* pRhi);

  Vector3                   m_staticViewerPos;
  r32                       m_staticShadowViewportDim;

  // Maps that contain shadow information on the directional, sunlight/moonlight object.
  // These are 2D arrays containing cascaded shadow maps.
  Texture*                  m_pStaticMap;       
  Texture*                  m_pDynamicMap;      
  Texture*                  m_pShadowMergeMap;  // Merging map of the static and dynamic shadow maps.
  // common used pipelines.
  // TODO(): We don't need to create multiple instances of these pipelines, we must place them in 
  // a static object.
  static GraphicsPipeline*  k_pSkinnedPipeline;
  static GraphicsPipeline*  k_pSkinnedMorphPipeline;
  static GraphicsPipeline*  k_pStaticPipeline;
  static GraphicsPipeline*  k_pStaticMorphPipeline;
  static GraphicsPipeline*  k_pStaticSkinnedPipeline;
  static GraphicsPipeline*  k_pStaticSkinnedMorphPipeline;
  static GraphicsPipeline*  k_pStaticStaticPipeline;
  static GraphicsPipeline*  k_pStaticStaticMorphPipeline;
  
  
  static RenderPass*        k_pDynamicRenderPass;
  static RenderPass*        k_pStaticRenderPass;

  // Items that are dependent on each instance of a Shadow Mapping system.
  FrameBuffer*              m_pStaticFrameBuffer;
  FrameBuffer*              m_pDynamicFrameBuffer;

  struct Cascade {
    FrameBuffer*            _framebuffer;
    ImageView*              _view;
    DescriptorSet*          _set;
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
  FrameBuffer*              m_pStaticOmniFrameBuffer;
  FrameBuffer*              m_pDynamicOmniFrameBuffer;
  Buffer*                   m_pLightViewBuffer;
  Buffer*                   m_pStaticLightViewBuffer;
  DescriptorSet*            m_pLightViewDescriptorSet;
  DescriptorSet*            m_pStaticLightViewDescriptorSet;
  Texture*                  m_pOmniMapArray;
  Texture*                  m_pStaticOmniMapArray;
  Texture*                  m_pSpotLightMapArray;
  Texture*                  m_pStaticSpotLightMapArray;
  LightViewSpace            m_viewSpace;
  LightViewSpace            m_staticViewSpace;
  r32                       m_rShadowViewportDim;
  b32                       m_staticMapNeedsUpdate;
  u32                       m_numPointLights;
  std::vector<Vector3>      m_pointMapPositions;
  std::vector<Cascade>      m_dynamicCascades;
  std::vector<Cascade>      m_staticCascades;
  std::vector<ShadowMapLayer> m_spotLightShadowMaps;
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
  void                Update(VulkanRHI* pRhi, GlobalBuffer* gBuffer);

  // Initialize. 
  void                Initialize(VulkanRHI* pRhi, GraphicsQuality shadowDetail, b32 enableSoftShadows = true);

  // Cleanup.
  void                CleanUp(VulkanRHI* pRhi);

  LightBuffer*        Data() { return &m_Lights; }

  DescriptorSet*      Set() { return m_pLightDescriptorSet; }
#if 0
  DescriptorSet*      ViewSet() { return m_pLightViewDescriptorSet; }

  Texture*            PrimaryShadowMap() { return m_pOpaqueShadowMap; }
#endif
  Sampler*            ShadowSampler() { return m_pShadowSampler; }
  void                SetViewerPosition(Vector3 viewer) { m_vViewerPos = viewer; }
  Vector3             ViewerPos() const { return m_vViewerPos; }

  void                SetShadowViewport(r32 width, r32 height) { m_rShadowViewportWidth = width; m_rShadowViewportHeight = height; }
  
  void                EnablePrimaryShadow(b8 enable) { m_PrimaryShadowEnable = enable; }

  b32                  PrimaryShadowEnabled() const { return m_PrimaryShadowEnable; }

  ShadowMapSystem&    PrimaryShadowMapSystem() { return m_primaryMapSystem; }

private:
  void                InitializeNativeLights(VulkanRHI* pRhi);

#if 0
  void                InitializePrimaryShadow(VulkanRHI* pRhi);
#endif
  Vector3             m_vViewerPos;
  // Descriptor Set.
  DescriptorSet*      m_pLightDescriptorSet;

  ShadowMapSystem     m_primaryMapSystem;

  // Light list is this!
  Buffer*             m_pLightBuffer;

#if 0
  DescriptorSet*      m_pLightViewDescriptorSet;

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
  Sampler*            m_pShadowSampler;
  LightBuffer         m_Lights;

  r32                 m_rShadowViewportWidth;
  r32                 m_rShadowViewportHeight;

  b32                  m_PrimaryShadowEnable;

  friend class Renderer;
};
} // Recluse