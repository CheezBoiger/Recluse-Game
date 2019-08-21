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

#define R_ENABLE_PHYSICS_DEBUG 1


namespace Recluse {


struct physics_configs_t {
  Vector3       _vGravity;
};


typedef UUID64 physics_uuid_t;


class PhysicsObject {
  static physics_uuid_t genIdx;
public:
  PhysicsObject()
    : m_uuid(genIdx++) { }

  virtual ~PhysicsObject() { }

  physics_uuid_t                getUUID() const { return m_uuid; }

  virtual MeshData*  getDebugMeshData() { return nullptr;  }

  virtual SimpleRenderCmd getRenderCmd() { return SimpleRenderCmd(); }

private:
  physics_uuid_t                m_uuid;
};
} // Recluse