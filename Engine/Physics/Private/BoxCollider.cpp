// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "BoxCollider.hpp"
#include "Core/Exception.hpp"

#include "Game/Geometry/Cube.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/MeshData.hpp"

namespace Recluse {

MeshData* BoxCollider::kBoxMesh = nullptr;

void BoxCollider::InitMeshDebugData()
{
  if (!gRenderer().IsActive()) {
    R_DEBUG(rWarning, "Sphere Collider mesh data can not be initialized, renderer is not active!\n");
    return;
  }

  auto vertices = Cube::MeshInstance(1.0f);
  auto indices = Cube::IndicesInstance();

  kBoxMesh = new MeshData();
  kBoxMesh->Initialize(&gRenderer(), vertices.size(), vertices.data(), sizeof(StaticVertex), 
    indices.size(), indices.data());
}


void BoxCollider::CleanUpMeshDebugData()
{
  if (kBoxMesh) {
    kBoxMesh->CleanUp(&gRenderer());
    delete kBoxMesh;
    kBoxMesh = nullptr;
  }
}
} // Recluse