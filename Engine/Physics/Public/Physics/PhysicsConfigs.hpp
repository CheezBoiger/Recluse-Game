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


class PhysicsObject {
  static uuid64 genIdx;
public:
  PhysicsObject()
    : m_uuid(genIdx++) { }

  virtual ~PhysicsObject() { }

  uuid64                GetUUID() const { return m_uuid; }
private:
  uuid64                m_uuid;
};
} // Recluse