// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "FreeType.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


b8 FreeType::Initialize()
{
  FT_Error error = FT_Init_FreeType(&mLibrary);
  if (error != FT_Err_Ok) {
    R_DEBUG("ERROR: Failed to initialize FreeType library.\n");
    return false;
  }
  
  R_DEBUG("NOTIFY: Free type library initialized.\n");
  return true;
}


void FreeType::CleanUp()
{
  FT_Error error = FT_Done_FreeType(mLibrary);
  if (error != FT_Err_Ok) {
    R_DEBUG("ERROR: Failed to free up FreeType library!\n");
  }
}
} // Recluse