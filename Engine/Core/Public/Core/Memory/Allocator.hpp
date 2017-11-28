// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


// Base allocator object.
class Allocator {
public:
  Allocator(size_t size, void* memory)
    : m_Memory(memory)
    , m_Size(size)
    , m_Used(0)
    , m_NumAllocations(0) { }
  
  virtual void* Allocate(size_t size, u8 align) { return nullptr; }
  virtual void  Deallocate(void* ptr) { }

  void*         RawMemory() { return m_Memory; }

  size_t        NumAllocs() const { return m_NumAllocations; }
  size_t        TotalSize() const { return m_Size; }
  size_t        UsedMem() const { return m_Used; }

protected:
  void*         m_Memory;
  size_t        m_Size;
  size_t        m_Used;
  size_t        m_NumAllocations;
};
} // Recluse