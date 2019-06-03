// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Win32/Win32Configs.hpp"
#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"

#include "Audio/Audio.hpp"


#if RECLUSE_AUDIO_TYPE == RECLUSE_TYPE_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include <map>
#include <vector>



namespace Recluse {


class FMODAudioEngine : public Audio {
  typedef std::map<std::string, FMOD::Sound*>                   SoundMap;
  typedef std::map<u32, FMOD::Channel*>                         ChannelMap;
  typedef std::map<std::string, FMOD::Studio::EventInstance*>   EventMap;
  typedef std::map<std::string, FMOD::Studio::Bank*>            BankMap;
public:
  FMODAudioEngine()
    : m_pSystem(nullptr)
    , m_pStudioSystem(nullptr)
    , m_nextChannelId(0) { }

  virtual void onStartUp() override { initialize(); }
  virtual void onShutDown() override { cleanUp(); }
  AudioResult initialize();
  void cleanUp();


  AudioId createAudioObject() override;

  virtual void updateState(r64 dt) override;

  virtual void loadSound(const std::string& soundName, b32 is3D = true, b32 looping = false, b32 stream = false) override;
  virtual void unLoadSound(const std::string& soundName) override;
  virtual u32 initiateSound(const std::string& soundName, const Vector3& pos, r32 volume) override;
  virtual void setChannel3DPosition(u32 nChannelId, const Vector3& pos, const Vector3& vel) override;
  virtual void setChannelVolume(u32 nChannelId, r32 volume) override;
  virtual void loadEventBank(const std::string& bankPath) override;
    

  virtual void setListener3DOrientation(const Vector3& pos, const Vector3& forward, const Vector3& up) override;

private:
  SoundMap m_soundMap;
  ChannelMap m_channelMap;
  EventMap m_eventMap;
  BankMap m_bankMap;
  FMOD::System* m_pSystem;
  FMOD::Studio::System* m_pStudioSystem;
  u32 m_nextChannelId;
  Vector3 m_globalListenerPosition;
  Vector3 m_globalListenerForward;
  Vector3 m_globalListenerUp;
};
} // Recluse
#endif