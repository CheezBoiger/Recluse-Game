// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "AudioEngine.hpp"
#include "Core/Exception.hpp"

#include <al.h>
#include <alc.h>


namespace Recluse {


AudioResult AudioEngine::Initialize()
{
  AudioResult result = Audio_Success;
  R_DEBUG(rNotify, "Audio has been initialized.\n");
  return result;
}


void AudioEngine::CleanUp()
{
}


void AudioEngine::Update(r64 dt)
{
}

AudioId AudioEngine::CreateAudioObject()
{
  return ~0ul;
}
} // Recluse