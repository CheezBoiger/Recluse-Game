// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "RigidBody.hpp"
#include "Core/Exception.hpp"
#include "Collision.hpp"
#include "Game/GameObject.hpp"
#include "Physics.hpp"


namespace Recluse {


uuid64 PhysicsObject::genIdx = 0;


void RigidBody::InvokeCollision(Collision* collision)
{
  R_ASSERT(_gameObj, "NULL game object assigned to this rigid body!");
  _gameObj->dispatchCollisionEvent(collision);
}
} // Recluse 