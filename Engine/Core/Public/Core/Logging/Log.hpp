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
  static b8     display;
  static b8     store;
public:
  static b8     DisplayingToConsole();
  static b8     Storing() { return store; }
  static b8     Disable(Verbosity verbose);
  static b8     setEnable(Verbosity verbose);
  static void   displayToConsole(b8 enable);
  static void   StoreLogs(b8 enable);

  Log(Verbosity verbosity = rNormal) : Type(verbosity) { }

  template<typename DataType>
  void Store(DataType& type) {
  }

  Verbosity     Type;
};


// Generic Modifier for debugging and printing onto the screen.
template<typename Type>
Log& operator<<(Log& log, Type val) {
  if (Log::DisplayingToConsole()) {
    switch (log.Type) {
      case rError: std::cout << "Error: "; break;
      case rWarning: std::cout << "Warning: "; break;
      case rVerbose: std::cout << "Verbose: "; break;
      case rNotify: std::cout << "Notify: "; break;
      case rDebug: std::cout << "Debug: "; break;
      default: break;
    }
  
    std::cout << val;
  }

  if (Log::Storing()) {
    log.Store(val);
  }
  // Set back to normal to prevent redundant logging.
  log.Type = rNormal;
  return log;
}


Log& operator<<(Log& log, Verbosity verbosity);
} // Recluse