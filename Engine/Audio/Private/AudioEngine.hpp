// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Win32/Win32Configs.hpp"

#include "Audio/Audio.hpp"


namespace Recluse {


class AudioEngine : public Audio {
public:
  AudioResult                         Initialize();
  void                                CleanUp();


  AudioId                             CreateAudioObject() override;

  void                                Update(r64 dt);



private:

};
} // Recluse