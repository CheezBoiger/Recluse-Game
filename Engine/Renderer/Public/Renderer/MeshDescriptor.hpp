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
  R32     _lod;            // Level of Detail.
  U32     _hasJoints;      // does object have joints?
  R32     _w0;              // Morph Target 0 weight.
  R32     _w1;              // Morph Target 1 weight.
};


struct JointBuffer {
  static const U32 kMaxNumberOfJointMatrices = 64;
  static Matrix4 defaultMatrices[kMaxNumberOfJointMatrices];

  Matrix4 _mJoints[kMaxNumberOfJointMatrices];
};


struct UpdateManager {
  DescriptorSet*  _pSet;
  Buffer*         _pBuf;
  B32             _updates;
};

// MeshDesciptor is a descriptor that defines how to render an object. This is needed in order
// to show something on display, as the renderer relies heavily on this object for
// command buffer creation. It basically defines the transform of the meshdata to render, as well
// as whether that meshdata is defined as transparent, translucent, visible, and/or renderable.
class MeshDescriptor {
public:

  MeshDescriptor();
  ~MeshDescriptor();

  void  initialize(Renderer* pRenderer);
  void  cleanUp(Renderer* pRenderer);

  void  update(Renderer* pRenderer, U32 resourceIndex);

  void          setVisible(B32 enable) { m_Visible = enable; }
  void          pushUpdate(B32 updateBits = MESH_BUFFER_UPDATE_BIT) 
   { for (U32 i = 0; i < m_pGpuHandles.size(); ++i) { m_pGpuHandles[i]._updates |= updateBits; } }

  ObjectBuffer* getObjectData() { return &m_ObjectData; }
  DescriptorSet*          getCurrMeshSet(U32 resourceIndex) { return m_pGpuHandles[resourceIndex]._pSet; }

  B32           isVisible() const { return m_Visible; }
  B32           isStatic() const { return m_Static; }
  Buffer*       getNativeObjectBuffer(U32 resourceIndex) { return m_pGpuHandles[resourceIndex]._pBuf; }

protected:

  //void                    SwapDescriptorSet() { m_currIdx = (m_currIdx == 0 ? 1 : 0); }

  ObjectBuffer  m_ObjectData;

  B32             m_Visible;
  B32             m_Static;
  U32             m_currIdx;

  std::vector<UpdateManager> m_pGpuHandles;

  friend class Renderer;
};


class JointDescriptor {
public:
  JointDescriptor();
  ~JointDescriptor();
  
  void  initialize(Renderer* pRenderer);
  void cleanUp(Renderer* pRenderer);
  void update(Renderer* pRenderer, U32 resourceIndex);  

  JointBuffer*  getJointData() { return &m_jointsData; }
  Buffer*       getNativeJointBuffer(U32 resourceIndex) { return m_pJointHandles[resourceIndex]._pBuf; }
  DescriptorSet*  getCurrJointSet(U32 resourceIndex) { return m_pJointHandles[resourceIndex]._pSet; }
  void          pushUpdate(B32 bits = JOINT_BUFFER_UPDATE_BIT) 
    { for(U32 i = 0; i < m_pJointHandles.size(); ++i)  m_pJointHandles[i]._updates |= bits; }

  void          updateJointSets(U32 resourceIndex);

  U32   numJoints() { return JointBuffer::kMaxNumberOfJointMatrices; }

private:
  std::vector<UpdateManager> m_pJointHandles;
  JointBuffer   m_jointsData;
  friend class  Renderer;
};
} // Recluse