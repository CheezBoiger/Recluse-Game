// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Allocator.hpp"
#include "Core/Types.hpp"

namespace Recluse {


class StackAllocator : public Allocator {
public:
  StackAllocator(size_t sz, void* rawMemBlock)
    : Allocator(sz, rawMemBlock) { }

  
private:
  
};
} // Recluse 