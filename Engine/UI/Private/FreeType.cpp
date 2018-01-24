// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "FreeType.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


b8 FreeType::Initialize()
{
  FT_Error error = FT_Init_FreeType(&mLibrary);
  if (error != FT_Err_Ok) {
    R_DEBUG(rError, "Failed to initialize FreeType library.\n");
    return false;
  }
  
  R_DEBUG(rNotify, "Free type library initialized.\n");
  return true;
}


void FreeType::CleanUp()
{
  FT_Error error = FT_Done_FreeType(mLibrary);
  if (error != FT_Err_Ok) {
    R_DEBUG(rError, "Failed to free up FreeType library!\n");
  }
}


FT_Face FreeType::CreateFace(const std::string ttf_path)
{
  FT_Face face = nullptr;

  auto err = FT_New_Face(mLibrary, ttf_path.c_str(), 0, &face);
  if (err != FT_Err_Unknown_File_Format) {
    R_DEBUG(rError, "FreeType tried loading an unsupported font file...\n");
  } else if (err) {
    R_DEBUG(rError, "Unknown error triggered by FreeType!\n");
  }

  return face;
}


void FreeType::FreeFace(FT_Face face)
{
  FT_Done_Face(face);
}
} // Recluse