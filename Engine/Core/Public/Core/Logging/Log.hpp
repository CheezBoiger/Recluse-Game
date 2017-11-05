// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once
#include "Core/Types.hpp"
#include <iostream>

namespace Recluse {


enum Verbosity {
  rNormal,
  rError,
  rWarning,
  rDebug,
  rVerbose,
  rNotify
};


// TODO(): Log should be reading into a file for keeping history of events happening in the engine.
class Log {
public:
  static b8     Disable(Verbosity verbose);
  static b8     Enable(Verbosity verbose);

  Log(Verbosity verbosity = rNormal) : Type(verbosity) { }


  void          StoreOutput();

  Verbosity     Type;
};


// Generic Modifier for debugging and printing onto the screen.
template<typename Type>
Log& operator<<(Log& log, Type val) {
  switch (log.Type) {
    case rError: std::cout << "Error: "; break;
    case rWarning: std::cout << "Warning: "; break;
    case rVerbose: std::cout << "Verbose: "; break;
    case rNotify: std::cout << "Notify: "; break;
    case rDebug: std::cout << "Debug: "; break;
    default: break;
  }

  // Set back to normal to prevent redundant logging.
  log.Type = rNormal;

  std::cout << val;
  return log;
}


Log& operator<<(Log& log, Verbosity verbosity);
} // Recluse