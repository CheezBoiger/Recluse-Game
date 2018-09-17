// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Core/Math/AABB.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Utility/Vector.hpp"

namespace Recluse {


class VulkanRHI;
class MaterialDescriptor;
class MeshData;


struct Primitive {
  Primitive()
    : _firstIndex(0)
    , _indexCount(0)
    , _pMat(nullptr) { }

  MaterialDescriptor*   _pMat;
  u32                   _firstIndex;
  u32                   _indexCount;
};

// Mesh data represents data, in the form of gpu friendly buffers, to which we draw onto the 
// frame. We use mesh data to represent the model we are drawing.
class MeshData {
  static const size_t kMaxLodMeshCount = 4;
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
  void            CleanUp();

  VertexBuffer*   VertexData() { return &m_vertexBuffer; }
  
  IndexBuffer*    IndexData() { 
    if (m_indexBuffer.IndexCount() > 0) return &m_indexBuffer; 
    else return nullptr; 
  }

  Primitive*              GetPrimitiveData() { return m_primitives.data(); }
  u32                     GetPrimitiveCount() const { return static_cast<u32>(m_primitives.size()); }
  Primitive*              GetPrimitive(u32 idx) { return &m_primitives[static_cast<size_t>(idx)]; }
  inline void             ClearPrimitives() { m_primitives.clear(); }
  inline void             PushPrimitive(const Primitive& primitive) { m_primitives.push_back(primitive); }

  // Optimize the mesh to a certain specification.
  void                    Optimize();

  // Compress mesh data to become optimal by lod.
  void                    CompressByLod(u32 lod);

private:
  VertexBuffer                        m_vertexBuffer;
  IndexBuffer                         m_indexBuffer;
  std::vector<Primitive>              m_primitives;
  VulkanRHI*                          mRhi;
  friend class Renderer;
};
} // Recluse