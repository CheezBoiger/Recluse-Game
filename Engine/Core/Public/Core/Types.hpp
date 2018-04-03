// Copyright (c) Recluse Project. All rights reserved.
#pragma once


#include <cstdint>
#include <string>

namespace Recluse {

typedef uint8_t           b8;
typedef uint8_t           u8;
typedef int8_t            i8;
typedef uint16_t          u16;
typedef int16_t           i16;
typedef uint32_t          u32;
typedef int32_t           i32;
typedef uint64_t          u64;
typedef int64_t           i64;

typedef float             r32;
typedef double            r64;

typedef char              tchar;


// Globally unique identifier.
typedef u64               uuid64;
} // Recluse

#define RTEXT(s)          u8##s