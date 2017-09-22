// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Types.hpp"
#include "Core/Win32/Win32Configs.hpp"

#include "AK/SoundEngine/Common/AkMemoryMgr.h"
#include "AK/SoundEngine/Common/AkSoundEngine.h"
#include "AK/MusicEngine/Common/AkMusicEngine.h"
#include "AK/SoundEngine/Common/AkModule.h"

#include "AK/SoundEngine/Common/IAkStreamMgr.h"
#include "AK/SoundEngine/Common/AkStreamMgrModule.h"
#include "AK/Tools/Common/AkPlatformFuncs.h"

#include "AkFilePackageLowLevelIOBlocking.h"

// AL library externs.
namespace AK {
  void* AllocHook(size_t size);
  void  FreeHook(void* inPointer);
  
  void* VirtualAllocHook(void* inMemAddr, size_t inSize, DWORD inDwAllocType, DWORD inDwProtec);
  void  VirtualFreeHook(void* inMemAddr, size_t inSize, DWORD inDWFreeType);
} // AK


namespace Recluse {


class WwiseEngine {
public:
  b8                                  Initialize();
  void                                CleanUp();

  CAkFilePackageLowLevelIOBlocking&   FilePackage() { return mLowLevelFilePackage; }

private:
  CAkFilePackageLowLevelIOBlocking    mLowLevelFilePackage;

};
} // Recluse