// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Utility/Time.hpp"
#include "Win32/Win32Configs.hpp"


namespace Recluse {


r64 currentTime = 0.0;
r64 oldTime = 0.0;

r64 Time::scaleTime = 1.0;
r64 Time::fixTime = 1.0 / 60.0; //< 0.0166666 s
r64 Time::deltaTime = 0.0;

static r64 Frequency = 0.0;
static r64 AppStart = 0.0;

void Time::onStartUp()
{
  LARGE_INTEGER li;
  QueryPerformanceFrequency(&li);
  Frequency = r64(li.QuadPart);
  QueryPerformanceCounter(&li);
  AppStart = r64(li.QuadPart);
  update();
}


r64 Time::currentTime()
{
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  return r64(li.QuadPart - AppStart) / Frequency;
}


void Time::update()
{
  r64 latestTime = currentTime();
  deltaTime = latestTime - oldTime;
  oldTime = latestTime;
}
} // Recluse