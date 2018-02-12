// Copyright (c) 2017 Recluse Project. All rights reserved.
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
}


void Core::OnShutDown()
{
  Time::Instance().ShutDown();
}


void Core::Sync()
{

  // Wait for threads to finish current jobs in queue.
  m_Pool.WaitAll();
}
} // Recluse