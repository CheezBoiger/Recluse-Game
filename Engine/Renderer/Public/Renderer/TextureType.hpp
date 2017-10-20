// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class VulkanRHI;
class Texture;
class Sampler;

// 1 Dimensional texture object.
class Texture1D {
public:
};

// 2 Dimensional texture object.
class Texture2D {
public:
  Texture2D()
    : texture(nullptr)
    , mRhi(nullptr) { }

  void      Initialize();
  void      CleanUp();

  Texture*  Handle() { return texture; }
private:
  Texture* texture;
  VulkanRHI* mRhi;
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


class TextureSampler {
public:
};
} // Recluse