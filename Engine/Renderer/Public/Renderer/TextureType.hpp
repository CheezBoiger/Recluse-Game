// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class VulkanRHI;
class Texture;
class Sampler;
struct Image;


enum RFormat {
  RFORMAT_R8G8B8A8_UNORM,
  RFORMAT_R16G16_UNORM
};


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
  void        Initialize(RFormat format, u32 width, u32 height, b32 genMips = false);
  
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

  // width, and height must be the same size for every layer!
  void            Initialize(RFormat format, u32 width, u32 height, u32 layers);

  // Image to update the texture array, will be sliced and distributed according to
  // each layer.
  void            Update(const Image& img, u32 x, u32 y);

  void            CleanUp() override;
  friend class Renderer;
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

  void Initialize(u32 dim);
  void Update(Image const& image);
  void CleanUp() override;
  void Save(const std::string filename) override;
  u32   WidthPerFace() const;
  u32   HeightPerFace() const;

  friend class Renderer;
};


// Cube Map array texture object.
class TextureCubeArray : public TextureBase {
public:
  TextureCubeArray() : TextureBase(TEXTURE_CUBE_ARRAY) { }

  void Initialize(u32 dim = 512, u32 cubeLayers = 1);
  void CleanUp() override;
  void Update(const Image& arrayimage);
  void Update(const TextureCube* pCubeMaps, u32 count);
  //void Update(const Image& imgCubeMap, u32 count); 
  void Save(const std::string filename) override;
  u32 LayerCount() const;
  u32 MipMapCount() const;

  friend class Renderer;
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