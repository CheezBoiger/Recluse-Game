// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Core.hpp"

#include "Renderer/MeshData.hpp"
#include "Renderer/Vertex.hpp"
#include "Renderer/Mesh.hpp"


namespace Recluse {


struct physics_configs_t {
  Vector3       _vGravity;
};


typedef uuid64 physics_uuid_t;


class PhysicsObject {
  static physics_uuid_t genIdx;
public:
  PhysicsObject()
    : m_uuid(genIdx++) { }

  virtual ~PhysicsObject() { }

  physics_uuid_t                GetUUID() const { return m_uuid; }

  virtual MeshData*  GetDebugMeshData() { return nullptr;  }

  virtual BasicDebugRenderCmd GetRenderCmd() { return BasicDebugRenderCmd(); }

private:
  physics_uuid_t                m_uuid;
};
} // Recluse