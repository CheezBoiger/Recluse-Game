// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Game/GameObject.hpp"
#include "AudioConfigs.hpp"


namespace Recluse {


class AudioEngine;


enum AudioResult {
  Audio_Success,
  Audio_Fail
};

typedef uuid64 AudioId;

// Audio Engine stuff.
class Audio : public EngineModule<Audio> {
public:
  virtual ~Audio() { }

  // TODO():
  virtual AudioId           CreateAudioObject() { return ~0u; }
  virtual AudioResult       FreeAudioObject(AudioId id) { return Audio_Success; }

  virtual void              OnStartUp() override { }
  virtual void              OnShutDown() override { }

  virtual void              UpdateState(r64 dt) { }
};


Audio& gAudio();
} // Recluse