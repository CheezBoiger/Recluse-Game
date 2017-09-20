// Copyright (c) 2017 Recluse Project.
#include "WwiseEngine.hpp"
#include "Core/Exception.hpp"


namespace Recluse {





b8 WwiseEngine::Initialize()
{
  AkMemSettings memSettings;
  memSettings.uMaxNumPools = 20;

  if (AK::MemoryMgr::Init(&memSettings) != AK_Success) {
    R_DEBUG("ERROR: Could not initialize AK memory manager!\n");
    return false;
  }

  AkStreamMgrSettings stmSettings;
  AK::StreamMgr::GetDefaultSettings(stmSettings);

  if (!AK::StreamMgr::Create(stmSettings)) {
    R_DEBUG("ERROR: Failed to create audio stream manager!\n");
    return false;
  }

  AkDeviceSettings deviceSettings;
  AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

  // NOTE(): Perhaps one day, we can do some customizations to our streaming device...
  if (mLowLevelFilePackage.Init(deviceSettings) != AK_Success) {
    R_DEBUG("ERROR: Failed to create streaming device and low level I/O system.\n");
    return false;
  }

  AkInitSettings initSettings;
  AkPlatformInitSettings platformInitSettings;
  AK::SoundEngine::GetDefaultInitSettings(initSettings);
  AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);

  if (AK::SoundEngine::Init(&initSettings, &platformInitSettings) != AK_Success) {
    R_DEBUG("ERROR: Failed to initialize wwise sound engine!\n");
    return false;
  }


  AkMusicSettings musicSettings;
  AK::MusicEngine::GetDefaultInitSettings(musicSettings);
  if (AK::MusicEngine::Init(&musicSettings) != AK_Success) {
    R_DEBUG("ERROR: Failed to initialize wwise music engine!\n");
    return false;
  }

  R_DEBUG("NOTIFY: Audio has been initialized.\n");
  return true;
}


void WwiseEngine::CleanUp()
{
}
} // Recluse