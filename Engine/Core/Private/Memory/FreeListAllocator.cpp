// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Memory/FreeListAllocator.hpp"
#include "Logging/Log.hpp"
#include "Exception.hpp"


namespace Recluse {


size_t AlignAdjust(const void* Addr, size_t Align)
{
  size_t Mask = (Align - 1);
  size_t MisAlign = (reinterpret_cast<size_t>(Addr) & Mask);
  size_t Adjustment = Align - MisAlign; 
  if (Adjustment == Align) return 0;
  return Adjustment;
}


size_t AlignHeaderAdjust(const void* Addr, size_t Align, size_t HeaderSz)
{
  size_t Adjustment = AlignAdjust(Addr, Align);
  size_t SpaceNeed = HeaderSz;
  if (Adjustment < SpaceNeed) {
    SpaceNeed -= Adjustment;
    Adjustment += Align * (SpaceNeed / Align);
    if (SpaceNeed % Align > 0) Adjustment += Align;
  }

  return Adjustment;
}


FreeListAllocator::FreeListAllocator(size_t Sz, void* Mem)
  : Allocator(Sz, Mem)
  , m_MemBlocks((struct MemBlock*)Mem)
{
  R_ASSERT(Sz > sizeof(MemBlock), "Allocator: Heap memory must be larger than avg Memory Block!\n");
  m_MemBlocks->Next = nullptr;
  m_MemBlocks->Sz = Sz;
}


FreeListAllocator::~FreeListAllocator()
{
  m_MemBlocks = nullptr;
}


void* FreeListAllocator::Allocate(size_t Sz, size_t Align)
{
  R_ASSERT(Sz != 0 && Align != 0, "Size, or align, parameter passed as 0 !\n");
  MemBlock* PrevBlock = nullptr;
  MemBlock* TraverseBlock = m_MemBlocks;
  while (TraverseBlock) {
    size_t Adjustment = AlignHeaderAdjust(TraverseBlock, Align, sizeof(AllocHeader));
    size_t TotalSz = Sz + Adjustment;
    if (TraverseBlock->Sz < TotalSz) {
      PrevBlock = TraverseBlock;
      TraverseBlock = TraverseBlock->Next;
      continue;
    }

    if (TraverseBlock->Sz - TotalSz <= sizeof(AllocHeader)) {
      TotalSz = TraverseBlock->Sz;
      if (PrevBlock) {
        PrevBlock = TraverseBlock->Next;
      } else {
        m_MemBlocks = TraverseBlock->Next;
      }
    } else {
      MemBlock* NextBlock = (MemBlock*)((u8*)(TraverseBlock + 1) + TotalSz);
      NextBlock->Sz = TraverseBlock->Sz - TotalSz;
      NextBlock->Next = TraverseBlock->Next;
      if (PrevBlock) {
        PrevBlock->Next = NextBlock;
      } else {
        m_MemBlocks = NextBlock;
      }
    }
    
    size_t AlignedAddr = (uintptr_t )TraverseBlock + Adjustment;
    AllocHeader* Header = (AllocHeader* )(AlignedAddr - sizeof(AllocHeader));
    Header->Sz = TotalSz;
    Header->Adjust = Adjustment;
    m_Used += TotalSz;
    m_NumAllocations += 1;
    return (void* )AlignedAddr;
  }

  return nullptr;
}


void FreeListAllocator::Deallocate(void* Ptr)
{
  R_ASSERT(Ptr, "Pointer is null, can not deallocate!!\n");
  AllocHeader* Header = (AllocHeader* )((uintptr_t )Ptr - sizeof(AllocHeader));
  uintptr_t BlockStart = reinterpret_cast<uintptr_t>(Ptr) - Header->Adjust;
  size_t BlockSz = Header->Sz;
  uintptr_t BlockEnd = BlockStart + BlockSz;
  
}
} // Recluse