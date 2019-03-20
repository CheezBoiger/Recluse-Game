// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core.hpp"
#include "Exception.hpp"


namespace Recluse {


Core& gCore()
{
  return Core::instance();
}


void Core::onStartUp()
{
  Window::initializeAPI();
}


void Core::onShutDown()
{
  Time::instance().shutDown();
}


void Core::Sync()
{

  // Wait for threads to finish current jobs in queue.
  m_Pool.WaitAll();
}
} // Recluse