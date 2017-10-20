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


// Global Material.
class GlobalMaterial {
public:
  struct GlobalBuffer {
    Matrix4         view;
    Matrix4         proj;
    Matrix4         viewProj;
    Vector4         cameraPos;
    r32             coffSH[9];
    i32             screenSize[2];
    i32             pad;
  };

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


// Light material.
class LightMaterial {
public:
  struct DirectionalLight {
    Vector4 direction;
    Vector4 color;
    b8      enable;
    b8      pad[15];
  };

  struct PointLight {
    Vector4 position;
    Vector4 color;
    r32     range;
    r32     pad0  [3];
    b8      enable;
    b8      pad1  [15];
  };
  
  struct LightBuffer {
    DirectionalLight  primaryLight;
    PointLight        pointLights[512];
  };

  void              SetShadowMap(Texture* shadow) { mShadowMap = shadow; }
  void              SetShadowSampler(Sampler* sampler) { mShadowSampler = sampler; }
  void              Update();
  void              Initialize();
  void              CleanUp();  

  LightBuffer*      Data() { return &mLights; }
  DescriptorSet*    Set() { return mDescriptorSet; }

  Texture*          ShadowMap() { return mShadowMap; }
  Sampler*          ShadowSampler() { return mShadowSampler; }
private:
  DescriptorSet*    mDescriptorSet;
  Buffer*           mLightBuffer;
  Texture*          mShadowMap;
  Sampler*          mShadowSampler;
  LightBuffer       mLights;
  VulkanRHI*        mRhi;

  friend class Renderer;
};


// Physically based material layout that our renderer uses as material for 
// meshes.
class Material {
public:
  struct ObjectBuffer {
    Matrix4 model;
    Matrix4 normalMatrix;
    u32     hasAlbedo;
    u32     hasMetallic;
    u32     hasRoughness;
    u32     hasNormal;
    u32     hasEmissive;
    u32     hasAO;
    u32     hasBones;
    u32     pad;
  };

  struct BonesBuffer {
    Matrix4 bones[64];
  };

  Material();

  void            SetSampler(Sampler* sampler) { mSampler = sampler; }
  void            SetAlbedo(Texture* albedo) { mAlbedo = albedo; }
  void            SetMetallic(Texture* metallic) { mMetallic = metallic; }
  void            SetRoughness(Texture* roughness) { mRoughness = roughness; }
  void            SetNormal(Texture* normal) { mNormal = normal; }
  void            SetAo(Texture* ao) { mAo = ao; }
  void            SetEmissive(Texture* emissive) { mEmissive = emissive; }

  ObjectBuffer*   ObjectData() { return &mObjectData; }
  BonesBuffer*    BonesData() { return &mBonesData; }

  DescriptorSet*  Set() { return mObjectBufferSet; }

  void            Initialize(b8 isStatic = true);
  void            CleanUp();
  void            Update();

private:
  ObjectBuffer    mObjectData;
  BonesBuffer     mBonesData;

  DescriptorSet*  mObjectBufferSet;

  Buffer*         mObjectBuffer;
  Buffer*         mBonesBuffer;
  Texture*        mAlbedo;
  Texture*        mMetallic;
  Texture*        mRoughness;
  Texture*        mNormal;
  Texture*        mAo;
  Texture*        mEmissive;

  Sampler*        mSampler;
  VulkanRHI*      mRhi;

  friend class Renderer;
};
} // Recluse