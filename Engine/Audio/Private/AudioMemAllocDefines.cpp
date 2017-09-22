// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "WwiseEngine.hpp"
#include "Core/Types.hpp"

#include <stdlib.h>


namespace AK {


void* AllocHook(size_t size)
{
  return malloc(size);
}


void FreeHook(void* inPointer)
{
  free(inPointer);
}


void* VirtualAllocHook(void* inMemAddr, size_t inSize, DWORD inDwAllocType, DWORD inDwProtec)
{
  return VirtualAlloc(inMemAddr, inSize, inDwAllocType, inDwProtec);
}


void  VirtualFreeHook(void* inMemAddr, size_t inSize, DWORD inDWFreeType)
{
  VirtualFree(inMemAddr, inSize, inDWFreeType);
}
} // AK