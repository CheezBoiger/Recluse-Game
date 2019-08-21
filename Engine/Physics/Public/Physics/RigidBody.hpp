// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"
#include "PhysicsConfigs.hpp"

#include "Collider.hpp"
#include "CompoundCollider.hpp"

#include <functional>


namespace Recluse {



class Collider;
struct Collision;
class GameObject;

enum ContactType {
  START_COLLISION,
  DURING_COLLISION,
  OFF_COLLISION
};


// RigidBody object create by Physics interface. Be sure to assign the proper
// game object id to this body, otherwise nullptr will be returned!
struct RigidBody : public PhysicsObject {
  RigidBody() 
    : _activated(true)
    , _gameObj(nullptr)
    , _kinematic(false)
    , _collider(nullptr)
    , _mass(1.0f)
    , _friction(0.0f)
    , _angleFactor(Vector3::ONE)
    , _linearFactor(Vector3::ONE) { }

  Vector3               _velocity;
  Vector3               _position;
  Vector3               _centerOfMass;
  Vector3               _linearFactor;
  Vector3               _angleFactor;
  R32                   _mass;
  R32                   _friction;
  R32                   _rollingFriction;
  R32                   _spinningFriction;
  Quaternion            _rotation;
  B32                   _kinematic;
  B32                   _activated;
  GameObject*           _gameObj;

  // TODO(): Need to separate compound to be optional instead.
  CompoundCollider      _compound;
  
  // The collider associated with this rigid body.
  Collider*               _collider;

  // forces that are cleared every update.
  Vector3                 _desiredVelocity;
  std::vector<Vector3>    _forces;
  std::vector<Vector3>    _impulses;
  std::vector<Vector3>    _forceRelativePositions;
  std::vector<Vector3>    _impulseRelativePositions;

public:
  friend class Physics;
};
} // Recluse