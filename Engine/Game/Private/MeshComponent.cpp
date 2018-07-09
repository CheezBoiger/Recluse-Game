// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "MeshComponent.hpp"
#include "GameObject.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Renderer.hpp"
#include "Engine.hpp"

namespace Recluse {


DEFINE_COMPONENT_MAP(MeshComponent);


MeshComponent::MeshComponent()
  : m_allowCulling(true)
  , m_frustumCull(0)
  , m_pMeshRef(nullptr)
{
}


void Mesh::Initialize(size_t elementCount, void* data, MeshData::VertexType type,size_t indexCount, 
  void* indices, const Vector3& min, const Vector3& max)
{
  m_pData = gRenderer().CreateMeshData();
  m_pData->Initialize(elementCount, data, type, indexCount, indices);
  m_pData->SetMin(min);
  m_pData->SetMax(max);
  m_pData->UpdateAABB();
  if (type == MeshData::VertexType::SKINNED) m_bSkinned = true;
}


void Mesh::CleanUp()
{
  gRenderer().FreeMeshData(m_pData);
  m_pData = nullptr;
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
  if (!AllowCulling()) return;
  if (!m_pMeshRef) return;

  size_t viewFrustumCount = gEngine().GetViewFrustumCount();
  if (viewFrustumCount == 0) return;

  ViewFrustum** viewFrustums = gEngine().GetViewFrustums();
  AABB aabb = m_pMeshRef->Native()->GetAABB();
  Matrix4 model = GetOwner()->GetTransform()->GetLocalToWorldMatrix();

  aabb.max = aabb.max * model;
  aabb.min = aabb.min * model;
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