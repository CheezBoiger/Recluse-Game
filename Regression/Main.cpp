// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Logging/Log.hpp"
#include "Math/TestMath.hpp"
#include "Game/TestGameObject.hpp"
#include "Game/Engine.hpp"
#include "Memory/TestMemory.hpp"

#include "Tester.hpp"

#include <iomanip>

using namespace Recluse;

U32 Tester::TestsFailed = 0;
U32 Tester::TestsPassed = 0;

std::vector<Tester::TestFunc> test = {
  Test::BasicVectorMath,
  Test::BasicMatrixMath,
  Test::TestGameObject,
  Test::TestAllocators
};

int main()
{
  Log::displayToConsole(true);
  Log() << "Initial testing of Recluse Engine Software Libraries.\n"
              << "Initializing testing data cache...\n";
  UserConfigParams params = { };
  params._windowWidth = 800;
  params._windowHeight = 600;
  params._windowType = WindowType_Border;
  gEngine().startUp("Test Engine.", &params);

  // TODO(): Add more regressions.
  Tester::RunAllTests(test);
  
  gEngine().cleanUp();

  Log() << "Tests Passed: " <<  std::setw(10)
        << Tester::GetTestsPassed() << "\n"
        << "Tests Failed: " << std::setw(10)
        << Tester::GetTestsFailed() << "\n";

  Log() << "All done!\n"
        << "Press Enter to continue...\n";
  std::cin.ignore();
  return 0;
}