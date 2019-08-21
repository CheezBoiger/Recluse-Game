// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Logging/Log.hpp"
#include "Exception.hpp"


namespace Recluse {

B8 Log::display = false;
B8 Log::store = false;


void Log::displayToConsole(B8 enable)
{
  display = enable;
}


void Log::StoreLogs(B8 enable)
{
  store = enable;
}


B8 Log::DisplayingToConsole()
{
  return Log::display;
}


Log& operator<<(Log& log, Verbosity verbosity)
{
  log.Type = verbosity;
  return log;
}
} // Recluse