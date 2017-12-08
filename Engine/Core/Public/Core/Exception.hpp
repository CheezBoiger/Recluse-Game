// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once
#include "Core/Types.hpp"

#if defined(_DEBUG)
 #include "Core/Logging/Log.hpp"
 #include <stdio.h>
 #include <cassert>
 #define R_DEBUG(type, ...) Recluse::Log(type) << ##__VA_ARGS__;
 #define R_ASSERT(cond, str) assert(cond && str)
 #define R_EXCEPT(e, s) Recluse::Log(Recluse::rDebug) << s << "\n";
#else
 #define R_DEBUG(p, ...)
 #define R_ASSERT(cond, str)
 #define R_EXCEPT(e, s)
#endif

