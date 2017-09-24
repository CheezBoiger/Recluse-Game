// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


class FileCache;
class File;
class AsyncFile;

// Filesystem module, intended to hold onto files lulz.
class Filesystem : public EngineModule<Filesystem> {
public:
  enum FilePathType {
    Absolute,
    Relative
  };

  Filesystem() 
    : mCache(nullptr) { }


  void        OnStartUp() override;
  void        OnShutDown() override;
  void        SetCurrentAppDirectory(tchar* applicationPath);
  void        SaveFileAsync(File* file);

  b8          SaveFile(File* file, tchar* path = nullptr);
  File*       LoadFile(tchar* filepath);
  File*       CreateFile(tchar* filename);

  AsyncFile*  LoadFileAsync(tchar* filepath);

  tchar*      CurrentAppDirectory();

  b8          FileExists(tchar* filepath);
  b8          DirectoryExists(tchar* directorypath);
  tchar*      GetApplicationSourcePath();
  tchar*      SetApplicationSourcePath(const std::string& srcPath);

  
private:
  FileCache*  mCache;
  std::string mCurrentDirectoryPath;
  std::string mAppSourcePath;
};


Filesystem& gFilesystem();
} // Recluse