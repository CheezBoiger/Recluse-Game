// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"

#include "Core/Math/Vector3.hpp"
#include "Core/Types.hpp"


namespace Recluse {


class SphereCollider : public Collider {
  static MeshData* kSphereMesh;
public:
  SphereCollider(R32 radius = 0.0f)
    : Collider(PHYSICS_COLLIDER_TYPE_SPHERE) 
    , m_radius(radius) { }

  R32             GetRadius() const { return m_radius; }

  void            SetRadius(R32 radius);

  static void InitMeshDebugData();
  static void CleanUpMeshDebugData();

  MeshData* getDebugMeshData() override { return kSphereMesh; }

private:
  R32 m_radius;
};
} // Recluse