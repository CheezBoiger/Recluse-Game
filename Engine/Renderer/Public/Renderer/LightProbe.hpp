// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"

namespace Recluse {


class LightProbe {
public:
};


class LightProbeManager {
public:
  static const u32 kMaxAllowedProbes = 32;

  std::vector<LightProbe> GenerateProbes(u32 count = kMaxAllowedProbes);

};
} // Recluse