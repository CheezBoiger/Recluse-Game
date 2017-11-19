// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Matrix3.hpp"
#include "Core/Math/Vector4.hpp"

namespace Recluse {

//
// TODO(): Replace Texture and Sampler with Texture2D and TextureSampler!
//
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


// TODO(): Need to add more information like mouse input,
// Possible SunDir(?), fog amount (?), and others.
struct GlobalBuffer {
  Matrix4         view;
  Matrix4         proj;
  Matrix4         viewProj;
  Vector4         cameraPos;
  Vector4         lPlane;
  Vector4         rPlane;
  Vector4         tPlane;
  Vector4         bPlane;
  Vector4         nPlane;
  Vector4         fPlane;
  i32             screenSize[2];
  i32             pad[2];
};


// Global Material.
class GlobalMaterial {
public:
  GlobalMaterial();

  void              Update();
  void              Initialize();
  void              CleanUp();  

  DescriptorSet*    Set() { return mDescriptorSet; }

  GlobalBuffer*     Data() { return &mGlobal; }
private:
  DescriptorSet*    mDescriptorSet;
  Buffer*           mGlobalBuffer;
  VulkanRHI*        mRhi;
  GlobalBuffer      mGlobal;

  friend class Renderer;
};


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
  PointLight        pointLights[128];
};


// Light material.
class LightMaterial {
public:

  LightMaterial();
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


// Physically based material layout that our renderer uses as material for 
// meshes.
class Material {
public:

  Material();

  void            SetSampler(TextureSampler* sampler) { mSampler = sampler; }
  void            SetAlbedo(Texture2D* albedo) { mAlbedo = albedo; }
  void            SetMetallic(Texture2D* metallic) { mMetallic = metallic; }
  void            SetRoughness(Texture2D* roughness) { mRoughness = roughness; }
  void            SetNormal(Texture2D* normal) { mNormal = normal; }
  void            SetAo(Texture2D* ao) { mAo = ao; }
  void            SetEmissive(Texture2D* emissive) { mEmissive = emissive; }

  Texture2D*      Albedo() { return mAlbedo; }
  Texture2D*      Metallic() { return mMetallic; }
  Texture2D*      Roughness() { return mRoughness; }
  Texture2D*      Normal() { return mNormal; }
  Texture2D*      Ao() { return mAo; }
  Texture2D*      Emissive() { return mEmissive; }

  TextureSampler* Sampler() { return mSampler; }

private:

  Texture2D*      mAlbedo;
  Texture2D*      mMetallic;
  Texture2D*      mRoughness;
  Texture2D*      mNormal;
  Texture2D*      mAo;
  Texture2D*      mEmissive;

  TextureSampler* mSampler;

  friend class Renderer;
};
} // Recluse