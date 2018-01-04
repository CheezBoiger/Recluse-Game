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
class MaterialDescriptor {
public:

  MaterialDescriptor();
  ~MaterialDescriptor();

  // Always initialize the Material.
  void            Initialize();
  void            Update();
  void            CleanUp();

  void            SetSampler(TextureSampler* sampler) { m_pSampler = sampler; }
  void            SetAlbedo(Texture2D* albedo) { m_pAlbedo = albedo; }
  void            SetMetallic(Texture2D* metallic) { m_pMetallic = metallic; }
  void            SetRoughness(Texture2D* roughness) { m_pRoughness = roughness; }
  void            SetNormal(Texture2D* normal) { m_pNormal = normal; }
  void            SetAo(Texture2D* ao) { m_pAo = ao; }
  void            SetEmissive(Texture2D* emissive) { m_pEmissive = emissive; }
  void            SetTransparent(b8 enable) { m_MaterialData._IsTransparent = enable; }

  MaterialBuffer* Data() { return &m_MaterialData; }
  Buffer*         Native() { return m_pBuffer; }

  b8              Transparent() const { return m_MaterialData._IsTransparent; }

  Texture2D*      Albedo() { return m_pAlbedo; }
  Texture2D*      Metallic() { return m_pMetallic; }
  Texture2D*      Roughness() { return m_pRoughness; }
  Texture2D*      Normal() { return m_pNormal; }
  Texture2D*      Ao() { return m_pAo; }
  Texture2D*      Emissive() { return m_pEmissive; }

  TextureSampler* Sampler() { return m_pSampler; }

private:

  MaterialBuffer  m_MaterialData;
  Buffer*         m_pBuffer;

  Texture2D*      m_pAlbedo;
  Texture2D*      m_pMetallic;
  Texture2D*      m_pRoughness;
  Texture2D*      m_pNormal;
  Texture2D*      m_pAo;
  Texture2D*      m_pEmissive;

  TextureSampler* m_pSampler;
  VulkanRHI*      m_pRhi;
  friend class Renderer;
};
} // Recluse