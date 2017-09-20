// Copyright (c) 2017 Recluse Project.
#include "Core.hpp"
#include "Exception.hpp"


namespace Recluse {


Core& gCore()
{
  return Core::Instance();
}


void Core::OnStartUp()
{
  Window::InitializeAPI();
  Time::Instance().StartUp();
}


void Core::OnShutDown()
{
  Time::Instance().ShutDown();
}
} // Recluse