// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class VulkanRHI;
class Texture;
class Sampler;
class Image;

// 1 Dimensional texture object.
class Texture1D {
public:
  Texture1D()
    : texture(nullptr)
    , mRhi(nullptr) { }

  void        Initialize(u32 width);
  void        CleanUp();
private:
  Texture*    texture;
  VulkanRHI*  mRhi;
};

// 2 Dimensional texture object.
class Texture2D {
public:
  Texture2D()
    : texture(nullptr)
    , mRhi(nullptr) { }

  // Initializes the texture object with fixed width and height.
  // All images that are uploaded to this texture must then be the 
  // same width and height in order to render properly. To write 
  // texture over to this object, call Update() after this call.
  void        Initialize(u32 width, u32 height);

  // Update texture with a new image to be written over.
  void        Update(Image const& image);
  void        CleanUp();

  Texture*    Handle() { return texture; }
private:
  Texture*    texture;
  VulkanRHI*  mRhi;

  friend class Renderer;
};

// 2 Dimensional array texture object.
class Texture2DArray {
public:
};

// 3 Dimensional texture object.
class Texture3D {
public:
};

// Cube Map texture object. This comprises of 6 2D textures.
class TextureCube {
public:
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


// Texture Smpler object, for sampling textures to/from the renderer.
class TextureSampler {
public:
  struct SamplerInfo {
    SamplerAddressMode  addrU;
    SamplerAddressMode  addrV;
    SamplerAddressMode  addrW;
    SamplerFilterMode   minFilter;
    SamplerFilterMode   maxFilter;
    r32                 mipLodBias;
    r32                 maxAniso;
    r32                 maxLod;
    r32                 minLod;
  };

  void          Initialize(SamplerInfo& info);
  void          CleanUp();

  Sampler*      Handle() { return mSampler; }

private:
  SamplerInfo   mInfo;
  Sampler*      mSampler;
};
} // Recluse