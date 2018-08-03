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


enum MeshUpdatBits { 
  MESH_BUFFER_UPDATE_BIT = (1 << 0),
  JOINT_BUFFER_UPDATE_BIT = (1 << 1),
  MESH_DESCRIPTOR_UPDATE_BIT = (1 << 2),   
  JOINT_DESCRIPTOR_UPDATE_BIT = (1 << 3),
};


struct ObjectBuffer {
  Matrix4 _Model;          // Model matrix
  Matrix4 _NormalMatrix;   // Normal matrix.
  r32     _LoD;            // Level of Detail.
  u32     _HasJoints;      // does object have joints?
  u32     _Pad[2];
};


struct JointBuffer {
  Matrix4 _mJoints[64];
};


// MeshDesciptor is a descriptor that defines how to render an object. This is needed in order
// to show something on display, as the renderer relies heavily on this object for
// command buffer creation. It basically defines the transform of the meshdata to render, as well
// as whether that meshdata is defined as transparent, translucent, visible, and/or renderable.
class MeshDescriptor {
public:

  MeshDescriptor();
  ~MeshDescriptor();

  void  Initialize(VulkanRHI* pRhi);
  void  CleanUp(VulkanRHI* pRhi);

  void  Update(VulkanRHI* pRhi);

  void          SetVisible(b32 enable) { m_Visible = enable; }
  void          PushUpdate(b32 updateBits = MESH_BUFFER_UPDATE_BIT) 
                  { m_bNeedsUpdate |= updateBits; }

  ObjectBuffer* ObjectData() { return &m_ObjectData; }
  DescriptorSet*          CurrMeshSet() { return m_meshSet; }

  b32           Visible() const { return m_Visible; }
  b32           Static() const { return m_Static; }
  Buffer*       NativeObjectBuffer() { return m_pObjectBuffer; }

protected:

  //void                    SwapDescriptorSet() { m_currIdx = (m_currIdx == 0 ? 1 : 0); }

  ObjectBuffer  m_ObjectData;
  Buffer*       m_pObjectBuffer;
  b32           m_bNeedsUpdate;

  b32             m_Visible;
  b32             m_Static;
  
  u32             m_currIdx;
  DescriptorSet*  m_meshSet;
  
  friend class Renderer;
};


class JointDescriptor {
public:
  JointDescriptor();
  ~JointDescriptor();
  
  void  Initialize(VulkanRHI* pRhi);
  void  CleanUp(VulkanRHI* pRhi);
  void  Update(VulkanRHI* pRhi);  

  JointBuffer*  JointData() { return &m_jointsData; }
  Buffer*       NativeJointBuffer() { return m_pJointsBuffer; }
  DescriptorSet*  CurrJointSet() { return m_jointSet; }
  void          PushUpdate(b32 bits = JOINT_BUFFER_UPDATE_BIT) 
    { m_bNeedsUpdate |= bits; }

  void          UpdateJointSets();

  u32   NumJoints() { return 64; }

private:
  b32           m_bNeedsUpdate;
  JointBuffer   m_jointsData;
  Buffer*       m_pJointsBuffer;
  DescriptorSet*  m_jointSet;
  friend class  Renderer;
};
} // Recluse