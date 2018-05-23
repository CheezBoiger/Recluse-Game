// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Utility/Time.hpp"

namespace Recluse {

class GameObject;
class RigidBody;

class CollisionShape {
};


struct ContactPoint {
  Vector3 _point;
  Vector3 _normal;
  r32     _distance;
};


struct Collision {
  GameObject*                             _gameObject; // Game object we collided with.
  RigidBody*                              _rigidBody;   // Body we collisded with.
  std::vector<ContactPoint>               _contactPoints;
};
} // Recluse