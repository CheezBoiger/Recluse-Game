// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Utility/Time.hpp"
#include "Win32/Win32Configs.hpp"


namespace Recluse {


R64 currentTime = 0.0;
R64 oldTime = 0.0;

R64 Time::scaleTime = 1.0;
R64 Time::fixTime = 1.0 / 60.0; //< 0.0166666 s
R64 Time::deltaTime = 0.0;

static R64 Frequency = 0.0;
static R64 AppStart = 0.0;

void Time::onStartUp()
{
  LARGE_INTEGER li;
  QueryPerformanceFrequency(&li);
  Frequency = R64(li.QuadPart);
  QueryPerformanceCounter(&li);
  AppStart = R64(li.QuadPart);
  update();
}


R64 Time::currentTime()
{
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  return R64(li.QuadPart - AppStart) / Frequency;
}


void Time::update()
{
  R64 latestTime = currentTime();
  deltaTime = latestTime - oldTime;
  oldTime = latestTime;
}
} // Recluse