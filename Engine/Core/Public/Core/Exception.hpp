// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Types.hpp"
#include "Core/Logging/Log.hpp"
#include <cassert>

#if defined(_DEBUG)
 #include <stdio.h>
 #define R_DEBUG(p, ...) printf(p, ##__VA_ARGS__) 
#else
 #define R_DEBUG(p, ...)
#endif

#define R_EXCEPT(e, s) Recluse::Log(Recluse::rDebug) << s << "\n";
#define R_ASSERT(cond, str) assert(cond && str)