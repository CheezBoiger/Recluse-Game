// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"

#include <vector>

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
  Matrix4 _model;          // Model matrix
  Matrix4 _normalMatrix;   // Normal matrix.
  r32     _lod;            // Level of Detail.
  u32     _hasJoints;      // does object have joints?
  r32     _w0;              // Morph Target 0 weight.
  r32     _w1;              // Morph Target 1 weight.
};


struct JointBuffer {
  static const u32 kMaxNumberOfJointMatrices = 64;
  static Matrix4 defaultMatrices[kMaxNumberOfJointMatrices];

  Matrix4 _mJoints[kMaxNumberOfJointMatrices];
};


struct UpdateManager {
  DescriptorSet*  _pSet;
  Buffer*         _pBuf;
  b32             _updates;
};

// MeshDesciptor is a descriptor that defines how to render an object. This is needed in order
// to show something on display, as the renderer relies heavily on this object for
// command buffer creation. It basically defines the transform of the meshdata to render, as well
// as whether that meshdata is defined as transparent, translucent, visible, and/or renderable.
class MeshDescriptor {
public:

  MeshDescriptor();
  ~MeshDescriptor();

  void  initialize(VulkanRHI* pRhi);
  void  cleanUp(VulkanRHI* pRhi);

  void  update(VulkanRHI* pRhi, u32 frameIndex);

  void          setVisible(b32 enable) { m_Visible = enable; }
  void          pushUpdate(b32 updateBits = MESH_BUFFER_UPDATE_BIT) 
   { for (u32 i = 0; i < m_pGpuHandles.size(); ++i) { m_pGpuHandles[i]._updates |= updateBits; } }

  ObjectBuffer* getObjectData() { return &m_ObjectData; }
  DescriptorSet*          getCurrMeshSet(u32 frameIndex) { return m_pGpuHandles[frameIndex]._pSet; }

  b32           isVisible() const { return m_Visible; }
  b32           isStatic() const { return m_Static; }
  Buffer*       getNativeObjectBuffer(u32 frameIndex) { return m_pGpuHandles[frameIndex]._pBuf; }

protected:

  //void                    SwapDescriptorSet() { m_currIdx = (m_currIdx == 0 ? 1 : 0); }

  ObjectBuffer  m_ObjectData;

  b32             m_Visible;
  b32             m_Static;
  
  u32             m_currIdx;

  std::vector<UpdateManager> m_pGpuHandles;

  friend class Renderer;
};


class JointDescriptor {
public:
  JointDescriptor();
  ~JointDescriptor();
  
  void  initialize(VulkanRHI* pRhi);
  void  cleanUp(VulkanRHI* pRhi);
  void  update(VulkanRHI* pRhi, u32 frameIndex);  

  JointBuffer*  getJointData() { return &m_jointsData; }
  Buffer*       getNativeJointBuffer(u32 frameIndex) { return m_pJointHandles[frameIndex]._pBuf; }
  DescriptorSet*  getCurrJointSet(u32 frameIndex) { return m_pJointHandles[frameIndex]._pSet; }
  void          pushUpdate(b32 bits = JOINT_BUFFER_UPDATE_BIT) 
    { for(u32 i = 0; i < m_pJointHandles.size(); ++i)  m_pJointHandles[i]._updates |= bits; }

  void          updateJointSets(u32 frameIndex);

  u32   numJoints() { return JointBuffer::kMaxNumberOfJointMatrices; }

private:
  std::vector<UpdateManager> m_pJointHandles;
  JointBuffer   m_jointsData;
  friend class  Renderer;
};
} // Recluse