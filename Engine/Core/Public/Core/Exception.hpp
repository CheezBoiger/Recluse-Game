// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Types.hpp"
#include "Core/Logging/Log.hpp"


#if defined(_DEBUG)
 #include <stdio.h>
 #define R_EXCEPT(e, s)
 #define R_DEBUG(p, ...) printf(p, ##__VA_ARGS__) 
#else
 #define R_EXCEPT(e, s)
 #define R_DEBUG(p, ...)
#endif