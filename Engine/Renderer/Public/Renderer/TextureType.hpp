// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class VulkanRHI;
class Texture;
class Sampler;
struct Image;


struct TextureBase {
  static std::string  kDefaultName;
  static u64          sIteration;
public:
  enum Type {
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_3D,
    TEXTURE_3D_ARRAY,
    TEXTURE_CUBE,
    TEXTURE_CUBE_ARRAY
  };

  Texture* Handle() { return texture; }
  VulkanRHI* GetRhi() { return mRhi; }
  
  Type TexType() const { return m_TexType; }

  // Stores texture image in a separate file. This varies on what the texture type is.
  virtual void  CacheToFile(std::string path) { }
  virtual void CleanUp() { }

  // Save this texture into a file for use in another life.
  virtual void Save(const std::string pathName) { }

  void          SetTextureHandle(Texture* newTex) { texture = newTex; }
  u64           UUID() const { return m_uuid; }
  // Name of Texture.
  std::string   _Name;

protected:
  TextureBase(Type type)
    : texture(nullptr)
    , mRhi(nullptr)
    , m_TexType(type) 
    , m_uuid(sIteration)
  {
    _Name = kDefaultName + std::to_string(sIteration++);  
  }

  Texture*    texture;
  VulkanRHI*  mRhi;

private:
  Type        m_TexType;
  u64         m_uuid;
};


// 1 Dimensional texture object.
class Texture1D : public TextureBase {
public:
  Texture1D() : TextureBase(TEXTURE_1D) { }

  void        Initialize(u32 width);
  u32         Width();
  void        CleanUp() override { }
private:
  friend class Renderer;

};


// 2 Dimensional texture object.
class Texture2D : public TextureBase {
public:
  Texture2D() : TextureBase(TEXTURE_2D) { }

  // Initializes the texture object with fixed width and height.
  // All images that are uploaded to this texture must then be the 
  // same width and height in order to render properly. To write 
  // texture over to this object, call Update() after this call.
  void        Initialize(u32 width, u32 height, b32 genMips = false);
  
  void        Save(const std::string filename) override;
  // Update texture with a new image to be written over.
  void        Update(Image const& image);
  void        CleanUp() override;
  
  u32         Width() const;
  u32         Height() const;
private:

  friend class Renderer;
};


// 2 Dimensional array texture object.
class Texture2DArray : public TextureBase {
public:
  Texture2DArray() : TextureBase(TEXTURE_2D_ARRAY) { }
};

// 3 Dimensional texture object.
class Texture3D : public TextureBase {
public:
  Texture3D() : TextureBase(TEXTURE_3D) { }
};

// Cube Map texture object. This comprises of 6 2D textures.
class TextureCube : public TextureBase {
public:
  TextureCube() : TextureBase(TEXTURE_CUBE) { }

  void Initialize(u32 extentX, u32 extentY, u32 extentZ = 1);
  void Update(u32 count, Image const* images);
  void CleanUp() override;
  void Save(const std::string filename) override;
  u32   WidthPerFace() const;
  u32   HeightPerFace() const;
};


// Cube Map array texture object.
class TextureCubeArray : public TextureBase {
public:
  TextureCubeArray() : TextureBase(TEXTURE_CUBE_ARRAY) { }

};


enum SamplerAddressMode {
  SAMPLER_ADDRESS_CLAMP_TO_EDGE,
  SAMPLER_ADDRESS_CLAMP_TO_BORDER,
  SAMPLER_ADDRESS_MIRROR_CLAMP_TO_EDGE,
  SAMPLER_ADDRESS_MIRRORED_REPEAT,
  SAMPLER_ADDRESS_REPEAT
};

enum SamplerFilterMode {
  SAMPLER_FILTER_LINEAR,
  SAMPLER_FILTER_NEAREST
};


enum SamplerMipMapMode {
  SAMPLER_MIPMAP_MODE_LINEAR,
  SAMPLER_MIPMAP_MODE_NEAREST
};


enum SamplerBorderColor {
  SAMPLER_BORDER_COLOR_OPAQUE_WHITE,
  SAMPLER_BORDER_COLOR_OPAQUE_BLACK
};


struct SamplerInfo {
  SamplerAddressMode  _addrU;
  SamplerAddressMode  _addrV;
  SamplerAddressMode  _addrW;
  SamplerFilterMode   _minFilter;
  SamplerFilterMode   _maxFilter;
  SamplerMipMapMode   _mipmapMode;
  SamplerBorderColor  _borderColor;
  r32                 _mipLodBias;
  b32                 _enableAnisotropy;
  r32                 _maxAniso;
  r32                 _maxLod;
  r32                 _minLod;
  b32                 _unnnormalizedCoordinates;
};


// Texture Sampler object, for sampling textures to/from the renderer.
class TextureSampler {
  static uuid64 sIteration;
public:
  TextureSampler() : mSampler(nullptr) 
                    , m_uuid(sIteration++) { }

  void          Initialize(VulkanRHI* pRhi, const SamplerInfo& info);
  void          CleanUp(VulkanRHI* pRhi);

  Sampler*      Handle() { return mSampler; }
  uuid64 UUID() const { return m_uuid; }

private:
  SamplerInfo   mInfo;
  Sampler*      mSampler;
  uuid64 m_uuid;
};
} // Recluse