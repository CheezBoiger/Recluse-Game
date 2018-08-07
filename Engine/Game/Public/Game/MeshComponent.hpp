// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Game/Component.hpp"
#include "Core/Math/AABB.hpp"
#include "Renderer/MeshData.hpp"

#include "Animation/Skeleton.hpp"


namespace Recluse {


class MeshData;

// A Single instance of a mesh stored in gpu memory.
class Mesh {
public:
  static const u32 kMaxMeshLodWidth = 5u;
  static const u32 kMeshLodZero = 0u;

  Mesh() :  m_bSkinned(false)
         ,  m_skeleId(Skeleton::kNoSkeletonId) 
         ,  m_pMeshDataLod{nullptr}
  {
  }

  // Initialize the mesh object.
  // Element count is the number of vertices in data. numOfVertices objects in data.
  // VertexType determines what type of vertex data is, and lod is the level of detail mapped to be mapped
  // to this initialized data.
  void                    InitializeLod(size_t elementCount, void* data, MeshData::VertexType type, u32 lod = kMeshLodZero,
                            size_t indexCount = 0, void* indices = nullptr);

  // Clean up the mesh object when no longer being used.
  void                    CleanUp();

  MeshData*               GetMeshDataLod(u32 lod = kMeshLodZero) { return m_pMeshDataLod[lod]; }
  b32                     Skinned() { return m_bSkinned; }

  void                    SetSkeletonReference(skeleton_uuid_t uuid) { m_skeleId = uuid; }
  skeleton_uuid_t         GetSkeletonReference() const { return m_skeleId; }

  Primitive*              GetPrimitiveData(u32 lod = kMeshLodZero) { return m_pMeshDataLod[lod]->GetPrimitiveData(); }
  u32                     GetPrimitiveCount(u32 lod = kMeshLodZero) const { return m_pMeshDataLod[lod]->GetPrimitiveCount(); }
  Primitive*              GetPrimitive(u32 idx, u32 lod = kMeshLodZero) { return m_pMeshDataLod[lod]->GetPrimitive(idx); }
  inline void             ClearPrimitives(u32 lod = kMeshLodZero) { m_pMeshDataLod[lod]->ClearPrimitives(); }
  inline void             PushPrimitive(const Primitive& primitive, u32 lod = kMeshLodZero) { m_pMeshDataLod[lod]->PushPrimitive(primitive); }

  void                    SetMin(const Vector3& min) { m_aabb.min = min; }
  void                    SetMax(const Vector3& max) { m_aabb.max = max; }

  void                    UpdateAABB() { m_aabb.ComputeCentroid(); m_aabb.ComputeSurfaceArea(); }
  const AABB&             GetAABB() const { return m_aabb; }

private:
  MeshData*               m_pMeshDataLod[5];
  b32                     m_bSkinned;
  skeleton_uuid_t         m_skeleId;
  AABB                    m_aabb;
};


// Mesh Component holds a reference to mesh objects that are loaded.
class MeshComponent : public Component {
  RCOMPONENT(MeshComponent);
protected:
  virtual void    OnInitialize(GameObject* owner) override;
  virtual void    OnCleanUp() override;
public:
  MeshComponent();

  void            SetMeshRef(Mesh* pData) { m_pMeshRef = pData; }
  
  Mesh*           MeshRef() { return m_pMeshRef; }

  // updates this mesh component instance frustum bit cull.
  void            Update() override;

  void            EnableCulling(b32 enable) { m_allowCulling = enable; }

  inline b32      AllowCulling() const { return m_allowCulling; }

  // Cull bit set map. Each bit represents a frustum, and when flipped, means this mesh is culled
  // for that given frustum. Max 32 frustums can be supported.
  b32             GetFrustumCullMap() const { return m_frustumCull; }

  // Set a frustum bit to 1.
  void            SetFrustumCull(b32 mask) { m_frustumCull |= mask; }

  // Unset a frustum bit to 0.
  void            UnsetFrustumCull(b32 mask) { m_frustumCull &= ~mask; }

  // Clear and reset all frustum bits to 0.
  void            ClearFrustumCullBits() { m_frustumCull &= 0; }

private:

  void            UpdateFrustumCullBits();

  Mesh*           m_pMeshRef;
  b32             m_frustumCull;
  b32             m_allowCulling;

};
} // Recluse