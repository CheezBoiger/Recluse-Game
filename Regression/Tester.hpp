// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Logging/Log.hpp"


#include "Core/Utility/Vector.hpp"


#define TASSERT_E(a, b) if (a != b) { Log(rError) << "Assertion Failed.\n"; \
  Log() << "issue with var : " << #a << " and " << #b << "\n"; \
  return false; \
}

#define TASSERT_L(a, b) if (a >= b) { Log(rError) << "Assertion Failed.\n"; \
  Log() << "issue with var : " << #a << " and " << #b << "\n"; \
  return false; \
}

#define TASSERT_LE(a, b) if (a > b) { Log(rError) << "Assertion Failed.\n"; \
  Log() << "issue with var : " << #a << " and " << #b << "\n"; \
  return false; \
} 

#define TASSERT_GE(a, b) if (a < b) { Log(rError) << "Assertion Failed.\n"; \
  Log() << "issue with var : " << #a << " and " << #b << "\n"; \
  return false; \
}

#define TASSERT_G(a, b) if (a <= b) { Log(rError) << "Assertion Failed.\n"; \
  Log() << "issue with var : " << #a << " and " << #b << "\n"; \
  return false; \
} 

#define TASSERT_NE(a, b) if (a == b) { Log(rError) << "Assertion Failed.\n"; \
  Log() << "issue with var : " << #a << " and " << #b << "\n"; \
  return false; \
} 


namespace Recluse {

struct Tester {
private:
  static U32    TestsPassed;
  static U32    TestsFailed;
public:
  typedef B8 (*TestFunc)();

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

  static U32    GetTestsPassed() { return TestsPassed; }
  static U32    GetTestsFailed() { return TestsFailed; }
};
} // Recluse