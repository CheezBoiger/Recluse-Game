// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/AABB.hpp"


namespace Recluse {


// parameters to be pushed to hdr pass. memory size must be kept low!
struct ParamsHDR {
  ParamsHDR() 
    : _bloomStrength(1.0) { }

  r32 _bloomStrength;
};


class HDR {
public:
  HDR() 
    { m_params._bloomStrength = 1.0f; }

  ParamsHDR   m_params;
};
} // Recluse