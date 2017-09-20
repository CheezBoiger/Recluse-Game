// Copyright (c) 2017 Recluse Project.
#include "Audio.hpp"

#include "WwiseEngine.hpp"

namespace Recluse {


Audio& gAudio()
{
  return Audio::Instance();
}


void Audio::OnStartUp()
{
  mWwise = new WwiseEngine();
  mWwise->Initialize();
}


void Audio::OnShutDown()
{
  mWwise->CleanUp();

  delete mWwise;
  mWwise = nullptr;
}


void Audio::UpdateState()
{
}
} // Recluse