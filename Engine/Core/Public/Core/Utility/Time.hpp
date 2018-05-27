// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"


#define SECONDS_PER_FRAME_TO_FPS(spf) ((u32)((1.0 / (spf)) + 0.5f)) 

namespace Recluse {


// 16.6 ms is 60 fps. This Time actually computes the time in seconds.
class Time : public EngineModule<Time> {
public:
  // Override on start up. This is not really needed, but win32
  // is required to initialize the sucker.
  void OnStartUp() override;

  // Get Time in seconds.
  static r64  CurrentTime();

  // Update delta time.
  static void Update();

  // Scaled time, which is multiplied to our other times to scale.
  static r64 ScaleTime;

  // Delta time, in seconds, between frames.
  static r64 DeltaTime;

  // Fix time, in seconds, to which some engine modules rely on, such as physics.
  static r64 FixTime;

  // Start up the time.
  static void Start() { Time::Instance().StartUp(); }
};
} // Recluse