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
  void            Initialize(size_t elementCount, void* data, MeshData::VertexType type, 
                    size_t indexCount, void* indices, const Vector3& min = Vector3(), const Vector3& max = Vector3());

  // Clean up the mesh object when no longer being used.
  void            CleanUp();

  MeshData*       Native() { return m_pData; }
  b32             Skinned() { return m_bSkinned; }

  void            SetSkeletonReference(skeleton_uuid_t uuid) { m_skeleId = uuid; }
  skeleton_uuid_t GetSkeletonReference() const { return m_skeleId; }

private:
  MeshData*       m_pData;
  b32             m_bSkinned;
  skeleton_uuid_t m_skeleId;
};


// Mesh Component holds a reference to mesh objects that are loaded.
class MeshComponent : public Component {
  RCOMPONENT(MeshComponent);
protected:
  virtual void OnInitialize(GameObject* owner) override { }
  virtual void OnCleanUp() override { }
public:

  void SetMeshRef(Mesh* pData) { m_pMeshRef = pData; }
  
  Mesh* MeshRef() { return m_pMeshRef; }

private:
  Mesh*       m_pMeshRef;
};
} // Recluse