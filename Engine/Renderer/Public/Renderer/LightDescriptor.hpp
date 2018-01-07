// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Matrix3.hpp"
#include "Core/Math/Vector4.hpp"

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


struct LightBuffer {
  // NOTE(): Do we want more directional lights? This can be done if needed.
  DirectionalLight  _PrimaryLight;
  DirectionalLight  _DirectionalLights [32];
  PointLight        _PointLights       [128];
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
  void                Update();

  // Initialize. 
  void                Initialize();

  // Cleanup.
  void                CleanUp();

  LightBuffer*        Data() { return &m_Lights; }

  DescriptorSet*      Set() { return m_pLightDescriptorSet; }
  DescriptorSet*      ViewSet() { return m_pLightViewDescriptorSet; }

  Texture*            PrimaryShadowMap() { return m_pShadowMap; }
  Sampler*            ShadowSampler() { return m_pShadowSampler; }

  void                EnablePrimaryShadow(b8 enable) { m_PrimaryShadowEnable = enable; }

  b8                  PrimaryShadowEnabled() const { return m_PrimaryShadowEnable; }

private:
  void                InitializeNativeLights();
  void                InitializePrimaryShadow();
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
  Texture*            m_pShadowMap;

  // Shadow map sampler.
  Sampler*            m_pShadowSampler;

  // Framebuffer object used for command buffer pass through. Shadow mapping can't be
  // statically stored into the renderer pipeline. Only possibility is to create this 
  // outside.
  FrameBuffer*        m_pFrameBuffer;

  // Information of our lights, to which we use this to modify light sources.
  LightViewSpace      m_PrimaryLightSpace;
  LightBuffer         m_Lights;

  // Vulkan Rhi.
  VulkanRHI*          m_pRhi;

  b8                  m_PrimaryShadowEnable;

  friend class Renderer;
};
} // Recluse