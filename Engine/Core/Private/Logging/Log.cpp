// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Logging/Log.hpp"
#include "Exception.hpp"


namespace Recluse {


Log& operator<<(Log& log, Verbosity verbosity)
{
  log.Type = verbosity;
  return log;
}
} // Recluse