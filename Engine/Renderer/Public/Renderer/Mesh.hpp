// Copyright (C) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/AABB.hpp"
#include "Material.hpp"
#include "Animation/Skeleton.hpp"
#include "RenderCmd.hpp"

namespace Recluse {


class MeshData;
class MorphTarget;
class Renderer;

struct Primitive {
  Primitive()
    : _firstIndex(0)
    , _indexCount(0)
    , _pMat(nullptr) { }

  Material*             _pMat;
  u32                   _firstIndex;
  u32                   _indexCount;
  CmdConfigBits         _localConfigs;
  AABB                  _aabb;
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

  Mesh() : m_bSkinned(false)
         , m_skeleId(Skeleton::kNoSkeletonId) 
         , m_pMeshData{nullptr}
  {
  }

  // Initialize the mesh object.
  // Element count is the number of vertices in data. numOfVertices objects in data.
  // VertexType determines what type of vertex data is, and lod is the level of detail mapped to be mapped
  // to this initialized data.
  void initialize(Renderer* pRenderer, 
                  size_t elementCount, 
                  void* data, 
                  VertexType  type,
                  size_t indexCount = 0, 
                  void* indices = nullptr);

  // Clean up the mesh object when no longer being used.size
  void cleanUp(Renderer* pRenderer);

  MeshData* getMeshData() { return m_pMeshData; }
  b32 isSkinned() { return m_bSkinned; }

  void setSkeletonReference(skeleton_uuid_t uuid) { m_skeleId = uuid; }
  skeleton_uuid_t getSkeletonReference() const { return m_skeleId; }

  Primitive* getPrimitiveData() { return m_primitives.data(); }
  u32 getPrimitiveCount() const { return static_cast<u32>(m_primitives.size()); }
  Primitive* getPrimitive(u32 idx) { return &m_primitives[idx]; }
  inline void clearPrimitives(u32 lod = kMeshLodZero) { m_primitives.clear(); }
  inline void pushPrimitive(const Primitive& primitive) { m_primitives.push_back(primitive); }

  void allocateMorphTargetBuffer(size_t newsize);
  MorphTarget* getMorphTarget(size_t idx) { return m_morphTargets[idx]; }
  u32 getMorphTargetCount() const { return static_cast<u32>(m_morphTargets.size()); }
  void initializeMorphTarget(Renderer* pRenderer,
                              size_t idx, 
                              size_t elementCount, 
                              void* data, 
                              size_t vertexSize);

  void setMin(const Vector3& min) { m_aabb.min = min; }
  void setMax(const Vector3& max) { m_aabb.max = max; }

  void updateAABB() { m_aabb.computeCentroid(); m_aabb.computeSurfaceArea(); }
  const AABB& getAABB() const { return m_aabb; }

  // Sort primitives based on the given types, determining that algorithm to use for the primitive list of 
  // this mesh object.
  void sortPrimitives(SortType type);

private:

  void clearMorphTargets(Renderer* pRenderer);

  // The actual mesh data used to bound and render.
  MeshData* m_pMeshData;

  // Primitives that correspond to this mesh.
  std::vector<Primitive> m_primitives;

  // Morph Targets that correspond to this mesh.
  std::vector<MorphTarget*> m_morphTargets;

  // skinned boolean.
  b32 m_bSkinned;

  // skeleton id reference.
  skeleton_uuid_t m_skeleId;

  // Bounding shape of this mesh.
  AABB m_aabb;
};
} // Recluse 