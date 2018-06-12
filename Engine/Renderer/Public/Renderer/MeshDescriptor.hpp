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
class DescriptorSet;


struct ObjectBuffer {
  Matrix4 _Model;          // Model matrix
  Matrix4 _NormalMatrix;   // Normal matrix.
  r32     _LoD;            // Level of Detail.
  u32     _HasJoints;      // does object have joints?
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
  enum UpdateBit {
    MESH_BUFFER_UPDATE = (1 << 0),
    JOINT_BUFFER_UPDATE = (1 << 1),
    MESH_DESCRIPTOR_UPDATE = (1 << 2),
    JOINT_DESCRIPTOR_UPDATE = (1 << 3),
  
  };

  MeshDescriptor();
  virtual ~MeshDescriptor();

  virtual void  Initialize();
  virtual void  CleanUp();

  virtual void  Update();

  void          SetVisible(b32 enable) { m_Visible = enable; }
  void          PushUpdate(b32 updateBits = (MESH_BUFFER_UPDATE | JOINT_BUFFER_UPDATE)) 
                  { m_bNeedsUpdate |= updateBits; }

  ObjectBuffer* ObjectData() { return &m_ObjectData; }
  virtual JointBuffer*  JointData() { return nullptr; }

  virtual DescriptorSet*  CurrJointSet() { return nullptr; }
  DescriptorSet*          CurrMeshSet() { return m_meshSet; }

  b32           Visible() const { return m_Visible; }
  b32           Static() const { return m_Static; }
  virtual b32   Skinned() const { return false; }
  Buffer*       NativeObjectBuffer() { return m_pObjectBuffer; }

  virtual u32                     NumJoints() { return 0; }

protected:

  //void                    SwapDescriptorSet() { m_currIdx = (m_currIdx == 0 ? 1 : 0); }

  ObjectBuffer  m_ObjectData;
  Buffer*       m_pObjectBuffer;
  b32           m_bNeedsUpdate;

  b32             m_Visible;
  b32             m_Static;
  
  u32             m_currIdx;
  DescriptorSet*  m_meshSet;
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
  virtual DescriptorSet*  CurrJointSet() override { return m_jointSet; }

  void          UpdateJointSets();

  virtual u32   NumJoints() override { return 128; }

private:
  JointBuffer   m_jointsData;
  Buffer*       m_pJointsBuffer;
  DescriptorSet*  m_jointSet;
  friend class  Renderer;
};
} // Recluse