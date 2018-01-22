// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Utility/Time.hpp"
#include "Win32/Win32Configs.hpp"


namespace Recluse {


r64 currentTime = 0.0;
r64 oldTime = 0.0;

r64 Time::ScaleTime = 1.0;
r64 Time::FixTime = 16.6 / 1000.0; //< 60 fps
r64 Time::DeltaTime = 0.0;

static r64 Frequency = 0.0;
static r64 AppStart = 0.0;

void Time::OnStartUp()
{
  LARGE_INTEGER li;
  QueryPerformanceFrequency(&li);
  Frequency = r64(li.QuadPart);
  QueryPerformanceCounter(&li);
  AppStart = r64(li.QuadPart);
  Update();
}


r64 Time::CurrentTime()
{
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  return r64(li.QuadPart - AppStart) / Frequency;
}


void Time::Update()
{
  r64 latestTime = CurrentTime();
  DeltaTime = latestTime - oldTime;
  oldTime = latestTime;
}
} // Recluse