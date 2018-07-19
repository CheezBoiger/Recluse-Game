// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "WwiseEngine.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


AudioResult WwiseEngine::Initialize()
{
  AudioResult result;
  result = InitMemoryManager();
  if (result != Audio_Success) return result;
  result = InitStreamManager();
  if (result != Audio_Success) return result;
  result = InitSoundEngine();
  if (result != Audio_Success) return result;
  result = InitMusicEngine();
  if (result != Audio_Success) return result;
  R_DEBUG(rNotify, "Audio has been initialized.\n");
  return result;
}


AudioResult WwiseEngine::InitMusicEngine()
{
  AkMusicSettings musicSettings;
  AK::MusicEngine::GetDefaultInitSettings(musicSettings);
  if (AK::MusicEngine::Init(&musicSettings) != AK_Success) {
    R_DEBUG(rError, "Failed to initialize wwise music engine!\n");
    return Audio_Failed_MusicEngine;
  }
  return Audio_Success;
}


AudioResult WwiseEngine::InitSoundEngine()
{
  AkInitSettings initSettings;
  AkPlatformInitSettings platformInitSettings;
  AK::SoundEngine::GetDefaultInitSettings(initSettings);
  AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);

  AKRESULT result = AK::SoundEngine::Init(&initSettings, &platformInitSettings);
  if (result != AK_Success) {
    R_DEBUG(rError, "Failed to initialize wwise sound engine!\n");
    return Audio_Failed_SoundEngine;
  }
  return Audio_Success;
}


AudioResult WwiseEngine::InitStreamManager()
{
  AkStreamMgrSettings stmSettings;
  AK::StreamMgr::GetDefaultSettings(stmSettings);

  if (!AK::StreamMgr::Create(stmSettings)) {
    R_DEBUG(rError, "Failed to create audio stream manager!\n");
    return Audio_Failed_StreamManager;
  }

  AkDeviceSettings deviceSettings;
  AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

  // NOTE(): Perhaps one day, we can do some customizations to our streaming device...
  if (m_lowLevelFilePackage.Init(deviceSettings) != AK_Success) {
    R_DEBUG(rError, "Failed to create streaming device and low level I/O system.\n");
    return Audio_Failed_StreamManager;
  }
  return Audio_Success;
}


AudioResult WwiseEngine::InitMemoryManager()
{
  AkMemSettings memSettings;
  memSettings.uMaxNumPools = 20;

  if (AK::MemoryMgr::Init(&memSettings) != AK_Success) {
    R_DEBUG(rError, "Could not initialize AK memory manager!\n");
    return Audio_Failed_MemoryManager;
  }
  return Audio_Success;
}


void WwiseEngine::CleanUp()
{
  // TODO:
  AK::MusicEngine::Term();
  AK::SoundEngine::Term();

  m_lowLevelFilePackage.Term();

  if ( AK::IAkStreamMgr::Get() ) {
    AK::IAkStreamMgr::Get()->Destroy();
  }
  
  
  // Terminate the memory manager when done.
  AK::MemoryMgr::Term();
}


void WwiseEngine::Update(r64 dt)
{
}
} // Recluse