// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "FontManager.hpp"
#include "Renderer/Renderer.hpp"
#include "Core/Exception.hpp"


#include "FreeType.hpp"


namespace Recluse {


FontManager& gFontManager()
{
  return FontManager::Instance();
}


void FontManager::OnStartUp()
{
  mFreeType = new FreeType();
  if (!mFreeType->Initialize()) {
    R_DEBUG(rError, "Freetype not initialized. Stopping font manager initialization.");
    return;
  }


  mRenderer = &gRenderer();
  R_DEBUG(rNotify, "FreeType library successfully initialized.");

  if (!mRenderer->IsActive()) {
    R_DEBUG(rWarning, "Global Renderer is not active! Be sure to start up the global renderer before using the FontManager!");
  }
}


void FontManager::OnShutDown()
{
  mFreeType->CleanUp();

  delete mFreeType;
  mFreeType = nullptr;
}
} // Recluse