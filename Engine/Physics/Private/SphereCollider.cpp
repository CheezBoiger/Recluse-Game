// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "SphereCollider.hpp"
#include "Core/Exception.hpp"


#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/Renderer.hpp"

namespace Recluse {

MeshData* SphereCollider::kSphereMesh = nullptr;


void SphereCollider::InitMeshDebugData()
{
  if (!gRenderer().isActive()) {
    R_DEBUG(rWarning, "Sphere Collider mesh data can not be initialized, renderer is not active!\n");
    return;
  } 

  kSphereMesh = new MeshData();
  auto vertices = UVSphere::meshInstance(1.0f, 32, 32);
  auto indices = UVSphere::indicesInstance(static_cast<U32>(vertices.size()), 32, 32);
  kSphereMesh->initialize(&gRenderer(), vertices.size(), 
    vertices.data(), sizeof(StaticVertex), indices.size(), indices.data()); 
}


void SphereCollider::CleanUpMeshDebugData()
{
  if (kSphereMesh) {
    kSphereMesh->cleanUp(&gRenderer());
    delete kSphereMesh;
    kSphereMesh = nullptr;
  }
}


void SphereCollider::SetRadius(R32 radius)
{
  SetDirty();
  m_radius = radius;
}
} // Recluse