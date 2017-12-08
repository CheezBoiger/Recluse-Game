// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Game/GameObject.hpp"
#include "AudioConfigs.hpp"


namespace Recluse {


class WwiseEngine;


// Audio Engine stuff.
class Audio : public EngineModule<Audio> {
public:
  Audio()
    : mWwise(nullptr) { }


  // TODO():
  void              AddGameObject(game_uuid_t uuid);
  void              RemoveGameObject(game_uuid_t uuid);

  void              OnStartUp() override;
  void              OnShutDown() override;

  void              UpdateState(r64 dt);

private:
  WwiseEngine*      mWwise;
};


Audio& gAudio();
} // Recluse