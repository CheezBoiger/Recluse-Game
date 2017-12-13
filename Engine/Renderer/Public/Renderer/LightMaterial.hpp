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
  Vector4 direction;
  Vector4 color;
  r32     intensity;
  i32     enable;
  i32     pad[2];

  DirectionalLight()
    : enable(false) { }
};


struct PointLight {
  Vector4 position;
  Vector4 color;
  r32     range;
  i32     enable;
  i32     pad[2];

  PointLight()
    : enable(false), range(1.0f) { }
};


struct LightBuffer {
  // NOTE(): Do we want more directional lights? This can be done if needed.
  DirectionalLight  primaryLight;
  DirectionalLight  directionalLights [32];
  PointLight        pointLights       [128];
};


// Light material.
class LightMaterial {
public:

  LightMaterial();
  ~LightMaterial();

  void              SetShadowMap(Texture* shadow) { mShadowMap = shadow; }
  void              SetShadowSampler(Sampler* sampler) { mShadowSampler = sampler; }

  // Update the light information on the gpu, for use in our shaders.
  void              Update();

  // Initialize. 
  void              Initialize();

  // Cleanup.
  void              CleanUp();

  LightBuffer*      Data() { return &mLights; }
  DescriptorSet*    Set() { return mDescriptorSet; }

  Texture*          ShadowMap() { return mShadowMap; }
  Sampler*          ShadowSampler() { return mShadowSampler; }

private:
  // Descriptor Set.
  DescriptorSet*    mDescriptorSet;

  // Light list is this!
  Buffer*           mLightBuffer;

  // After computing the clusters to shade our lights, we store them in here!
  Buffer*           mLightGrid;

  // Shadow map, this is mainly for our directional light, the primary light.
  Texture*          mShadowMap;

  // Shadow map sampler.
  Sampler*          mShadowSampler;

  // Framebuffer object used for command buffer pass through. Shadow mapping can't be
  // statically stored into the renderer pipeline. Only possibility is to create this 
  // outside.
  FrameBuffer*      mFrameBuffer;

  // Information of our lights, to which we use this to modify light sources.
  LightBuffer       mLights;

  // Vulkan Rhi.
  VulkanRHI*        mRhi;

  friend class Renderer;
};
} // Recluse