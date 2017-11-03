// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "FreeType.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


b8 FreeType::Initialize()
{
  FT_Error error = FT_Init_FreeType(&mLibrary);
  if (error != FT_Err_Ok) {
    R_DEBUG(rError, "Failed to initialize FreeType library.");
    return false;
  }
  
  R_DEBUG(rNotify, "Free type library initialized.");
  return true;
}


void FreeType::CleanUp()
{
  FT_Error error = FT_Done_FreeType(mLibrary);
  if (error != FT_Err_Ok) {
    R_DEBUG(rError, "Failed to free up FreeType library!");
  }
}
} // Recluse