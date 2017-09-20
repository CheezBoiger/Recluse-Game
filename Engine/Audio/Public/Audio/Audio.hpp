// Copyright (c) 2017 Recluse Project.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "AudioConfigs.hpp"


namespace Recluse {


class WwiseEngine;


// Audio Engine stuff.
class Audio : public EngineModule<Audio> {
public:
  Audio()
    : mWwise(nullptr) { }


  // TODO():
  void              OnStartUp() override;
  void              OnShutDown() override;

  void              UpdateState();

private:
  WwiseEngine*      mWwise;
};


Audio& gAudio();
} // Recluse