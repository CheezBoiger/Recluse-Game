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


enum MeshLod {
  MESH_LOD_0 = 0,
  MESH_LOD_1 = 1,
  MESH_LOD_2 = 2,
  MESH_LOD_3 = 3
};


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

  void            Initialize(MeshLod lod, size_t elementCount, void* data, VertexType type = STATIC,
    size_t indexCount = 0, void* indices = nullptr);
  void            SetMin(const Vector3& min) { m_aabb.min = min; }
  void            SetMax(const Vector3& max) { m_aabb.max = max; }

  void            CleanUp();

  VertexBuffer*   VertexData(MeshLod lod) { return &m_vertexBuffersLod[lod]; }
  
  IndexBuffer*    IndexData(MeshLod lod) { 
    if (m_indexBuffersLod[lod].IndexCount() > 0) return &m_indexBuffersLod[lod]; 
    else return nullptr; 
  }

  void            UpdateAABB() { m_aabb.ComputeCentroid(); m_aabb.ComputeSurfaceArea(); }

  const AABB&     GetAABB() const { return m_aabb; }

  Primitive*              GetPrimitiveData(MeshLod lod) { return m_primitivesLod[lod].data(); }
  u32                     GetPrimitiveCount(MeshLod lod) const { return static_cast<u32>(m_primitivesLod[lod].size()); }
  Primitive*              GetPrimitive(MeshLod lod, u32 idx) { return &m_primitivesLod[lod][static_cast<u32>(idx)]; }
  inline void             ClearPrimitives(MeshLod lod) { m_primitivesLod[lod].clear(); }
  inline void             PushPrimitive(MeshLod lod, const Primitive& primitive) { m_primitivesLod[lod].push_back(primitive); }

  // Optimize the mesh to a certain specification.
  void                    Optimize();

  // Compress mesh data to become optimal by lod.
  void                    CompressByLod(u32 lod);

private:
  VertexBuffer                        m_vertexBuffersLod[kMaxLodMeshCount];
  IndexBuffer                         m_indexBuffersLod[kMaxLodMeshCount];
  std::vector<Primitive>              m_primitivesLod[kMaxLodMeshCount];
  VulkanRHI*                          mRhi;
  AABB                                m_aabb;
  friend class Renderer;
};
} // Recluse