// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Game/Engine.hpp"


using namespace Recluse;

int main(int c, char* argv[])
{
  // TODO():
  // Before the game engine starts up, we want to read our configuration file, 
  // used to determine engine settings saved by user.

  gEngine().StartUp("Recluse", false);
  gEngine().Run();

  Window* pMainWindow = gEngine().GetWindow();
  pMainWindow->Show();

  while (gEngine().Running()) {
    Time::Update();
    gEngine().ProcessInput();
    // Scene updating goes here.

    gEngine().Update();
  }

  AssetManager::CleanUpAssets();
  gEngine().CleanUp();
  return 0;
}