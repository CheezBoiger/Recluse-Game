// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "WwiseEngine.hpp"
#include "Core/Exception.hpp"


namespace Recluse {





b8 WwiseEngine::Initialize()
{
  AkMemSettings memSettings;
  memSettings.uMaxNumPools = 20;

  if (AK::MemoryMgr::Init(&memSettings) != AK_Success) {
    R_DEBUG(rError, "Could not initialize AK memory manager!");
    return false;
  }

  AkStreamMgrSettings stmSettings;
  AK::StreamMgr::GetDefaultSettings(stmSettings);

  if (!AK::StreamMgr::Create(stmSettings)) {
    R_DEBUG(rError, "Failed to create audio stream manager!");
    return false;
  }

  AkDeviceSettings deviceSettings;
  AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

  // NOTE(): Perhaps one day, we can do some customizations to our streaming device...
  if (mLowLevelFilePackage.Init(deviceSettings) != AK_Success) {
    R_DEBUG(rError, "Failed to create streaming device and low level I/O system.");
    return false;
  }

  AkInitSettings initSettings;
  AkPlatformInitSettings platformInitSettings;
  AK::SoundEngine::GetDefaultInitSettings(initSettings);
  AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);

  if (AK::SoundEngine::Init(&initSettings, &platformInitSettings) != AK_Success) {
    R_DEBUG(rError, "Failed to initialize wwise sound engine!");
    return false;
  }


  AkMusicSettings musicSettings;
  AK::MusicEngine::GetDefaultInitSettings(musicSettings);
  if (AK::MusicEngine::Init(&musicSettings) != AK_Success) {
    R_DEBUG(rError, "Failed to initialize wwise music engine!");
    return false;
  }

  R_DEBUG(rNotify, "Audio has been initialized.");
  return true;
}


void WwiseEngine::CleanUp()
{
}
} // Recluse