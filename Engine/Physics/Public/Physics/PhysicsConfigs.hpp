// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Core.hpp"


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
private:
  physics_uuid_t                m_uuid;
};
} // Recluse