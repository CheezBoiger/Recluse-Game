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

enum MaterialUpdateBits {
  MATERIAL_BUFFER_UPDATE_BIT = (1 << 0),
  MATERIAL_DESCRIPTOR_UPDATE_BIT = (1 << 1)
};


struct MaterialBuffer {
  Vector4 _Color;             // object base color.
  Vector4 _AnisoSpec;         // Anisotropy control.
  Vector4 _offsetUV;          // offset values for texture coordinates of uv 0 (xy) and 1 (zw).
  R32     _Opacity;           // opacity [0.0, 1.0]
  R32     _metalFactor;       // object base metalness [0.0, 1.0]
  R32     _roughFactor;       // object base roughness [0.0, 1.0]
  R32     _emissiveFactor;    // emissive base [0.0, inf]
  U32     _HasAlbedo;         // does object have albedo map?
  U32     _HasMetallic;       // does object have metalness map?
  U32     _HasRoughness;      // does object have roughness map?
  U32     _HasNormal;         // does object have normal map?
  U32     _HasEmissive;       // does object have emissive map?
  U32     _HasAO;             // does object have ambient occlusion map?
  U32     _IsTransparent;     // is object transparent?
  U32     _Pad;
};

// Physically based material layout that our renderer uses as material for 
// meshes.
class MaterialDescriptor {
public:

  MaterialDescriptor();
  ~MaterialDescriptor();

  // Always initialize the Material.
  void            initialize(VulkanRHI* pRhi);
  void            update(VulkanRHI* pRhi);
  void            cleanUp(VulkanRHI* pRhi);

  void            setAlbedoSampler(TextureSampler* sampler) { m_pAlbedoSampler = sampler; }
  void            setNormalSampler(TextureSampler* sampler) { m_pNormalSampler = sampler; }
  void            setRoughMetalSampler(TextureSampler* sampler) { m_pRoughMetalSampler = sampler; }
  void            setAoSampler(TextureSampler* sampler) { m_pAoSampler = sampler; }
  void            setEmissiveSampler(TextureSampler* sampler) { m_pEmissiveSampler = sampler; }
  void            setAlbedo(Texture2D* albedo) { m_pAlbedo = albedo; }
  void            setRoughnessMetallic(Texture2D* roughMetal) { m_pRoughnessMetallic = roughMetal; }
  void            setNormal(Texture2D* normal) { m_pNormal = normal; }
  void            setAo(Texture2D* ao) { m_pAo = ao; }
  void            setEmissive(Texture2D* emissive) { m_pEmissive = emissive; }
  void            setTransparent(B8 enable) { m_MaterialData._IsTransparent = enable; }

  void            pushUpdate(B32 updateBits = MATERIAL_BUFFER_UPDATE_BIT) { m_bNeedsUpdate |= updateBits; }
  DescriptorSet*          CurrMaterialSet() { return m_materialSet; }

  MaterialBuffer* getData() { return &m_MaterialData; }
  Buffer*         getNative() { return m_pBuffer; }

  B32              isTransparent() const { return m_MaterialData._IsTransparent; }

  Texture2D*      getAlbedo() { return m_pAlbedo; }
  Texture2D*      getRoughnessMetallic() { return m_pRoughnessMetallic; }
  Texture2D*      getNormal() { return m_pNormal; }
  Texture2D*      getAo() { return m_pAo; }
  Texture2D*      getEmissive() { return m_pEmissive; }

  TextureSampler* getAlbedoSampler() { return m_pAlbedoSampler; }
  TextureSampler* getNormalSampler() { return m_pNormalSampler; }
  TextureSampler* getRoughMetalSampler() { return m_pRoughMetalSampler; }
  TextureSampler* getAoSampler() { return m_pAoSampler; }
  TextureSampler* getEmissiveSampler() { return m_pEmissiveSampler; }

private:

 // void                    SwapDescriptorSet() { m_currIdx = m_currIdx == 0 ? 1 : 0; }

  MaterialBuffer  m_MaterialData;
  Buffer*         m_pBuffer;

  Texture2D*      m_pAlbedo;
  Texture2D*      m_pRoughnessMetallic;
  Texture2D*      m_pNormal;
  Texture2D*      m_pAo;
  Texture2D*      m_pEmissive;

  TextureSampler* m_pAlbedoSampler;
  TextureSampler* m_pNormalSampler;
  TextureSampler* m_pRoughMetalSampler;
  TextureSampler* m_pAoSampler;
  TextureSampler* m_pEmissiveSampler;
  U32             m_bNeedsUpdate;
  DescriptorSet*  m_materialSet;
  friend class Renderer;
};
} // Recluse