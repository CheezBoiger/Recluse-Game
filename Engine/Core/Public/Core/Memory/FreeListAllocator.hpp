// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Allocator.hpp"
#include "Core/Types.hpp"


namespace Recluse {


// Linked list style memory allocator. This is a usefull allocator
// for handling dynamic data, without dealing with context switches and
// slow times from std malloc and new.
class FreeListAllocator : public Allocator {

  FreeListAllocator(const FreeListAllocator&) = delete;
  FreeListAllocator& operator=(const FreeListAllocator&) = delete;

public:
  FreeListAllocator(size_t Sz, void* Mem);

  ~FreeListAllocator();

  
  void* Allocate(size_t Size, size_t Align) override;
  void Deallocate(void* Ptr) override;

private:
  struct AllocHeader {
    size_t      Adjust;
    size_t      Sz;
  };
  
  struct MemBlock {
    size_t      Sz;
    MemBlock*   Next;
  };

  MemBlock* m_MemBlocks;
};
} // Recluse 