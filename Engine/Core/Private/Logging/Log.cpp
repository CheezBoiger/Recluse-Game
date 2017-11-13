// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Logging/Log.hpp"
#include "Exception.hpp"


namespace Recluse {

b8 Log::display = false;


void Log::DisplayToConsole(b8 enable)
{
  display = enable;
}


b8 Log::DisplayingToConsole()
{
  return Log::display;
}


Log& operator<<(Log& log, Verbosity verbosity)
{
  log.Type = verbosity;
  return log;
}
} // Recluse