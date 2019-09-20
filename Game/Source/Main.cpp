// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Game/Engine.hpp"

#include <string>
#include <cctype>

using namespace Recluse;


int main(int c, char* argv[])
{
  // TODO():
  // Before the game engine starts up, we want to read our configuration file, 
  // used to determine engine settings saved by user.
  {
    U32 width = 800;
    U32 height = 600;
    GraphicsConfigParams params = { };
    UserConfigParams userParams = { };
    gEngine().readGraphicsConfig(params);
    gEngine().readUserConfigs(userParams);

    gEngine().startUp("Recluse", &userParams, &params);
    gEngine().run();
    Window* pWindow = gEngine().getWindow();
    switch (userParams._windowType) {
    case WindowType_Borderless:
    {
      pWindow->setToWindowed(width, height, true);
      pWindow->setToCenter();
    } break;
    case WindowType_Border:
    {
      pWindow->setToWindowed(width, height);
      pWindow->setToCenter();
    } break;
    case WindowType_Fullscreen:
    default:
      pWindow->setToFullScreen();
    }
    pWindow->show();
  }
  
  while (gEngine().isRunning()) {
    Time::update();
    gEngine().processInput();
    // Scene updating goes here.

    gEngine().update();
  }

  AssetManager::cleanUpAssets();
  gEngine().cleanUp();
  return 0;
}