// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "../Tester.hpp"
#include "TestMemory.hpp"

#include "Core/Memory/Allocator.hpp"
#include "Core/Memory/FreeListAllocator.hpp"

#include "Game/Component.hpp"
#include "Game/GameObject.hpp"

#include <new>

#define DATA_BLOCK_SZ sizeof(size_t) * 2048

namespace Test {


struct TestStruct 
{
  char*  a;
  int   b;
  short c;

  TestStruct()
    : a(new char[32]), b(0), c('a') { }

  TestStruct(const TestStruct&) = delete;
  TestStruct& operator=(const TestStruct&) = delete;

  TestStruct(TestStruct&& o) { }
  TestStruct& operator=(TestStruct&& o) { return (*this);}

  ~TestStruct() { delete[] a; }
};


B8 TestAllocators()
{
  // Testing the data block.
  void* DataBlock = malloc(DATA_BLOCK_SZ);

  FreeListAllocator FreeListAlloc(DATA_BLOCK_SZ, DataBlock);
  float* d = (float* )FreeListAlloc.allocate(sizeof(float), 8);
  GameObject* obj = (GameObject* )FreeListAlloc.allocate(sizeof(GameObject), 8);
  *d = 5.023f;
  TestStruct* s = (TestStruct* )FreeListAlloc.allocate(sizeof(TestStruct), 8);
  *s = std::move(TestStruct());

  FreeListAlloc.Deallocate(obj);
  FreeListAlloc.Deallocate(d);
  FreeListAlloc.Deallocate(s);
  free(DataBlock);
  return true;
}
} // Test