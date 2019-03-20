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
  if (!gRenderer().isActive()) {
    R_DEBUG(rWarning, "Sphere Collider mesh data can not be initialized, renderer is not active!\n");
    return;
  }

  auto vertices = Cube::meshInstance(1.0f);
  auto indices = Cube::indicesInstance();

  kBoxMesh = new MeshData();
  kBoxMesh->initialize(&gRenderer(), vertices.size(), vertices.data(), sizeof(StaticVertex), 
    indices.size(), indices.data());
}


void BoxCollider::CleanUpMeshDebugData()
{
  if (kBoxMesh) {
    kBoxMesh->cleanUp(&gRenderer());
    delete kBoxMesh;
    kBoxMesh = nullptr;
  }
}
} // Recluse