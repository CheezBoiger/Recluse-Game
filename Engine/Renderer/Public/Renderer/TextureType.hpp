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
  static U64          sIteration;
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

  Texture* getHandle() { return texture; }
  VulkanRHI* getRhi() { return mRhi; }
  
  Type TexType() const { return m_TexType; }

  // Stores texture image in a separate file. This varies on what the texture type is.
  virtual void  cacheToFile(std::string path) { }
  virtual void cleanUp() { }

  // Save this texture into a file for use in another life.
  virtual void save(const std::string pathName) { }

  void          setTextureHandle(Texture* newTex) { texture = newTex; }
  U64           UUID() const { return m_uuid; }
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
  U64         m_uuid;
};


// 1 Dimensional texture object.
class Texture1D : public TextureBase {
public:
  Texture1D() : TextureBase(TEXTURE_1D) { }

  void        initialize(U32 width);
  U32         getWidth();
  void        cleanUp() override { }
private:
  friend class Renderer;

};


// 2 Dimensional texture object.
class Texture2D : public TextureBase {
public:
  Texture2D() : TextureBase(TEXTURE_2D)
              , m_bGenMips(false) { }

  // Initializes the texture object with fixed width and height.
  // All images that are uploaded to this texture must then be the 
  // same width and height in order to render properly. To write 
  // texture over to this object, call Update() after this call.
  void        initialize(RFormat format, U32 width, U32 height, B32 genMips = false);
  
  void        save(const std::string filename) override;
  // Update texture with a new image to be written over.
  void        update(Image const& image);
  void        cleanUp() override;
  
  U32         getWidth() const;
  U32         getHeight() const;
private:
  B32         m_bGenMips;
  friend class Renderer;
};


// 2 Dimensional array texture object.
class Texture2DArray : public TextureBase {
public:
  Texture2DArray() : TextureBase(TEXTURE_2D_ARRAY) { }

  // width, and height must be the same size for every layer!
  void            initialize(RFormat format, U32 width, U32 height, U32 layers);

  // Image to update the texture array, will be sliced and distributed according to
  // each layer.
  void            update(const Image& img, U32 x, U32 y);

  void            cleanUp() override;
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

  void initialize(U32 dim);
  void update(Image const& image);
  void cleanUp() override;
  void save(const std::string filename) override;
  U32   WidthPerFace() const;
  U32   HeightPerFace() const;

  friend class Renderer;
};


// Cube Map array texture object.
class TextureCubeArray : public TextureBase {
public:
  TextureCubeArray() : TextureBase(TEXTURE_CUBE_ARRAY) { }

  void initialize(U32 dim = 512, U32 cubeLayers = 1);
  void cleanUp() override;
  void update(const Image& arrayimage);
  void update(const TextureCube* pCubeMaps, U32 count);
  //void Update(const Image& imgCubeMap, U32 count); 
  void save(const std::string filename) override;
  U32 layerCount() const;
  U32 mipMapCount() const;

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
  R32                 _mipLodBias;
  B32                 _enableAnisotropy;
  R32                 _maxAniso;
  R32                 _maxLod;
  R32                 _minLod;
  B32                 _unnnormalizedCoordinates;
};


// Texture Sampler object, for sampling textures to/from the renderer.
class TextureSampler {
  static UUID64 sIteration;
public:
  TextureSampler() : mSampler(nullptr) 
                    , m_uuid(sIteration++) { }

  void          initialize(VulkanRHI* pRhi, const SamplerInfo& info);
  void          cleanUp(VulkanRHI* pRhi);

  Sampler*      getHandle() { return mSampler; }
  UUID64 UUID() const { return m_uuid; }

private:
  SamplerInfo   mInfo;
  Sampler*      mSampler;
  UUID64 m_uuid;
};
} // Recluse