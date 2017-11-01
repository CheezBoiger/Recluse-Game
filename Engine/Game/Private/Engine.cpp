// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Engine.hpp"

#include "Core/Exception.hpp"


namespace Recluse {

Engine& gEngine()
{
  static Engine engine;
  return engine;
}


Engine::Engine()
  : mCamera(nullptr)
{
}


Engine::~Engine()
{
}


void Engine::StartUp(std::string appName, i32 width, i32 height)
{
  // NOTE(): Always start up the core first, before starting anything else up.
  gCore().StartUp();
  gFilesystem().StartUp();
  gRenderer().StartUp();
  gAnimation().StartUp();
  gUI().StartUp();

  mWindow.Create(appName, width, height);

  gRenderer().Initialize(&mWindow);
}


void Engine::CleanUp()
{
  gUI().ShutDown();
  gAnimation().ShutDown();
  gRenderer().ShutDown();
  gFilesystem().ShutDown();
  gCore().ShutDown();
}


void Engine::Update(r64 dt)
{

}
} // Recluse