// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Math/Matrix4.hpp"

namespace Recluse {

class Buffer;
class Renderer;
class MeshData;


struct ObjectBuffer {
  Matrix4 _Model;          // Model matrix
  Matrix4 _NormalMatrix;   // Normal matrix.
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
  u32     _HasBones;       // does object have bones?
  u32     _IsTransparent;  // is object transparent?
  u32     _Pad[3];
};


struct BonesBuffer {
  Matrix4 _Bones[64];
};


// MeshDesciptor is a descriptor that defines how to render an object. This is needed in order
// to show something on display, as the renderer relies heavily on this object for
// command buffer creation. It basically defines the transform of the meshdata to render, as well
// as whether that meshdata is defined as transparent, translucent, visible, and/or renderable.
class MeshDescriptor {
public:
  MeshDescriptor();
  virtual ~MeshDescriptor();

  virtual void  Initialize(Renderer* renderer);
  virtual void  CleanUp();

  virtual void  Update();

  void          SetVisible(b8 enable) { m_Visible = enable; }
  void          SetRenderable(b8 enable) { m_Renderable = enable; }
  void          SetTranslucent(b8 enable) { m_Translucent = enable; }
  void          SetTransparent(b8 enable) { m_ObjectData._IsTransparent = enable; }

  ObjectBuffer* ObjectData() { return &m_ObjectData; }


  b8            Visible() const { return m_Visible; }
  b8            Renderable() const { return m_Renderable; }
  b8            Transparent() const { return m_ObjectData._IsTransparent; }
  b8            Translucent() const { return m_Translucent; }
  b8            Static() const { return m_Static; }
  b8            Skinned() const { return m_Skinned; }
  Buffer*       NativeObjectBuffer() { return m_pObjectBuffer; }

protected:
  ObjectBuffer  m_ObjectData;
  Buffer*       m_pObjectBuffer;

  b8            m_Visible;
  b8            m_Renderable;
  b8            m_Skinned;

  b8            m_Translucent;
  b8            m_Static;
  
  Renderer*     m_Renderer;
  
  friend class Renderer;
};


class SkinnedMeshDescriptor : public MeshDescriptor {
public:
  SkinnedMeshDescriptor();
  virtual ~SkinnedMeshDescriptor();
  
  virtual void  Initialize(Renderer* renderer) override;
  virtual void  CleanUp() override;
  virtual void  Update() override;

  BonesBuffer*  BonesData() { return &m_BonesData; }
  Buffer*       NativeBoneBuffer() { return m_pBonesBuffer; }

private:
  BonesBuffer   m_BonesData;
  Buffer*       m_pBonesBuffer;
  friend class  Renderer;
};
} // Recluse