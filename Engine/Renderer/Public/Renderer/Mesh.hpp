// Copyright (C) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/AABB.hpp"
#include "Material.hpp"
#include "Animation/Skeleton.hpp"


namespace Recluse {


class MeshData;

struct Primitive {
  Primitive()
    : _firstIndex(0)
    , _indexCount(0)
    , _pMat(nullptr) { }

  Material*             _pMat;
  u32                   _firstIndex;
  u32                   _indexCount;
};

// A Single instance of a mesh stored in gpu memory.
class Mesh {
public:
  enum SortType {
    TRANSPARENCY_LAST,
    TRANSPARENCY_FIRST,
    START_INDEX_GREATEST,
    START_INDEX_LEAST
  };

  enum VertexType {
    STATIC,
    SKINNED,
    QUAD
  }; 

  static const u32 kMaxMeshLodWidth = 5u;
  static const u32 kMeshLodZero = 0u;

  Mesh() :  m_bSkinned(false)
         ,  m_skeleId(Skeleton::kNoSkeletonId) 
         ,  m_pMeshData{nullptr}
  {
  }

  // Initialize the mesh object.
  // Element count is the number of vertices in data. numOfVertices objects in data.
  // VertexType determines what type of vertex data is, and lod is the level of detail mapped to be mapped
  // to this initialized data.
  void                    Initialize(size_t elementCount, void* data, VertexType  type,
                            size_t indexCount = 0, void* indices = nullptr);

  // Clean up the mesh object when no longer being used.
  void                    CleanUp();

  MeshData*               GetMeshData() { return m_pMeshData; }
  b32                     Skinned() { return m_bSkinned; }

  void                    SetSkeletonReference(skeleton_uuid_t uuid) { m_skeleId = uuid; }
  skeleton_uuid_t         GetSkeletonReference() const { return m_skeleId; }

  Primitive*              GetPrimitiveData() { return m_primitives.data(); }
  u32                     GetPrimitiveCount() const { return m_primitives.size(); }
  Primitive*              GetPrimitive(u32 idx) { return &m_primitives[idx]; }
  inline void             ClearPrimitives(u32 lod = kMeshLodZero) { m_primitives.clear(); }
  inline void             PushPrimitive(const Primitive& primitive) { m_primitives.push_back(primitive); }

  void                    SetMin(const Vector3& min) { m_aabb.min = min; }
  void                    SetMax(const Vector3& max) { m_aabb.max = max; }

  void                    UpdateAABB() { m_aabb.ComputeCentroid(); m_aabb.ComputeSurfaceArea(); }
  const AABB&             GetAABB() const { return m_aabb; }

  // Sort primitives based on the given types, determining that algorithm to use for the primitive list of 
  // this mesh object.
  void                    SortPrimitives(SortType type);

private:
  MeshData*               m_pMeshData;
  std::vector<Primitive>  m_primitives;
  b32                     m_bSkinned;
  skeleton_uuid_t         m_skeleId;
  AABB                    m_aabb;
};
} // Recluse 