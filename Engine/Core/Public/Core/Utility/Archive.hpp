// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


// Interface archive for serializing data into a storage unit.
// This is mainly used to store data into a file for use from the game
// object.
class IArchive {
public:
  virtual ~IArchive() { }

  virtual IArchive& operator<<(u8 val) = 0;
  virtual IArchive& operator<<(i8 val) = 0;
  virtual IArchive& operator<<(u16 val) = 0;
  virtual IArchive& operator<<(i16 val) = 0;
  virtual IArchive& operator<<(u32 val) = 0;
  virtual IArchive& operator<<(i32 val) = 0;
  virtual IArchive& operator<<(u64 val) = 0;
  virtual IArchive& operator<<(i64 val) = 0;
};
} // Recluse