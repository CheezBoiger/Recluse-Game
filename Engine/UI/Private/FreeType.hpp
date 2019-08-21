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

  B8              initialize();
  void            cleanUp();
  FT_Library&     getHandle() { return mLibrary; }

  
  FT_Face         CreateFace(const std::string ttf_path); 
  void            FreeFace(FT_Face face);
private:
  FT_Library      mLibrary;
};
} // Recluse