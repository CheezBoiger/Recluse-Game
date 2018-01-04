// Copyright (c) 2017 Recluse Project. All rights reserved.
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
class Renderer;
class FrameBuffer;


struct MaterialBuffer {
  Vector4 _Color;          // object base color.
  r32     _LodBias;        // object level of detail bias.
  r32     _Transparency;   // transparency [0.0, 1.0]
  r32     _BaseMetal;      // object base metalness [0.0, 1.0]
  r32     _BaseRough;      // object base roughness [0.0, 1.0]
  r32     _BaseEmissive;   // emissive base [0.0, inf]
  u32     _HasAlbedo;      // does object have albedo map?
  u32     _HasMetallic;    // does object have metalness map?
  u32     _HasRoughness;   // does object have roughness map?
  u32     _HasNormal;      // does object have normal map?
  u32     _HasEmissive;    // does object have emissive map?
  u32     _HasAO;          // does object have ambient occlusion map?
  u32     _IsTransparent;  // is object transparent?
};

// Physically based material layout that our renderer uses as material for 
// meshes.
class Material {
public:

  Material();

  void            Initialize(Renderer* renderer);
  void            Update();
  void            CleanUp();

  void            SetSampler(TextureSampler* sampler) { mSampler = sampler; }
  void            SetAlbedo(Texture2D* albedo) { mAlbedo = albedo; }
  void            SetMetallic(Texture2D* metallic) { mMetallic = metallic; }
  void            SetRoughness(Texture2D* roughness) { mRoughness = roughness; }
  void            SetNormal(Texture2D* normal) { mNormal = normal; }
  void            SetAo(Texture2D* ao) { mAo = ao; }
  void            SetEmissive(Texture2D* emissive) { mEmissive = emissive; }
  void            SetTransparent(b8 enable) { m_MaterialData._IsTransparent = enable; }

  MaterialBuffer* Data() { return &m_MaterialData; }
  Buffer*         NativeBuffer() { return m_pBuffer; }

  b8              Transparent() const { return m_MaterialData._IsTransparent; }

  Texture2D*      Albedo() { return mAlbedo; }
  Texture2D*      Metallic() { return mMetallic; }
  Texture2D*      Roughness() { return mRoughness; }
  Texture2D*      Normal() { return mNormal; }
  Texture2D*      Ao() { return mAo; }
  Texture2D*      Emissive() { return mEmissive; }

  TextureSampler* Sampler() { return mSampler; }

private:

  MaterialBuffer  m_MaterialData;
  Buffer*         m_pBuffer;

  Texture2D*      mAlbedo;
  Texture2D*      mMetallic;
  Texture2D*      mRoughness;
  Texture2D*      mNormal;
  Texture2D*      mAo;
  Texture2D*      mEmissive;

  TextureSampler* mSampler;
  Renderer*       m_pRenderer;
  friend class Renderer;
};
} // Recluse