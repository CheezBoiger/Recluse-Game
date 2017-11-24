// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Logging/Log.hpp"
#include "Math/TestMath.hpp"

#include "Tester.hpp"

#include <iomanip>

using namespace Recluse;

u32 Tester::TestsFailed = 0;
u32 Tester::TestsPassed = 0;

std::vector<Tester::TestFunc> test = {
  Test::BasicVectorMath,
  Test::BasicMatrixMath
};

int main()
{
  Log::DisplayToConsole(true);
  Log() << "Initial testing of Recluse Engine Software Libraries.\n"
              << "Initializing testing data cache...\n";

  // TODO(): Add more regressions.
  Tester::RunAllTests(test);

  Log() << "Tests Passed: " <<  std::setw(10)
        << Tester::GetTestsPassed() << "\n"
        << "Tests Failed: " << std::setw(10)
        << Tester::GetTestsFailed() << "\n";

  Log() << "All done!\n"
        << "Press Enter to continue...\n";
  std::cin.ignore();
  return 0;
}