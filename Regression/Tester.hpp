// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Logging/Log.hpp"


#include "Core/Utility/Vector.hpp"

namespace Recluse {

struct Tester {
private:
  static u32    TestsPassed;
  static u32    TestsFailed;
public:
  typedef b8 (*TestFunc)();

  static void   RunAllTests(std::vector<TestFunc> tests) {
    Log() << "Total tests: " << tests.size() << "\nRunning Tests...\n\n";

    for (TestFunc func : tests) {
      if (func()) {
        ++TestsPassed;
      } else {
        ++TestsFailed;
      }
    }

    Log() << "Finished...\n\n";  
  }

  static u32    GetTestsPassed() { return TestsPassed; }
  static u32    GetTestsFailed() { return TestsFailed; }
};
} // Recluse