// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Matrix3.hpp"
#include "Core/Math/Vector4.hpp"

#include "Renderer/UserParams.hpp"

namespace Recluse {

class VulkanRHI;
class DescriptorSet;
class Buffer;
class Texture;
class Sampler;
class Texture2D;
class TextureSampler;
class Texture3D;
class Texture1D;
class Texture2DArray;
class TextureCube;
class FrameBuffer;
class RenderPass;


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
  Vector4           _Color;
  r32               _Range;
  i32               _Enable;
  i32               _Pad;
};


struct LightViewSpace {
  Matrix4           _ViewProj;
};


struct LightGridBuffer {
  
};

#define MAX_DIRECTIONAL_LIGHTS  4
#define MAX_POINT_LIGHTS        64


struct LightBuffer {
  static u32        MaxNumDirectionalLights();
  static u32        MaxNumPointLights();
  // NOTE(): Do we want more directional lights? This can be done if needed.
  DirectionalLight  _PrimaryLight;
  DirectionalLight  _DirectionalLights [MAX_DIRECTIONAL_LIGHTS];
  PointLight        _PointLights       [MAX_POINT_LIGHTS];
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
  void                Update(VulkanRHI* pRhi);

  // Initialize. 
  void                Initialize(VulkanRHI* pRhi, ShadowDetail shadowDetail);

  // Cleanup.
  void                CleanUp(VulkanRHI* pRhi);

  LightBuffer*        Data() { return &m_Lights; }

  DescriptorSet*      Set() { return m_pLightDescriptorSet; }
  DescriptorSet*      ViewSet() { return m_pLightViewDescriptorSet; }

  Texture*            PrimaryShadowMap() { return m_pOpaqueShadowMap; }
  Sampler*            ShadowSampler() { return m_pShadowSampler; }
  void                SetViewerPosition(Vector3 viewer) { m_vViewerPos = viewer; }
  Vector3             ViewerPos() const { return m_vViewerPos; }

  void                SetShadowViewport(r32 width, r32 height) { m_rShadowViewportWidth = width; m_rShadowViewportHeight = height; }
  
  void                EnablePrimaryShadow(b8 enable) { m_PrimaryShadowEnable = enable; }

  b32                  PrimaryShadowEnabled() const { return m_PrimaryShadowEnable; }

private:
  void                InitializeNativeLights(VulkanRHI* pRhi);
  void                InitializePrimaryShadow(VulkanRHI* pRhi);

  Vector3             m_vViewerPos;
  // Descriptor Set.
  DescriptorSet*      m_pLightDescriptorSet;
  DescriptorSet*      m_pLightViewDescriptorSet;

  // Light list is this!
  Buffer*             m_pLightBuffer;

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

  // Shadow map sampler.
  Sampler*            m_pShadowSampler;

  // Framebuffer object used for command buffer pass through. Shadow mapping can't be
  // statically stored into the renderer pipeline. Only possibility is to create this 
  // outside.
  FrameBuffer*        m_pFrameBuffer;

  // RenderPass instance for the framebuffer.
  RenderPass*         m_pRenderPass;

  // Information of our lights, to which we use this to modify light sources.
  LightViewSpace      m_PrimaryLightSpace;
  LightBuffer         m_Lights;

  r32                 m_rShadowViewportWidth;
  r32                 m_rShadowViewportHeight;

  b32                  m_PrimaryShadowEnable;

  friend class Renderer;
};
} // Recluse