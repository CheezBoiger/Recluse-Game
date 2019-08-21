// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include <math.h>

#define CONST_PI                3.141592653589793238462643383279502884197169399375
#define CONST_PI_HALF           1.57079632679489661923   // pi/2
#define CONST_PI_QUARTER        0.785398163397448309616 // pi/4
#define CONST_2_PI              6.283185307 // 2 * pi
#define CONST_TOLERANCE         0.0001     // 
#define EPSILON                 0.0000001 // 
#define R_E                     2.71828182845904523536   // e

#define Radians(deg)            ((deg) * (static_cast<R32>(CONST_PI) / 180.0f))
#define Degrees(rad)            ((rad) * (180.0f / static_cast<R32>(CONST_PI)))
#define Clamp(v, min, max)      (v) = ((v) > (max) ? (max) : ((v) < (min) ? (min) : (v)))  
#define R_Max(a, b)             ((a) > (b) ? (a) : (b))
#define R_Min(a, b)             ((a) < (b) ? (a) : (b))
#define Absf(a)                 ((a) >= 0.0f ? (a) : -(a))
#define SmoothStepf(e0, e1, t)  Clamp(((t) - (e0)) / ((e1) - (e0))), 0.0f, 1.0f) 
#define Floorf(a)               floorf((a))
#define Log2f(a)                log2f((a))
#define Ceilf(a)                ceilf((a))
#define Lerpf(a, b, t)          ((a) * (1.0f - (t)) + (b) * (t))