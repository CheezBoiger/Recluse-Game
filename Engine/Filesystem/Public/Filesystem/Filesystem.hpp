// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


// Filesystem module, intended to hold onto files lulz.
class Filesystem : public EngineModule<Filesystem> {
public:
  class FileHandle {
    u64   size;
    u8*   buffer;
  };

  class AsyncFileHandle {
    b8    finished;

    u64   size;
    u8*   buffer;
  };

  enum FilePathType {
    Absolute,
    Relative
  };

  Filesystem() { }


  void                      OnStartUp() override;
  void                      OnShutDown() override;
  void                      SetCurrentAppDirectory(tchar* applicationPath);
  void                      AppendSearchPath(tchar* path);

  // Current application directory of the executable.
  const tchar*              CurrentAppDirectory();

  b8                        FileExists(tchar* filepath);
  b8                        DirectoryExists(tchar* directorypath);
  tchar*                    GetApplicationSourcePath();
  tchar*                    SetApplicationSourcePath(const tchar* srcPath);
  std::vector<std::string>  DirectoryContents(std::string& path);
  std::vector<std::string>  SearchPaths() { return mSearchPath; }  

private:
  std::string               mCurrentDirectoryPath;
  std::string               mAppSourcePath;
  std::vector<std::string>  mSearchPath;
};


Filesystem& gFilesystem();
} // Recluse