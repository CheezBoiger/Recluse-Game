// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Audio.hpp"

#include "AudioEngine.hpp"

namespace Recluse {


Audio& gAudio()
{
  static FMODAudioEngine engine;
  return engine;
}
} // Recluse