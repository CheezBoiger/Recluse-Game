// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Filesystem.hpp"

#include "Core/Core.hpp"
#include "Core/Win32/Win32Configs.hpp"
#include "Core/Exception.hpp"

#include <algorithm>

namespace Recluse {


Filesystem& gFilesystem() 
{
  return Filesystem::Instance();
}


void Filesystem::OnStartUp()
{
  if (!gCore().IsActive()) {
    R_DEBUG(rError, "Core is not active! Activate first!\n");
    return;
  }

  char buffer[MAX_PATH];
  GetModuleFileName(NULL, buffer, MAX_PATH);
  std::string::size_type pos = std::string(buffer).find_last_of("\\/");
  mCurrentDirectoryPath = std::string(buffer).substr(0, pos);
  std::replace(mCurrentDirectoryPath.begin(), mCurrentDirectoryPath.end(), '\\', '/');  
}


void Filesystem::OnShutDown()
{
}


const tchar* Filesystem::CurrentAppDirectory()
{
  return mCurrentDirectoryPath.data();
}
} // Recluse