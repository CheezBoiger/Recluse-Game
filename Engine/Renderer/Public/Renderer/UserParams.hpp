// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Core/Utility/Vector.hpp"

namespace Recluse {


enum FrameBuffering {
  SINGLE_BUFFER,
  DOUBLE_BUFFER,
  TRIPLE_BUFFER
};


enum WindowType {
  WINDOW_BORDER,
  WINDOW_BORDERLESS,
  WINDOW_FULLSCREEN
};


class GpuConfigParams {
public:
  FrameBuffering  _Buffering;
  r32             _Lod;
  u32             _EnableVsync;
};
} // Recluse