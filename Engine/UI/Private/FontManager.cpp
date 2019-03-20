// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "FontManager.hpp"
#include "Renderer/Renderer.hpp"
#include "Core/Exception.hpp"


#include "FreeType.hpp"


namespace Recluse {


FontManager& gFontManager()
{
  return FontManager::instance();
}


void FontManager::onStartUp()
{
  mFreeType = new FreeType();
  if (!mFreeType->initialize()) {
    R_DEBUG(rError, "Freetype not initialized. Stopping font manager initialization.\n");
    return;
  }


  mRenderer = &gRenderer();
  R_DEBUG(rNotify, "FreeType library successfully initialized.\n");

  if (!mRenderer->isActive()) {
    R_DEBUG(rWarning, "Global Renderer is not active! Be sure to start up the global renderer before using the FontManager!\n");
  }
}


void FontManager::onShutDown()
{
  mFreeType->cleanUp();

  delete mFreeType;
  mFreeType = nullptr;
}
} // Recluse