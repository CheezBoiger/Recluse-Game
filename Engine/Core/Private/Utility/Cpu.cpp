// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Utility/Cpu.hpp"
#include <intrin.h>

namespace Recluse {



b8 Cpu::CheckVendorString(char* str)
{
  int b[4];
  __cpuid(b, 0);
  if (b[1] == *reinterpret_cast<const int*>(str)
    && b[2] == *reinterpret_cast<const int*>(str + 8)
    && b[3] == *reinterpret_cast<const int*>(str + 4))
  {
    return true;
  }
  return false;
}


i32 Cpu::BitsFlippedCount(i32 v)
{
  v = v - ((v >> 1) & 0x55555555);
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
  return (((v + (v >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 24;
}


b8 Cpu::IsIntel()
{
  return CheckVendorString("GenuineIntel");
}


b8 Cpu::IsAmd()
{
  return CheckVendorString("AuthenticAMD");
}
} // Recluse