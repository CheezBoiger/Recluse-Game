// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Audio.hpp"

#include "AudioEngine.hpp"

namespace Recluse {


Audio& gAudio()
{
  static AudioEngine engine;
  return engine;
}
} // Recluse