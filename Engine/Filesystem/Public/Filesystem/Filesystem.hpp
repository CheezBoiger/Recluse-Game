// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"

#include "Game/Scene/Scene.hpp"


namespace Recluse {


struct FileHandle {
  U64   Sz;
  TChar*   Buf;

  static const U64 kNoFile = 0xffffffffffffffff; 
  
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
    B8    Finished;
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


  void                      onStartUp() override;
  void                      onShutDown() override;
  void                      SetCurrentAppDirectory(TChar* ApplicationPath);
  void                      AppendSearchPath(TChar* path);
  FilesystemResult          ReadFrom(const TChar* filepath, FileHandle* Buf);
  FilesystemResult          WriteTo(const TChar* filepath, TChar* in, U32 sz);
  void                      AsyncReadFile(AsyncFileHandle* Buf);

  // Current application directory of the executable.
  const TChar*              CurrentAppDirectory();

  B8                        FileExists(TChar* Filepath);
  B8                        DirectoryExists(TChar* DirectoryPath);
  TChar*                    GetApplicationSourcePath();
  TChar*                    SetApplicationSourcePath(const TChar* SrcPath);
  std::vector<std::string>  DirectoryContents(std::string& Path);
  std::vector<std::string>  SearchPaths() { return m_SearchPath; }  

  // Load a scene from a file.
  B8                        LoadScene(Scene* output, std::string filepath);

private:
  std::string               m_CurrentDirectoryPath;
  std::string               m_AppSourcePath;
  std::vector<std::string>  m_SearchPath;
};


Filesystem& gFilesystem();
} // Recluse