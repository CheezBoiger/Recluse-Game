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


class Log {
public:
  static b8     Disable(Verbosity verbose);
  static b8     Enable(Verbosity verbose);

  Log(Verbosity verbosity = rNormal) : mType(verbosity) { }


  void          StoreOutput();

  // Logging system overload for the logger.
  template<typename Type>
  Log&          operator<<(Type val) {
    switch (mType) {
      case rError: std::cout << "Error: "; break;
      case rWarning: std::cout << "Warning: "; break;
      case rVerbose: std::cout << "Verbose: "; break;
      case rNotify: std::cout << "Notify: "; break;
      case rDebug: std::cout << "Debug: "; break;
      default: break;
    }
    std::cout << val << "\n";
    return (*this);
  }
private:
  Verbosity     mType;
};
} // Recluse