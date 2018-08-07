// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "MeshComponent.hpp"
#include "GameObject.hpp"
#include "Engine.hpp"
#include "Camera.hpp"

#include "Renderer/MeshData.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Utility/Profile.hpp"

namespace Recluse {


DEFINE_COMPONENT_MAP(MeshComponent);


MeshComponent::MeshComponent()
  : m_allowCulling(true)
  , m_frustumCull(0)
  , m_pMeshRef(nullptr)
{
}


void Mesh::InitializeLod(size_t elementCount, void* data, MeshData::VertexType type, u32 lod, 
  size_t indexCount, void* indices)
{
  R_ASSERT(!m_pMeshDataLod[lod], "Mesh data at specified lod is already initialized.");
  m_pMeshDataLod[lod] = gRenderer().CreateMeshData();
  m_pMeshDataLod[lod]->Initialize(elementCount, data, type, indexCount, indices);
  if (type == MeshData::VertexType::SKINNED) m_bSkinned = true;
}


void Mesh::CleanUp()
{
  for (size_t i = 0; i < kMaxMeshLodWidth; ++i) {
    if (m_pMeshDataLod[i]) {
      gRenderer().FreeMeshData(m_pMeshDataLod[i]);
      m_pMeshDataLod[i] = nullptr;
    }
  }
}


void MeshComponent::OnInitialize(GameObject* owner)
{
  REGISTER_COMPONENT(MeshComponent, this);
}


void MeshComponent::OnCleanUp()
{
  UNREGISTER_COMPONENT(MeshComponent);
}


void MeshComponent::Update()
{
  R_TIMED_PROFILE_GAME();

  if (!m_pMeshRef) return;
  UpdateFrustumCullBits();
}


void MeshComponent::UpdateFrustumCullBits()
{
  if (!AllowCulling()) return;

  size_t viewFrustumCount = gEngine().GetViewFrustumCount();
  if (viewFrustumCount == 0) return;

  ViewFrustum** viewFrustums = gEngine().GetViewFrustums();
  AABB aabb = m_pMeshRef->GetAABB();
  Transform* transform = GetOwner()->GetTransform();

  aabb.max = (aabb.max * transform->Scale) + transform->Position;
  aabb.min = (aabb.min * transform->Scale) + transform->Position;
  aabb.ComputeCentroid();

  ClearFrustumCullBits();

  for (size_t i = 0; i < viewFrustumCount; ++i) {
    ViewFrustum* viewFrustum = viewFrustums[i];
    ViewFrustum::Result intersects = viewFrustum->Intersect(aabb);
    if (intersects != ViewFrustum::Result_Outside) {
      m_frustumCull = m_frustumCull | (1 << i);
    }
  }
}
} // Recluse