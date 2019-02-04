// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Time.hpp"
#include "Collider.hpp"

namespace Recluse {


class BoxCollider : public Collider {
public:
  BoxCollider()
    : Collider(PHYSICS_COLLIDER_TYPE_BOX) { }


#if R_ENABLE_PHYSICS_DEBUG
private:
  static MeshData* kBoxMesh;
public:
  static void InitMeshDebugData();
  static void CleanUpMeshDebugData();

  MeshData* GetDebugMeshData() override { return kBoxMesh; }
#endif
};
} // Recluse