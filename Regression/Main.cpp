// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Logging/Log.hpp"
#include "Math/TestMath.hpp"

using namespace Recluse;

int main()
{
  Log() << "Initial testing of Recluse Engine Software Libraries.\n"
              << "Initializing testing data cache...\n";

  // TODO(): Add more regressions.
  Test::BasicVectorMath();

  Log() << "All done!\n"
        << "Press Enter to continue...\n";
  std::cin.ignore();
  return 0;
}