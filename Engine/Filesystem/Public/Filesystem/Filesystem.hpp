// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


struct FileHandle {
    u64   Sz;
    u8*   Buf;
};


struct AsyncFileHandle {
    // Check if the file is finished reading. If not, 
    // Buf and Sz are not readable!
    b8    Finished;
    // Size of the buffer.
    u64   Sz;
    // The buffer to read from.
    u8*   Buf;
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
  void                      ReadFile(FileHandle* Buf);
  void                      AsyncReadFile(AsyncFileHandle* Buf);

  // Current application directory of the executable.
  const tchar*              CurrentAppDirectory();

  b8                        FileExists(tchar* Filepath);
  b8                        DirectoryExists(tchar* DirectoryPath);
  tchar*                    GetApplicationSourcePath();
  tchar*                    SetApplicationSourcePath(const tchar* SrcPath);
  std::vector<std::string>  DirectoryContents(std::string& Path);
  std::vector<std::string>  SearchPaths() { return m_SearchPath; }  

private:
  std::string               m_CurrentDirectoryPath;
  std::string               m_AppSourcePath;
  std::vector<std::string>  m_SearchPath;
};


Filesystem& gFilesystem();
} // Recluse