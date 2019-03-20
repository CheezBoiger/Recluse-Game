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
  virtual AudioId           createAudioObject() { return ~0u; }
  virtual AudioResult       freeAudioObject(AudioId id) { return Audio_Success; }

  virtual void              onStartUp() override { }
  virtual void              onShutDown() override { }

  virtual void              loadSound(const std::string& soundName, b32 is3D, b32 looping, b32 stream) {}
  virtual void              unLoadSound(const std::string& soundName) { }
  virtual u32               initiateSound(const std::string& soundName, const Vector3& pos, r32 volume) { return 0; }
  virtual void              SetChannel3DPosition(u32 nChannelId, const Vector3& pos, const Vector3& vel = Vector3()) { }
  virtual void              SetChannelVolume(u32 nChannelId, r32 volume) { }

  virtual void              updateState(r64 dt) { }

  virtual void              setListener3DOrientation(const Vector3& pos, const Vector3& forward, const Vector3& up) { }

};


Audio& gAudio();
} // Recluse