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
  Mesh() :  m_bSkinned(false)
          , m_currLod(MESH_LOD_0)
          , m_skeleId(Skeleton::kNoSkeletonId) { }

  // Initialize the mesh object.
  void                    Initialize(MeshLod lod, size_t elementCount, void* data, MeshData::VertexType type, 
                            size_t indexCount, void* indices, const Vector3& min = Vector3(), const Vector3& max = Vector3());

  // Clean up the mesh object when no longer being used.
  void                    CleanUp();

  MeshData*               Native() { return m_pMeshData; }
  b32                     Skinned() { return m_bSkinned; }

  void                    SetSkeletonReference(skeleton_uuid_t uuid) { m_skeleId = uuid; }
  skeleton_uuid_t         GetSkeletonReference() const { return m_skeleId; }

  Primitive*              GetPrimitiveData(MeshLod lod = MESH_LOD_0) { return m_pMeshData->GetPrimitiveData(lod); }
  u32                     GetPrimitiveCount(MeshLod lod = MESH_LOD_0) const { return m_pMeshData->GetPrimitiveCount(lod); }
  Primitive*              GetPrimitive(MeshLod lod, u32 idx) { return m_pMeshData->GetPrimitive(lod, idx); }
  inline void             ClearPrimitives(MeshLod lod) { m_pMeshData->ClearPrimitives(lod); }
  inline void             PushPrimitive(MeshLod lod, const Primitive& primitive) { m_pMeshData->PushPrimitive(lod, primitive); }

  MeshLod                 GetCurrentLod() const { return m_currLod; }

private:
  MeshData*               m_pMeshData;
  b32                     m_bSkinned;
  MeshLod                 m_currLod;
  skeleton_uuid_t         m_skeleId;
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
  Mesh*           m_pMeshRef;
  b32             m_frustumCull;
  b32             m_allowCulling;
};
} // Recluse