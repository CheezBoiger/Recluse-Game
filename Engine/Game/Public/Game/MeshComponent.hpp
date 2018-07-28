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
  Mesh() : m_pData(nullptr) 
          , m_bSkinned(false)
          , m_skeleId(Skeleton::kNoSkeletonId) { }

  // Initialize the mesh object.
  void                    Initialize(size_t elementCount, void* data, MeshData::VertexType type, 
                            size_t indexCount, void* indices, const Vector3& min = Vector3(), const Vector3& max = Vector3());

  // Clean up the mesh object when no longer being used.
  void                    CleanUp();

  MeshData*               Native() { return m_pData; }
  b32                     Skinned() { return m_bSkinned; }

  void                    SetSkeletonReference(skeleton_uuid_t uuid) { m_skeleId = uuid; }
  skeleton_uuid_t         GetSkeletonReference() const { return m_skeleId; }

  Primitive*              GetPrimitiveData() { return m_primitives.data(); }
  u32                     GetPrimitiveCount() const { return static_cast<u32>(m_primitives.size()); }
  Primitive*              GetPrimitive(u32 idx) { return &m_primitives[static_cast<u32>(idx)]; }
  inline void             ClearPrimitives() { m_primitives.clear(); }
  inline void             PushPrimitive(const Primitive& primitive) { m_primitives.push_back(primitive); }

private:
  MeshData*               m_pData;
  std::vector<Primitive>  m_primitives;
  b32                     m_bSkinned;
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