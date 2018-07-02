// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Core/Math/AABB.hpp"
#include "Core/Math/Vector3.hpp"


namespace Recluse {


class VulkanRHI;
class MaterialDescriptor;
class MeshData;


struct Primitive {
  Primitive()
    : _firstIndex(0)
    , _indexCount(0)
    , _pMesh(nullptr)
    , _pMat(nullptr) { }

  MeshData*             _pMesh;
  MaterialDescriptor*   _pMat;
  u32                   _firstIndex;
  u32                   _indexCount;
};

// Mesh data represents data, in the form of gpu friendly buffers, to which we draw onto the 
// frame. We use mesh data to represent the model we are drawing.
class MeshData {
public:
  enum VertexType {
    QUAD,
    STATIC,
    SKINNED
  };

  MeshData();
  ~MeshData();

  void            Initialize(size_t elementCount, void* data, VertexType type = STATIC,
    size_t indexCount = 0, void* indices = nullptr);
  void            SetMin(const Vector3& min) { m_aabb.min = min; }
  void            SetMax(const Vector3& max) { m_aabb.max = max; }

  void            CleanUp();

  VertexBuffer*   VertexData() { return &mVertexBuffer; }
  
  IndexBuffer*    IndexData() { 
    if (mIndexBuffer.IndexCount() > 0) return &mIndexBuffer; 
    else return nullptr; 
  }

  void            UpdateAABB() { m_aabb.ComputeCentroid(); m_aabb.ComputeSurfaceArea(); }

  const AABB&     GetAABB() const { return m_aabb; }
private:
  VertexBuffer    mVertexBuffer;
  IndexBuffer     mIndexBuffer;
  VulkanRHI*      mRhi;
  AABB            m_aabb;

  friend class Renderer;
};
} // Recluse