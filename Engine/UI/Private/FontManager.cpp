// Copyright (c) 2017 Recluse Project.
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
    R_DEBUG("ERROR: Freetype not initialized. Stopping font manager initialization.\n");
    return;
  }


  mRenderer = &gRenderer();
  R_DEBUG("NOTIFY: FreeType library successfully initialized.\n");

  if (!mRenderer->IsActive()) {
    R_DEBUG("WARNING: Global Renderer is not active! Be sure to start up the global renderer before using the FontManager!");
  }
}


void FontManager::OnShutDown()
{
  mFreeType->CleanUp();

  delete mFreeType;
  mFreeType = nullptr;
}
} // Recluse