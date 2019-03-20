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

void MeshComponent::onInitialize(GameObject* owner)
{
  REGISTER_COMPONENT(MeshComponent, this);
}


void MeshComponent::onCleanUp()
{
  UNREGISTER_COMPONENT(MeshComponent);
}


void MeshComponent::update()
{
  R_TIMED_PROFILE_GAME();

  if (!m_pMeshRef) return;
  UpdateFrustumCullBits();
}


void MeshComponent::UpdateFrustumCullBits()
{
  if (!AllowCulling()) return;

  size_t viewFrustumCount = gEngine().getViewFrustumCount();
  if (viewFrustumCount == 0) return;

  ViewFrustum** viewFrustums = gEngine().getViewFrustums();
  AABB aabb = m_pMeshRef->getAABB();
  Transform* transform = getOwner()->getTransform();

  aabb.max = (aabb.max * transform->_scale) + transform->_position;
  aabb.min = (aabb.min * transform->_scale) + transform->_position;
  aabb.computeCentroid();

  ClearFrustumCullBits();

  for (size_t i = 0; i < viewFrustumCount; ++i) {
    ViewFrustum* viewFrustum = viewFrustums[i];
    ViewFrustum::Result intersects = viewFrustum->intersect(aabb);
    if (intersects != ViewFrustum::Result_Outside) {
      m_frustumCull = m_frustumCull | (1 << i);
    }
  }
}
} // Recluse