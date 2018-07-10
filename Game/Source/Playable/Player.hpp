// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "IActor.hpp"

#include "../Camera/PlayerCamera.hpp"


namespace rs {


// Player script. User controls are managed by this class.
class Player : public IActor {
  R_GAME_OBJECT(Player)
public:

  virtual void OnStart() override
  {
    if (m_playerCamera) {
      m_playerCamera->SetTargetActor(this);
    }
  }

  virtual void Update(r32 tick) override
  {
  }


  virtual void OnCleanUp() override
  {
  }

  
  virtual void OnCollision(Collision* collision) override
  {
  }

private:

  PlayerCamera* m_playerCamera;
};
} // rs