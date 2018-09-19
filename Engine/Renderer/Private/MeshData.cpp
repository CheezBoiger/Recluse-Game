// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshData.hpp"
#include "Vertex.hpp"

#include "MaterialDescriptor.hpp"

#include <algorithm>

namespace Recluse {


MeshData::MeshData()
 : mRhi(nullptr)
{

}


MeshData::~MeshData()
{
}


void MeshData::Initialize(size_t elementCount, void* data, size_t vertexSize, size_t indexCount, void* indices)
{
  m_vertexBuffer.Initialize(mRhi, elementCount, vertexSize, data);

  if (indexCount) {
    m_indexBuffer.Initialize(mRhi, indexCount, sizeof(u32), indices);
  }
}


void MeshData::CleanUp()
{
  m_vertexBuffer.CleanUp();
  m_indexBuffer.CleanUp();
}
} // Recluse