// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"

#include "ft2build.h"
#include FT_FREETYPE_H



namespace Recluse {


// A free type handle.
class FreeType {
public:
  FreeType() { }

  b8              Initialize();
  void            CleanUp();
  FT_Library&     Handle() { return mLibrary; }

private:
  FT_Library      mLibrary;
};
} // Recluse