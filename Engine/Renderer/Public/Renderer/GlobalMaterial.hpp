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


// TODO(): Need to add more information like mouse input,
// Possible SunDir(?), fog amount (?), and others.
struct GlobalBuffer {
  Matrix4         view;
  Matrix4         proj;
  Matrix4         viewProj;
  Vector4         cameraPos;
  Vector4         lPlane;
  Vector4         rPlane;
  Vector4         tPlane;
  Vector4         bPlane;
  Vector4         nPlane;
  Vector4         fPlane;
  i32             screenSize[2];
  i32             pad[2];
};


// Global Material.
class GlobalMaterial {
public:
  GlobalMaterial();

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
} // Recluse 