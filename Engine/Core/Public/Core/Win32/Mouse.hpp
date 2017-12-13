// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Win32Configs.hpp"
#include "Core/Types.hpp"

namespace Recluse {



class Mouse {
public:
  static void Enable(b8 enable);
  static void Track(b8 enable) { mouseTrack = enable ; }
  static void Show(b8 enable);

  static b8 Enabled() { return mouseEnabled; }
  static b8 Showing() { return mouseShow; }
  static b8 Tracking() { return mouseTrack; }
  static void SetPosition(r64 x, r64 y);

  static r64      XPos;
  static r64      YPos;

  static r64      LastXPos;
  static r64      LastYPos;

private:
  static b8       mouseTrack;
  static b8       mouseEnabled;
  static b8       mouseShow;
  static HCURSOR  cursor;
};
} // Recluse