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
  Matrix4         _View;
  Matrix4         _Proj;
  Matrix4         _InvView;
  Matrix4         _InvProj;
  Matrix4         _ViewProj;
  Matrix4         _InvViewProj;
  Vector4         _CameraPos;
  Vector4         _LPlane;
  Vector4         _RPlane;
  Vector4         _TPlane;
  Vector4         _BPlane;
  Vector4         _NPlane;
  Vector4         _FPlane;
  Vector4         _clipPlane0;
  Vector2         _MousePos;
  i32             _ScreenSize[2];
  Vector3         _vSunDir; // Sun direction, must be normalized!
  r32             _vSunBrightness;
  Vector4         _vAirColor;
  Vector4         _fov;
  r32             _fEngineTime; // Current time passed within the engine.
  r32             _fDeltaTime;  // Elapsed time between frames.
  r32             _Gamma;
  r32             _Exposure;
  r32             _Rayleigh;
  r32             _Mie;
  r32             _MieDist;
  r32             _fScatterStrength;
  r32             _fRayleighStength;
  r32             _fMieStength;
  r32             _fIntensity;
  r32             _zNear;
  r32             _zFar;
  i32             _BloomEnabled;
  i32             _EnableShadows;
  i32             _EnableAA;
};


// Global MaterialDescriptor.
class GlobalDescriptor {
public:
  GlobalDescriptor();
  ~GlobalDescriptor();

  void              update(VulkanRHI* pRhi, u32 frameIndex);
  void              initialize(VulkanRHI* pRhi);
  void              cleanUp(VulkanRHI* pRhi);

  DescriptorSet*    getDescriptorSet(u32 idx) { return m_pDescriptorSets[idx]; }

  GlobalBuffer*     getData() { return &m_Global; }
  Buffer*           getHandle(u32 idx) { return m_pGlobalBuffers[idx]; }

private:
  std::vector<DescriptorSet*>    m_pDescriptorSets;
  std::vector<Buffer*>           m_pGlobalBuffers;
  GlobalBuffer      m_Global;

  friend class Renderer;
};
} // Recluse 