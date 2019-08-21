// Copyright (c) Recluse Project. All rights reserved.
#pragma once


#include <cstdint>
#include <string>

namespace Recluse {

typedef uint8_t           B8;
typedef uint8_t           U8;
typedef int8_t            I8;
typedef uint16_t          U16;
typedef int16_t           I16;
typedef uint32_t          U32;
typedef int32_t           I32;
typedef uint64_t          U64;
typedef int64_t           I64;
typedef U32               B32;

typedef float             R32;
typedef double            R64;

typedef char              TChar;


// Globally unique identifier.
typedef U64               UUID64;
} // Recluse

#define RTEXT(s)          u8##s

#if _DEBUG
 #define DEBUG_OP(param) param
#else
 #define DEBUG_OP(param)
#endif

#define __USE_INTEL_INTRINSICS__ 0