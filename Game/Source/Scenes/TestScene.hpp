// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once


#include "Game/Engine.hpp"
#include "Game/Camera.hpp"
#include "Game/Scene/Scene.hpp"

#include "../Playable/Player.hpp"

using namespace Recluse;


class TestScene : public Scene {
public:

  void SetUp() override 
  {
  }


  void Update(r32 tick) override
  {
  }
  

  Player* pPlayer;
};