// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"

#include "Game/Scene/Scene.hpp"


namespace Recluse {


struct FileHandle {
  u64   Sz;
  tchar*   Buf;

  static const u64 kNoFile = 0xffffffffffffffff; 
  
  FileHandle()
    : Sz(kNoFile)
    , Buf(nullptr) { }

  virtual ~FileHandle()
  {
    if (Sz != kNoFile) { delete Buf; }
    Buf = nullptr;
  }
};


struct AsyncFileHandle : public FileHandle {
    // Check if the file is finished reading. If not, 
    // Buf and Sz are not readable!
    b8    Finished;
};


enum FilesystemResult {
  FilesystemResult_Success,
  FilesystemResult_Failed,
  FilesystemResult_NotFound
};

// Filesystem module, intended to hold onto files lulz.
class Filesystem : public EngineModule<Filesystem> {
public:

  enum FilePathType {
    Absolute,
    Relative
  };

  Filesystem() { }


  void                      OnStartUp() override;
  void                      OnShutDown() override;
  void                      SetCurrentAppDirectory(tchar* ApplicationPath);
  void                      AppendSearchPath(tchar* path);
  FilesystemResult          ReadFrom(const tchar* filepath, FileHandle* Buf);
  FilesystemResult          WriteTo(const tchar* filepath, tchar* in, u32 sz);
  void                      AsyncReadFile(AsyncFileHandle* Buf);

  // Current application directory of the executable.
  const tchar*              CurrentAppDirectory();

  b8                        FileExists(tchar* Filepath);
  b8                        DirectoryExists(tchar* DirectoryPath);
  tchar*                    GetApplicationSourcePath();
  tchar*                    SetApplicationSourcePath(const tchar* SrcPath);
  std::vector<std::string>  DirectoryContents(std::string& Path);
  std::vector<std::string>  SearchPaths() { return m_SearchPath; }  

  // Load a scene from a file.
  b8                        LoadScene(Scene* output, std::string filepath);

private:
  std::string               m_CurrentDirectoryPath;
  std::string               m_AppSourcePath;
  std::vector<std::string>  m_SearchPath;
};


Filesystem& gFilesystem();
} // Recluse