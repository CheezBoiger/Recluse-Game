// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Game/Engine.hpp"


using namespace Recluse;


namespace rs {


class IActor;
class PlayerActor;


// Camera used for player to look.
class PlayerCamera {
public:

  void SetTargetActor(IActor* actor) { m_actorTarget = actor; }

private:
  // Actor that this player camera looks through.
  IActor* m_actorTarget;

  // Camera object used by the engine.
  Camera* camera;

  r32     m_yaw;
  r32     m_pitch;
};
} // rs