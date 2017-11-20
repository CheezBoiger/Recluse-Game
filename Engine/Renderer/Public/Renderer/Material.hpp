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