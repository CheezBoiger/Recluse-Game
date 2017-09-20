// Copyright (c) 2017 Recluse Project.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"


namespace Recluse {


class FontCache;
class FreeType;
class Renderer;

class FontManager : public EngineModule<FontManager> {
public:
  typedef i32   font_key_t;

  FontManager() 
    : mCache(nullptr)
    , mFreeType(nullptr)
    , mRenderer(nullptr) { }

  void          OnStartUp() override;
  void          OnShutDown() override;

  // Get the FreeType wrapper object reference.
  FreeType*     Handle() { return mFreeType; }

  // Import the font into the Font Manager, which will return the integer
  // key to the glyphs. This is used to determine which font to be used
  // in certain situations. Returns -1 if font can not be stored. 
  font_key_t    ImportFont(const std::string& filepath); 

private:
  FontCache*    mCache;
  FreeType*     mFreeType;
  Renderer*     mRenderer;
};


// Global font manager.
FontManager& gFontManager();
} // Recluse