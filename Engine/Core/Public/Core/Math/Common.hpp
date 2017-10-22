// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include <math.h>

#define CONST_PI      3.141592653589793238462643383279502884197169399375
#define Radians(deg) (deg * (static_cast<r32>(CONST_PI) / 180.0f))
#define Degrees(rad) (rad * (180.0f / static_cast<r32>(CONST_PI)))
#define Clamp(v, min, max)  v = (v > max ? max : (v < min ? min : v))  
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))