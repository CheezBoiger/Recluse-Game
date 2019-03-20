// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Logging/Log.hpp"
#include "Exception.hpp"


namespace Recluse {

b8 Log::display = false;
b8 Log::store = false;


void Log::displayToConsole(b8 enable)
{
  display = enable;
}


void Log::StoreLogs(b8 enable)
{
  store = enable;
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