// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshData.hpp"

namespace Recluse {


MeshData::MeshData()
 : mRhi(nullptr)
{

}


MeshData::~MeshData()
{
}


void MeshData::Initialize(size_t elementCount, size_t sizeType, void* data,
  b8 isStatic, size_t indexCount, void* indices)
{
  mVertexBuffer.Initialize(mRhi, elementCount, sizeType, data,
    isStatic ? VertexBuffer::STATIC_BUFFER : VertexBuffer::DYNAMIC_BUFFER);

  if (indexCount) {
    mIndexBuffer.Initialize(mRhi, indexCount, sizeof(u32), indices);
  }
}


void MeshData::CleanUp()
{
  mVertexBuffer.CleanUp();
  mIndexBuffer.CleanUp();
}
} // Recluse