// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"

namespace Recluse {

class Buffer;
class Renderer;
class MeshData;


struct ObjectBuffer {
  Matrix4 _Model;          // Model matrix
  Matrix4 _NormalMatrix;   // Normal matrix.
  r32     _LoD;            // Level of Detail.
  u32     _HasBones;       // does object have bones?
  u32     _Pad[2];
};


struct JointBuffer {
  Matrix4 _mJoints[128];
};


// MeshDesciptor is a descriptor that defines how to render an object. This is needed in order
// to show something on display, as the renderer relies heavily on this object for
// command buffer creation. It basically defines the transform of the meshdata to render, as well
// as whether that meshdata is defined as transparent, translucent, visible, and/or renderable.
class MeshDescriptor {
public:
  MeshDescriptor();
  virtual ~MeshDescriptor();

  virtual void  Initialize();
  virtual void  CleanUp();

  virtual void  Update();

  void          SetVisible(b32 enable) { m_Visible = enable; }
  void          SetRenderable(b32 enable) { m_Renderable = enable; }
  void          SetTranslucent(b32 enable) { m_Translucent = enable; }
  void          SignalUpdate() { m_bNeedsUpdate = true; }

  ObjectBuffer* ObjectData() { return &m_ObjectData; }
  virtual JointBuffer*  JointData() { return nullptr; }


  b32           Visible() const { return m_Visible; }
  b32           Renderable() const { return m_Renderable; }
  b32           Translucent() const { return m_Translucent; }
  b32           Static() const { return m_Static; }
  virtual b32   Skinned() const { return false; }
  Buffer*       NativeObjectBuffer() { return m_pObjectBuffer; }

  virtual u32                     NumJoints() { return 0; }

protected:
  ObjectBuffer  m_ObjectData;
  Buffer*       m_pObjectBuffer;
  b32           m_bNeedsUpdate;

  b32            m_Visible;
  b32            m_Renderable;
  b32            m_Translucent;
  b32            m_Static;
  
  VulkanRHI*     m_pRhi;
  
  friend class Renderer;
};


class SkinnedMeshDescriptor : public MeshDescriptor {
public:
  SkinnedMeshDescriptor();
  virtual ~SkinnedMeshDescriptor();
  
  virtual void  Initialize() override;
  virtual void  CleanUp() override;
  virtual void  Update() override;  
  virtual b32   Skinned() const override { return true; }

  JointBuffer*  JointData() override { return &m_jointsData; }
  Buffer*       NativeJointBuffer() { return m_pJointsBuffer; }

  virtual u32 NumJoints() override { return 128; }

private:
  JointBuffer   m_jointsData;
  Buffer*       m_pJointsBuffer;
  friend class  Renderer;
};
} // Recluse