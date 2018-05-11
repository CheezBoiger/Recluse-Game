// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "PhysicsConfigs.hpp"


namespace Recluse {


class Actor;

typedef uuid64 collider_uuid_t;

class Collider : public PhysicsObject {
public:

  // Center point of this collider.
  Vector3     center;
};



class CompoundCollider : public Collider {
public:

};
} // Recluse