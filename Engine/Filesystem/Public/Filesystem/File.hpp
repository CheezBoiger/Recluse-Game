// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include <fstream>

namespace Recluse {


class File {
public:
  File() { }

  b8            Opened() { return mFileHandle.is_open(); }
  b8            Open(std::string filepath);
  b8            Close();
  
  std::string   ReadLine();
  tchar         ReadChar();
  

  const tchar*  Extension() { return mExtension.c_str(); }
  const tchar*  Filename() { return mFilename.c_str(); }
  const tchar*  Path() { return mPath.c_str(); }

private:
  std::ifstream mFileHandle;
  std::string   mFilename;
  std::string   mExtension;
  std::string   mPath;
};
} // Recluse